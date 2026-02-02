# ft_irc Automated Tester Guide

An automated test runner for the ft_irc project that tests IRC protocol compliance, handles edge cases, and includes optional Valgrind memory leak detection.

---

## Quick Start

```bash
# From project root directory
cd /path/to/ft_irc

# Run all tests (builds first)
python3 ft_irc_tester.py --make

# Run tests without rebuilding
python3 ft_irc_tester.py
```

---

## Command Line Options

| Option            | Description                           | Default     |
|-------------------|---------------------------------------|-------------|
| `--make`          | Run `make` before tests               | Off         |
| `--ircserv PATH`  | Path to ircserv executable            | `./ircserv` |
| `--port PORT`     | Server port                           | `6667`      |
| `--password PASS` | Server password                       | `pass`      |
| `--host HOST`     | Server host                           | `127.0.0.1` |
| `--timeout SEC`   | Socket timeout                        | `1.5`       |
| `--quiet-server`  | Suppress server output                | Off         |
| `--no-color`      | Disable ANSI colors                   | Off         |
| `--keep-server`   | Don't stop server after tests (debug) | Off         |

### Special Test Options

| Option                    | Description                                      |
|---------------------------|--------------------------------------------------|
| `--include-flood`         | Run slow-reader flood test (SIGSTOP equivalent)  |
| `--valgrind`              | Run Valgrind leak check after normal tests       |
| `--valgrind-timeout-mult` | Multiply timeouts during valgrind (default: 2.5) |

### Report Options

| Option | Description |
|--------|-------------|
| `--json-report FILE` | Write JSON report to file |
| `--junit-report FILE` | Write JUnit XML report (for CI) |

---

## Usage Examples

### Basic Testing

```bash
# Standard test run
python3 ft_irc_tester.py --make

# Test with custom port/password
python3 ft_irc_tester.py --port 6668 --password mypass

# Quiet mode (no server output)
python3 ft_irc_tester.py --make --quiet-server
```

### Memory Leak Testing

```bash
# Run with Valgrind leak check
python3 ft_irc_tester.py --valgrind

# Valgrind with increased timeouts (for slow machines)
python3 ft_irc_tester.py --valgrind --valgrind-timeout-mult 4.0

# Full test suite including flood + valgrind
python3 ft_irc_tester.py --make --include-flood --valgrind
```

### CI/Automated Testing

```bash
# Generate reports for CI
python3 ft_irc_tester.py --make --valgrind \
    --json-report results.json \
    --junit-report results.xml

# Check exit code (0 = all pass, 1 = failures)
python3 ft_irc_tester.py --make && echo "All tests passed!"
```

### Debugging

```bash
# Keep server running after tests (for manual inspection)
python3 ft_irc_tester.py --keep-server

# Then connect manually:
nc localhost 6667
```

### Bot Testing (Bonus)

```bash
# Build the bot
make bonus

# Start server in one terminal
./ircserv 6667 pass

# Start bot in another terminal
./ircbot 127.0.0.1 6667 pass "#test"

# Test bot manually with nc in a third terminal
nc localhost 6667
PASS pass
NICK alice
USER alice 0 * :Alice
JOIN #test
PRIVMSG #test :!help
PRIVMSG #test :!time
PRIVMSG #test :!weather Berlin
PRIVMSG BotDaddy :!help
QUIT

# Quick automated bot test
python3 << 'EOF'
import socket, time
s = socket.socket()
s.connect(('127.0.0.1', 6667))
s.settimeout(3)
s.send(b'PASS pass\r\nNICK tester\r\nUSER tester 0 * :Test\r\n')
time.sleep(0.5)
s.send(b'JOIN #test\r\n')
time.sleep(1)
s.send(b'PRIVMSG #test :!help\r\n')
time.sleep(1)
data = s.recv(4096).decode()
print("PASS" if "Available commands" in data else "FAIL")
s.close()
EOF
```

---

## Test Coverage

The tester covers **37 tests** across these categories:

### 1. Registration (Tests 1.1-1.3)
- ✅ Successful registration (PASS, NICK, USER → 001)
- ✅ Wrong password (464 ERR_PASSWDMISMATCH)
- ✅ Duplicate nickname (433 ERR_NICKNAMEINUSE)

### 2. JOIN Command (Tests 2.1-2.3)
- ✅ Create and join channel (becomes operator)
- ✅ Second user joins, sees names list
- ✅ Join with key (+k mode)

