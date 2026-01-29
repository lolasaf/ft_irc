# IRC Bot Documentation

## Overview

The IRC Bot is a bonus feature for the ft_irc project. It's a standalone C++98 program that connects to the IRC server as a regular client and responds to user commands in channels or private messages.

## Files

| File | Description |
|------|-------------|
| `include/bot.hpp` | Bot class header with all declarations |
| `src/bot.cpp` | Bot class implementation |
| `src/botMain.cpp` | Entry point with argument parsing |

## Building

```bash
# Build bot only
make bot

# Build server and bot together
make bonus

# Clean everything including bot
make fclean
```

Output binary: `ircbot`

## Usage

```bash
./ircbot <server> <port> <password> [options]
```

### Required Arguments

| Argument | Description |
|----------|-------------|
| `server` | Hostname or IP address of IRC server |
| `port` | Port number (1-65535) |
| `password` | Server password for PASS command |

### Optional Arguments

| Option | Description | Default |
|--------|-------------|---------|
| `-n <nick>` | Bot's nickname | `ircbot` |
| `-c <channel>` | Channel to join (can be repeated) | none |
| `-p <prefix>` | Command prefix | `!` |
| `-h, --help` | Show help message | - |

### Examples

```bash
# Basic usage - connect and join #general
./ircbot localhost 6667 mypassword -c general

# Custom nickname and multiple channels
./ircbot localhost 6667 mypassword -n MyBot -c help -c support

# Custom command prefix
./ircbot 192.168.1.100 6667 pass123 -n HelpBot -c main -p .
```

## Bot Commands

All commands are prefixed with the command prefix (default: `!`).

| Command | Description | Example |
|---------|-------------|---------|
| `!help` | List available commands | `!help` |
| `!time` | Show current UTC time | `!time` |
| `!ping` | Check if bot is alive | `!ping` |
| `!roll [N]` | Roll random 1-N (default 6) | `!roll 20` |
| `!users` | Info about channel users | `!users` |

### Command Response Targets

- **Channel message** (`PRIVMSG #channel :!help`): Bot replies to the channel
- **Private message** (`PRIVMSG BotNick :!help`): Bot replies to the sender directly

## Architecture

### Class Structure

```
Bot
├── Connection
│   ├── _socket          - TCP socket fd
│   ├── _server          - Server hostname
│   ├── _port            - Server port
│   └── _password        - Server password
├── Identity
│   ├── _nickname        - Bot's nick
│   ├── _username        - Bot's username
│   └── _realname        - Bot's realname
├── Configuration
│   ├── _channels        - Channels to join
│   └── _commandPrefix   - Command trigger prefix
├── State
│   ├── _inputBuffer     - Incoming data buffer
│   ├── _running         - Main loop flag
│   ├── _registered      - Registration complete
│   └── _lastCommandTime - Rate limit tracking
└── Methods
    ├── Connection management
    ├── IRC protocol
    ├── Message processing
    └── Command handlers
```

### Connection Flow

```
1. connectToServer()
   └── getaddrinfo() → socket() → connect()

2. Registration
   ├── sendPass()  → "PASS <password>"
   ├── sendNick()  → "NICK <nickname>"
   └── sendUser()  → "USER <user> 0 * :<realname>"

3. Wait for RPL_WELCOME (001)
   └── joinChannels() → "JOIN #channel" for each

4. Main Loop (run())
   ├── poll() for incoming data
   ├── recv() → _inputBuffer
   ├── Extract complete lines (\r\n)
   └── processLine() each message
```

### Message Processing

```
processLine(line)
├── Parse: [:prefix] command [params] [:trailing]
├── Handle PING → sendPong()
├── Handle 001 (welcome) → mark registered, join channels
├── Handle 433 (nick in use) → append "_" and retry
└── Handle PRIVMSG → handlePrivmsg()
    └── If isCommand() → handleCommand()
```

### Rate Limiting

The bot implements a simple rate limiter to prevent spam:
- Minimum 1 second between command responses
- Uses `_lastCommandTime` to track last response
- `rateLimitOk()` checks and updates timestamp

## IRC Protocol Used

### Commands Sent

| Command | Format | When |
|---------|--------|------|
| PASS | `PASS <password>` | On connect |
| NICK | `NICK <nickname>` | On connect, nick collision |
| USER | `USER <user> 0 * :<real>` | On connect |
| JOIN | `JOIN <channel>` | After registration |
| PRIVMSG | `PRIVMSG <target> :<msg>` | Command responses |
| PONG | `PONG :<token>` | Reply to PING |

