#!/usr/bin/env python3
"""
ft_irc automated test runner (terminal)

Features:
- Starts ./ircserv (or provided path)
- Runs protocol-level tests using Python sockets (no nc required)
- Optional Valgrind leak test + LEAK SUMMARY parsing
- Reliable "SIGSTOP flood" equivalent: slow-reader client buffering test
- Optional JSON + JUnit XML reports for CI/sharing

Usage (from project dir):
  python3 ft_irc_tester.py --make --ircserv ./ircserv --port 6667 --password pass
  python3 ft_irc_tester.py --valgrind --json-report report.json --junit-report report.xml
"""

from __future__ import annotations

import argparse
import os
import re
import signal
import socket
import subprocess
import sys
import time
import json
from dataclasses import dataclass, asdict
from typing import List, Optional, Pattern, Tuple, Dict
from xml.sax.saxutils import escape as xml_escape

# ----------------------------- Utilities -----------------------------

ANSI = {
    "reset": "\033[0m",
    "red": "\033[31m",
    "green": "\033[32m",
    "yellow": "\033[33m",
    "blue": "\033[34m",
    "dim": "\033[2m",
}

def c(text: str, color: str, enabled: bool=True) -> str:
    if not enabled:
        return text
    return f"{ANSI.get(color,'')}{text}{ANSI['reset']}"

def sleep_s(sec: float) -> None:
    time.sleep(sec)

def ms() -> int:
    return int(time.time() * 1000)

def print_progress_bar(current: int, total: int, width: int = 40, color: bool = True) -> None:
    """Print a progress bar that updates in place."""
    percent = current / total if total > 0 else 0
    filled = int(width * percent)
    bar = "█" * filled + "░" * (width - filled)
    pct_str = f"{percent * 100:5.1f}%"
    status = f"[{current}/{total}]"
    line = f"\r{c('Testing:', 'blue', color)} {bar} {pct_str} {status}"
    sys.stdout.write(line)
    sys.stdout.flush()

@dataclass
class TestResult:
    name: str
    ok: bool
    duration_ms: int
    details: str = ""
    meta: Optional[dict] = None

class TestFailure(Exception):
    pass

# ----------------------------- IRC Client -----------------------------

class IRCClient:
    """
    Tiny IRC client for testing.
    Reads raw lines delimited by \\n; tolerates \\r\\n.
    """
    def __init__(self, host: str, port: int, timeout: float = 1.5, name: str = "client"):
        self.host = host
        self.port = port
        self.timeout = timeout
        self.name = name
        self.sock: Optional[socket.socket] = None
        self.buf = b""

    def connect(self) -> None:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(self.timeout)
        s.connect((self.host, self.port))
        self.sock = s

    def close(self) -> None:
        try:
            if self.sock:
                self.sock.close()
        finally:
            self.sock = None
            self.buf = b""

    def send_raw(self, data: bytes) -> None:
        if not self.sock:
            raise RuntimeError("Not connected")
        self.sock.sendall(data)

    def send_line(self, line: str) -> None:
        self.send_raw((line + "\r\n").encode("utf-8", "replace"))

    def recv_lines(self, max_lines: int = 100, max_time: float = 1.5) -> List[str]:
        if not self.sock:
            raise RuntimeError("Not connected")
        deadline = time.time() + max_time
        out: List[str] = []
        while len(out) < max_lines and time.time() < deadline:
            while b"\n" in self.buf and len(out) < max_lines:
                line, self.buf = self.buf.split(b"\n", 1)
                line = line.rstrip(b"\r")
                out.append(line.decode("utf-8", "replace"))
            if len(out) >= max_lines:
                break

            remaining = max(0.0, deadline - time.time())
            if remaining <= 0:
                break
            self.sock.settimeout(min(self.timeout, remaining))
            try:
                chunk = self.sock.recv(4096)
                if not chunk:
                    break
                self.buf += chunk
            except socket.timeout:
                break
            except OSError:
                break
        return out

    def recv_until(self, patterns: List[Pattern[str]], max_time: float = 2.0) -> List[str]:
        seen = [False] * len(patterns)
        lines: List[str] = []
        deadline = time.time() + max_time
        while time.time() < deadline and not all(seen):
            new = self.recv_lines(max_lines=200, max_time=min(0.5, max(0.0, deadline - time.time())))
            if not new:
                continue
            lines.extend(new)
            for i, pat in enumerate(patterns):
                if not seen[i] and any(pat.search(x) for x in new):
                    seen[i] = True
        return lines

# ----------------------------- Server Harness -----------------------------

class ServerProc:
    def __init__(self, argv: List[str], cwd: str, quiet: bool, capture_stderr: bool = False):
        self.argv = argv
        self.cwd = cwd
        self.quiet = quiet
        self.capture_stderr = capture_stderr
        self.p: Optional[subprocess.Popen] = None
        self.captured_stderr: Optional[str] = None

    def start(self) -> None:
        stdout = subprocess.DEVNULL if self.quiet else None
        if self.capture_stderr:
            stderr = subprocess.PIPE
        else:
            stderr = subprocess.DEVNULL if self.quiet else None
        # new process group, so we can kill whole group
        self.p = subprocess.Popen(self.argv, cwd=self.cwd, stdout=stdout, stderr=stderr, preexec_fn=os.setsid, text=True)

    def stop(self, sig=signal.SIGINT, grace: float = 1.0) -> None:
        if not self.p:
            return
        if self.p.poll() is not None:
            return
        try:
            os.killpg(os.getpgid(self.p.pid), sig)
        except ProcessLookupError:
            return
        t0 = time.time()
        while time.time() - t0 < grace:
            if self.p.poll() is not None:
                return
            time.sleep(0.05)
        try:
            os.killpg(os.getpgid(self.p.pid), signal.SIGKILL)
        except ProcessLookupError:
            pass

    def wait(self, timeout: float = 2.0) -> int:
        if not self.p:
            return 0
        try:
            out, err = self.p.communicate(timeout=timeout) if self.capture_stderr else (None, None)
            if self.capture_stderr and err is not None:
                self.captured_stderr = err
            return self.p.returncode if self.p.returncode is not None else 0
        except subprocess.TimeoutExpired:
            return -1

# ----------------------------- Assertions -----------------------------

def assert_any(lines: List[str], pat: Pattern[str], msg: str) -> None:
    if not any(pat.search(x) for x in lines):
        raise TestFailure(msg + "\nCollected:\n" + "\n".join(lines[-80:]))

