# ft_irc

A C++98 IRC server implementation for 42 school.

## Status

✅ **Mandatory Part Complete** — All required features implemented and tested.

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

## Testing

### Automated Testing
```bash
# Run all tests (builds first)
python3 ft_irc_tester.py --make

# Run with Valgrind memory leak check
python3 ft_irc_tester.py --make --valgrind
```

See [ft_irc_tester_instructions.md](Tests/ft_irc_tester_instructions.md) for full tester documentation.

### Manual Testing
See [TESTS.md](Tests/TESTS.md) for comprehensive manual test commands.

## Project Structure

```
ft_irc/
├── include/              # Header files
├── src/                  # Source files (17 files)
│   ├── channel*.cpp      # Channel class (5 files)
│   ├── server*.cpp       # Server class (7 files)
│   └── ...               # Utils, user, message
├── memory_bank/          # Documentation & planning
├── Tests/                # Test documentation
│   ├── TESTS.md          # Manual test commands
│   └── ft_irc_tester_instructions.md
├── ft_irc_tester.py      # Automated test runner
└── Makefile
```

## Documentation

- [PROGRESS.md](memory_bank/PROGRESS.md) — Detailed progress tracker with all completed work
- [TESTS.md](Tests/TESTS.md) — Test commands for all features
- [ft_irc_tester_instructions.md](Tests/ft_irc_tester_instructions.md) — Automated tester guide
- [Knowledge_base.md](memory_bank/Knowledge_base.md) — IRC protocol reference