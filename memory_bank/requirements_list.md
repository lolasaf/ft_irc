# ft_irc — Requirements Checklist

> **Project goal:**  
> Build a fully functional Internet Relay Chat (IRC) server in **C++ 98**, capable of handling multiple simultaneous clients through **non-blocking sockets** and a single **poll()/select()** event loop.  
> The server must behave like a minimal real IRC server and be testable with a real IRC client.

---

## 🧱 1. General Rules

| Requirement | Status | Notes |
|--------------|--------|-------|
| Program never crashes or quits unexpectedly | ☐ | Even on memory exhaustion |
| Must compile with `c++ -Wall -Wextra -Werror -std=c++98` | ☐ | |
| Must provide a **Makefile** with rules: `NAME`, `all`, `clean`, `fclean`, `re` | ☐ | No unnecessary relinking |
| No external libraries (including Boost) | ☐ | Only system headers allowed |
| Use C++ equivalents over C (e.g., `<cstring>` vs `<string.h>`) | ☐ | |
| Forbidden to fork() | ☐ | Single-process event loop only |
| All file descriptors must be **non-blocking** | ☐ | |
| Only **one** poll()/select()/epoll()/kqueue() allowed | ☐ | Must handle read, write, listen, accept |

---

## ⚙️ 2. Program Setup

| Item | Specification |
|------|----------------|
| Executable name | `ircserv` |
| Invocation | `./ircserv <port> <password>` |
| Arguments | `port`: TCP listening port<br>`password`: connection password for clients |
| Allowed system functions | `socket`, `close`, `setsockopt`, `getsockname`, `getprotobyname`, `gethostbyname`, `getaddrinfo`, `freeaddrinfo`, `bind`, `connect`, `listen`, `accept`, `htons`, `htonl`, `ntohs`, `ntohl`, `inet_addr`, `inet_ntoa`, `inet_ntop`, `send`, `recv`, `signal`, `sigaction`, `sigemptyset`, `sigfillset`, `sigaddset`, `sigdelset`, `sigismember`, `lseek`, `fstat`, `fcntl`, `poll` (or equivalent) |
| For macOS | Only `fcntl(fd, F_SETFL, O_NONBLOCK)` is allowed for non-blocking setup |

---

## 💻 3. Mandatory Features

| Feature | Description | Status |
|----------|--------------|--------|
| **Multi-client support** | Server can handle many clients simultaneously without blocking | ☐ |
| **TCP/IP communication** | Works with IPv4 or IPv6 | ☐ |
| **Non-blocking I/O** | Implemented using poll()/select() | ☐ |
| **Authentication** | Clients must provide correct password | ☐ |
| **Nickname management** | Handle `NICK` command and unique nickname enforcement | ☐ |
| **Username setup** | Handle `USER` command | ☐ |
| **Channel system** | Clients can `JOIN` and `PART` channels | ☐ |
| **Message broadcasting** | Messages sent to a channel are delivered to all members | ☐ |
| **Private messaging** | Support `PRIVMSG` between users and within channels | ☐ |
| **Operators** | Channel operators exist separately from regular users | ☐ |
| **Operator commands** | Implement the following with correct behavior: | |
| → `KICK` | Remove user from a channel | ☐ |
| → `INVITE` | Invite user to a channel | ☐ |
| → `TOPIC` | Change/view channel topic | ☐ |
| → `MODE` | Manage channel modes (see below) | ☐ |

---

### Channel Modes (`MODE` command options)

| Mode | Meaning | Status |
|------|----------|--------|
| `i` | Invite-only toggle | ☐ |
| `t` | Topic change restricted to operators | ☐ |
| `k` | Channel key (password) management | ☐ |
| `o` | Grant/revoke operator status | ☐ |
| `l` | User limit setting | ☐ |

---

## 🧠 4. Behavioral Expectations

| Requirement | Status | Notes |
|--------------|--------|-------|
| Server properly aggregates partial TCP packets into full commands | ☐ | Must rebuild messages before processing |
| Server correctly handles malformed or incomplete input | ☐ | |
| Reference IRC client connects successfully | ☐ | Choose one (e.g., `irssi`, `WeeChat`, or `netcat`) |
| Behavior mimics real IRC protocol (RFC 1459 subset) | ☐ | Minimal compliance sufficient |
| Server replies with correct numeric reply codes | ☐ | Recommended but not explicitly required |
| Clean modular code following C++ design principles | ☐ | Separate headers/classes |

---

## 🧪 5. Testing and Validation

| Test | Description | Status |
|------|--------------|--------|
| **Connection test** | Client can connect via `<port>` and `<password>` | ☐ |
| **Partial message test** | Send command split into parts (`nc -C 127.0.0.1 6667`) | ☐ |
| **Low bandwidth test** | Verify handling of delayed or fragmented data | ☐ |
| **Multiple clients test** | Ensure simultaneous messaging works | ☐ |
| **Crash-resistance test** | Server remains stable under stress | ☐ |
| **Peer evaluation readiness** | Can explain all parts of the implementation | ☐ |

---

## ⭐ 6. Bonus Part (Only if Mandatory is Perfect)

| Bonus | Description | Status |
|--------|-------------|--------|
| File transfer | Implement DCC-like file send/receive | ☐ |
| IRC bot | Add automated bot for messages or moderation | ☐ |

> ⚠️ The bonus is evaluated **only** if every mandatory requirement is fully met and working perfectly.

---

## 📦 7. Submission

- Submit your work to your Git repository (`intra` project repo).  
- Verify filenames, Makefile, and header guards before pushing.  
- Be ready during defense to:
  - Make small live changes or fixes.  
  - Explain any part of your code and logic clearly.  

---

## ✅ Progress Tracker

| Section | Completion % | Notes |
|----------|---------------|-------|
| General rules | 0% |  |
| Setup | 0% |  |
| Mandatory features | 0% |  |
| Behavioral tests | 0% |  |
| Bonus | 0% |  |
| Documentation | 0% |  |

---

> 🧩 Tip: Review this file weekly and use it with your AI agent to evaluate what’s missing, e.g.  
> “Check my current ft_irc repo and compare with 02_requirements_checklist.md — which requirements are still incomplete?”