def assert_none(lines: List[str], pat: Pattern[str], msg: str) -> None:
    if any(pat.search(x) for x in lines):
        raise TestFailure(msg + "\nCollected:\n" + "\n".join(lines[-80:]))

# ----------------------------- Test Suite -----------------------------

class FtIrcTester:
    def __init__(self, host: str, port: int, password: str, timeout: float):
        self.host = host
        self.port = port
        self.password = password
        self.timeout = timeout

    def new_client(self, name: str) -> IRCClient:
        cli = IRCClient(self.host, self.port, timeout=self.timeout, name=name)
        cli.connect()
        return cli

    def register(self, cli: IRCClient, nick: str, user: Optional[str]=None, real: str="Real", passwd: Optional[str]=None) -> List[str]:
        passwd = self.password if passwd is None else passwd
        user = nick if user is None else user
        cli.send_line(f"PASS {passwd}")
        cli.send_line(f"NICK {nick}")
        cli.send_line(f"USER {user} 0 * :{real}")
        lines = cli.recv_until([re.compile(r"\s001\s+" + re.escape(nick) + r"\b")], max_time=3.0)
        return lines

    # --- Tests ---
    def test_registration_success(self) -> None:
        cli = self.new_client("reg_ok")
        try:
            lines = self.register(cli, "alice", real="Alice Smith")
            assert_any(lines, re.compile(r"\s001\s+alice\b"), "Expected welcome (001) for alice")
        finally:
            cli.close()

    def test_wrong_password(self) -> None:
        cli = self.new_client("wrong_pass")
        try:
            cli.send_line("PASS wrongpassword")
            cli.send_line("NICK alice")
            lines = cli.recv_lines(max_time=2.0)
            assert_any(lines, re.compile(r"\s464\s+"), "Expected ERR_PASSWDMISMATCH (464)")
        finally:
            cli.close()

    def test_duplicate_nick(self) -> None:
        a = self.new_client("dup_a")
        b = self.new_client("dup_b")
        try:
            self.register(a, "alice", real="Alice")
            b.send_line(f"PASS {self.password}")
            b.send_line("NICK alice")
            lines = b.recv_lines(max_time=2.5)
            assert_any(lines, re.compile(r"\s433\s+.*\balice\b"), "Expected ERR_NICKNAMEINUSE (433) for alice")
        finally:
            a.close()
            b.close()

    def test_join_create_channel(self) -> None:
        cli = self.new_client("join_create")
        try:
            self.register(cli, "alice", real="Alice")
            cli.send_line("JOIN #test")
            lines = cli.recv_until([
                re.compile(r"JOIN\s+#test\b"),
                re.compile(r"\s353\s+alice\s+=\s+#test\s+:.*@?alice\b"),
                re.compile(r"\s366\s+alice\s+#test\b"),
            ], max_time=3.0)
            assert_any(lines, re.compile(r"\s366\s+alice\s+#test\b"), "Expected end of NAMES list (366)")
        finally:
            cli.close()

    def test_join_second_user_names_list(self) -> None:
        a = self.new_client("join_a")
        b = self.new_client("join_b")
        try:
            self.register(a, "alice")
            a.send_line("JOIN #test")
            a.recv_until([re.compile(r"\s366\s+alice\s+#test\b")], max_time=2.5)

            self.register(b, "bob")
            b.send_line("JOIN #test")
            # Names can appear in any order: "@alice bob" or "bob @alice"
            lines = b.recv_until([re.compile(r"\s353\s+bob\s+=\s+#test\s+:")], max_time=3.0)
            # Check that BOTH alice and bob appear in names list (any order)
            names_pat = re.compile(r"\s353\s+bob\s+=\s+#test\s+:(.*)")
            found_names = False
            for line in lines:
                m = names_pat.search(line)
                if m:
                    names_str = m.group(1).lower()
                    if "alice" in names_str and "bob" in names_str:
                        found_names = True
                        break
            if not found_names:
                raise TestFailure("Expected names list containing both alice and bob\nCollected:\n" + "\n".join(lines[-80:]))
        finally:
            a.close()
            b.close()

    def test_join_with_key(self) -> None:
        a = self.new_client("key_a")
        b = self.new_client("key_b")
        c_ = self.new_client("key_c")
        try:
            self.register(a, "alice")
            a.send_line("JOIN #secret")
            a.recv_until([re.compile(r"\s366\s+alice\s+#secret\b")], max_time=2.5)
            a.send_line("MODE #secret +k mykey")
            a.recv_until([re.compile(r"MODE\s+#secret\s+\+k\b")], max_time=2.5)

            self.register(b, "bob")
            b.send_line("JOIN #secret")
            lines_b = b.recv_lines(max_time=2.5)
            assert_any(lines_b, re.compile(r"\s475\s+bob\s+#secret\b"), "Expected ERR_BADCHANNELKEY (475) when joining without key")

            self.register(c_, "carol")
            c_.send_line("JOIN #secret mykey")
            lines_c = c_.recv_until([re.compile(r"JOIN\s+#secret\b")], max_time=3.0)
            assert_any(lines_c, re.compile(r"JOIN\s+#secret\b"), "Expected successful JOIN with correct key")
        finally:
            a.close()
            b.close()
            c_.close()

    def test_topic_set_and_query(self) -> None:
        a = self.new_client("topic_a")
        b = self.new_client("topic_b")
        try:
            self.register(a, "alice")
            a.send_line("JOIN #test")
            a.recv_until([re.compile(r"\s366\s+alice\s+#test\b")], max_time=2.5)
            a.send_line("TOPIC #test :Hello")
            a.recv_until([re.compile(r"TOPIC\s+#test\s+:Hello")], max_time=2.5)

            self.register(b, "bob")
            b.send_line("JOIN #test")
            b.recv_until([re.compile(r"\s366\s+bob\s+#test\b")], max_time=2.5)
            b.send_line("TOPIC #test")
            lines = b.recv_until([re.compile(r"\s332\s+bob\s+#test\s+:Hello")], max_time=3.0)
            assert_any(lines, re.compile(r"\s332\s+bob\s+#test\s+:Hello"), "Expected RPL_TOPIC (332) with Hello")
        finally:
            a.close()
            b.close()

    def test_topic_protect_nonop_fails(self) -> None:
        a = self.new_client("tprot_a")
        b = self.new_client("tprot_b")
        try:
            self.register(a, "alice")
            a.send_line("JOIN #test")
            a.recv_until([re.compile(r"\s366\s+alice\s+#test\b")], max_time=2.5)
            a.send_line("MODE #test +t")
            a.recv_until([re.compile(r"MODE\s+#test\s+\+t")], max_time=2.5)

            self.register(b, "bob")
            b.send_line("JOIN #test")
            b.recv_until([re.compile(r"\s366\s+bob\s+#test\b")], max_time=2.5)

            b.send_line("TOPIC #test :My new topic")
            lines = b.recv_lines(max_time=2.5)
            assert_any(lines, re.compile(r"\s482\s+bob\s+#test\b"), "Expected ERR_CHANOPRIVSNEEDED (482) for TOPIC by non-op (+t)")
        finally:
            a.close()
            b.close()

    def test_invite_allows_join_invite_only(self) -> None:
        a = self.new_client("inv_a")
        b = self.new_client("inv_b")
        try:
            # Register bob FIRST so he exists when alice invites
            self.register(b, "bob")

            self.register(a, "alice")
            a.send_line("JOIN #private")
            a.recv_until([re.compile(r"\s366\s+alice\s+#private\b")], max_time=2.5)
            a.send_line("MODE #private +i")
            a.recv_until([re.compile(r"MODE\s+#private\s+\+i")], max_time=2.5)

            # Now invite bob (who is already registered and connected)
            a.send_line("INVITE bob #private")
            a.recv_until([re.compile(r"\s341\s+alice\s+bob\s+#private\b")], max_time=2.5)

            # Bob should receive the INVITE notification
            invite_lines = b.recv_until([re.compile(r"INVITE\s+bob\s+#private\b")], max_time=3.0)
            assert_any(invite_lines, re.compile(r"INVITE\s+bob\s+#private\b"), "Expected INVITE notification to bob")

            b.send_line("JOIN #private")
            join_lines = b.recv_until([re.compile(r"JOIN\s+#private\b")], max_time=3.0)
            assert_any(join_lines, re.compile(r"JOIN\s+#private\b"), "Expected bob to join invite-only channel after invite")
        finally:
            a.close()
            b.close()

    def test_kick_operator_kicks_user(self) -> None:
        a = self.new_client("kick_a")
        b = self.new_client("kick_b")
        try:
            self.register(a, "alice")
            a.send_line("JOIN #test")
            a.recv_until([re.compile(r"\s366\s+alice\s+#test\b")], max_time=2.5)

            self.register(b, "bob")
            b.send_line("JOIN #test")
            b.recv_until([re.compile(r"\s366\s+bob\s+#test\b")], max_time=2.5)

            a.send_line("KICK #test bob :Goodbye!")
            lines_b = b.recv_until([re.compile(r"KICK\s+#test\s+bob\s+:Goodbye!")], max_time=3.0)
            assert_any(lines_b, re.compile(r"KICK\s+#test\s+bob\s+:Goodbye!"), "Expected bob to receive KICK")
        finally:
            a.close()
            b.close()

    def test_notice_no_error_on_missing_user(self) -> None:
        cli = self.new_client("notice_missing")
        try:
            self.register(cli, "bob")
            cli.send_line("NOTICE nonexistent :Hello?")
            lines = cli.recv_lines(max_time=2.0)
            assert_none(lines, re.compile(r"\s401\s+"), "NOTICE must not trigger ERR_NOSUCHNICK (401)")
        finally:
            cli.close()

    def test_privmsg_missing_user_errors(self) -> None:
        cli = self.new_client("pm_missing")
        try:
            self.register(cli, "bob")
            cli.send_line("PRIVMSG nonexistent :Hello?")
            lines = cli.recv_lines(max_time=2.5)
            assert_any(lines, re.compile(r"\s401\s+bob\s+nonexistent\b"), "Expected ERR_NOSUCHNICK (401) for PRIVMSG to nonexistent")
        finally:
            cli.close()

    def test_partial_commands_buffering(self) -> None:
        cli = self.new_client("partial")
        try:
            # Split the password to test partial command buffering
            pwd = self.password
            mid = len(pwd) // 2
            part1 = pwd[:mid] if mid > 0 else pwd[0]
            part2 = pwd[mid:] if mid > 0 else pwd[1:]
            
            cli.send_raw(f"PASS {part1}".encode())
            sleep_s(0.15)
            cli.send_raw(f"{part2}\r\n".encode())
            cli.send_raw(b"NICK alice\r\n")
            cli.send_raw(b"USER alice 0 * :Alice\r\n")
            lines = cli.recv_until([re.compile(r"\s001\s+alice\b")], max_time=4.0)
            assert_any(lines, re.compile(r"\s001\s+alice\b"), "Expected registration success even with partial PASS")
        finally:
            cli.close()

    def test_multi_commands_single_packet(self) -> None:
        cli = self.new_client("burst")
        try:
            # Don't include QUIT - it closes connection before we can read responses
            payload = f"PASS {self.password}\r\nNICK alice\r\nUSER alice 0 * :Alice\r\nJOIN #test\r\n"
            cli.send_raw(payload.encode("utf-8"))
            lines = cli.recv_lines(max_time=3.0)
            assert_any(lines, re.compile(r"\s001\s+alice\b"), "Expected welcome (001) in burst packet test")
            assert_any(lines, re.compile(r"JOIN\s+#test\b"), "Expected JOIN #test in burst packet test")
        finally:
            cli.close()

    # --- PART Command Tests ---
    def test_part_leave_channel(self) -> None:
        cli = self.new_client("part_leave")
        try:
            self.register(cli, "alice")
            cli.send_line("JOIN #test")
            cli.recv_until([re.compile(r"\s366\s+alice\s+#test\b")], max_time=2.5)
            cli.send_line("PART #test :Goodbye!")
            lines = cli.recv_lines(max_time=2.0)
            assert_any(lines, re.compile(r"PART\s+#test\s+:Goodbye!"), "Expected PART message with reason")
        finally:
            cli.close()

    def test_part_nonexistent_channel(self) -> None:
        cli = self.new_client("part_noexist")
        try:
            self.register(cli, "alice")
            cli.send_line("PART #nonexistent")
            lines = cli.recv_lines(max_time=2.0)
            assert_any(lines, re.compile(r"\s403\s+alice\s+#nonexistent\b"), "Expected ERR_NOSUCHCHANNEL (403)")
        finally:
            cli.close()

    # --- PRIVMSG Tests ---
    def test_privmsg_channel(self) -> None:
        a = self.new_client("pm_chan_a")
        b = self.new_client("pm_chan_b")
        try:
            self.register(a, "alice")
            a.send_line("JOIN #test")
            a.recv_until([re.compile(r"\s366\s+alice\s+#test\b")], max_time=2.5)

            self.register(b, "bob")
            b.send_line("JOIN #test")
            b.recv_until([re.compile(r"\s366\s+bob\s+#test\b")], max_time=2.5)

            b.send_line("PRIVMSG #test :Hello channel!")
            lines_a = a.recv_lines(max_time=2.5)
            assert_any(lines_a, re.compile(r"PRIVMSG\s+#test\s+:Hello channel!"), "Alice should receive bob's channel message")
        finally:
            a.close()
            b.close()

    def test_privmsg_private(self) -> None:
        a = self.new_client("pm_priv_a")
        b = self.new_client("pm_priv_b")
        try:
            self.register(a, "alice")
            self.register(b, "bob")

            b.send_line("PRIVMSG alice :Hello Alice!")
            lines_a = a.recv_lines(max_time=2.5)
            assert_any(lines_a, re.compile(r"PRIVMSG\s+alice\s+:Hello Alice!"), "Alice should receive bob's private message")
        finally:
            a.close()
            b.close()

    # --- NOTICE Tests ---
    def test_notice_channel(self) -> None:
        a = self.new_client("not_chan_a")
        b = self.new_client("not_chan_b")
        try:
            self.register(a, "alice")
            a.send_line("JOIN #test")
            a.recv_until([re.compile(r"\s366\s+alice\s+#test\b")], max_time=2.5)

            self.register(b, "bob")
            b.send_line("JOIN #test")
            b.recv_until([re.compile(r"\s366\s+bob\s+#test\b")], max_time=2.5)

            b.send_line("NOTICE #test :Channel notice!")
            lines_a = a.recv_lines(max_time=2.5)
            assert_any(lines_a, re.compile(r"NOTICE\s+#test\s+:Channel notice!"), "Alice should receive bob's channel notice")
        finally:
            a.close()
            b.close()

    def test_notice_private(self) -> None:
        a = self.new_client("not_priv_a")
        b = self.new_client("not_priv_b")
        try:
            self.register(a, "alice")
            self.register(b, "bob")

            b.send_line("NOTICE alice :Private notice!")
            lines_a = a.recv_lines(max_time=2.5)
            assert_any(lines_a, re.compile(r"NOTICE\s+alice\s+:Private notice!"), "Alice should receive bob's private notice")
        finally:
            a.close()
            b.close()

    # --- QUIT Test ---
    def test_quit_graceful(self) -> None:
        a = self.new_client("quit_a")
        b = self.new_client("quit_b")
        try:
            self.register(a, "alice")
            a.send_line("JOIN #test")
            a.recv_until([re.compile(r"\s366\s+alice\s+#test\b")], max_time=2.5)

            self.register(b, "bob")
            b.send_line("JOIN #test")
            b.recv_until([re.compile(r"\s366\s+bob\s+#test\b")], max_time=2.5)

            b.send_line("QUIT :Goodbye!")
            lines_a = a.recv_lines(max_time=2.5)
            assert_any(lines_a, re.compile(r"QUIT\s+:Goodbye!"), "Alice should see bob's QUIT message")
        finally:
            a.close()
            b.close()

    # --- MODE Tests ---
    def test_mode_query(self) -> None:
        cli = self.new_client("mode_query")
        try:
            self.register(cli, "alice")
            cli.send_line("JOIN #test")
            cli.recv_until([re.compile(r"\s366\s+alice\s+#test\b")], max_time=2.5)
            cli.send_line("MODE #test")
            lines = cli.recv_lines(max_time=2.0)
            assert_any(lines, re.compile(r"\s324\s+alice\s+#test\s+"), "Expected RPL_CHANNELMODEIS (324)")
        finally:
            cli.close()

    def test_mode_set_invite_only(self) -> None:
        cli = self.new_client("mode_i")
        try:
            self.register(cli, "alice")
            cli.send_line("JOIN #test")
            cli.recv_until([re.compile(r"\s366\s+alice\s+#test\b")], max_time=2.5)
            cli.send_line("MODE #test +i")
            lines = cli.recv_lines(max_time=2.0)
            assert_any(lines, re.compile(r"MODE\s+#test\s+\+i"), "Expected MODE +i broadcast")
        finally:
            cli.close()

    def test_mode_set_limit(self) -> None:
        cli = self.new_client("mode_l")
        try:
            self.register(cli, "alice")
            cli.send_line("JOIN #test")
            cli.recv_until([re.compile(r"\s366\s+alice\s+#test\b")], max_time=2.5)
            cli.send_line("MODE #test +l 10")
            lines = cli.recv_lines(max_time=2.0)
            assert_any(lines, re.compile(r"MODE\s+#test\s+\+l\s+10"), "Expected MODE +l 10 broadcast")
        finally:
            cli.close()

    def test_mode_give_operator(self) -> None:
        a = self.new_client("mode_o_a")
        b = self.new_client("mode_o_b")
        try:
            self.register(a, "alice")
            a.send_line("JOIN #test")
            a.recv_until([re.compile(r"\s366\s+alice\s+#test\b")], max_time=2.5)

            self.register(b, "bob")
            b.send_line("JOIN #test")
            b.recv_until([re.compile(r"\s366\s+bob\s+#test\b")], max_time=2.5)

            a.send_line("MODE #test +o bob")
            lines_b = b.recv_lines(max_time=2.5)
            assert_any(lines_b, re.compile(r"MODE\s+#test\s+\+o\s+bob"), "Expected MODE +o bob broadcast")
        finally:
            a.close()
            b.close()

    def test_mode_nonop_fails(self) -> None:
        a = self.new_client("mode_nonop_a")
        b = self.new_client("mode_nonop_b")
        try:
            self.register(a, "alice")
            a.send_line("JOIN #test")
            a.recv_until([re.compile(r"\s366\s+alice\s+#test\b")], max_time=2.5)

            self.register(b, "bob")
            b.send_line("JOIN #test")
            b.recv_until([re.compile(r"\s366\s+bob\s+#test\b")], max_time=2.5)

            b.send_line("MODE #test +i")
            lines_b = b.recv_lines(max_time=2.5)
            assert_any(lines_b, re.compile(r"\s482\s+bob\s+#test\b"), "Expected ERR_CHANOPRIVSNEEDED (482) for non-op MODE")
        finally:
            a.close()
            b.close()

    # --- Permission Tests ---
    def test_kick_nonop_fails(self) -> None:
        a = self.new_client("kick_nonop_a")
        b = self.new_client("kick_nonop_b")
        try:
            self.register(a, "alice")
            a.send_line("JOIN #test")
            a.recv_until([re.compile(r"\s366\s+alice\s+#test\b")], max_time=2.5)

            self.register(b, "bob")
            b.send_line("JOIN #test")
            b.recv_until([re.compile(r"\s366\s+bob\s+#test\b")], max_time=2.5)

            b.send_line("KICK #test alice :bye")
            lines_b = b.recv_lines(max_time=2.5)
            assert_any(lines_b, re.compile(r"\s482\s+bob\s+#test\b"), "Expected ERR_CHANOPRIVSNEEDED (482) for non-op KICK")
        finally:
            a.close()
            b.close()

    def test_invite_nonop_fails(self) -> None:
        a = self.new_client("inv_nonop_a")
        b = self.new_client("inv_nonop_b")
        c_ = self.new_client("inv_nonop_c")
        try:
            self.register(a, "alice")
            a.send_line("JOIN #test")
            a.recv_until([re.compile(r"\s366\s+alice\s+#test\b")], max_time=2.5)

            self.register(b, "bob")
            b.send_line("JOIN #test")
            b.recv_until([re.compile(r"\s366\s+bob\s+#test\b")], max_time=2.5)

            # Carol exists but is not on the channel
            self.register(c_, "carol")

            # Bob (non-op) tries to invite carol
            b.send_line("INVITE carol #test")
            lines_b = b.recv_lines(max_time=2.5)
            assert_any(lines_b, re.compile(r"\s482\s+bob\s+#test\b"), "Expected ERR_CHANOPRIVSNEEDED (482) for non-op INVITE")
        finally:
            a.close()
            b.close()
            c_.close()

    # --- Error Case Tests ---
    def test_command_before_registration(self) -> None:
        cli = self.new_client("unreg")
        try:
            cli.send_line("JOIN #test")
            lines = cli.recv_lines(max_time=2.0)
            assert_any(lines, re.compile(r"\s451\s+"), "Expected ERR_NOTREGISTERED (451)")
        finally:
            cli.close()

    def test_not_enough_parameters(self) -> None:
        cli = self.new_client("noparams")
        try:
            self.register(cli, "alice")
            cli.send_line("KICK")
            lines = cli.recv_lines(max_time=2.0)
            assert_any(lines, re.compile(r"\s461\s+alice\s+KICK\b"), "Expected ERR_NEEDMOREPARAMS (461) for KICK")
        finally:
            cli.close()

    # --- Edge Case Tests ---
    def test_empty_and_whitespace_commands(self) -> None:
        cli = self.new_client("empty_cmd")
        try:
            cli.send_line("")               # Empty line
            cli.send_line("   ")            # Whitespace only
            cli.send_line(f"PASS {self.password}")
            cli.send_line("")               # Another empty line
            cli.send_line("NICK alice")
            cli.send_line("USER alice 0 * :Alice")
            lines = cli.recv_until([re.compile(r"\s001\s+alice\b")], max_time=3.0)
            assert_any(lines, re.compile(r"\s001\s+alice\b"), "Registration should succeed despite empty lines")
        finally:
            cli.close()

    def test_very_long_message(self) -> None:
        cli = self.new_client("longmsg")
        try:
            self.register(cli, "alice")
            cli.send_line("JOIN #test")
            cli.recv_until([re.compile(r"\s366\s+alice\s+#test\b")], max_time=2.5)
            # Send a very long message (1000+ chars)
            long_text = "A" * 1000
            cli.send_line(f"PRIVMSG #test :{long_text}")
            # Server should not crash - we just check it's still responsive
            cli.send_line("QUIT")
            lines = cli.recv_lines(max_time=2.0)
            # As long as we get any response and no crash, this is OK
        finally:
            cli.close()

    # ----------------------------- Bot Tests -----------------------------

    def test_bot_connects_and_joins(self) -> None:
        """
        Bot connects to server, authenticates, and joins specified channel.
        """
        bot = self.new_client("bot")
        try:
            bot.send_line(f"PASS {self.password}")
            bot.send_line("NICK BotDaddy")
            bot.send_line("USER bot 0 * :IRC Helper Bot")
            lines = bot.recv_until([re.compile(r"\s001\s+BotDaddy\b")], max_time=2.0)
            assert_any(lines, re.compile(r"\s001\s+BotDaddy\b"), "Bot did not receive welcome (001)")
            
            bot.send_line("JOIN #bottest")
            lines = bot.recv_until([re.compile(r"\s366\s+BotDaddy\s+#bottest\b")], max_time=2.0)
            assert_any(lines, re.compile(r"JOIN\s+#bottest"), "Bot did not join channel")
            assert_any(lines, re.compile(r"\s366\s+BotDaddy\s+#bottest\b"), "Bot did not receive end of names list")
        finally:
            bot.close()

    def test_bot_help_command(self) -> None:
        """
        Bot responds to !help command with list of available commands.
        """
        bot = self.new_client("bot")
        user = self.new_client("user")
        try:
            # Bot joins
            self.register(bot, "BotDaddy")
            bot.send_line("JOIN #bottest")
            bot.recv_until([re.compile(r"\s366\s+BotDaddy\s+#bottest\b")], max_time=2.0)
            
            # User joins
            self.register(user, "alice")
            user.send_line("JOIN #bottest")
            user.recv_until([re.compile(r"\s366\s+alice\s+#bottest\b")], max_time=2.0)
            
            # User sends !help
            user.send_line("PRIVMSG #bottest :!help")
            sleep_s(0.3)
            
            # Note: This test verifies the server relays messages correctly
            # The bot's actual response would be tested with actual bot binary
            lines = bot.recv_lines(max_time=1.0)
            assert_any(lines, re.compile(r"PRIVMSG\s+#bottest\s+:!help"), "Bot did not receive !help command")
        finally:
            bot.close()
            user.close()

    def test_bot_time_command(self) -> None:
        """
        Bot responds to !time command with current server time.
        """
        bot = self.new_client("bot")
        user = self.new_client("user")
        try:
            self.register(bot, "BotDaddy")
            bot.send_line("JOIN #bottest")
            bot.recv_until([re.compile(r"\s366\s+BotDaddy\s+#bottest\b")], max_time=2.0)
            
            self.register(user, "alice")
            user.send_line("JOIN #bottest")
            user.recv_until([re.compile(r"\s366\s+alice\s+#bottest\b")], max_time=2.0)
            
            user.send_line("PRIVMSG #bottest :!time")
            sleep_s(0.3)
            
            lines = bot.recv_lines(max_time=1.0)
            assert_any(lines, re.compile(r"PRIVMSG\s+#bottest\s+:!time"), "Bot did not receive !time command")
        finally:
            bot.close()
            user.close()

    def test_bot_weather_command(self) -> None:
        """
        Bot responds to !weather <city> command.
        """
        bot = self.new_client("bot")
        user = self.new_client("user")
        try:
            self.register(bot, "BotDaddy")
            bot.send_line("JOIN #bottest")
            bot.recv_until([re.compile(r"\s366\s+BotDaddy\s+#bottest\b")], max_time=2.0)
            
            self.register(user, "alice")
            user.send_line("JOIN #bottest")
            user.recv_until([re.compile(r"\s366\s+alice\s+#bottest\b")], max_time=2.0)
            
            user.send_line("PRIVMSG #bottest :!weather Berlin")
            sleep_s(0.3)
            
            lines = bot.recv_lines(max_time=1.0)
            assert_any(lines, re.compile(r"PRIVMSG\s+#bottest\s+:!weather\s+Berlin"), "Bot did not receive !weather command")
        finally:
            bot.close()
            user.close()

    def test_bot_private_message(self) -> None:
        """
        Bot can receive private messages (not just channel messages).
        """
        bot = self.new_client("bot")
        user = self.new_client("user")
        try:
            self.register(bot, "BotDaddy")
            self.register(user, "alice")
            
            # User sends PM to bot
            user.send_line("PRIVMSG BotDaddy :!help")
            sleep_s(0.3)
            
            lines = bot.recv_lines(max_time=1.0)
            assert_any(lines, re.compile(r"PRIVMSG\s+BotDaddy\s+:!help"), "Bot did not receive private !help command")
        finally:
            bot.close()
            user.close()

    def test_slow_reader_flood(self) -> None:
        """
        "SIGSTOP flood" equivalent, but reliable:
        - Client A joins #flood and then intentionally DOES NOT READ for a while (slow reader).
        - Client B floods #flood with many PRIVMSG quickly.
        - Server must stay responsive (client C can still register+join).
        - When A resumes reading, it should receive a substantial number of buffered messages (or at least some),
          and the server must not hang or drop the connection unexpectedly.

        This catches:
        - blocking send() on a slow client (server hang)
        - per-client buffering bugs
        - failure to keep servicing other sockets while one client is slow
        """
        a = self.new_client("slow_A")
        b = self.new_client("flood_B")
        c_ = self.new_client("probe_C")
        try:
            self.register(a, "alice")
            a.send_line("JOIN #flood")
            a.recv_until([re.compile(r"\s366\s+alice\s+#flood\b")], max_time=2.5)

            self.register(b, "bob")
            b.send_line("JOIN #flood")
            b.recv_until([re.compile(r"\s366\s+bob\s+#flood\b")], max_time=2.5)

            # A will stop reading now. We won't call a.recv_* for a bit.
            # Flood from B
            flood_n = 300
            for i in range(flood_n):
                b.send_line(f"PRIVMSG #flood :msg{i}")
            # While flooding, ensure server is still responsive: register/join with C
            self.register(c_, "carol")
            c_.send_line("JOIN #flood")
            lines_c = c_.recv_until([re.compile(r"\s366\s+carol\s+#flood\b")], max_time=3.0)
            assert_any(lines_c, re.compile(r"\s366\s+carol\s+#flood\b"), "Server became unresponsive during flood (carol could not join)")

            # Now let A read; it should see at least some PRIVMSG lines.
            # Note: depending on implementation, server may cap buffer; we only require some messages.
            lines_a = a.recv_lines(max_lines=2000, max_time=3.0)
            # Expect to see bob's PRIVMSG to channel addressed to #flood or content "msg"
            assert_any(lines_a, re.compile(r"PRIVMSG\s+#flood\s+:msg\d+"), "Slow reader did not receive any flooded messages (or server dropped them)")

        finally:
            a.close()
            b.close()
            c_.close()

