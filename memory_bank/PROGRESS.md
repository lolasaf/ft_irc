# ft_irc — Project Progress Tracker

This file tracks all completed work and remaining tasks for the ft_irc project.

---

## 🎓 Learning Guidelines (ALWAYS FOLLOW)

**For every new concept or code implementation, follow these steps:**

1. **Explain** — Understand the concept/topic/function before writing code
2. **Pseudo-code** — Get code with blanks to fill in (learning sections marked with `// TODO: [YOUR CODE]`)
3. **Try first** — Attempt to solve it yourself, ask questions for help
4. **Review** — Compare your solution, understand what was right/wrong

**⚠️ NO CODE GENERATION WITHOUT LEARNING FIRST**

---

## Current Status: **Week 1, Day 6 — JOIN + PART + PRIVMSG (Up Next)**

---

## ✅ Completed

### Day 1 — Project Skeleton & Socket Setup ✅
- [x] Repository initialized
- [x] Basic directory structure (`src/`, `include/`, `memory_bank/`)
- [x] Makefile created with required rules (`all`, `clean`, `fclean`, `re`)
- [x] Compilation flags: `-Wall -Wextra -Werror -std=c++98`
- [x] Argument validation (port 1-65535, password)
- [x] `setupSocket()` implemented:
  - [x] `socket()` — create TCP socket
  - [x] `setsockopt()` — SO_REUSEADDR
  - [x] `sockaddr_in` — configured with AF_INET, INADDR_ANY, htons(port)
  - [x] `bind()` — bind to address
  - [x] `fcntl()` — set non-blocking
  - [x] `listen()` — start listening
- [x] Server successfully listens on port (tested with `nc -zv`)

### Day 2 — Event Loop + Accept ✅ (switched to poll() in Day 4.5)
- [x] Implement `fd_set` preparation (later replaced with pollfd vector)
- [x] Main loop with `select()` (later replaced with `poll()`)
- [x] Proper `errno == EINTR` handling for signals
- [x] Accept new users on listen fd ready
- [x] Set accepted sockets non-blocking
- [x] Create `User` class with fd and buffers
- [x] Store users in `std::map<int, User*>`
- [x] Clean up users in Server destructor
- [x] Add client fds to `select()` readFds
- [x] Read data from clients with `recv()`
- [x] Detect client disconnect (recv returns 0)
- [x] Clean disconnect handling (close, delete, erase from map)
- [x] Safe map iteration (increment before potential erase)

### Day 3 — Input Buffering & Packet Reassembly ✅
- [x] Append received data to User's input buffer
- [x] Extract complete messages on `\n` using `.find('\n')`
- [x] Handle both `\r\n` and `\n` line endings (strip `\r`)
- [x] Handle partial packets (data split across recv calls)
- [x] Loop to extract multiple messages from single recv()
- [x] C++98 compatible string operations (no `.back()` or `.pop_back()`)
- [x] Tested: Multiple messages received in single packet properly separated
- [x] Tested: Malformed packets handled gracefully

### Day 4 — Output Buffering & Write Handling ✅
- [x] Added `writeFds` fd_set for tracking writable fds
- [x] Only add fd to writeFds when outputBuffer is non-empty
- [x] `select()` now watches both readFds AND writeFds
- [x] Implemented `handleClientWrite()` with `send()`
- [x] Proper partial write handling (erase only sent bytes)
- [x] EAGAIN/EWOULDBLOCK error handling
- [x] Never call `send()` directly — always queue to outputBuffer
- [x] Tested: Welcome message sent to clients on connect

### Day 4.5 — Switch from select() to poll() ✅
- [x] Replaced `fd_set` with `std::vector<struct pollfd>`
- [x] Replaced `FD_ZERO/FD_SET/FD_ISSET` with pollfd array manipulation
- [x] Replaced `select()` with `poll()`
- [x] Added `POLLIN` for read watching, `POLLOUT` for write watching
- [x] Added `POLLERR | POLLHUP | POLLNVAL` error detection
- [x] Removed `maxFd` tracking (not needed with poll)
- [x] Tested: Server works identically with poll()
- [x] Updated `Functions_explained.md` with poll() documentation

