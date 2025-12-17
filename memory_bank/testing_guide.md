# ft_irc — Testing & Validation Guide

> **Goal:** Ensure your IRC server behaves like a real-world IRC daemon and satisfies all ft_irc evaluation criteria.

---

## 🧩 1. Preparation

### 1.1 Build & Run
```bash
make re
./ircserv 6667 password123
```

### 1.2 Tools
| Tool | Purpose |
|------|----------|
| `nc` (netcat) | Simulate raw TCP client input/output |
| `irssi` | Full-featured IRC client |
| `WeeChat` | Alternative IRC client |
| `telnet` | Simple testing (less preferred) |

---

## 🧠 2. Basic Connectivity Tests

| Test | Command | Expected Result |
|------|----------|-----------------|
| Start server | `./ircserv 6667 testpass` | Server listening on port |
| Connect | `nc -C 127.0.0.1 6667` | Connection opens, no crash |
| Send partial input | Type `com^Dman^Dd` | Reconstructed as `command` |
| Invalid port | `./ircserv abc test` | Graceful error exit |
| Reconnect after close | Ctrl+C and reconnect | Works repeatedly |

---

## 🔑 3. Authentication Flow

### Commands
```
PASS password123
NICK djordje
USER george 0 * :George Goatlord
```

| Scenario | Expected Outcome |
|-----------|------------------|
| Wrong password | Connection closed immediately |
| Missing PASS | Error until provided |
| Reused NICK | `ERR_NICKNAMEINUSE` |
| Re-register | `ERR_ALREADYREGISTRED` |
| Successful auth | `001–004` welcome messages |

---

## 💬 4. Messaging Tests

| Command | Expected Result |
|----------|-----------------|
| `PRIVMSG #general :hello` | Broadcasts to channel |
| `PRIVMSG user2 :hi` | Sends DM to target |
| Empty message | `ERR_NOTEXTTOSEND` |
| Invalid user | `ERR_NOSUCHNICK` |

---

## 🧱 5. Channel Lifecycle

### Creating and Joining
```
JOIN #general
```
→ Creates new channel if missing  
→ Replies with topic and member list

### Leaving
```
PART #general :bye
```
→ Sends PART message to all  
→ Removes user from channel list

### Topic management
```
TOPIC #general :Welcome!
```
→ Changes topic if operator or mode allows  
→ `TOPIC #general` shows it

---

## 🧭 6. Operator & Mode Tests

| Command | Description | Expected |
|----------|--------------|----------|
| `MODE #general +i` | Make invite-only | Only invited can join |
| `MODE #general +k secret` | Set channel key | Requires `JOIN #general secret` |
| `MODE #general +o user2` | Grant operator | user2 can now KICK |
| `MODE #general +l 5` | Limit 5 users | Rejects 6th join |

### Kick test
```
KICK #general bob :spamming
```
→ Removes bob and notifies others.

### Invite test
```
INVITE bob #general
```
→ Sends invite notice to bob.

---

## 🔁 7. Multi-Client Simulation

1. Open **3 terminals**:
   - `nc -C 127.0.0.1 6667`
   - `nc -C 127.0.0.1 6667`
   - `nc -C 127.0.0.1 6667`
2. Register each client with unique nick.
3. Have one client create channel (`JOIN #42`).
4. Others join and exchange `PRIVMSG` commands.
5. Verify all messages echo correctly between clients.

---

## ⚙️ 8. Real Client (irssi) Setup

### Start your server
```bash
./ircserv 6667 testpass
```

### Open irssi
```bash
irssi
/connect 127.0.0.1 6667 testpass
/nick george
/join #ftirc
/msg #ftirc Hello everyone!
```

| Action | Expected |
|--------|-----------|
| Connects successfully | ✅ |
| Joins channel | ✅ |
| Sends messages visible in other irssi sessions | ✅ |
| Operator actions | Behave as per RFC logic |

---

## 💥 9. Edge & Error Tests

| Case | Input | Expected |
|------|--------|----------|
| Send empty command | `\r\n` | Ignored |
| Flood of small packets | loop 1000× `PING :1234` | No crash |
| Long message (>512 bytes) | Custom script | Truncate or reject gracefully |
| Client disconnect mid-message | Ctrl+C | Clean removal from channels |
| Invalid mode flag | `MODE #ch +z` | `ERR_UNKNOWNMODE` |

---

## 🧹 10. Memory & Stability Checks

| Tool | Purpose |
|------|----------|
| `valgrind --leak-check=full ./ircserv 6667 testpass` | Detect memory leaks |
| `htop` | Observe CPU/memory usage |
| `lsof -p <pid>` | Check open FDs after clients disconnect |

**Pass criteria:**
- 0 memory leaks
- No FD leaks
- No zombie processes
- Stable CPU usage under load

---

## 🧪 11. Bonus Verification

| Feature | Test | Expected Result |
|----------|------|-----------------|
| Bot | Auto-replies to messages | Works on join |
| File transfer | `/dcc send file.txt` | Transfer accepted |

(Only relevant if mandatory part is perfect.)

---

## ✅ 12. Defense Checklist

| Area | Question | You should be able to explain… |
|------|-----------|--------------------------------|
| Architecture | "How does poll() work here?" | Explain non-blocking event loop |
| Parsing | "How do you handle partial commands?" | Describe buffer aggregation |
| IRC logic | "What happens when a user joins a channel?" | Lifecycle of Channel class |
| Commands | "Which modes did you implement?" | i, t, k, o, l |
| Stability | "What happens if client disconnects mid-send?" | FD cleanup |
| Error handling | "How do you manage EAGAIN?" | Retry mechanism |

---

> 🧩 *Next step recommendation:*  
> Proceed to **`07_bonus_ideas.md`** — concept blueprints for implementing a bot and file transfer system if you achieve a perfect mandatory.
