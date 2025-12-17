# ft_irc — 3-Week Execution Plan (C++98, 42-Style)

This is a **tight, evaluator-safe, 3-week plan** to complete *ft_irc* from zero to defense-ready.

The plan ensures:
- You always have a **working server**
- High-risk areas (select/poll, buffers, MODE) are tackled early
- Final days reserved for **hardening + defense prep**

---

## OVERALL STRATEGY

**Principles**
- Build **vertically**, not horizontally
- Never move on until the previous layer is stable
- Test continuously with a real IRC client + `nc`
- Keep everything explainable in <2 minutes per topic

**Weekly focus**
- **Week 1** → Networking core + registration + messaging
- **Week 2** → Channels, operators, MODE (highest risk)
- **Week 3** → Edge cases, stability, evaluator traps, defense prep

---

# WEEK 1 — NETWORKING CORE & BASIC IRC (FOUNDATION)

### Goal by end of week
✅ Multiple clients connect  
✅ Non-blocking, single `select()` loop  
✅ PASS / NICK / USER registration  
✅ JOIN + PART + PRIVMSG works reliably  

If Week 1 is solid, passing is realistic.

---

## Day 1 — Project Skeleton & Socket Setup

**Deliverables**
- Repo + Makefile (`-Wall -Wextra -Werror -std=c++98`)
- Basic directory structure (`src/` `include/`)
- Server starts and listens

**Tasks**
- Create folder structure
- Parse CLI args: `./ircserv <port> <password>`
- Create listening socket:
  ```cpp
  socket(AF_INET, SOCK_STREAM, 0)  // or SOCK_STREAM | SOCK_NONBLOCK on Linux
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))
  bind(fd, addr, sizeof(addr))
  listen(fd, SOMAXCONN)
  ```
- Set listening socket **non-blocking**:
  - Linux: `SOCK_NONBLOCK` flag or `fcntl()`
  - macOS: `fcntl(fd, F_SETFL, O_NONBLOCK)`
- Minimal `main()` → `Server` constructor → `Server::run()`

**Files to create**
- `include/Server.hpp`, `include/defines.hpp`, `include/utils.hpp`
- `src/main.cpp`, `src/Server.cpp`, `src/ServerSocket.cpp`, `src/utils.cpp`
- `Makefile` with OS detection

**Tests**
- Server starts without crash
- Port open: `nc 127.0.0.1 <port>` connects

---

## Day 2 — select() Event Loop + Accept

**Deliverables**
- One working `select()` loop
- Multiple clients can connect

**Tasks**
- Implement `fd_set` preparation:
  ```cpp
  int Server::prepareReadSet(fd_set& readFds) {
      FD_ZERO(&readFds);
      FD_SET(_fd, &readFds);  // listening socket
      int maxFd = _fd;
      // Add all user fds...
      return maxFd;
  }
  ```
- Main loop with `select()`:
  ```cpp
  while (g_running) {
      maxFd = prepareReadSet(readFds);
      writeMaxFd = prepareWriteSet(writeFds);
      select(max(maxFd, writeMaxFd) + 1, &readFds, &writeFds, NULL, NULL);
      // Handle ready fds...
  }
  ```
- On listen fd ready → `accept()` new users
- Set accepted sockets non-blocking
- Create `User` class with fd and buffers

**Files to create**
- `include/User.hpp`
- `src/User.cpp`, `src/ServerUser.cpp`

**Tests**
- Connect with multiple `nc` clients simultaneously
- No blocking / no crashes

---

## Day 3 — Input Buffering & Packet Reassembly

**Deliverables**
- Correct handling of partial commands

**Tasks**
- Per-user input buffer (`std::string _inputBuffer`)
- `recv()` only when `select()` says fd is ready
- Append received data to buffer
- Extract complete messages on `\n` (accept both `\r\n` and `\n`):
  ```cpp
  while ((pos = buffer.find('\n')) != std::string::npos) {
      std::string msg = buffer.substr(0, pos);
      buffer.erase(0, pos + 1);
      if (!msg.empty() && msg[msg.size()-1] == '\r')
          msg.erase(msg.size() - 1);
      messages.push_back(msg);
  }
  ```

**Tests**
```sh
# Partial packet test
nc -C 127.0.0.1 6667
NIC
K test
# Should work as "NICK test"
```

---

## Day 4 — Output Buffering & Write Handling

**Deliverables**
- Safe, non-blocking writes

**Tasks**
- Per-user output buffer (`std::string _outputBuffer`)
- **Never** `send()` directly in command handlers
- All output via `user->getOutputBuffer() += message`
- Add fd to write set only if output buffer non-empty:
  ```cpp
  int Server::prepareWriteSet(fd_set& writeFds) {
      FD_ZERO(&writeFds);
      for (each user) {
          if (!user->getOutputBuffer().empty())
              FD_SET(userFd, &writeFds);
      }
  }
  ```
- On write ready: `send()` and erase sent portion

**Tests**
- Spam messages to client
- Ensure no busy loop / CPU spike
- Check `top` - CPU should be ~0% when idle

