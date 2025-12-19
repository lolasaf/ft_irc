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

## Current Status: **Week 1, Day 3 — Input Buffering & Packet Reassembly (Up Next)**

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

### Day 2 — select() Event Loop + Accept ✅
- [x] Implement `fd_set` preparation
- [x] Main loop with `select()`
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

---

## 🔄 In Progress

### Day 3 Tasks — Input Buffering & Packet Reassembly
- [ ] Append received data to User's input buffer
- [ ] Extract complete messages on `\n`
- [ ] Handle both `\r\n` and `\n` line endings
- [ ] Parse and tokenize IRC commands
- [ ] Handle partial packets (data split across recv calls)

---

## 📋 TODO — Week 1 (Networking Core & Basic IRC)

### Day 4 — Output Buffering & Write Handling
- [ ] Per-user output buffer (`std::string _outputBuffer`)
- [ ] Never `send()` directly in command handlers
- [ ] Add fd to write set only if output buffer non-empty
- [ ] On write ready: `send()` and erase sent portion

### Day 5 — PASS / NICK / USER Registration
- [ ] Create `Command` class (static methods)
- [ ] Implement tokenizer for IRC messages
- [ ] Implement `handlePass()` — check password
- [ ] Implement `handleNick()` — validate and check uniqueness
- [ ] Implement `handleUser()` — extract username/realname
- [ ] Track registration state in User
- [ ] Send `001` welcome on registration

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
│   ├── server.hpp
│   └── user.hpp
├── memory_bank/
│   ├── PROGRESS.md (this file)
│   ├── ft_irc_3_week_execution_plan.md
│   ├── ft_irc_architecture_cxx98.md
│   ├── ft_irc_command_by_command_plan.md
│   ├── ft_irc_evaluator_traps_common_mistakes.md
│   └── ft_irc_subject.md
└── src/
    ├── main.cpp
    ├── server.cpp
    └── user.cpp
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

---

## Quick Reference

### Mandatory Commands
- Registration: `PASS`, `NICK`, `USER`
- Channel: `JOIN`, `PART`, `TOPIC`, `KICK`, `INVITE`
- Messaging: `PRIVMSG`, `NOTICE`
- Modes: `MODE` (+i, +t, +k, +l, +o)
- Connection: `QUIT`

### Critical Evaluator Traps
1. ❌ read/write outside select() readiness = grade 0
2. ❌ Blocking sockets
3. ❌ Multiple select()/poll() calls
4. ❌ Assuming one recv() == one command
5. ❌ Direct send() in command handlers