### Day 5 — PASS / NICK / USER Registration (Part 1: Message Parsing) ✅
- [x] Created `Message` struct (command + params vector)
- [x] Implemented `parseMessage()` in `message.cpp`
  - [x] Skip optional prefix (starts with `:`)
  - [x] Extract command (convert to uppercase)
  - [x] Parse space-separated params
  - [x] Handle trailing param (`:` prefix for spaces)
- [x] Refactored `handleClientData()` into 3 functions:
  - [x] `handleClientData()` — recv and buffer append
  - [x] `extractMessages()` — extract complete lines from buffer
  - [x] `processMessage()` — parse and route to handlers
- [x] Updated User class with registration fields:
  - [x] `passOk` (bool) — password accepted?
  - [x] `nickname`, `username`, `realname`, `hostname` (strings)
  - [x] `isRegistered()` — checks passOk && nickname && username
  - [x] Getters/setters for all fields
  - [x] Proper initialization in constructor (`passOk(false)`)
- [x] Tested: parseMessage correctly tokenizes IRC commands

#### Day 5 Tasks — PASS / NICK / USER Registration (Part 2: Handlers) ✅
- [x] Implemented `handlePass()` — checks password, sets `passOk`, uses `sendNumeric` for errors
- [x] Added `include/replies.hpp` — central enum for numeric reply codes
- [x] Implemented `sendNumeric()` helper in `Server` — formats and queues numeric replies
- [x] Implemented `handleNick()` — validate and check uniqueness, allow change after registration
- [x] Implemented `handleUser()` — extract username/realname, truncate as needed, validate, handle errors
- [x] Send `001` RPL_WELCOME and all registration numerics on registration complete
- [x] Implemented and tested ISUPPORT (005) numeric with USERLEN, NICKLEN, REALLEN
- [x] Added error handling for missing/invalid parameters in registration commands
- [x] Added debug output for registration and command handling
- [x] Tested registration flow with various valid and invalid input cases
- [x] Confirmed protocol compliance for registration and error numerics

---

## 🔄 In Progress

## 📋 TODO — Week 1 (Networking Core & Basic IRC)

### Day 6 — JOIN + PART + PRIVMSG
- [ ] Create `Channel` class
- [ ] Implement `handleJoin()` — create/join channel, first joiner is op
- [ ] Implement `handlePart()` — leave channel, delete if empty
- [ ] Implement `handlePrivmsg()` — to channel or nick

### Day 7 — QUIT, NOTICE & Cleanup
- [ ] Implement `handleQuit()` — broadcast and cleanup
- [ ] Implement `handleNotice()` — same as PRIVMSG, no auto-reply
- [ ] Handle disconnect cleanup properly

---

## 📋 TODO — Week 2 (Channel Operators & MODE)

### Day 8 — Channel Operators (+o / -o)
- [ ] Store operators in Channel
- [ ] First user to JOIN becomes operator
- [ ] MODE +o/-o implementation

### Day 9 — MODE Core Parsing
- [ ] Parse mode strings (`+itkl`, `+o-o`, etc.)
- [ ] Track current sign (`+` or `-`)
- [ ] Consume arguments in correct order

### Day 10 — MODE Enforcement (+i +t +k +l)
- [ ] `+i` invite-only
- [ ] `+t` topic protection
- [ ] `+k` channel key
- [ ] `+l` user limit
- [ ] `can_user_join()` checks all modes

### Day 11 — INVITE
- [ ] `handleInvite()` — require op, store invite
- [ ] Check invitation list for +i channels

### Day 12 — TOPIC
- [ ] `handleTopic()` — view and set topic
- [ ] Respect +t mode (ops only)

### Day 13 — KICK
- [ ] `handleKick()` — require op, remove target

### Day 14 — LIST + Stress Testing
- [ ] `handleList()` — channel list with user counts
- [ ] Test mode combinations

---

## 📋 TODO — Week 3 (Hardening & Defense)

### Day 15-21
- [ ] Evaluator trap audit
- [ ] Edge case testing
- [ ] IRC formatting verification (`\r\n`)
- [ ] Client compatibility testing (HexChat, irssi)
- [ ] Valgrind memory testing
- [ ] Defense rehearsal

---

## 🐛 Known Issues

1. ~~**Makefile references non-existent files**~~ — Fixed
2. ~~**`setupSocket()` not implemented**~~ — Fixed  
3. ~~**Duplicate socket variables**~~ — Fixed (removed `serverSocket`)