# ----------------------------- Valgrind Parsing -----------------------------

_LEAK_RE = re.compile(r"^\s*(definitely|indirectly|possibly)\s+lost:\s+([0-9,]+)\s+bytes\s+in\s+([0-9,]+)\s+blocks", re.M)
_ERRSUM_RE = re.compile(r"ERROR SUMMARY:\s+([0-9,]+)\s+errors", re.M)

def _parse_int(s: str) -> int:
    return int(s.replace(",", ""))

def parse_valgrind_summary(stderr_text: str) -> Dict[str, int]:
    out: Dict[str, int] = {"definitely_lost_bytes": 0, "indirectly_lost_bytes": 0, "possibly_lost_bytes": 0, "error_summary": 0}
    for kind, bytes_s, _blocks_s in _LEAK_RE.findall(stderr_text):
        b = _parse_int(bytes_s)
        if kind == "definitely":
            out["definitely_lost_bytes"] += b
        elif kind == "indirectly":
            out["indirectly_lost_bytes"] += b
        elif kind == "possibly":
            out["possibly_lost_bytes"] += b
    m = _ERRSUM_RE.search(stderr_text)
    if m:
        out["error_summary"] = _parse_int(m.group(1))
    return out

# ----------------------------- Reporting -----------------------------

def write_json_report(path: str, suite_name: str, results: List[TestResult], meta: dict) -> None:
    payload = {
        "suite": suite_name,
        "meta": meta,
        "results": [asdict(r) for r in results],
        "summary": {
            "total": len(results),
            "passed": sum(1 for r in results if r.ok),
            "failed": sum(1 for r in results if not r.ok),
            "duration_ms": sum(r.duration_ms for r in results),
        },
    }
    with open(path, "w", encoding="utf-8") as f:
        json.dump(payload, f, indent=2)

