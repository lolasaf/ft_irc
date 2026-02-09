*This project has been created as part of the 42 curriculum by wel-safa.*

# ft_irc — Internet Relay Chat Server

## Description

**ft_irc** is a fully functional IRC (Internet Relay Chat) server implemented in C++98. The project demonstrates mastery of network programming, non-blocking I/O, and the IRC protocol specification (RFC 1459/2812).

### Goal

Create an IRC server capable of handling multiple simultaneous client connections, supporting real-time text communication through channels and private messages, while implementing proper user authentication, channel management, and operator privileges.

### Overview

The server uses a single-threaded event-driven architecture built around `poll()` for scalable I/O multiplexing. It supports all mandatory IRC commands required for basic chat functionality, along with bonus features including an automated IRC bot and DCC file transfer relay capabilities.

**Key characteristics:**
- Non-blocking socket operations for all network I/O
- Per-client input/output buffers handling partial TCP segments
- Channel operator privileges with comprehensive mode support
- Reference client compatibility (HexChat, irssi, WeeChat)
- Memory-safe implementation verified with Valgrind

## Instructions

### Prerequisites

- C++ compiler with C++98 support (g++, clang++)
- GNU Make
- Python 3.6+ (for automated testing, optional)
- Valgrind (for memory checking, optional)

### Compilation

```bash
# Build the IRC server
make

# Build the bonus IRC bot
make bonus

# Clean object files
make clean

# Clean all generated files
make fclean

# Rebuild from scratch
make re
```

### Installation

No installation required. The compilation produces standalone executables in the project root:
- `ircserv` — The IRC server
- `ircbot` — The IRC bot (bonus)

### Execution

#### Starting the Server

```bash
./ircserv <port> <password>
```

| Parameter  | Description |
|------------|-------------|
| `port`     | Port number to listen on (1024-65535 recommended) |
| `password` | Server connection password (required for all clients) |

**Example:**
```bash
./ircserv 6667 mypassword
```

#### Connecting with an IRC Client

Configure your IRC client (HexChat, irssi, WeeChat) with:
- **Server:** `localhost` (or server IP)
- **Port:** `6667` (or your chosen port)
- **Password:** `mypassword` (your server password)

#### Connecting with netcat (Testing)

```bash
nc localhost 6667
PASS mypassword
NICK alice
USER alice 0 * :Alice Smith
JOIN #general
PRIVMSG #general :Hello everyone!
QUIT :Goodbye
```

#### Running the Bot (Bonus)

```bash
./ircbot <host> <port> <password> <channel>
```

**Example:**
```bash
./ircbot 127.0.0.1 6667 mypassword "#bot"
```

### Testing

```bash
# Run automated test suite
python3 ft_irc_tester.py

# Build and test
python3 ft_irc_tester.py --make

# Test with memory leak detection
python3 ft_irc_tester.py --make --valgrind

# Include stress tests
python3 ft_irc_tester.py --include-flood
```

## Features

### Supported Commands

| Command | Syntax | Description |
|---------|--------|-------------|
| `PASS` | `PASS <password>` | Authenticate with server |
| `NICK` | `NICK <nickname>` | Set or change nickname |
| `USER` | `USER <user> 0 * :<realname>` | Complete registration |
| `JOIN` | `JOIN <#channel> [key]` | Join a channel |
| `PART` | `PART <#channel> [message]` | Leave a channel |
| `PRIVMSG` | `PRIVMSG <target> :<message>` | Send message to user/channel |
| `NOTICE` | `NOTICE <target> :<message>` | Send notice (no auto-reply) |
| `TOPIC` | `TOPIC <#channel> [topic]` | View or set channel topic |
| `KICK` | `KICK <#channel> <user> [reason]` | Remove user (operator only) |
| `INVITE` | `INVITE <user> <#channel>` | Invite user (operator only) |
| `MODE` | `MODE <#channel> <modes> [params]` | Set channel modes (operator only) |
| `QUIT` | `QUIT [message]` | Disconnect from server |

### Channel Modes

