# ft_irc — Bonus & Expansion Ideas

> **Goal:** Extend your IRC server with creative, real-world functionality once the mandatory part is perfect.  
> These ideas are optional but demonstrate mastery and initiative.

---

## 🧠 1. Bonus Rule Reminder

- Bonuses are evaluated **only if the mandatory part is 100% complete and bug-free**.  
- Implement them cleanly and modularly — never break the core stability.

---

## 📁 2. Bonus 1 — File Transfer (DCC-like)

### Overview
Implement a simplified **Direct Client-to-Client (DCC)** file transfer system between connected users.

### Behavior
1. User A sends a file offer to User B:
   ```
   /dcc send <nick> <filename>
   ```
2. Server forwards the request and both users exchange IP/port.
3. Direct peer-to-peer connection handles file transfer.

### Implementation Notes
- Add new command `DCC SEND`.
- Server only mediates metadata; file data is not routed through the IRC server.
- Validate file size and ensure safe path handling.
- Optional progress report for debug logs.

---

## 🤖 3. Bonus 2 — IRC Bot

### Concept
Add a lightweight bot that listens on all channels and responds to predefined triggers.

| Feature | Example |
|----------|----------|
| Keyword reply | “hello” → “Hello there, human!” |
| Command stats | “!users” → Lists connected users |
| Channel logs | Writes messages to a `.log` file |
| Ping responder | Auto-replies to keep connection alive |

### Implementation Steps
1. Treat the bot as an internal `Client` instance.
2. Add a `Bot` class derived from `Client`.
3. Register it automatically on server start.
4. Implement command parsing in `Bot::onMessage()`.

---

## 📜 4. Bonus 3 — Logging System

### Purpose
Persist messages, joins, and errors to log files.

| Log Type | Filename | Example Content |
|-----------|-----------|----------------|
| General | `server.log` | Connection events |
| Channels | `#general.log` | Chat history |
| Errors | `error.log` | Exceptions, kicks, etc. |

**Implementation Tips:**
- Create `Logger` class.
- Use timestamped entries.
- Flush to disk on each write or at regular intervals.

---

## 📈 5. Bonus 4 — Statistics Command

### Command
```
STATS
```
Shows:
- Number of connected users
- Active channels
- Server uptime

### Implementation Notes
- Add `CommandHandler::cmdStats()`.
- Use server start time for uptime calculation.

---

## 🔒 6. Bonus 5 — Operator Authentication Extension

Allow server operators to authenticate with a special password to gain global privileges.

| Command | Example | Effect |
|----------|----------|--------|
| `OPER <name> <password>` | `OPER admin secretpass` | Grants operator status |

Operators can:
- View all channels
- Send global announcements
- Shutdown server (`/DIE`)

---

## 💬 7. Bonus 6 — Channel Persistence

- Save channels and member lists to file on shutdown.
- Reload them on restart.
- Allows long-term communities across sessions.

---

## 🧩 8. Recommended Implementation Order

| Bonus | Complexity | Recommendation |
|--------|-------------|----------------|
| IRC Bot | 🟢 Easy | Fun and low risk |
| File Transfer | 🟠 Medium | Add only after perfect stability |
| Logging System | 🟢 Easy | Useful for debugging |
| Statistics Command | 🟢 Easy | Simple extension |
| Operator System | 🔴 Hard | Adds security layer |
| Channel Persistence | 🔴 Hard | Requires serialization |

---

> ⚙️ *Next step recommendation:*  
> Proceed to **ft_irc_agent_prompts.md** to configure specialized AI prompts for implementation, debugging, testing, and documentation.