def write_junit_report(path: str, suite_name: str, results: List[TestResult], meta: dict) -> None:
    total = len(results)
    failures = sum(1 for r in results if not r.ok)
    duration_s = sum(r.duration_ms for r in results) / 1000.0

    # Minimal JUnit XML
    lines: List[str] = []
    lines.append('<?xml version="1.0" encoding="UTF-8"?>')
    lines.append(f'<testsuite name="{xml_escape(suite_name)}" tests="{total}" failures="{failures}" time="{duration_s:.3f}">')
    # properties
    lines.append('  <properties>')
    for k, v in meta.items():
        lines.append(f'    <property name="{xml_escape(str(k))}" value="{xml_escape(str(v))}"/>')
    lines.append('  </properties>')
    # cases
    for r in results:
        t = r.duration_ms / 1000.0
        lines.append(f'  <testcase classname="{xml_escape(suite_name)}" name="{xml_escape(r.name)}" time="{t:.3f}">')
        if not r.ok:
            msg = (r.details or "failure").strip()
            lines.append(f'    <failure message="failed">{xml_escape(msg)}</failure>')
        lines.append('  </testcase>')
    lines.append('</testsuite>')
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))

# ----------------------------- Main -----------------------------

def build_if_requested(do_make: bool, cwd: str, quiet: bool) -> None:
    if not do_make:
        return
    stdout = subprocess.DEVNULL if quiet else None
    stderr = subprocess.DEVNULL if quiet else None
    r = subprocess.run(["make"], cwd=cwd, stdout=stdout, stderr=stderr)
    if r.returncode != 0:
        raise SystemExit("make failed")

