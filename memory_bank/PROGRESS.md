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

## Current Status: **Mandatory Part Complete ✅ + Bot Bonus Complete ✅**

### Quick Summary
| Category | Status |
|----------|--------|
| **Registration** | ✅ PASS, NICK, USER |
| **Channels** | ✅ JOIN, PART, TOPIC, KICK, INVITE |
| **Messaging** | ✅ PRIVMSG, NOTICE |
| **Modes** | ✅ +i, +t, +k, +l, +o |
| **Connection** | ✅ QUIT |
| **Security** | ✅ CR/LF injection prevented, input sanitization |
| **Memory** | ✅ Valgrind: 0 leaks |
| **Evaluator Traps** | ✅ All avoided (poll-only I/O, non-blocking, buffered) |
| **Bonus: Bot** | ✅ IRC bot with !help, !time, !weather, greeting, logging |

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

### Day 6 — JOIN + PART Commands Implementation ✅
- [x] Created `Channel` class (`include/channel.hpp`, `src/channel.cpp`)
  - [x] Member management (`std::set<User*>` for members and operators)
  - [x] Mode tracking (invite-only, topic protection, key, user limit)
  - [x] Topic management (get/set topic, topic setter, timestamp)
  - [x] Operator management (add/remove operators, check operator status)
  - [x] `canJoin()` method with mode checks (invite-only, key, user limit)
  - [x] `getNamesList()` for NAMES reply (with @ prefix for operators)
  - [x] `broadcast()` method for sending messages to all members