### Replies Handled

| Code | Name | Action |
|------|------|--------|
| 001 | RPL_WELCOME | Mark registered, join channels |
| 433 | ERR_NICKNAMEINUSE | Append "_" to nick, retry |
| PING | - | Reply with PONG |
| PRIVMSG | - | Check for commands |

## Signal Handling

The bot handles graceful shutdown:
- `SIGINT` (Ctrl+C): Stops main loop
- `SIGTERM`: Stops main loop

Global pointer `g_bot` allows signal handler to call `bot->stop()`.

## Error Handling

| Scenario | Behavior |
|----------|----------|
| Connection failed | Exit with error message |
| Server closes connection | Exit main loop |
| recv() error | Exit main loop |
| poll() error | Continue (if EINTR) or exit |
| Nickname in use | Append "_" and retry |

## Code Examples

### Adding a New Command

1. Add declaration in `bot.hpp`:
```cpp
void cmdNewCommand(const std::string& replyTo, const std::string& args);
```

2. Implement in `bot.cpp`:
```cpp
void Bot::cmdNewCommand(const std::string& replyTo, const std::string& args)
{
    // Your logic here
    sendPrivmsg(replyTo, "Response message");
}
```

3. Add to `handleCommand()`:
```cpp
else if (cmd == "newcommand")
    cmdNewCommand(target, args);
```

### Sending Messages

```cpp
// Send to channel
sendPrivmsg("#channel", "Hello channel!");

// Send to user
sendPrivmsg("username", "Hello user!");

// Send raw IRC command
sendRaw("JOIN #newchannel");
```

## Compilation Requirements

- C++98 standard (`-std=c++98`)
- No external dependencies
- POSIX sockets (Linux/macOS)

### Headers Used

```cpp
#include <sys/socket.h>  // socket(), connect(), send(), recv()
#include <netinet/in.h>  // sockaddr_in
#include <arpa/inet.h>   // inet_pton()
#include <netdb.h>       // getaddrinfo()
#include <unistd.h>      // close()
#include <poll.h>        // poll()
#include <cstring>       // memset(), strerror()
#include <cstdlib>       // atoi(), rand(), srand()
#include <cstdio>        // sprintf()
#include <ctime>         // time(), gmtime(), strftime()
#include <sstream>       // ostringstream
#include <iostream>      // cout, cerr
#include <csignal>       // signal()
```

## Testing

### Manual Testing

```bash
# Terminal 1: Start server
./ircserv 6667 testpass

# Terminal 2: Start bot
./ircbot localhost 6667 testpass -n TestBot -c test

# Terminal 3: Connect as user with nc
nc localhost 6667
PASS testpass
NICK tester
USER tester 0 * :Tester
JOIN #test
PRIVMSG #test :!help
PRIVMSG #test :!roll 100
PRIVMSG TestBot :!time
```

### Expected Output

```
# !help response
:TestBot!ircbot@* PRIVMSG #test :Available commands:
:TestBot!ircbot@* PRIVMSG #test :!help   - Show this help message
:TestBot!ircbot@* PRIVMSG #test :!time   - Show current server time
:TestBot!ircbot@* PRIVMSG #test :!ping   - Check if bot is alive
:TestBot!ircbot@* PRIVMSG #test :!roll N - Roll a random number 1-N (default 6)
:TestBot!ircbot@* PRIVMSG #test :!users  - List users in current channel

# !roll 100 response
:TestBot!ircbot@* PRIVMSG #test :Rolling 1-100: 42

# !time response (PM)
:TestBot!ircbot@* PRIVMSG tester :Current time: 2026-01-29 14:30:00 UTC
```

## Troubleshooting

| Issue | Cause | Solution |
|-------|-------|----------|
| "Failed to connect" | Server not running | Start server first |
| "Nick in use" | Another bot running | Use different `-n` nick |
| No command response | Rate limited | Wait 1 second |
| Bot disconnects | Server closed | Check server logs |
| Commands ignored | Wrong prefix | Check `-p` option |

## Future Improvements

Potential enhancements (not implemented):
- [ ] Configuration file support
- [ ] Multiple server support
- [ ] Channel operator commands
- [ ] Persistent settings
- [ ] Logging to file
- [ ] Custom command plugins
- [ ] Reconnection on disconnect