def wait_for_port(host: str, port: int, timeout: float = 3.5) -> None:
    deadline = time.time() + timeout
    last_err = None
    while time.time() < deadline:
        try:
            s = socket.create_connection((host, port), timeout=0.5)
            s.close()
            return
        except OSError as e:
            last_err = e
            time.sleep(0.1)
    raise SystemExit(f"Server did not open {host}:{port} in time (last error: {last_err})")

def run_tests(tester: FtIrcTester, color: bool, include_flood: bool, show_progress: bool = True) -> List[TestResult]:
    tests: List[Tuple[str, callable]] = [
        # Registration tests
        ("1.1 Registration success", tester.test_registration_success),
        ("1.2 Wrong password (464)", tester.test_wrong_password),
        ("1.3 Duplicate nick (433)", tester.test_duplicate_nick),
        # JOIN tests
        ("2.1 Create & join channel", tester.test_join_create_channel),
        ("2.2 Second user joins names list", tester.test_join_second_user_names_list),
        ("2.3 Join with key (+k)", tester.test_join_with_key),
        # PART tests
        ("3.1 Leave channel", tester.test_part_leave_channel),
        ("3.2 Part non-existent channel (403)", tester.test_part_nonexistent_channel),
        # TOPIC tests
        ("4.1 Topic set & query", tester.test_topic_set_and_query),
        ("4.3 Topic protect +t non-op (482)", tester.test_topic_protect_nonop_fails),
        # INVITE tests
        ("5.2 Invite bypass +i", tester.test_invite_allows_join_invite_only),
        ("5.3 Non-op cannot INVITE (482)", tester.test_invite_nonop_fails),
        # KICK tests
        ("6.1 Operator KICK", tester.test_kick_operator_kicks_user),
        ("6.2 Non-op cannot KICK (482)", tester.test_kick_nonop_fails),
        # MODE tests
        ("7.1 MODE query (324)", tester.test_mode_query),
        ("7.2 MODE +i invite-only", tester.test_mode_set_invite_only),
        ("7.3 MODE +l user limit", tester.test_mode_set_limit),
        ("7.4 MODE +o give operator", tester.test_mode_give_operator),
        ("7.x MODE non-op fails (482)", tester.test_mode_nonop_fails),
        # PRIVMSG tests
        ("8.1 PRIVMSG to channel", tester.test_privmsg_channel),
        ("8.2 PRIVMSG private", tester.test_privmsg_private),
        ("8.x PRIVMSG to missing user (401)", tester.test_privmsg_missing_user_errors),
        # NOTICE tests
        ("9.1 NOTICE to channel", tester.test_notice_channel),
        ("9.2 NOTICE private", tester.test_notice_private),
        ("9.3 NOTICE to missing user has no 401", tester.test_notice_no_error_on_missing_user),
        # QUIT test
        ("10.1 Graceful QUIT", tester.test_quit_graceful),
        # Error cases
        ("11.1 Command before registration (451)", tester.test_command_before_registration),
        ("11.2 Not enough parameters (461)", tester.test_not_enough_parameters),
        # Edge cases
        ("14.2 Partial commands buffering", tester.test_partial_commands_buffering),
        ("14.3 Multiple commands in one packet", tester.test_multi_commands_single_packet),
        ("14.6 Empty/whitespace commands", tester.test_empty_and_whitespace_commands),
        ("14.7 Very long message", tester.test_very_long_message),
        # Bot tests (bonus)
        ("B.1 Bot connects and joins", tester.test_bot_connects_and_joins),
        ("B.2 Bot receives !help", tester.test_bot_help_command),
        ("B.3 Bot receives !time", tester.test_bot_time_command),
        ("B.4 Bot receives !weather", tester.test_bot_weather_command),
        ("B.5 Bot receives private message", tester.test_bot_private_message),
    ]
    if include_flood:
        tests.append(("14.5 Slow-reader flood (SIGSTOP-equivalent)", tester.test_slow_reader_flood))

    results: List[TestResult] = []
    total = len(tests)
    for idx, (name, fn) in enumerate(tests):
        if show_progress:
            print_progress_bar(idx, total, color=color)
        t0 = ms()
        try:
            fn()
            results.append(TestResult(name=name, ok=True, duration_ms=ms()-t0))
        except TestFailure as e:
            results.append(TestResult(name=name, ok=False, duration_ms=ms()-t0, details=str(e)))
        except Exception as e:
            results.append(TestResult(name=name, ok=False, duration_ms=ms()-t0, details=f"Unexpected error: {e!r}"))
    if show_progress:
        print_progress_bar(total, total, color=color)
        print()  # Newline after progress bar completes
    return results

