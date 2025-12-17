# ft_irc — Architecture Plan

> **Goal:** Build a non-blocking, single-process IRC server in C++ 98 that supports multiple clients via `poll()` (or `select()`).

---

## 🧠 1. High-Level Overview

```
         ┌─────────────┐
         │ IRC Clients │  (irssi, netcat, etc.)
         └─────┬───────┘
               │  TCP
     ┌─────────▼──────────┐
     │      ircserv       │
     │────────────────────│
     │  Server class      │
     │  - listens sockets │
     │  - polls fds       │
     │  - dispatches cmds │
     └────────┬───────────┘
              │
     ┌────────▼────────┐
     │  CommandHandler │
     │  - parse input  │
     │  - run commands │
     └────────┬────────┘
              │
      ┌───────▼────────┐
      │  Channel / User│
      │  - state mgmt  │
      └────────────────┘
```

All communication passes through the **Server** event loop.  
No threads, no forks — only one `poll()` array monitoring all sockets.

---

## 🧩 2. Core Modules and Classes

| Module | Responsibility | Key Members / Methods |
|---------|----------------|-----------------------|
| **Server** | Main controller. Manages sockets, poll events, and dispatching. | `initSocket()`, `run()`, `acceptClient()`, `removeClient()`, `broadcast()`, `getClient(fd)` |
| **Client** | Represents each connected user. Tracks nickname, username, status, and message buffer. | `fd`, `nickname`, `username`, `authenticated`, `recvBuffer`, `sendBuffer`, `channels` |
| **Channel** | Represents an IRC channel. Tracks members, operators, topic, and modes. | `name`, `topic`, `modes`, `key`, `userLimit`, `members`, `operators`, `inviteList` |
| **CommandHandler** | Parses incoming raw messages and executes the correct command. | `parseCommand()`, `execute(Command, Client&)` |
| **Parser / Utils** | Helper for tokenizing IRC messages and building replies. | `split()`, `trim()`, `toUpper()`, etc. |
| **Replies** | Stores numeric reply codes and messages for consistent output. | Constants and helper functions |
| **Config (optional)** | Load optional server settings from a file. | `parseConfig()` |

---

## ⚙️ 3. Data Flow & Event Loop

### 3.1 Poll Loop Core Logic

```cpp
void Server::run() {
    while (true) {
        int ret = poll(_fds, _fdsCount, -1);
        if (ret < 0) continue;

        for (int i = 0; i < _fdsCount; ++i) {
            if (_fds[i].revents & POLLIN) {
                if (_fds[i].fd == _listenFd)
                    acceptClient();
                else
                    handleClientRead(i);
            }
            if (_fds[i].revents & POLLOUT)
                handleClientWrite(i);
        }
    }
}
```

- **acceptClient()** — accept new connection, set non-blocking mode.
- **handleClientRead()** — read into the client buffer; once a `\r\n` is found, pass the full command to `CommandHandler`.
- **handleClientWrite()** — send pending messages from `sendBuffer`.
- **removeClient()** — close FD and update poll set.

---

### 3.2 Command Execution Flow

1. **Client sends:** `JOIN #general`
2. **Server** → detects readable FD
3. **Client.recvBuffer** collects full line
4. **CommandHandler::parseCommand()** tokenizes: command=`JOIN`, params=`#general`
5. **CommandHandler::execute()** calls the matching method → `cmdJoin()`
6. **Server.broadcast()** forwards message to all members of `#general`

---

## 🧱 4. Suggested File & Directory Layout

```
ft_irc/
│
├── Makefile
├── main.cpp
│
├── include/
│   ├── Server.hpp
│   ├── Client.hpp
│   ├── Channel.hpp
│   ├── CommandHandler.hpp
│   ├── Replies.hpp
│   ├── Utils.hpp
│   └── IRC.hpp            (global includes, typedefs)
│
└── src/
    ├── Server.cpp
    ├── Client.cpp
    ├── Channel.cpp
    ├── CommandHandler.cpp
    ├── Replies.cpp
    ├── Utils.cpp
    └── main.cpp
```

> Each class header has include guards and forward declarations to reduce coupling.

---

## 🧰 5. Implementation Guidelines

| Area | Best Practice |
|------|----------------|
| **Sockets** | Use `socket()`, `bind()`, `listen()`, `accept()` in `Server::initSocket()`. |
| **Non-blocking** | `fcntl(fd, F_SETFL, O_NONBLOCK)` on every socket. |
| **poll() setup** | Maintain a `std::vector<struct pollfd>` for all FDs. |
| **Buffering** | Accumulate incoming data until `\r\n`, then process. |
| **String parsing** | Avoid `stringstream`; use manual parsing for C++ 98 compatibility. |
| **Memory safety** | No raw new/delete; prefer stack objects and STL containers. |
| **Command map** | Use `std::map<std::string, CmdFnPtr>` to map command names to handler methods. |
| **Error handling** | Wrap system calls, handle `EAGAIN`, `EWOULDBLOCK`. |
| **Logging** | Optional debug mode printing connection events. |

---

## 💬 6. Mandatory Commands & Responsibilities (summary)

| Command | Purpose | Module |
|----------|----------|--------|
| `PASS` | Authenticate client with server password | Server / Client |
| `NICK` | Set nickname (must be unique) | CommandHandler |
| `USER` | Set username and mark client as registered | CommandHandler |
| `JOIN` | Join or create a channel | Channel |
| `PART` | Leave a channel | Channel |
| `PRIVMSG` | Send private message to user/channel | CommandHandler |
| `KICK` | Operator removes user from channel | Channel |
| `INVITE` | Operator invites user | Channel |
| `TOPIC` | View/change channel topic | Channel |
| `MODE` | Manage channel modes (`i`, `t`, `k`, `o`, `l`) | Channel |

---

## 🧪 7. Testing Workflow

| Step | Tool | Expected Result |
|------|------|-----------------|
| 1. Connect with `nc -C 127.0.0.1 6667` | Netcat | Connection accepted |
| 2. Send `PASS`, `NICK`, `USER` | | Registration success |
| 3. Join channel with `JOIN #test` | | Channel created or joined |
| 4. Send messages | | Broadcast works |
| 5. Test operator commands | | Only operators succeed |
| 6. Use official client (e.g., irssi) | | Same behavior as standard IRC server |

---

## 🚀 8. Future Expansion (Bonus)

| Feature | Description |
|----------|--------------|
| **File transfer (DCC)** | Add file send/receive between clients |
| **Bot integration** | Create a small bot responding to keywords |
| **Logging system** | Persist chat history to file |
| **TLS** | Secure the connection (for curiosity only, not graded) |

---

## 🧩 9. Quick Reference — Socket Setup Example

```cpp
int Server::initSocket(int port) {
    _listenFd = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(_listenFd, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(_listenFd, (sockaddr*)&addr, sizeof(addr));
    listen(_listenFd, SOMAXCONN);

    struct pollfd pfd = { _listenFd, POLLIN, 0 };
    _fds.push_back(pfd);
    return 0;
}
```

---

> ⚙️ *Next step recommendation:*  
> Proceed to `04_command_reference.md` — a full breakdown of all mandatory IRC commands, syntax, expected behavior, and interaction with your `Server`, `Client`, and `Channel` classes.