| Mode | Parameter | Description |
|------|-----------|-------------|
| `+i` | — | Invite-only channel |
| `+t` | — | Only operators can change topic |
| `+k` | `<key>` | Channel requires password to join |
| `+l` | `<limit>` | Maximum number of users |
| `+o` | `<nick>` | Grant/revoke operator status |

### Bonus Features

**IRC Bot (`ircbot`):**
- `!help` — Display available commands
- `!time` — Show current server time
- `!weather <city>` — Display mock weather information
- Automatic greeting on user joins
- Logging to `bot.log`

**DCC File Transfer:**
- Server correctly relays CTCP messages between clients
- Preserves `\x01` markers required for DCC protocol
- Supports DCC SEND and DCC CHAT handshakes

## Technical Choices

### Architecture

- **Event Loop:** Single `poll()` call monitors all file descriptors
- **Non-blocking I/O:** All sockets set to `O_NONBLOCK` via `fcntl()`
- **Buffering:** Ring buffers handle TCP stream fragmentation
- **No Threads:** Fully single-threaded design avoids race conditions

### Design Decisions

| Decision | Rationale |
|----------|-----------|
| Single poll() | Subject requirement; efficient for moderate connections |
| Per-client buffers | Handle partial commands across TCP segments |
| Case-insensitive channels | IRC standard compliance |
| 9-character nick limit | Traditional IRC compatibility |

### Error Handling

The server implements comprehensive IRC numeric replies:
- `ERR_NEEDMOREPARAMS (461)` — Missing command parameters
- `ERR_NOTREGISTERED (451)` — Command before authentication
- `ERR_PASSWDMISMATCH (464)` — Wrong server password
- `ERR_NICKNAMEINUSE (433)` — Nickname already taken
- `ERR_CHANOPRIVSNEEDED (482)` — Operator privilege required

## Resources

### Documentation & References

- [RFC 1459 — Internet Relay Chat Protocol](https://datatracker.ietf.org/doc/html/rfc1459) — Original IRC specification
- [RFC 2812 — IRC Client Protocol](https://datatracker.ietf.org/doc/html/rfc2812) — Updated IRC specification
- [Modern IRC Documentation](https://modern.ircdocs.horse/) — Practical IRC implementation guide
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/) — Socket programming fundamentals
- [poll(2) man page](https://man7.org/linux/man-pages/man2/poll.2.html) — Linux poll() system call

### AI Usage Disclosure

AI assistance (Claude/GitHub Copilot) was utilized in the following areas:

| Task | AI Contribution |
|------|-----------------|
| **Documentation** | README files, test documentation, evaluation cheatsheet |
| **Test Suite** | Python automated tester (`ft_irc_tester.py`) structure and test cases |
| **Code Review** | Identifying edge cases, RFC compliance verification |
| **Debugging** | Analyzing protocol-level issues, buffer handling edge cases |

**Core implementation** (server logic, command handlers, channel management, socket handling) was written by the project author with AI serving as a reference and review tool.

### Project Documentation

- [TESTS.md](Tests/TESTS.md) — Comprehensive manual test commands
- [EVALUATION_CHEATSHEET.md](EVALUATION_CHEATSHEET.md) — Quick reference for peer evaluation
- [Knowledge_base.md](memory_bank/Knowledge_base.md) — IRC protocol reference notes
- [PROGRESS.md](memory_bank/PROGRESS.md) — Development progress tracker

## File Structure

```
ft_irc/
├── src/                    # Source files
│   ├── main.cpp           # Entry point
│   ├── server.cpp         # Server core & event loop
│   ├── serverCommands.cpp # Command handlers
│   ├── channel.cpp        # Channel management
│   ├── user.cpp           # User/client state
│   └── ...                # Additional modules
├── include/               # Header files
│   ├── server.hpp
│   ├── channel.hpp
│   └── ...
├── bonus/                 # Bot source files
├── Tests/                 # Test documentation
├── memory_bank/           # Project documentation
├── Makefile
├── ft_irc_tester.py       # Automated test suite
└── IRC_README.md          # This file
```

## License

This project is developed for educational purposes as part of the 42 school curriculum.