---

## 📁 Current File Structure

```
ft_irc/
├── .gitignore
├── Makefile
├── README.md
├── include/
│   ├── message.hpp       ← Message struct + parseMessage()
│   ├── replies.hpp       ← NEW: ReplyCode enum (001-502)
│   ├── server.hpp        ← UPDATED: sendNumeric, registerUser, ISUPPORT
│   └── user.hpp          ← UPDATED: isRegistered field + getters
├── memory_bank/
│   ├── PROGRESS.md (this file)
│   ├── Functions_explained.md
│   ├── Knowledge_base.md
│   ├── Research.md
│   ├── ft_irc_3_week_execution_plan.md
│   ├── ft_irc_architecture_cxx98.md
│   ├── ft_irc_command_by_command_plan.md
│   ├── ft_irc_evaluator_traps_common_mistakes.md
│   └── ft_irc_subject.md
└── src/
    ├── main.cpp
    ├── message.cpp        ← parseMessage() implementation
    ├── server.cpp         ← Core server (poll loop, accept, recv/send)
    ├── serverMessage.cpp  ← NEW: extractMessages, processMessage
    ├── serverUserReg.cpp  ← NEW: PASS/NICK/USER handlers, sendNumeric
    └── user.cpp           ← User class with registration fields
```

---

## 📝 Session Log

### Session 1 — December 17, 2025
- Reviewed memory_bank documentation
- Analyzed current codebase state
- Created PROGRESS.md to track project status
- **Completed Day 1**: Implemented `setupSocket()`
  - Learned: `socket()`, `setsockopt()`, `bind()`, `fcntl()`, `listen()`
  - Learned: `sockaddr_in` structure, `INADDR_ANY`, `htons()` byte order conversion
  - Tested: Server successfully accepts connections on port 6667

### Session 2 — December 19, 2025
- **Completed Day 2**: Event loop + User class + recv()
  - Learned: `select()`, `fd_set`, `FD_ZERO`, `FD_SET`, `FD_ISSET`
  - Learned: `accept()` creates new fd for each client
  - Learned: `errno == EINTR` handling for signals
  - Learned: `recv()` return values (>0 data, 0 disconnect, -1 error)
  - Learned: Safe map iteration when erasing (increment before erase)
  - Created `User` class with fd and input/output buffers
  - Users stored in `std::map<int, User*>`
  - Implemented `handleClientData()` with recv and disconnect handling
  - Tested: Server receives messages and handles disconnects cleanly
- Added `.gitignore` file
- Updated `Functions_explained.md` with select(), accept(), User class
- **Next step**: Day 3 — Input buffering & packet reassembly