### 3. PART Command (Tests 3.1-3.2)
- ✅ Leave channel with message
- ✅ Part non-existent channel (403)

### 4. TOPIC Command (Tests 4.1, 4.3)
- ✅ Set and query topic
- ✅ Non-operator blocked by +t (482)

### 5. INVITE Command (Tests 5.2-5.3)
- ✅ Invite bypasses +i mode
- ✅ Non-operator cannot invite (482)

### 6. KICK Command (Tests 6.1-6.2)
- ✅ Operator kicks user
- ✅ Non-operator cannot kick (482)

### 7. MODE Command (Tests 7.1-7.x)
- ✅ Query channel modes (324)
- ✅ Set invite-only (+i)
- ✅ Set user limit (+l)
- ✅ Give operator (+o)
- ✅ Non-operator mode change blocked (482)

### 8. PRIVMSG Command (Tests 8.1-8.x)
- ✅ Channel message
- ✅ Private message
- ✅ Message to non-existent user (401)

### 9. NOTICE Command (Tests 9.1-9.3)
- ✅ Channel notice
- ✅ Private notice
- ✅ Notice to non-existent user (no error per RFC)

### 10. QUIT Command (Test 10.1)
- ✅ Graceful quit with broadcast

### 11. Error Cases (Tests 11.1-11.2)
- ✅ Command before registration (451)
- ✅ Not enough parameters (461)

### 14. Edge Cases (Tests 14.2-14.7)
- ✅ Partial commands (byte-by-byte)
- ✅ Multiple commands in single packet
- ✅ Empty/whitespace commands
- ✅ Very long messages
- ✅ Slow-reader flood test (optional, `--include-flood`)

### B. Bot Tests (Tests B.1-B.5)
- ✅ Bot connects and joins channel
- ✅ Bot receives !help command
- ✅ Bot receives !time command
- ✅ Bot receives !weather command
- ✅ Bot receives private messages

### Valgrind Test
- ✅ Memory leak detection (definitely/indirectly lost = 0)

---

## Understanding Output

### Successful Run

```
=== ft_irc automated tests ===
[PASS] 1.1 Registration success  (502 ms)
[PASS] 1.2 Wrong password (464)  (1502 ms)
[PASS] 1.3 Duplicate nick (433)  (2004 ms)
...
[PASS] B.1 Bot connects and joins  (1002 ms)
[PASS] B.2 Bot receives !help  (3305 ms)
...
All protocol tests passed (37/37)
```

### Failed Test

```
[FAIL] 2.2 Second user joins names list  (4508 ms)
  Expected names list containing both alice and bob
  Collected:
  :bob!bob@* JOIN #test
  :SugarDaddyFinderIRC 353 bob = #test :bob @alice
  :SugarDaddyFinderIRC 366 bob #test :End of /NAMES list
```

The "Collected" section shows what the server actually sent, helping you debug.

### Valgrind Output

```
[PASS] VALGRIND leak summary  (5234 ms)
  No definite/indirect leaks detected and ERROR SUMMARY is 0.
  possibly lost: 0 bytes (informational)
```

---

## Troubleshooting

### "Server did not open port in time"

```bash
# Kill any existing server
pkill -9 ircserv

# Try again
python3 ft_irc_tester.py --make
```

### Tests timing out

```bash
# Increase socket timeout
python3 ft_irc_tester.py --timeout 3.0

# For valgrind (already slower)
python3 ft_irc_tester.py --valgrind --valgrind-timeout-mult 5.0
```

### "make failed"

```bash
# Check compilation manually
make clean && make

# Then run without --make
python3 ft_irc_tester.py
```

### Debugging a specific failure

```bash
# Keep server running
python3 ft_irc_tester.py --keep-server

# Connect manually and reproduce
nc localhost 6667
PASS pass
NICK testuser
USER testuser 0 * :Test
JOIN #test
# ... test the failing command
```

---

## Exit Codes

| Code | Meaning |
|------|---------|
| `0` | All tests passed |
| `1` | One or more tests failed |

---

## Requirements

- Python 3.6+
- No external dependencies (uses only stdlib)
- `valgrind` in PATH (only if using `--valgrind`)

---

## See Also

- [TESTS.md](../memory_bank/TESTS.md) — Manual test commands
- [ft_irc_evaluation.md](../memory_bank/ft_irc_evaluation.md) — Evaluation criteria