---

## Day 5 — PASS / NICK / USER Registration

**Deliverables**
- Proper registration pipeline

**Tasks**
- Create `Command` class (static methods)
- Implement tokenizer for IRC messages:
  ```cpp
  // "USER max 0 * :Max Power" → ["USER", "max", "0", "*", ":Max Power"]
  ```
- Implement handlers:
  - `handlePass()` - check password, set `_hasPassed`
  - `handleNick()` - validate, check uniqueness, set `_hasNick`
  - `handleUser()` - extract username/realname, set `_hasUser`
- Track registration state in User:
  ```cpp
  bool _hasNick, _hasUser, _hasPassed, _isRegistered;
  void tryRegister();  // Called after each command
  ```
- Send `001` welcome on registration

**Files to create**
- `include/Command.hpp`
- `src/Command.cpp`, `src/CommandRegistration.cpp`, `src/CommandUtils.cpp`
- `src/UserRegistration.cpp`, `src/UserMessaging.cpp`

**Tests**
- Wrong password → error
- Duplicate nick → `433`
- Registration in any order (NICK before PASS, etc.)

---

## Day 6 — JOIN + PART + PRIVMSG (Minimum IRC)

**Deliverables**
- Functional chat server

**Tasks**
- Create `Channel` class:
  ```cpp
  std::map<std::string, User*> _members;  // by normalized nick
  std::map<std::string, User*> _operators;
  ```
- Implement `handleJoin()`:
  - Validate channel name (starts with `#`)
  - Create channel if doesn't exist
  - First joiner becomes operator
  - Broadcast JOIN to all members
  - Send topic (331/332) and names list (353/366)
- Implement `handlePart()`:
  - Remove user from channel
  - Broadcast PART message
  - Delete channel if empty
- Implement `handlePrivmsg()`:
  - To channel: broadcast to members (except sender)
  - To nick: send directly

**Files to create**
- `include/Channel.hpp`
- `src/Channel.cpp`, `src/ServerChannel.cpp`, `src/CommandChannel.cpp`, `src/CommandMessaging.cpp`

**Tests**
- Multiple users join channel and chat
- PART removes user correctly
- Private messages work

---

## Day 7 — Cleanup, QUIT, NOTICE & Review

**Deliverables**
- Stable Week-1 build

**Tasks**
- Implement `handleQuit()`:
  - Broadcast QUIT to all shared channels
  - Remove user from all channels
  - Delete empty channels
  - Close fd and delete User
- Implement `handleNotice()` (same as PRIVMSG, no auto-reply)
- Implement `disconnectUser()` for error cases
- Handle disconnect cleanup properly:
  - Remove from `_usersFd` and `_usersNick` maps
  - Remove from all channels
  - Broadcast QUIT message

**Files to create**
- `src/CommandConnection.cpp`

**Checkpoint**
✅ Demo-able:
- Multiple clients connect
- JOIN/PART channels
- Chat works
- Clean disconnect
- No crashes

---

# WEEK 2 — CHANNEL OPERATORS & MODE (HIGH RISK)

### Goal by end of week
✅ All mandatory channel modes (+i +t +k +l +o)  
✅ Operator privileges enforced  
✅ KICK / INVITE / TOPIC / MODE fully working  

---

## Day 8 — Channel Operators (+o / -o)

**Tasks**
- Store operators in Channel:
  ```cpp
  std::map<std::string, User*> _channel_operators_by_nickname;
  bool is_user_operator(const User* user) const;
  void make_user_operator(User* user);
  void remove_user_operator_status(User* user);
  ```
- First user to JOIN becomes operator automatically
- Implement `MODE #channel +o nick` / `-o nick`
- Enforce op-only for: KICK, MODE changes

**Tests**
- Creator is op
- Only ops can KICK/MODE
- +o grants op, -o removes

---

## Day 9 — MODE Core Parsing

**Tasks**
- Parse mode strings properly:
  ```cpp
  // MODE #chan +itkl key 10
  // MODE #chan +o-o nick1 nick2
  // MODE #chan -k
  ```
- Track current sign (`+` or `-`)
- Consume arguments in correct order:
  - `+k` needs key argument
  - `-k` no argument needed
  - `+l` needs limit argument
  - `-l` no argument needed
  - `+o`/`-o` needs nick argument
- Return `461` for missing required args

**Files to create**
- `src/CommandModes.cpp`

**Important**: This is one of the **most failed parts** at evaluation.

---

## Day 10 — MODE Enforcement (+i +t +k +l)

**Tasks**
- Add mode flags to Channel:
  ```cpp
  bool _invite_only;      // +i
  bool _topic_protection; // +t
  std::string _channel_key; // +k
  int _user_limit;        // +l
  ```
- Implement `can_user_join()` that checks all modes:
  ```cpp
  bool Channel::can_user_join(User* user, const std::string& key, 
                              JoinResult& result) const {
      if (_invite_only && !is_invited(user->getNicknameLower())) {
          result = JOIN_INVITE_ONLY;
          return false;
      }
      if (!_channel_key.empty() && key != _channel_key) {
          result = JOIN_BAD_KEY;
          return false;
      }
      if (_user_limit > 0 && get_connected_user_number() >= _user_limit) {
          result = JOIN_FULL;
          return false;
      }
      return true;
  }
  ```
