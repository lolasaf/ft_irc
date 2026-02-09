# ft_irc

An IRC server implementation in C++98.

## About

ft_irc is an Internet Relay Chat server that handles multiple clients simultaneously using non-blocking I/O with `poll()`. The server implements the core IRC protocol allowing users to authenticate, join channels, send messages, and manage channel modes.

**Reference client:** HexChat / irssi / WeeChat

## Build

```bash
make          # Build server
make bonus    # Build bot (bonus)
make clean    # Remove objects
make fclean   # Remove all
make re       # Rebuild
```

## Usage

```bash
./ircserv <port> <password>
```

**Example:**
```bash
./ircserv 6667 mypassword
```

**Connect with nc:**
```bash
nc localhost 6667
PASS mypassword
NICK alice
USER alice 0 * :Alice Smith
JOIN #general
PRIVMSG #general :Hello!
```

**Connect with IRC client:**
```
Server: localhost
Port: 6667
Password: mypassword
```

## Features

### Commands
| Command | Description |
|---------|-------------|
| PASS | Authenticate with server password |
| NICK | Set/change nickname |
| USER | Set username and realname |
| JOIN | Join a channel |
| PART | Leave a channel |
| PRIVMSG | Send message to user/channel |
| NOTICE | Send notice (no auto-reply) |
| TOPIC | View/set channel topic |
| KICK | Remove user from channel (op) |
| INVITE | Invite user to channel (op) |
| MODE | Set channel/user modes (op) |
| QUIT | Disconnect from server |

### Channel Modes
| Mode | Description |
|------|-------------|
| +i | Invite-only channel |
| +t | Topic restricted to operators |
| +k | Channel requires key (password) |
| +l | User limit |
| +o | Grant/revoke operator status |

## Bonus

### IRC Bot
```bash
./ircbot <host> <port> <password> <channel>
./ircbot 127.0.0.1 6667 mypassword "#bot"
```

**Bot commands:**
- `!help` — List commands
- `!time` — Show server time
- `!weather <city>` — Mock weather info

### File Transfer
DCC file transfer supported (server relays CTCP messages).

## Testing

```bash
# Automated tests
python3 ft_irc_tester.py --make

# With memory check
python3 ft_irc_tester.py --make --valgrind

# Manual tests
cat Tests/TESTS.md

# Evaluation reference
cat EVALUATION_CHEATSHEET.md
```

## Technical Details

- **I/O Model:** Single `poll()` event loop, non-blocking sockets
- **Buffering:** Per-client input/output buffers for partial packet handling
- **Memory:** Valgrind verified, 0 leaks
- **Security:** CR/LF injection prevention on all user input
- **Standard:** C++98 compliant (`-std=c++98 -Wall -Wextra -Werror`)

## Files

```
├── src/           # Source files (19 files)
├── include/       # Headers (7 files)
├── Tests/         # Test documentation
├── memory_bank/   # Project documentation
├── Makefile
└── ft_irc_tester.py
```

## Authors

- wel-safa

## Documentation

- [PROGRESS.md](memory_bank/PROGRESS.md) — Detailed progress tracker with all completed work
- [TESTS.md](Tests/TESTS.md) — Test commands for all features
- [ft_irc_tester_instructions.md](Tests/ft_irc_tester_instructions.md) — Automated tester guide
- [Knowledge_base.md](memory_bank/Knowledge_base.md) — IRC protocol reference