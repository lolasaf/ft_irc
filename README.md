# ft_irc

A C++98 IRC server implementation for 42 school.

## Status

✅ **Mandatory Part Complete** — All required features implemented and tested.
✅ **Bonus: IRC Bot Complete** — Bot with commands, greeting, and logging.
✅ **Bonus: File Transfer** — DCC relay supported for client-to-client transfers.

## Features

### Commands Implemented
| Category | Commands |
|----------|----------|
| Registration | `PASS`, `NICK`, `USER` |
| Channels | `JOIN`, `PART`, `TOPIC`, `KICK`, `INVITE` |
| Messaging | `PRIVMSG`, `NOTICE` |
| Modes | `MODE` (+i, +t, +k, +l, +o) |
| Connection | `QUIT` |

### Technical Highlights
- **Non-blocking I/O** with `poll()` event loop
- **Buffered input/output** — handles partial reads/writes correctly
- **Protocol injection prevention** — CR/LF sanitization on all user input
- **Memory safe** — Valgrind verified, 0 leaks
- **C++98 compliant** — compiles with `-std=c++98 -Wall -Wextra -Werror`

### Bonus: IRC Bot
| Feature | Description |
|---------|-------------|
| `!help` | Lists all available bot commands |
| `!time` | Shows current server time |
| `!weather <city>` | Mock weather data for city |
| Greeting | Welcomes users when they JOIN |
| Logging | Logs all channel messages to `bot.log` |

### Bonus: File Transfer (DCC)
- Server relays DCC SEND/ACCEPT messages between clients
- CTCP markers (`\x01`) preserved intact
- Actual file transfer happens directly between clients (peer-to-peer)
- Compatible with IRC clients like HexChat, irssi, WeeChat

## Build & Run

```bash
# Build the server
make

# Run the server (port 6667, password "secret")
./ircserv 6667 secret

# Connect with netcat for testing
nc localhost 6667

# Or use an IRC client (HexChat, irssi, etc.)
```

### Bot (Bonus)
```bash
# Build the bot
make bonus

# Run the bot ./ircbot <server> <port> <password> <channel>
./ircbot 127.0.0.1 6667 secret "#bot"

# Bot will join #bot channel and respond to commands
```

## Testing

### Automated Testing
```bash
# Run all tests (builds first)
python3 ft_irc_tester.py --make

# Run with Valgrind memory leak check
python3 ft_irc_tester.py --make --valgrind

# Run bot tests only (requires bot built)
python3 ft_irc_tester.py --make-bonus --categories bot
```

See [ft_irc_tester_instructions.md](Tests/ft_irc_tester_instructions.md) for full tester documentation.

### Manual Testing
See [TESTS.md](Tests/TESTS.md) for comprehensive manual test commands.

## Project Structure

```
ft_irc/
├── include/              # Header files (7 files)
│   ├── bot.hpp           # Bot class (bonus)
│   └── ...               # Server, channel, user, etc.
├── src/                  # Source files (19 files)
│   ├── bot*.cpp          # Bot (bonus, 2 files)
│   ├── channel*.cpp      # Channel class (5 files)
│   ├── server*.cpp       # Server class (7 files)
│   └── ...               # Utils, user, message
├── memory_bank/          # Documentation & planning
├── Tests/                # Test documentation
│   ├── TESTS.md          # Manual test commands (16 sections)
│   └── ft_irc_tester_instructions.md
├── ft_irc_tester.py      # Automated test runner (37 tests)
└── Makefile              # Build rules (ircserv + bonus)
```

## Documentation

- [PROGRESS.md](memory_bank/PROGRESS.md) — Detailed progress tracker with all completed work
- [TESTS.md](Tests/TESTS.md) — Test commands for all features
- [ft_irc_tester_instructions.md](Tests/ft_irc_tester_instructions.md) — Automated tester guide
- [Knowledge_base.md](memory_bank/Knowledge_base.md) — IRC protocol reference