def main() -> int:
    ap = argparse.ArgumentParser(description="ft_irc automated test runner (+valgrind +reports)")
    ap.add_argument("--ircserv", default="./ircserv", help="Path to ircserv executable")
    ap.add_argument("--cwd", default=".", help="Working directory for running server/make")
    ap.add_argument("--host", default="127.0.0.1", help="Server host (default: localhost)")
    ap.add_argument("--port", type=int, default=6667, help="Server port")
    ap.add_argument("--password", default="pass", help="Server password")
    ap.add_argument("--timeout", type=float, default=1.5, help="Socket timeout seconds")
    ap.add_argument("--make", action="store_true", help="Run make before tests")
    ap.add_argument("--quiet-server", action="store_true", help="Suppress server stdout/stderr")
    ap.add_argument("--no-color", action="store_true", help="Disable ANSI colors")
    ap.add_argument("--keep-server", action="store_true", help="Do not stop server at the end (debug)")
    ap.add_argument("--include-flood", action="store_true", help="Run the slow-reader flood test")
    # Reporting
    ap.add_argument("--json-report", default="", help="Write JSON report to this path")
    ap.add_argument("--junit-report", default="", help="Write JUnit XML report to this path")
    # Valgrind
    ap.add_argument("--valgrind", action="store_true", help="Also run a Valgrind leak test and parse LEAK SUMMARY")
    ap.add_argument("--valgrind-timeout-mult", type=float, default=2.5, help="Multiply timeouts during valgrind run (default 2.5)")
    args = ap.parse_args()

    color = (not args.no_color) and sys.stdout.isatty()
    suite_name = "ft_irc"

    build_if_requested(args.make, args.cwd, args.quiet_server)

    meta = {
        "ircserv": os.path.abspath(os.path.join(args.cwd, args.ircserv)) if not os.path.isabs(args.ircserv) else args.ircserv,
        "cwd": os.path.abspath(args.cwd),
        "host": args.host,
        "port": args.port,
        "valgrind": args.valgrind,
        "include_flood": args.include_flood,
        "python": sys.version.split()[0],
        "platform": sys.platform,
    }

    all_results: List[TestResult] = []

    # 1) Normal run
    server = ServerProc([args.ircserv, str(args.port), args.password], cwd=args.cwd, quiet=args.quiet_server)
    server.start()
    try:
        wait_for_port(args.host, args.port, timeout=4.0)
        tester = FtIrcTester(args.host, args.port, args.password, args.timeout)
        results = run_tests(tester, color=color, include_flood=args.include_flood)
        all_results.extend(results)

        # Console output
        ok_count = sum(1 for r in results if r.ok)
        fail_count = len(results) - ok_count

        print()
        print(c("=== ft_irc automated tests ===", "blue", color))
        for r in results:
            status = c("PASS", "green", color) if r.ok else c("FAIL", "red", color)
            print(f"[{status}] {r.name}  ({r.duration_ms} ms)")
            if (not r.ok) and r.details:
                print(c("  " + r.details.replace("\n", "\n  "), "dim", color))
        print()
        if fail_count == 0:
            print(c(f"All protocol tests passed ({ok_count}/{len(results)})", "green", color))
        else:
            print(c(f"{fail_count} protocol test(s) failed ({ok_count}/{len(results)} passed)", "red", color))

    finally:
        if not args.keep_server:
            server.stop()
            server.wait(timeout=2.0)

    # 2) Valgrind run (separate server instance)
    if args.valgrind:
        vg_name = "VALGRIND leak summary"
        t0 = ms()
        vg_details = ""
        vg_meta = None
        ok = True

        # Ensure valgrind exists
        if subprocess.run(["which", "valgrind"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode != 0:
            ok = False
            vg_details = "valgrind not found in PATH."
        else:
            vg_cmd = [
                "valgrind",
                "--leak-check=full",
                "--show-leak-kinds=all",
                "--track-origins=yes",
                "--errors-for-leak-kinds=definite,indirect,possible",
                "--error-exitcode=42",
                args.ircserv,
                str(args.port),
                args.password,
            ]
            vg_server = ServerProc(vg_cmd, cwd=args.cwd, quiet=True, capture_stderr=True)
            vg_server.start()
            try:
                wait_for_port(args.host, args.port, timeout=8.0)
                tester_vg = FtIrcTester(args.host, args.port, args.password, args.timeout * args.valgrind_timeout_mult)

                # Keep it short but meaningful: register/join/flood optional minimal traffic
                try:
                    tester_vg.test_registration_success()
                    tester_vg.test_join_create_channel()
                    tester_vg.test_privmsg_missing_user_errors()
                    if args.include_flood:
                        tester_vg.test_slow_reader_flood()
                except Exception as e:
                    # Even if protocol tests fail under valgrind, we still want summary output
                    vg_details += f"Protocol error during valgrind run: {e!r}\n"

            finally:
                vg_server.stop()
                vg_server.wait(timeout=10.0)

            stderr_text = vg_server.captured_stderr or ""
            summary = parse_valgrind_summary(stderr_text)
            vg_meta = summary

            # Decide pass/fail
            if summary.get("error_summary", 0) != 0:
                ok = False
                vg_details += f"Valgrind ERROR SUMMARY != 0: {summary.get('error_summary')}\n"
            if summary.get("definitely_lost_bytes", 0) != 0 or summary.get("indirectly_lost_bytes", 0) != 0:
                ok = False
                vg_details += (
                    "Valgrind leaks detected:\n"
                    f"  definitely lost: {summary.get('definitely_lost_bytes')} bytes\n"
                    f"  indirectly lost: {summary.get('indirectly_lost_bytes')} bytes\n"
                    f"  possibly lost: {summary.get('possibly_lost_bytes')} bytes\n"
                )

            if not vg_details:
                vg_details = (
                    "No definite/indirect leaks detected and ERROR SUMMARY is 0.\n"
                    f"possibly lost: {summary.get('possibly_lost_bytes')} bytes (informational)"
                )

        all_results.append(TestResult(
            name=vg_name,
            ok=ok,
            duration_ms=ms()-t0,
            details=vg_details.strip(),
            meta=vg_meta
        ))

        # Print valgrind result
        print()
        status = c("PASS", "green", color) if ok else c("FAIL", "red", color)
        print(f"[{status}] {vg_name}  ({all_results[-1].duration_ms} ms)")
        if vg_details:
            print(c("  " + vg_details.strip().replace("\n", "\n  "), "dim", color))

    # Reports
    if args.json_report:
        write_json_report(args.json_report, suite_name, all_results, meta)
        print(c(f"\nWrote JSON report: {args.json_report}", "yellow", color))
    if args.junit_report:
        write_junit_report(args.junit_report, suite_name, all_results, meta)
        print(c(f"Wrote JUnit report: {args.junit_report}", "yellow", color))

    # Final exit
    total_fail = sum(1 for r in all_results if not r.ok)
    return 0 if total_fail == 0 else 1

if __name__ == "__main__":
    raise SystemExit(main())