- [x] Implemented `handleJoin()` — create/join channel, first joiner is operator
  - [x] Channel validation (must start with #, max 50 chars)
  - [x] Case-insensitive channel lookup (O(log n) with lowercase keys)
  - [x] Channel creation (first joiner becomes operator)
  - [x] Mode enforcement (invite-only, key, user limit)
  - [x] JOIN broadcast to all members
  - [x] Topic replies (331/332/333)
  - [x] Names list (353/366)
  - [x] Invite removal after successful join
- [x] Implemented `handlePart()` — leave channel, delete if empty
  - [x] Multiple channel support (comma-separated)
  - [x] Optional PART message parameter
  - [x] PART broadcast to all channel members
  - [x] Bidirectional cleanup (channel → user, user → channel)
  - [x] Automatic channel deletion when empty
  - [x] Error handling (ERR_NOSUCHCHANNEL, ERR_NOTONCHANNEL)
- [x] Created utility functions (`include/utils.hpp`, `src/utils.cpp`)
  - [x] String manipulation (toUpper, toLower, trim)
  - [x] Parsing utilities (splitCommaList, split)
  - [x] Case-insensitive comparison (caseInsensitiveCompare, caseInsensitiveLess)
- [x] Updated User class with channel tracking
  - [x] `std::set<Channel*>` to track user's channels
  - [x] `addChannel()`, `removeChannel()`, `getChannels()` methods
- [x] Server channel management
  - [x] `findChannel()` — O(log n) case-insensitive lookup with lowercase keys
  - [x] `createChannel()` — creates channel, stores with lowercase key
  - [x] `deleteChannel()` — removes empty channels
  - [x] Channel storage in `std::map<std::string, Channel*>` (lowercase keys)
  - [x] All channel names stored in lowercase (Channel object stores lowercase)
- [x] User disconnect cleanup (`handleDisconnect()`)
  - [x] Removes user from all channels on disconnect
  - [x] Broadcasts QUIT message to channel members
  - [x] Bidirectional cleanup (prevents dangling pointers)
  - [x] Automatic channel deletion when empty
  - [x] Integrated into all disconnect paths (error, hangup, recv=0, send error)
  - [x] Server destructor cleanup
- [x] Makefile improvements
  - [x] Separate object directory (`obj/`) for all `.o` files
  - [x] Automatic directory creation
  - [x] Clean removes entire `obj/` directory
  - [x] Updated `.gitignore` to exclude `obj/`
- [x] Fixed critical bugs:
  - [x] JOIN message format (removed extra colon before channel name)
  - [x] `canJoin()` logic (check user limit before invite check)
  - [x] PART message parameter extraction
  - [x] Dangling pointer cleanup on user disconnect
- [x] Tested: JOIN creates channel, first joiner becomes operator, receives topic and names list
- [x] Tested: PART leaves channel, broadcasts message, deletes empty channels

### Day 6 (Part 3) — PRIVMSG Implementation ✅
- [x] Created comprehensive PRIVMSG pseudo code (`memory_bank/PRIVMSG_pseudo_code.md`)
- [x] Implemented `handlePrivmsg()` in `serverMessage.cpp`
  - [x] Registration check (ERR_NOTREGISTERED 451)
  - [x] Parameter validation (ERR_NORECIPIENT 411, ERR_NOTEXTTOSEND 412)
  - [x] Multiple target support (comma-separated with `splitCommaList()`)
  - [x] Channel vs user detection (`#` prefix check)
  - [x] Channel membership check (ERR_CANNOTSENDTOCHAN 404)
  - [x] Case-insensitive user lookup
  - [x] Error handling (ERR_NOSUCHCHANNEL 403, ERR_NOSUCHNICK 401)
- [x] Added PRIVMSG route in `processMessage()`
- [x] Tested: Channel messages broadcast to all members (except sender)
- [x] Tested: Private messages delivered to specific user
- [x] Tested: All error cases return correct numeric codes

---

## 🔄 In Progress

*None — All mandatory commands implemented!*

## ✅ Week 2 Progress

### Day 8-10 — MODE Command ✅
- [x] MODE +o/-o operator management
- [x] MODE core parsing (parse mode strings, track sign, consume args)
- [x] MODE enforcement (+i, +t, +k, +l)
- [x] Mode query (RPL_CHANNELMODEIS 324)
- [x] Operator check before mode changes
- [x] Error handling (ERR_CHANOPRIVSNEEDED, ERR_USERNOTINCHANNEL, ERR_UNKNOWNMODE)
- [x] Broadcast MODE changes to channel
- [x] Added MODE route in `processMessage()`
- [x] Refactored `handleMode()` into smaller functions:
  - [x] `sendChannelModes()` — query and send current modes
  - [x] `applySingleMode()` — handle one mode character
  - [x] `applyChannelModes()` — parse mode string and apply
  - [x] `findUserByNick()` — reusable utility (moved to serverUtils.cpp)
- [x] Created `serverCommandsMode.cpp` for MODE-related functions
- [x] Tested: +i, +t, +k, +l, +o/-o all working
- [x] Tested: Error cases (unknown mode, missing params, not op)

---

## ✅ Week 1 Complete

### Day 7 — QUIT, NOTICE ✅
- [x] Handle disconnect cleanup properly (`handleDisconnect()` implemented)
- [x] Implement `handleQuit()` — graceful quit with broadcast
  - [x] Extract quit reason from params (default "Client Quit")
  - [x] Build QUIT message with hostmask
  - [x] Track notified users with `std::set<User*>` to avoid duplicates
  - [x] Iterate all channels, broadcast to members sharing channels
  - [x] Mark user for disconnection (flag approach for safety)
- [x] Added `markedForDisconnection` flag to User class
  - [x] Private bool member + getter `isMarkedForDisconnection()` + setter `markForDisconnection()`
  - [x] Initialized to false in constructor
- [x] Poll loop checks flag after `processMessage()` returns
  - [x] Calls `handleDisconnect()` for channel cleanup
  - [x] Closes socket, deletes user, erases from map
  - [x] Uses `return` instead of `continue` to exit `handleClientData()` safely
- [x] Added `getMembers()` method to Channel class (returns `const std::set<User*>&`)
- [x] Added QUIT route in `processMessage()`
- [x] Refactored PRIVMSG/NOTICE into shared `handleMessageCommand()`
- [x] Implemented `handleNotice()` — thin wrapper calling `handleMessageCommand()`
- [x] Added NOTICE route in `processMessage()`
- [x] Tested: NOTICE works for both channel and user targets

---

## 📋 TODO — Week 2 (Channel Operators & MODE)

### Day 8-10 — MODE ✅ (Completed above)

### Day 11 — INVITE ✅
- [x] `handleInvite()` — require op, store invite
- [x] RPL_INVITING (341) — confirmation to inviter
- [x] INVITE notification sent to invitee
- [x] Store invite in channel's invitation list for +i bypass
- [x] Error handling:
  - [x] ERR_NOSUCHNICK (401) — target user doesn't exist
  - [x] ERR_NOSUCHCHANNEL (403) — channel doesn't exist
  - [x] ERR_NOTONCHANNEL (442) — inviter not on channel
  - [x] ERR_USERONCHANNEL (443) — target already on channel
  - [x] ERR_CHANOPRIVSNEEDED (482) — inviter not operator
- [x] Added INVITE route in `processMessage()`
- [x] Added RPL_INVITING to replies.hpp
- [x] Tested: Invite allows +i channel bypass
- [x] Tested: All error cases return correct numerics

### Day 12 — TOPIC ✅
- [x] `handleTopic()` — view and set topic
- [x] Respect +t mode (ops only can set topic, anyone can view)
- [x] RPL_TOPIC (332) — send current topic
- [x] RPL_NOTOPIC (331) — no topic set
- [x] RPL_TOPICWHOTIME (333) — who set topic and when
- [x] Topic broadcast on change to all channel members
- [x] Error handling (ERR_NOTONCHANNEL, ERR_CHANOPRIVSNEEDED)
- [x] Extracted `sendTopicInfo()` helper to serverUtils.cpp
  - Reused in both `handleTopic()` and `joinChannel()`
  - Reduces code duplication (~16 lines saved)

### Day 13 — KICK ✅
- [x] `handleKick()` — require op, remove target from channel
- [x] Registration check (ERR_NOTREGISTERED 451)
- [x] Parameter validation (ERR_NEEDMOREPARAMS 461)
- [x] Find channel (ERR_NOSUCHCHANNEL 403)
- [x] Kicker on channel check (ERR_NOTONCHANNEL 442)
- [x] Operator check (ERR_CHANOPRIVSNEEDED 482)
- [x] Target user exists (ERR_NOSUCHNICK 401)
- [x] Target on channel check (ERR_USERNOTINCHANNEL 441)
- [x] KICK broadcast to all channel members (including target)
- [x] Remove target from channel (bidirectional cleanup)
- [x] Delete empty channel if needed
- [x] Optional kick reason (defaults to kicker's nickname)
- [x] Added KICK route in `processMessage()`
- [x] Tested: Operator kicks user successfully
- [x] Tested: Non-operator gets ERR_CHANOPRIVSNEEDED

### Day 13.5 — Refactoring & Bug Fixes ✅
- [x] **Helper function extraction** — Reduced code duplication (~66 lines saved)
  - [x] `requireRegistered(user)` — returns false if not registered
  - [x] `requireParams(user, msg, count, cmd)` — validates param count
  - [x] `requireChannel(user, name)` — finds channel or sends error
  - [x] `requireOnChannel(user, chan)` — membership check
  - [x] `requireOperator(user, chan)` — operator check
  - [x] `requireUser(user, nick)` — finds user or sends error
  - [x] Refactored: handleJoin, handlePart, handleTopic, handleInvite, handleKick
- [x] **Memory leak testing** — Valgrind confirmed 0 leaks
- [x] **Created TESTS.md** — Comprehensive test commands for all features
- [x] **Bug fix: MODE +l validation**
  - [x] Previously: `atoi()` + `size_t` allowed negative → huge, non-numeric → 0
  - [x] Fixed: Digits-only validation, range 1-10000
- [x] **Bug fix: MODE query security**
  - [x] Previously: Non-members could query MODE and see channel key
  - [x] Fixed: MODE query now requires channel membership (ERR_NOTONCHANNEL 442)
- [x] **Bug fix: findUserByNick case sensitivity**
  - [x] Previously: Used `==` comparison (case-sensitive)
  - [x] Fixed: Uses `caseInsensitiveCompare()` for consistency with NICK/PRIVMSG
- [x] **Bug fix: handleDisconnect QUIT duplication**
  - [x] Previously: Sent QUIT once per channel (duplicates to shared peers)
  - [x] Fixed: Deduplicated using `std::set<User*> notified`
- [x] **Bug fix: QUIT reason and double-broadcast**
  - [x] Previously: `handleDisconnect()` ignored real reason, double-broadcast possible
  - [x] Fixed: Added `quitReason` and `quitBroadcast` fields to User
  - [x] `handleQuit()` stores reason and marks broadcast=true
  - [x] `handleDisconnect()` skips broadcast if already done, uses stored reason
- [x] **Bug fix: MODE empty target guard**
  - [x] Previously: `target[0]` accessed without checking `target.empty()`
  - [x] Parser produces empty param for `MODE :` → undefined behavior
  - [x] Fixed: Check `target.empty()` before `target[0]`, send ERR_NEEDMOREPARAMS
- [x] **Updated TESTS.md** — Added new test cases:
  - [x] Test 5.4: Case-insensitive INVITE
  - [x] Test 6.4: Case-insensitive KICK
  - [x] Test 7.3b: MODE +l validation (invalid values rejected)
  - [x] Test 7.4b: Case-insensitive MODE +o
  - [x] Test 7.5: MODE query security (non-member blocked)
- [x] **Bug fix: const_iterator for getMembers()**
  - [x] Previously: Used `std::set<User*>::iterator` on `const std::set<User*>&`
  - [x] Fixed: Changed to `std::set<User*>::const_iterator` in handleDisconnect/handleQuit
- [x] **Bug fix: Channel key exposure in MODE query**
  - [x] Previously: `sendChannelModes()` returned actual key value (`+k secretkey`)
  - [x] Fixed: Key is now masked as `*` (`+k *`) — defense-in-depth
- [x] **Bug fix: MODE +o/-o error codes**
  - [x] Previously: Both "user not found" and "user not on channel" returned ERR_USERNOTINCHANNEL (441)
  - [x] Fixed: Returns ERR_NOSUCHNICK (401) if user doesn't exist, ERR_USERNOTINCHANNEL (441) if not on channel
- [x] **Bug fix: MODE partial application without broadcast**
  - [x] Previously: On error, `applyChannelModes()` returned false → no broadcast, but earlier modes already applied
  - [x] Fixed: Refactored to track successful modes, broadcast only what succeeded, continue on errors
- [x] **Bug fix: Server destructor fd leak**
  - [x] Previously: `~Server()` deleted Users without closing client socket fds
  - [x] Fixed: Added `close(it->first)` before `delete it->second` in destructor
- [x] **Optimization: handleQuit channel iteration**
  - [x] Previously: Iterated all server channels, called `isMember()` on each → O(total_channels)
  - [x] Fixed: Iterate `user->getChannels()` directly → O(user's_channels)
- [x] **Updated TESTS.md** — Added more test cases:
  - [x] Test 7.4c: MODE +o with non-existent user (401 not 441)
  - [x] Test 7.5b: MODE query key masking (`+k *`)
  - [x] Test 7.6: MODE partial application
- [x] **Bug fix: Disconnect path ignored stored quit reason**
  - [x] Previously: `handleClientData()` called `handleDisconnect(user, "Client Quit")` with hardcoded string
  - [x] Fixed: Now retrieves `user->getQuitReason()` and passes the actual reason
- [x] **Bug fix: sendChannelModes() embedded spaces in param**
  - [x] Previously: Built `modeStr + modeArgs` as one param with embedded spaces (`"+tkl * 50"`)
  - [x] Fixed: Mode string and each argument are separate params (`["+tkl", "*", "50"]`)
- [x] **Security fix: IRC protocol injection via CR/LF**
  - [x] Vulnerability: User-controlled text (QUIT, KICK, PART, TOPIC) embedded directly in IRC lines
  - [x] Attack: Malicious client sends `QUIT :bye\r\nPRIVMSG #admin :hacked` → injects commands
  - [x] Fixed: Added `sanitizeIrcText()` helper that strips `\r` and `\n` characters
  - [x] Applied to: QUIT reason, KICK reason, PART message, TOPIC text
- [x] **Security fix: MODE +k key injection**
  - [x] Vulnerability: Channel key stored/broadcast without sanitization
  - [x] Fixed: Sanitize key with `sanitizeIrcText()`, reject if empty after sanitization
- [x] **Makefile fix: Stray .o files in src/**
  - [x] Issue: `.o` files in `src/` not cleaned by `make fclean` (only `obj/` was removed)
  - [x] Fixed: Added `rm -f $(SRCS_DIR)/*.o` to `clean` rule
- [x] **Refactoring: handleMode() uses shared helpers**
  - [x] Replaced inline checks with `requireRegistered`, `requireParams`, `requireChannel`, etc.
  - [x] Reduced ~20 lines, consistent error messages across commands
- [x] **Security fix: MODE broadcast args sanitization**
  - [x] Previously: `appliedArgs` collected raw `msg.params[j]` for broadcast
  - [x] Fixed: Sanitize args with `sanitizeIrcText()` before adding to `appliedArgs`
  - [x] Defense-in-depth: Even if applySingleMode sanitizes, broadcast path is now safe
- [x] **Security fix: Invite tracking by User* instead of nickname**
  - [x] Previously: Stored nickname string in invite list (transferable, breaks on NICK change)
  - [x] Fixed: Store User* pointer for stable identity
  - [x] Added cleanup: handleDisconnect() removes user from all channel invite lists
- [x] **Refactoring: PRIVMSG/NOTICE uses findUserByNick()**
  - [x] Previously: Duplicated nickname lookup loop in handleMessageCommand()
  - [x] Fixed: Now uses shared findUserByNick() helper
  - [x] Single source of truth for case-insensitive nick lookups
- [x] **Bug fix: Channel +t default**
  - [x] Previously: _topic_protection initialized to false
  - [x] Fixed: Now defaults to true (matches IRC convention and test expectations)
- [x] **Bug fix: Iterator safety in disconnect paths**
  - [x] Previously: Deleted user before erasing from map, iterator could be invalidated
  - [x] Fixed: Erase from map first, then delete user (safer iterator handling)
  - [x] Applied to: handleClientData (marked disconnection), bytesRead == 0, error branches, handleClientWrite error branch
- [x] **Bug fix: Idempotent MODE operations broadcasting**
  - [x] Previously: MODE changes broadcasted even when state didn't change (e.g., `MODE +i` when already invite-only)
  - [x] Fixed: Check if state would change before applying mode
  - [x] Applied to: +i/-i, +t/-t, +k/-k, +l/-l (already correct for +o/-o via addOperator/removeOperator)
  - [x] Result: Idempotent operations silently ignored (no broadcast), matches IRC standard behavior
- [x] **Bug fix: TOPIC command empty trailing param**
  - [x] Previously: `TOPIC #chan :` (empty trailing) and `TOPIC #chan : ` (whitespace) didn't clear topic correctly
  - [x] Fixed: Added `trim()` after sanitization to handle whitespace-only topics
  - [x] Parser already preserves empty trailing params correctly
  - [x] Result: Both `TOPIC #chan :` and `TOPIC #chan : ` now correctly clear the topic

### Day 14 — LIST + Stress Testing
- [ ] `handleList()` — channel list with user counts
- [ ] Test mode combinations

---

## ✅ Bonus — IRC Bot Complete

### Bot Implementation ✅
- [x] Separate binary `ircbot` (not part of main server)
- [x] Connects as IRC client to ircserv
- [x] C++98 compliant (poll-based, non-blocking I/O)
- [x] Command system with `!` prefix

### Bot Features ✅
- [x] **!help** — Lists all available bot commands
- [x] **!time** — Shows current server time
- [x] **!weather <city>** — Mock weather data for requested city
- [x] **Greeting** — Welcomes users when they JOIN the channel
- [x] **Channel Logging** — Logs all channel messages to `bot.log`

### Bot Files
- `src/bot.cpp` — Main bot logic (connect, run loop, message parsing)
- `src/botHandlers.cpp` — Command handlers (handleHelp, handleTime, handleWeather, handleUserJoin)
- `include/bot.hpp` — Bot class header

### Bot Testing ✅
- [x] 5 automated tests added to `ft_irc_tester.py` (B.1–B.5)
- [x] 9 manual test cases added to `Tests/TESTS.md` (Section 16)
- [x] Updated `Tests/ft_irc_tester_instructions.md` with bot testing section

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
4. ~~**Dangling pointers on user disconnect**~~ — Fixed (implemented `handleDisconnect()`)
5. ~~**O(n) channel lookups**~~ — Fixed (lowercase storage, O(log n) lookups)

---

## 📁 Current File Structure

```
ft_irc/
├── .gitignore
├── Makefile
├── README.md
├── ft_irc_tester.py    ← Automated test runner (37 tests)
├── include/
│   ├── bot.hpp           ← Bot class (bonus)
│   ├── channel.hpp       ← Channel class (members, operators, modes)
│   ├── message.hpp       ← Message struct + parseMessage()
│   ├── replies.hpp       ← Numeric reply codes
│   ├── server.hpp        ← Server class with all handlers
│   ├── user.hpp          ← User class with channel tracking
│   └── utils.hpp         ← Utility functions (string manipulation)
├── memory_bank/
│   ├── PROGRESS.md (this file)
│   ├── Knowledge_base.md
│   └── ... (other docs)
├── Tests/
│   ├── TESTS.md              ← Manual test commands (16 sections)
│   └── ft_irc_tester_instructions.md ← Automated tester guide
└── src/
    ├── main.cpp           ← Entry point
    ├── bot.cpp            ← Bot main logic (bonus)
    ├── botHandlers.cpp    ← Bot command handlers (bonus)
    ├── channel.cpp        ← Channel core (constructor, destructor, getName)
    ├── channelMembers.cpp ← Member management (add, remove, canJoin, broadcast, getNamesList)
    ├── channelModes.cpp   ← Mode getters/setters (+i, +t, +k, +l)
    ├── channelOperators.cpp ← Operator & invite management
    ├── channelTopic.cpp   ← Topic management (get/set topic, setter, timestamp)
    ├── message.cpp        ← parseMessage() implementation
    ├── server.cpp         ← Core server (poll loop, accept, recv/send)
    ├── serverChannel.cpp  ← Channel utilities (find, create, delete, join, leave)
    ├── serverCommands.cpp ← Command handlers (JOIN, PART, QUIT, TOPIC, KICK, INVITE, handleDisconnect)
    ├── serverCommandsMode.cpp ← MODE command (handleMode + helper functions)
    ├── serverMessage.cpp  ← Message routing (extractMessages, processMessage)
    ├── serverUserReg.cpp  ← Registration (PASS, NICK, USER, registerUser)
    ├── serverUtils.cpp    ← Utilities (sendNumeric, buildHostmask, findUserByNick, sendTopicInfo)
    ├── serverUtilsPreconditions.cpp ← Precondition helpers (require* functions)
    ├── user.cpp           ← User class implementation (includes quit tracking)
    └── utils.cpp          ← Global utilities (toUpper, toLower, split, sanitizeIrcText)
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

### Session 7 — January 22-25, 2026
- **Completed Day 6 (Part 1)**: JOIN Command Implementation ✅
  - Created `Channel` class with full functionality:
    - Member and operator management using `std::set<User*>`
    - Mode tracking (invite-only, topic protection, key, user limit)
    - Topic management with setter and timestamp
    - `canJoin()` method with proper mode checking order
    - `getNamesList()` for IRC NAMES reply format
    - `broadcast()` method for channel-wide messaging
  - Implemented `handleJoin()` command:
    - Channel name validation (starts with #, max 50 chars)
    - Case-insensitive channel lookup (O(log n) with lowercase keys)
    - First joiner automatically becomes operator
    - Mode enforcement (invite-only, key, user limit)
    - Proper IRC protocol responses (JOIN broadcast, topic, names list)
  - Created utility functions module:
    - String manipulation (toUpper, toLower, trim)
    - Parsing utilities (splitCommaList, split)
    - Case-insensitive comparison functions
  - Updated User class:
    - Added `std::set<Channel*>` for channel tracking
    - Methods: `addChannel()`, `removeChannel()`, `getChannels()`
  - Server channel management:
    - `findChannel()` — O(log n) case-insensitive lookup
    - `createChannel()` — creates channel, stores with lowercase key
    - `deleteChannel()` — removes empty channels
  - Fixed critical bugs:
    - JOIN message format (removed extra colon)
    - `canJoin()` logic (check limit before invite check)
  - Updated Makefile:
    - Fixed variable expansion issue (blank line between SRCS_FILES and SRCS)
    - Added fallback `rm -f src/*.o` in clean target
  - Updated .gitignore:
    - Added `.cache/` directory (clangd language server cache)
  - Tested: JOIN command works correctly, first joiner becomes operator, receives proper IRC responses
- **Key patterns**:
  - Case-insensitive channel names with original case preservation
  - Channel stored with first user's case, but lookup is case-insensitive
  - Mode checks in proper order (limit → invite → key)
  - All channel operations use actual channel name (preserves case)
- **Next step**: Day 6 (Part 2) — PART + PRIVMSG commands

### Session 8 — January 25, 2026
- **Completed Day 6 (Part 2)**: PART Command + Cleanup Logic ✅
  - Implemented `handlePart()` command:
    - Multiple channel support (comma-separated targets)
    - Optional PART message parameter (defaults to "Leaving")
    - PART broadcast to all channel members (IRC format)
    - Bidirectional cleanup (channel → user, user → channel)
    - Automatic channel deletion when empty
    - Error handling (ERR_NOSUCHCHANNEL, ERR_NOTONCHANNEL)
  - Optimized channel storage:
    - All channel names stored in lowercase everywhere
    - Channel object stores lowercase name
    - Map keys are lowercase (O(log n) lookups)
    - Removed O(n) case-insensitive iteration
  - Implemented `handleDisconnect()` cleanup function:
    - Removes user from all channels on disconnect
    - Broadcasts QUIT message to channel members
    - Prevents dangling pointers (bidirectional cleanup)
    - Automatic channel deletion when empty
    - Integrated into all disconnect paths:
      - POLLERR/POLLHUP/POLLNVAL errors
      - recv() returns 0 (client disconnect)
      - recv() error (non-EAGAIN errors)
      - send() error (non-EAGAIN errors)
      - Server destructor (shutdown cleanup)
  - Makefile improvements:
    - Separate `obj/` directory for all object files
    - Automatic directory creation with order-only prerequisite
    - `make clean` removes entire `obj/` directory
    - Updated `.gitignore` to exclude `obj/`
  - Created PRIVMSG pseudo code:
    - Comprehensive implementation guide
    - Covers channel and user messaging
    - Multiple target support
    - All error cases documented
    - Saved in `memory_bank/PRIVMSG_pseudo_code.md`
  - Tested: PART command works correctly, broadcasts message, cleans up properly
  - Tested: User disconnect properly cleans up all channel relationships
- **Key patterns**:
  - Lowercase channel storage for O(log n) lookups
  - Proper cleanup prevents memory leaks and dangling pointers
  - Bidirectional relationship management (User ↔ Channel)
  - All disconnect paths use centralized cleanup function
- **Next step**: Day 6 (Part 3) — PRIVMSG implementation

### Session 9 — January 26, 2026
- **Completed Day 6 (Part 3)**: PRIVMSG Implementation ✅
  - Implemented `handlePrivmsg()` in `serverMessage.cpp`
  - Followed pseudo code from `PRIVMSG_pseudo_code.md`
  - Channel message flow:
    1. Registration check
    2. Parameter validation (target + message)
    3. Find channel (case-insensitive)
    4. Check sender is member
    5. Build hostmask and broadcast (exclude sender)
  - Private message flow:
    1. Registration check
    2. Parameter validation
    3. Find user by nickname (case-insensitive loop)
    4. Send directly to user's output buffer
  - Added route in `processMessage()`
  - Tested with multiple users:
    - Channel messages correctly broadcast to all members
    - Private messages delivered to target user
    - Error 401 for non-existent users
    - Error 403 for non-existent channels
    - Error 404 for non-members messaging channels
- **Key patterns**:
  - Case-insensitive nickname lookup (O(n) iteration)
  - `broadcastToChannel()` with sender exclusion
  - Continue processing on error (for multiple targets)
- **Refactored PRIVMSG/NOTICE**:
  - Created shared `handleMessageCommand(User*, Message&, string command)`
  - `handlePrivmsg()` and `handleNotice()` are thin wrappers
  - Command name passed as parameter, used in error messages and output
  - No code duplication between PRIVMSG and NOTICE
- **Implemented NOTICE**:
  - Added route in `processMessage()`
  - Tested: NOTICE delivers to users and channels correctly
  - Tested: Error 401 for non-existent users
- **Next step**: Day 7 — QUIT

### Session 10 — January 29, 2026
- **Completed Day 13.5**: Refactoring & Bug Fixes ✅
  - **Helper function extraction** — Reduced code duplication
    - Created 6 `require*` helper functions in serverUtils.cpp
    - Refactored handleJoin, handlePart, handleTopic, handleInvite, handleKick
    - ~66 lines of code saved
  - **Memory testing** — Valgrind confirmed 0 leaks ("definitely lost: 0 bytes")
  - **Created TESTS.md** — Comprehensive test commands for all features
  - **Security fix: MODE +l validation**
    - Bug: `atoi()` + `size_t` cast allowed negative → huge, non-numeric → 0
    - Fix: Digits-only validation, range 1-10000
  - **Security fix: MODE query leak**
    - Bug: Non-members could query MODE and see channel key
    - Fix: MODE query requires channel membership
  - **Consistency fix: findUserByNick()**
    - Bug: Used `==` (case-sensitive) while NICK/PRIVMSG used case-insensitive
    - Fix: Uses `caseInsensitiveCompare()` for INVITE/KICK/MODE +o
  - **Bug fix: handleDisconnect() QUIT duplication**
    - Bug: Sent QUIT once per channel (duplicates to peers sharing channels)
    - Fix: Deduplicated using `std::set<User*> notified`
  - **Bug fix: QUIT reason and double-broadcast**
    - Bug: `handleDisconnect()` ignored client's QUIT reason, double-broadcast possible
    - Fix: Added `quitReason`, `quitBroadcast` fields to User
    - `handleQuit()` stores reason and marks broadcast=true
    - `handleDisconnect()` skips broadcast if already done
- **Key patterns**:
  - `require*` helpers return bool/pointer, send error on failure
  - Two-phase QUIT: broadcast in handler, cleanup in disconnect
  - Track state on User to prevent duplicate work
- **Next step**: Day 14 — LIST + Stress Testing (optional), then Week 3 hardening

### Mandatory Commands — ALL COMPLETE ✅
- [x] Registration: `PASS`, `NICK`, `USER`
- [x] Channel: `JOIN`, `PART`, `TOPIC`, `KICK`, `INVITE`
- [x] Messaging: `PRIVMSG`, `NOTICE`
- [x] Modes: `MODE` (+i, +t, +k, +l, +o)
- [x] Connection: `QUIT`

### Critical Evaluator Traps — AVOIDED ✅
1. ❌ read/write outside poll()/select() readiness = grade 0
2. ❌ Blocking sockets
3. ❌ Multiple poll()/select() calls
4. ❌ Assuming one recv() == one command
5. ❌ Direct send() in command handlers

### Bug Fix: time_t for Topic Timestamp (Y2038 Safety)
- **Issue**: `_topic_set_at` was `int`, but `std::time(NULL)` returns `time_t`
- **Risk**: Implicit narrowing loses data; Y2038 overflow on 32-bit systems
- **Fix**: Changed `int` → `time_t` in Channel class
  - Added `#include <ctime>` to channel.hpp
  - Updated member, getter, and setter types

### Security Fix: sendNumeric() Protocol Injection Prevention
- **Issue**: `sendNumeric()` didn't sanitize params/trailing before writing to output buffer
- **Risk**: Embedded CR/LF in user-influenced tokens (nick, channel, error messages) allows response-splitting
- **Fix**: Applied `sanitizeIrcText()` to nick, all params, and trailing in `sendNumeric()`
- **Result**: All numeric replies now safe regardless of input source

### Fix: ERR_UNKNOWNCOMMAND Uses sendNumeric()
- **Issue**: Unknown command path built raw `"421 * CMD :Unknown command"` string
- **Problems**: Missing server prefix, `*` instead of user's nick, bypassed sanitization
- **Fix**: Changed to `sendNumeric(user, ERR_UNKNOWNCOMMAND, {msg.command}, "Unknown command")`
- **Result**: Proper format `:server 421 nick CMD :Unknown command` + sanitization

### Session 11 — January 30, 2026
- **Completed**: Additional Bug Fixes & Code Quality Improvements ✅
  - **Iterator safety fix**
    - Issue: User deletion before map erasure could invalidate iterators
    - Fix: Erase from map first, then delete user pointer
    - Applied to all disconnect paths (marked disconnection, recv=0, errors, send errors)
    - Pattern: `users.erase(it); handleDisconnect(user); delete user; close(fd);`
  - **Idempotent MODE operations**
    - Issue: MODE changes broadcasted even when state didn't change
    - Examples: `MODE +i` when already invite-only, `MODE +o bob` when bob already operator
    - Fix: Check current state before applying mode, return false if no change needed
    - Applied to: +i/-i, +t/-t, +k/-k, +l/-l modes
    - Result: Idempotent operations silently ignored (no broadcast), matches IRC standard
  - **TOPIC empty trailing param fix**
    - Issue: `TOPIC #chan :` (empty) and `TOPIC #chan : ` (whitespace) didn't clear topic
    - Root cause: Parser correctly preserved empty trailing, but whitespace-only topics weren't trimmed
    - Fix: Added `trim()` after `sanitizeIrcText()` in handleTopic()
    - Result: Both empty and whitespace-only trailing params now correctly clear the topic
- **Key patterns**:
  - Always erase from containers before deleting objects (iterator safety)
  - Check state before mutating (idempotent operations)
  - Trim user input when whitespace-only should be treated as empty
- **Files modified**:
  - `src/server.cpp`: Iterator safety in disconnect paths
  - `src/serverCommandsMode.cpp`: Idempotent checks for all mode operations
  - `src/serverCommands.cpp`: TOPIC whitespace trimming

### Session 12 — February 1, 2026
- **Completed**: Codebase Refactoring & Documentation Update ✅
  - **Channel class file split**:
    - Split `channel.cpp` into 5 focused files:
      - `channel.cpp` — Core (constructor, destructor, getName)
      - `channelMembers.cpp` — Member management (add, remove, canJoin, broadcast, getNamesList)
      - `channelModes.cpp` — Mode getters/setters (+i, +t, +k, +l)
      - `channelOperators.cpp` — Operator & invite management
      - `channelTopic.cpp` — Topic management
    - Improved code organization and maintainability
  - **Automated tester improvements**:
    - Added progress bar to `ft_irc_tester.py`
    - 34 comprehensive tests covering all mandatory features
  - **Documentation updates**:
    - Updated PROGRESS.md file structure to reflect new channel files
    - Updated README.md with correct paths (Tests/ instead of memory_bank/)
    - Added automated tester documentation references
  - **Cleanup**:
    - Removed outdated TODO comment in channelMembers.cpp (feature was already implemented)
- **Key patterns**:
  - Split large files by logical grouping (members, modes, operators, topic)
  - Keep documentation in sync with code structure
- **Files modified**:
  - `src/channel.cpp` — Reduced to core functionality
  - `src/channelMembers.cpp` — New file for member management
  - `src/channelModes.cpp` — New file for mode operations
  - `src/channelOperators.cpp` — New file for operator/invite management
  - `src/channelTopic.cpp` — New file for topic management
  - `memory_bank/PROGRESS.md` — Updated file structure & session log
  - `README.md` — Updated paths and testing info
  - `Makefile` — Updated to include new source files

### Session 13 — Bot Bonus Implementation ✅
- **Completed**: IRC Bot (Bonus Feature) ✅
  - **Bot Implementation**:
    - Created separate `ircbot` binary (connects as client to ircserv)
    - C++98 compliant with poll-based non-blocking I/O
    - Proper IRC registration flow (PASS, NICK, USER)
    - Joins configured channel after registration
    - Graceful shutdown handling
  - **Bot Features**:
    - `!help` — Lists all available bot commands
    - `!time` — Shows current server time (uses `ctime()`)
    - `!weather <city>` — Mock weather data for requested city
    - **Greeting** — Welcomes users when they JOIN the channel
    - **Channel Logging** — Logs all channel messages to `bot.log` with timestamps
  - **Bot Files Created**:
    - `src/bot.cpp` — Main bot logic (connect, run loop, message parsing)
    - `src/botHandlers.cpp` — Command handlers (handleHelp, handleTime, handleWeather, handleUserJoin)
    - `include/bot.hpp` — Bot class header
  - **Makefile Updates**:
    - Added `make bonus` target for building ircbot
    - Added `bot.log` removal to `make fclean`
    - Fixed BOT_SRCS path (added missing `$(SRCS_DIR)/` prefix)
  - **Testing Infrastructure**:
    - Added 5 automated bot tests to `ft_irc_tester.py` (B.1–B.5)
    - Added Section 16 "Bot Tests (Bonus)" to `Tests/TESTS.md` (9 manual tests)
    - Updated `Tests/ft_irc_tester_instructions.md` with bot testing section
    - Total automated tests now 37 (up from 32)
  - **Bug Fixes**:
    - Fixed log timestamp newline issue (stripped `\n` from `ctime()` output)
    - Fixed test 14.2 partial commands test (hardcoded password → `self.password`)
- **Key patterns**:
  - Bot is separate program, connects as regular IRC client
  - Uses same poll-based I/O pattern as server for non-blocking operations
  - Command handlers in separate file for maintainability
  - Mock weather data (no external API required)
- **Files created/modified**:
  - `src/bot.cpp` — New file (bot main logic)
  - `src/botHandlers.cpp` — New file (command handlers)
  - `include/bot.hpp` — New file (bot class header)
  - `Makefile` — Added bonus target, fclean cleanup
  - `ft_irc_tester.py` — Added 5 bot tests, fixed test 14.2
  - `Tests/TESTS.md` — Added Section 16 (bot tests)
  - `Tests/ft_irc_tester_instructions.md` — Added bot testing section
  - `memory_bank/PROGRESS.md` — This update
