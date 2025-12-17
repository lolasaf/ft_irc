# ft_irc — Command Reference

> **Purpose:** Define the syntax, parameters, expected behavior, and reply patterns for each IRC command required by the 42 **ft_irc** project.  
> These commands correspond to a subset of RFC 1459 / RFC 2812 and are mandatory for evaluation.

---

## 🧩 1. Connection & Authentication Commands

### 🔑 PASS — Set connection password
**Syntax:**
```
PASS <password>
```

**Rules:**
- Must be sent before any registration (`NICK`/`USER`).
- If already registered → reply with error `ERR_ALREADYREGISTRED`.
- If the password does not match the one passed to `ircserv`, disconnect the client.

**Example:**
```
C → PASS hunter2
```

**Handled by:** `CommandHandler::cmdPass(Client&)`

---

### 🧍‍♂️ NICK — Set nickname
**Syntax:**
```
NICK <nickname>
```

**Rules:**
- Nicknames must be unique.
- If taken → `ERR_NICKNAMEINUSE`.
- If invalid format → `ERR_ERRONEUSNICKNAME`.
- Once both `PASS` and `USER` are valid, mark client as *registered*.

**Example:**
```
C → NICK djordje
```

**Handled by:** `CommandHandler::cmdNick(Client&)`

---

### 💻 USER — Register username and realname
**Syntax:**
```
USER <username> 0 * :<realname>
```

**Rules:**
- Finalizes registration once `PASS` + `NICK` are valid.
- Reply with welcome messages (`001`–`004`) after registration.
- Ignore extra parameters after `0 *`.

**Example:**
```
C → USER george 0 * :George Goatlord
```

**Handled by:** `CommandHandler::cmdUser(Client&)`

---

## 💬 2. Messaging Commands

### 📢 PRIVMSG — Send private or channel message
**Syntax:**
```
PRIVMSG <target> :<message>
```

**Rules:**
- `<target>` can be a nickname or channel.
- If target is channel → broadcast to all users in that channel except sender.
- If target is nickname → send direct message to that client.
- Empty or unknown target → `ERR_NORECIPIENT` or `ERR_NOSUCHNICK`.

**Example:**
```
C → PRIVMSG #general :Hello everyone!
C → PRIVMSG alice :Hey there
```

**Handled by:** `CommandHandler::cmdPrivmsg(Client&)`

---

### 📜 NOTICE (optional but recommended)
Same behavior as `PRIVMSG` but must never trigger auto-replies or errors.

---

## 🧱 3. Channel Management Commands

### 🚪 JOIN — Join or create a channel
**Syntax:**
```
JOIN <channel>{,<channel>} [<key>{,<key>}]
```

**Rules:**
- Create channel if it doesn’t exist.
- If channel has key → verify password.
- Add user to channel member list.
- Send topic and user list to new member.
- Broadcast join message to others.

**Example:**
```
C → JOIN #general
```

**Handled by:** `CommandHandler::cmdJoin(Client&)`

---

### 🏃 PART — Leave a channel
**Syntax:**
```
PART <channel>{,<channel>} [:<message>]
```

**Rules:**
- Remove user from channel list.
- Notify other members.
- If last user leaves → delete channel.

**Example:**
```
C → PART #general :See you
```

**Handled by:** `CommandHandler::cmdPart(Client&)`

---

### 🗣️ TOPIC — View or change a channel topic
**Syntax:**
```
TOPIC <channel> [:<topic>]
```

**Rules:**
- Without argument → show topic.
- With argument → change topic.
- If mode `+t` → only operators can change.
- Empty topic removes it.

**Examples:**
```
C → TOPIC #general
C → TOPIC #general :Welcome to ft_irc!
```

**Handled by:** `CommandHandler::cmdTopic(Channel&)`

---

### 🧭 MODE — Manage channel modes
**Syntax:**
```
MODE <channel> [<+|-><modes> [parameters]]
```

**Modes supported:**
| Mode | Effect |
|------|---------|
| `i` | Invite-only toggle |
| `t` | Topic lock (operators only) |
| `k` | Channel key (password) |
| `o` | Grant/revoke operator rights |
| `l` | Set user limit |