- Implement `get_mode_string()` for MODE query

**Tests**
- `+i` blocks JOIN → `473`
- `+k secret` requires key → `475`
- `+l 2` limits users → `471`

---

## Day 11 — INVITE

**Tasks**
- Implement `handleInvite()`:
  - Require op status
  - Store invited nick in channel:
    ```cpp
    std::set<std::string> _channel_invitation_list;
    void add_invite(const std::string& nick_lower);
    bool is_invited(const std::string& nick_lower) const;
    ```
  - Notify inviter (`341`) and target
- Update JOIN to check invitation list for `+i` channels
- Consume invite on successful JOIN (optional but recommended)

**Tests**
- Invite allows JOIN to +i channel
- Non-op cannot INVITE → `482`

---

## Day 12 — TOPIC

**Tasks**
- Implement `handleTopic()`:
  - View: `TOPIC #channel` → send `331` or `332`
  - Set: `TOPIC #channel :new topic`
  - If `+t` mode: only ops can set → `482`
- Store topic info:
  ```cpp
  std::string _channel_topic;
  std::string _channel_topic_set_by;
  int _channel_topic_set_at;
  ```
- Broadcast topic changes to channel

**Tests**
- View topic works
- `+t` prevents non-op from setting

---

## Day 13 — KICK

**Tasks**
- Implement `handleKick()`:
  - Require op status → `482`
  - Target must be member → `441`
  - Broadcast KICK message
  - Remove target from channel
  - Delete channel if empty

**Tests**
- Op kicks user successfully
- Non-op fails with `482`
- Kicked user removed from channel

---

## Day 14 — LIST + Stress Testing

**Tasks**
- Implement `handleList()`:
  - Send channel list with user counts
  - Use `321`, `322`, `323` numerics
- Test mode combinations:
  - `+i +k +l` together
  - Multiple mode changes: `MODE #chan +itk secret`
- Test disconnect during channel activity
- Clean up MODE logic
- Simplify handlers

**Checkpoint**
✅ All mandatory features implemented

---

# WEEK 3 — HARDENING, TRAPS & DEFENSE PREP

### Goal by end of week
✅ No evaluator traps  
✅ No crashes  
✅ You can explain everything  

---

## Day 15 — Evaluator Trap Audit

**Tasks**
- Verify single `select()`/`poll()` - no hidden polling
- Verify no read/write outside select readiness
- Verify all sockets non-blocking
- Review evaluator traps document
- Test with `nc` partial packets

---

## Day 16 — Edge Cases

**Tasks**
- Nick change while in channel → broadcast to shared channels
- Disconnect during MODE/KICK → no crash
- Duplicate commands in quick succession
- Missing parameters → proper errors
- Empty channel name, invalid nick chars

---

## Day 17 — IRC Formatting Pass

**Tasks**
- Ensure **every** message ends with `\r\n`
- Validate all prefixes: `:<nick>!<user>@<host>`
- Server numerics format: `:<server> <code> <target> :<message>`
- Test with strict IRC client

---

## Day 18 — Client Compatibility

**Tasks**
- Test with real IRC clients:
  - HexChat
  - irssi
  - weechat
- Fix any behavior inconsistencies
- Ensure welcome messages are complete

---

## Day 19 — Memory & Stability

**Tasks**
- Valgrind testing (Linux):
  ```sh
  valgrind --leak-check=full ./ircserv 6667 password
  ```
- Stress test with many clients
- Rapid connect/disconnect cycles
- Ensure no leaks / invalid frees

---

## Day 20 — Defense Rehearsal

**Tasks**
- Prepare answers to common questions:
  - Why single select/poll?
  - How do you handle partial packets?
  - How does MODE parsing work?
  - What happens on disconnect?
  - How is output buffering done?
- Practice explaining **without looking at code**
- Time yourself: each answer < 2 minutes

---

## Day 21 — Final Buffer Day

**Tasks**
- Final code cleanup
- Add/improve comments
- Remove dead code and debug prints
- Final test pass
- Push clean commit

---

# FINAL PASS CRITERIA

All must be true:

- [ ] Single `select()` or `poll()` only
- [ ] All sockets non-blocking
- [ ] No read/write outside readiness check
- [ ] Partial packets handled correctly
- [ ] Output buffered properly
- [ ] PASS/NICK/USER registration works
- [ ] JOIN/PART/PRIVMSG/NOTICE work
- [ ] All modes enforced (+i +t +k +l +o)
- [ ] KICK/INVITE/TOPIC work
- [ ] Operator privileges enforced
- [ ] Clean disconnect (no crashes, no leaks)
- [ ] `\r\n` on every message
- [ ] Can explain design confidently

---

**This plan prioritizes passing over perfection.  
Bonus features only after mandatory is rock solid.**
