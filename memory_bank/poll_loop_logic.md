# ft_irc — Poll Loop & Non-Blocking I/O Logic

> **Purpose:** Define the core event loop, non-blocking socket setup, data aggregation, and message dispatch strategy for your ft_irc server.

---

## 🧩 1. Overview

The ft_irc server must:
- Run in a **single process** using **poll()** (or select/kqueue/epoll equivalent).
- Handle **multiple clients** simultaneously.
- Perform **non-blocking** read/write operations.
- Aggregate fragmented TCP packets into complete IRC commands.

---

## ⚙️ 2. Poll Loop Lifecycle

```
┌────────────────────────────────────────────────────────────┐
│ Server::run()                                             │
│────────────────────────────────────────────────────────────│
│ 1. Setup listening socket (non-blocking)                  │
│ 2. Add listen fd to pollfds[]                             │
│ 3. while(true):                                            │
│     → poll(pollfds)                                       │
│     → if (listenFd) → acceptClient()                      │
│     → if (clientFd) → handleRead() / handleWrite()         │
│     → cleanup closed clients                              │
└────────────────────────────────────────────────────────────┘
```

---

## 🧱 3. Non-Blocking Socket Setup

```cpp
int Server::initSocket(int port) {
    _listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_listenFd < 0)
        throw std::runtime_error("socket() failed");

    int opt = 1;
    setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    fcntl(_listenFd, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(_listenFd, (sockaddr*)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("bind() failed");

    if (listen(_listenFd, SOMAXCONN) < 0)
        throw std::runtime_error("listen() failed");

    struct pollfd pfd = { _listenFd, POLLIN, 0 };
    _fds.push_back(pfd);
    return 0;
}
```

**Key points:**
- Use `fcntl(fd, F_SETFL, O_NONBLOCK)` for all sockets.
- Never block on `recv()` or `send()` — handle `EAGAIN`/`EWOULDBLOCK` properly.
- Track each client’s buffers and state.

---

## 🔁 4. Event Loop Skeleton

```cpp
void Server::run() {
    while (true) {
        int activity = poll(&_fds[0], _fds.size(), -1);
        if (activity < 0) continue;

        for (size_t i = 0; i < _fds.size(); ++i) {
            if (_fds[i].revents & POLLIN) {
                if (_fds[i].fd == _listenFd)
                    acceptClient();
                else
                    handleClientRead(_fds[i].fd);
            }
            if (_fds[i].revents & POLLOUT) {
                handleClientWrite(_fds[i].fd);
            }
        }
        cleanupClosedClients();
    }
}
```

---

## 📥 5. Accepting Clients

```cpp
void Server::acceptClient() {
    sockaddr_in clientAddr;
    socklen_t len = sizeof(clientAddr);
    int clientFd = accept(_listenFd, (sockaddr*)&clientAddr, &len);
    if (clientFd < 0) return;

    fcntl(clientFd, F_SETFL, O_NONBLOCK);
    _clients[clientFd] = Client(clientFd);
    struct pollfd pfd = { clientFd, POLLIN, 0 };
    _fds.push_back(pfd);
}
```

**Tips:**
- Send welcome notice or prompt on connect.
- Do not block waiting for messages — only react to POLLIN events.

---

## 📨 6. Reading Data (Aggregation Logic)

**Goal:** Collect incoming TCP fragments into complete IRC commands ending with `\r\n`.

```cpp
void Server::handleClientRead(int fd) {
    char buffer[BUFFER_SIZE + 1];
    ssize_t bytes = recv(fd, buffer, BUFFER_SIZE, 0);

    if (bytes <= 0) {
        removeClient(fd);
        return;
    }
    buffer[bytes] = '\0';
    Client &c = _clients[fd];
    c.recvBuffer += buffer;

    size_t pos;
    while ((pos = c.recvBuffer.find("\r\n")) != std::string::npos) {
        std::string cmd = c.recvBuffer.substr(0, pos);
        c.recvBuffer.erase(0, pos + 2);
        _cmdHandler.execute(cmd, c);
    }
}
```

**Key points:**
- Always append new data to buffer.
- Parse only full lines ending with `\r\n`.
- Keep incomplete fragments for the next iteration.

---

## 📤 7. Writing Data (Send Buffer)

```cpp
void Server::handleClientWrite(int fd) {
    Client &c = _clients[fd];
    if (c.sendBuffer.empty()) return;

    ssize_t sent = send(fd, c.sendBuffer.c_str(), c.sendBuffer.size(), 0);
    if (sent > 0)
        c.sendBuffer.erase(0, sent);
}
```

- Always attempt to send non-blockingly.
- If not all data is sent, the remainder stays in the buffer until the next POLLOUT.

---

## 🧹 8. Cleanup & Disconnect

```cpp
void Server::removeClient(int fd) {
    close(fd);
    _clients.erase(fd);
    for (size_t i = 0; i < _fds.size(); ++i) {
        if (_fds[i].fd == fd) {
            _fds.erase(_fds.begin() + i);
            break;
        }
    }
}
```

- Always close and remove from `_fds` and `_clients`.
- Inform other clients in the same channels of the disconnect via broadcast.

---

## 💬 9. Broadcasting Utility

```cpp
void Server::broadcast(const std::string &msg, Channel &ch, int exceptFd) {
    for (std::set<int>::iterator it = ch.members.begin(); it != ch.members.end(); ++it) {
        if (*it != exceptFd)
            _clients[*it].sendBuffer += msg + "\r\n";
    }
}
```

- Used for channel messages, JOIN/PART/KICK notifications, etc.

---

## 🧠 10. Error Handling

| Error | Cause | Resolution |
|--------|--------|------------|
| `EAGAIN` / `EWOULDBLOCK` | Socket temporarily unavailable | Ignore and retry later |
| `ECONNRESET` | Client closed abruptly | Close and remove client |
| `EPIPE` | Broken connection | Close client |
| `poll()` returns -1 | Interrupted system call | continue |

---

## 🧪 11. Testing Recommendations

| Test | Method | Expected Result |
|------|---------|-----------------|
| **Partial send** | Send fragmented command using netcat (`com^Dman^Dd`) | Reconstructed correctly |
| **Multiple clients** | Connect 3+ clients and send messages | All receive updates |
| **Idle client** | Keep connection open with no data | Server remains stable |
| **High load** | Flood server with small packets | CPU usage steady, no crashes |
| **Error resilience** | Force close sockets mid-transfer | Clean disconnect |

---

## ✅ 12. Summary

| Component | Purpose |
|------------|----------|
| `poll()` | Monitors all FDs (listen + clients) |
| Non-blocking sockets | Prevents server hang |
| Recv buffer | Aggregates partial data |
| Send buffer | Queues unsent messages |
| CommandHandler | Parses complete IRC commands |
| Cleanup | Ensures memory and FD integrity |

---

> ⚙️ *Next step recommendation:*  
> Proceed to `06_testing_guide.md` — outlining complete functional test scenarios using netcat and a reference IRC client.