**Examples:**
```
C → MODE #general +i
C → MODE #general +k secret
C → MODE #general +o george
```

**Handled by:** `CommandHandler::cmdMode(Channel&)`

---

### 🦶 KICK — Operator removes a user
**Syntax:**
```
KICK <channel> <user> [:<reason>]
```

**Rules:**
- Only operators allowed.
- Target must be in the channel.
- Broadcast removal to others.

**Example:**
```
C → KICK #general alice :spamming
```

**Handled by:** `CommandHandler::cmdKick(Channel&)`

---

### 💌 INVITE — Invite user to invite-only channel
**Syntax:**
```
INVITE <nickname> <channel>
```

**Rules:**
- Only operator can invite if mode `+i`.
- Target receives notice and may `JOIN`.

**Example:**
```
C → INVITE alice #general
```

**Handled by:** `CommandHandler::cmdInvite(Channel&)`

---

## 🧰 4. Utility & Server Maintenance

### 📶 PING / PONG
**Purpose:** Keep connection alive and check latency.

**Example:**
```
C → PING :12345
S → PONG :12345
```

Handled internally by `CommandHandler::cmdPing()` / `cmdPong()`.

---

### 🔚 QUIT — Disconnect
**Syntax:**
```
QUIT [:<message>]
```

**Behavior:**
- Send `ERROR` message to client.
- Remove from all joined channels.
- Broadcast quit message to peers.

**Handled by:** `CommandHandler::cmdQuit(Client&)`

---

## ⚙️ 5. Command Parsing & Dispatch

### Parsing Rules
- Commands terminated by `\r\n`.
- Case-insensitive command names.
- Parameters split by space unless prefixed by `:`, which absorbs the rest of the line.
- Empty lines ignored.

**Pseudocode:**
```cpp
CommandHandler::parseCommand(line):
    tokens = splitBySpace(line)
    cmd = toUpper(tokens[0])
    params = tokens[1:]
    execute(cmd, params)
```

---

## 🧪 6. Testing Scenarios

| Scenario | Input | Expected Result |
|-----------|--------|-----------------|
| Invalid password | `PASS wrong` | Connection closed |
| Reused nickname | `NICK djordje` (twice) | `ERR_NICKNAMEINUSE` |
| Join + message | `JOIN #42`, then `PRIVMSG #42 :Hi!` | Message broadcast |
| Operator actions | `MODE #42 +o alice`, `KICK #42 bob` | Works only if operator |
| Invite-only test | `MODE #42 +i`, non-invited user tries join | `ERR_INVITEONLYCHAN` |

---

## 🧩 7. Integration Map

| Command | Uses | Updates | Replies |
|----------|------|----------|----------|
| PASS | `Server::_password` | `Client::authenticated` | ERR_PASSWDMISMATCH |
| NICK | `Client::nickname` | `Server::_clients` | ERR_NICKNAMEINUSE |
| USER | `Client::username` | `Server::_clients` | RPL_WELCOME |
| JOIN | `Channel::_members` | `Client::_channels` | JOIN, RPL_NAMREPLY |
| PART | `Channel::_members` | `Client::_channels` | PART |
| PRIVMSG | `Channel::_members` | none | PRIVMSG |
| KICK | `Channel::_operators` | `Channel::_members` | KICK |
| INVITE | `Channel::_inviteList` | none | INVITE |
| TOPIC | `Channel::_topic` | none | RPL_TOPIC |
| MODE | `Channel::_modes` | none | RPL_CHANNELMODEIS |

---

## ✅ 8. Implementation Priority

1. `PASS`, `NICK`, `USER` (authentication)
2. `PING`, `PONG`
3. `JOIN`, `PART`
4. `PRIVMSG`
5. `MODE`, `TOPIC`
6. `KICK`, `INVITE`
7. `QUIT`
8. Edge-case handling and replies

---

> ⚙️ *Next step recommendation:*  
> Proceed to `05_poll_loop_logic.md` — explaining detailed non-blocking event handling, poll() management, and input buffering strategies.