### Session 3 — December 21, 2025
- **Completed Day 3**: Input buffering & packet reassembly ✅
  - Learned: Why IRC requires `\r\n` (historical protocol standard, multi-platform compatibility)
  - Learned: Why messages must be buffered (packets don't align with message boundaries)
  - Learned: Difference between blocking and non-blocking sockets + `select()` multiplexing
  - Learned: `std::string::find()`, `.substr()`, `.erase()` for message extraction
  - Learned: C++98 compatibility (no `.back()` or `.pop_back()`, use `.length()` and `.erase()`)
  - Implemented complete message extraction loop in `handleClientData()`
  - Implemented `\r\n` stripping while keeping `\n` to find message boundaries
  - Implemented safe buffer manipulation with `.find()`, `.substr()`, `.erase()`
  - Tested: Multiple messages in single packet properly separated
  - Tested: Partial packets buffered correctly
  - Verified: Each message printed on separate line (no `\r` or extra `\n`)
- **Key insights**:
  - Terminal display hiding the issue: `\n` in one blob looked like multiple messages
  - `erase(0, pos)` vs `erase(0, pos + 1)` — must include the `\n` in removal
  - Message extraction without including `\n` = cleaner approach
- **Next step**: Day 4 — Output buffering & write handling

### Session 4 — December 23, 2025
- **Completed Day 4**: Output buffering & write handling ✅
  - Learned: Why we can't call `send()` directly (evaluator trap, partial writes, blocking risk)
  - Learned: Output buffer pattern (queue message → select detects writable → send)
  - Learned: `send()` function and return values (>0 bytes sent, 0 closed, -1 error)
  - Learned: Partial write handling (only erase bytes actually sent)
  - Implemented `writeFds` fd_set preparation in `run()`
  - Implemented `handleClientWrite()` with proper error handling
  - Added welcome message test in `acceptNewClient()`
  - Tested: Client receives welcome message on connect
- **Key pattern**:
  - `user->getOutputBuffer() += "message\r\n";` (queue it)
  - Never call `send()` directly in command handlers
- **Next step**: Day 5 — PASS / NICK / USER registration

### Session 5 — January 11, 2026
- **Completed Day 4.5**: Switch from select() to poll() ✅
  - Learned: poll() vs select() differences (array of structs vs bitmasks)
  - Learned: pollfd structure (fd, events, revents)
  - Learned: Event flags (POLLIN, POLLOUT, POLLERR, POLLHUP, POLLNVAL)
  - Learned: No maxFd needed with poll(), no fd limit
  - Refactored `run()` to use `std::vector<struct pollfd>`
  - Added proper error/hangup detection with POLLERR|POLLHUP|POLLNVAL
  - Tested: Server works identically with poll()
- **Key insight**:
  - `events` = what you WANT to watch (input)
  - `revents` = what ACTUALLY happened (output, filled by poll)
  - Use `&` (bitwise AND) to check flags: `if (pfd.revents & POLLIN)`
- **Started Day 5**: PASS / NICK / USER Registration
  - Created `Message` struct and `parseMessage()` function
  - Learned: IRC message format `[:<prefix>] <command> [<params>] [:<trailing>]`
  - Learned: Trailing param (after `:`) is ONE param even with spaces
  - Refactored `handleClientData()` into 3 focused functions
  - Updated User class with registration fields and proper initialization
  - Tested: parseMessage correctly tokenizes all IRC commands
- **Key pattern**:
  - Separate tokenization (parseMessage) from routing (processMessage)
  - Keep functions focused: one job per function
- **Next step**: Implement handlePass(), handleNick(), handleUser()

### Session 6 — January 17-18, 2026
- **Completed Day 5**: PASS / NICK / USER Registration ✅
  - Created `include/replies.hpp` with ReplyCode enum (all IRC numeric codes)
  - Implemented `sendNumeric()` helper for formatted numeric replies
  - Implemented `handlePass()` with password validation and ERR_PASSWDMISMATCH
  - Implemented `handleNick()` with:
    - Validation (alphanumeric + underscore, max 9 chars)
    - Uniqueness check (case-insensitive)
    - ERR_NONICKNAMEGIVEN, ERR_ERRONEUSNICKNAME, ERR_NICKNAMEINUSE
  - Implemented `handleUser()` with:
    - Parameter extraction (username, realname)
    - Validation and truncation (USERLEN=18, REALLEN=50)
    - ERR_NEEDMOREPARAMS, ERR_ALREADYREGISTRED
  - Implemented `registerUser()` to send welcome sequence:
    - RPL_WELCOME (001), RPL_YOURHOST (002), RPL_CREATED (003)
    - RPL_MYINFO (004), RPL_ISUPPORT (005)
  - Added ISUPPORT tokens: USERLEN, NICKLEN, REALLEN
  - Implemented `handlePing()` with PONG response
  - Split server code into multiple files:
    - `serverMessage.cpp` — message extraction and routing
    - `serverUserReg.cpp` — registration handlers and sendNumeric
- **Key patterns**:
  - Centralized numeric codes in enum for consistency
  - `sendNumeric()` handles formatting (3-digit codes, server prefix)
  - `registerUser()` called after each registration command to check completion
- **Next step**: Day 6 — JOIN + PART + PRIVMSG

---

## Quick Reference

### Mandatory Commands
- Registration: `PASS`, `NICK`, `USER`
- Channel: `JOIN`, `PART`, `TOPIC`, `KICK`, `INVITE`
- Messaging: `PRIVMSG`, `NOTICE`
- Modes: `MODE` (+i, +t, +k, +l, +o)
- Connection: `QUIT`

### Critical Evaluator Traps
1. ❌ read/write outside poll()/select() readiness = grade 0
2. ❌ Blocking sockets
3. ❌ Multiple poll()/select() calls
4. ❌ Assuming one recv() == one command
5. ❌ Direct send() in command handlers
