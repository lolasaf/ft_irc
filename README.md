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

See [TESTS.md](memory_bank/TESTS.md) for comprehensive test commands.

## Project Structure

```
ft_irc/
├── include/          # Header files
├── src/              # Source files
├── memory_bank/      # Documentation & planning
│   ├── PROGRESS.md   # Detailed progress tracker
│   └── TESTS.md      # Test commands
└── Makefile
```

## Documentation

- [PROGRESS.md](memory_bank/PROGRESS.md) — Detailed progress tracker with all completed work
- [TESTS.md](memory_bank/TESTS.md) — Test commands for all features
- [Knowledge_base.md](memory_bank/Knowledge_base.md) — IRC protocol reference