# ft_irc — Evaluator Traps & Common Mistakes (C++98)

This document lists **real evaluation traps**, **common mistakes**, and **defense-time failure points** for the *ft_irc* project.

Use this as a **pre-defense checklist**.

---

## 1. select()/poll() & Non-Blocking I/O (MOST COMMON FAILURES)

### ❌ Reading or writing outside readiness check

**Trap:**  
Calling `recv()` or `send()` without the fd being marked ready by `select()`/`poll()`.

**Why it fails:**  
The subject explicitly states that **any read/write outside the readiness check will result in grade 0**, even if it "works".

**Checklist:**
- ✅ `accept()` only when listen_fd is in readFds after `select()`
- ✅ `recv()` only when user fd is in readFds after `select()`
- ✅ `send()` only when user fd is in writeFds after `select()`
- ❌ No "just try recv()" logic anywhere

**Correct pattern:**
```cpp
// In main loop
if (FD_ISSET(userFd, &readFds)) {
    // NOW we can recv()
    bytesRead = recv(userFd, buffer, sizeof(buffer), 0);
}

if (FD_ISSET(userFd, &writeFds)) {
    // NOW we can send()
    bytesSent = send(userFd, outbuf.c_str(), outbuf.length(), 0);
}
```

---

### ❌ Blocking sockets

**Trap:**  
Forgetting to set non-blocking on:
- Listening socket
- **Accepted client sockets** (commonly forgotten!)

**Why it fails:**  
Blocking read/write = server hangs = instant fail.

**Checklist:**
- ✅ Listening socket is non-blocking
- ✅ Every accepted socket is set non-blocking immediately after `accept()`
- ✅ Cross-platform handling:

```cpp
// Linux: can use SOCK_NONBLOCK flag
socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

// macOS: must use fcntl after socket creation
fcntl(fd, F_SETFL, O_NONBLOCK);
```

**Common mistake on macOS:**
```cpp
// WRONG - don't preserve existing flags, just set O_NONBLOCK
int flags = fcntl(fd, F_GETFL);
fcntl(fd, F_SETFL, flags | O_NONBLOCK);

// CORRECT for macOS - simpler
fcntl(fd, F_SETFL, O_NONBLOCK);
```

---

### ❌ Multiple select()/poll() calls or per-user polling

**Trap:**  
Creating one poll per user or calling select() in helper functions.

**Why it fails:**  
Subject explicitly allows **only one select/poll (or equivalent)**.

**Checklist:**
- ✅ Exactly one central `select()` or `poll()` in the main loop
- ❌ No nested or hidden polling in helper functions
- ❌ No polling inside command handlers

---

## 2. Partial Reads & Packet Fragmentation

### ❌ Assuming one `recv()` == one command

**Trap:**  
Parsing input directly from `recv()` buffer without accumulation.

**Evaluator test:**
```sh
nc -C 127.0.0.1 6667
NIC
K test
# Should work as "NICK test"
```

Or sending multiple commands in one packet:
```sh
printf "NICK test\r\nUSER test 0 * :Test\r\n" | nc 127.0.0.1 6667
```

**Why it fails:**  
IRC commands can arrive fragmented or combined.

**Checklist:**
- ✅ Each User has persistent `_inputBuffer`
- ✅ `recv()` appends to buffer, doesn't replace
- ✅ Commands extracted **only after finding `\n`**
- ✅ Leftover partial data stays in buffer for next recv

**Correct pattern:**
```cpp
// Append received data
user->getInputBuffer().append(buffer, bytesRead);

// Extract complete messages
while ((pos = inputBuffer.find('\n')) != std::string::npos) {
    std::string msg = inputBuffer.substr(0, pos);
    inputBuffer.erase(0, pos + 1);
    
    // Handle optional \r (accept both \r\n and \n)
    if (!msg.empty() && msg[msg.size()-1] == '\r')
        msg.erase(msg.size() - 1);
    
    // Process complete message
    processMessage(msg);
}
// Partial data remains in buffer for next recv()
```

---

## 3. Output Buffering & Write Handling

### ❌ Calling `send()` directly in command handlers

**Trap:**  
Sending responses immediately inside PASS, JOIN, etc.

**Why it fails:**  
Socket may not be writable → EAGAIN → message lost.

**Checklist:**
- ✅ All output appended to User's `_outputBuffer`
- ✅ `send()` happens ONLY in main loop when fd is write-ready
- ✅ Command handlers never call `send()` directly

**Correct pattern:**
```cpp
// In command handler - NEVER send() directly
void User::sendServerMsg(const std::string& message) {
    _outputBuffer += ":" + _server->getServerName() + " " + message + "\r\n";
}

// In main loop - only place where send() is called
if (FD_ISSET(userFd, &writeFds) && !user->getOutputBuffer().empty()) {
    ssize_t sent = send(userFd, outbuf.c_str(), outbuf.length(), 0);
    if (sent > 0)
        outbuf.erase(0, sent);
}
```

---

### ❌ Forgetting to manage write set properly

**Trap:**  
Always adding fds to write set, or never removing them.

**Why it fails:**  
- Always in write set → busy loop → 100% CPU
- Never in write set → messages never sent

**Checklist:**
- ✅ Add fd to write set **only if** output buffer is non-empty
- ✅ When buffer becomes empty, fd is naturally not added next iteration

**Correct pattern:**
```cpp
int Server::prepareWriteSet(fd_set& writeFds) {
    FD_ZERO(&writeFds);
    int maxFd = -1;
    
    for (each user) {
        // Only add if there's something to send
        if (!user->getOutputBuffer().empty()) {
            FD_SET(userFd, &writeFds);
            if (userFd > maxFd) maxFd = userFd;
        }
    }
    return maxFd;
}
```

---

## 4. Registration & Authentication Traps

### ❌ Allowing commands before registration

**Trap:**  
Letting users JOIN, PRIVMSG, MODE before PASS/NICK/USER complete.

**Checklist:**
- ✅ Only PASS, NICK, USER, QUIT allowed before registration
- ✅ Everything else returns `451 ERR_NOTREGISTERED`

```cpp
static bool checkRegistered(User* user, const std::string& command) {
    if (!user->isRegistered()) {
        user->sendError(451, command, "You have not registered");
        return false;
    }
    return true;
}
```

---

### ❌ Assuming registration order

**Trap:**  
Assuming PASS → NICK → USER order.

**Reality:**  
Clients may send in any order:
- NICK first
- USER first  
- PASS last

**Checklist:**
- ✅ Track separate flags: `_hasPass`, `_hasNick`, `_hasUser`
- ✅ Call `tryRegister()` after each command
- ✅ Register only when ALL required flags are set

```cpp
void User::tryRegister() {
    if (_isRegistered) return;
    if (!_hasPassed || !_hasNick || !_hasUser) return;
    
    _isRegistered = true;
    sendWelcome();
}
```

---

### ❌ Missing welcome numeric

**Trap:**  
Never sending `001` (RPL_WELCOME) after registration.

**Why it fails:**  
Many IRC clients consider connection unusable without welcome.

**Checklist:**
- ✅ Send at least `001` once registered
- ✅ Format: `:<server> 001 <nick> :Welcome to <network> <nick>!<user>@<host>`

---

## 5. Nickname Handling

### ❌ Duplicate nicknames allowed

**Trap:**  
Allowing two users with the same nick (case-insensitive!).

**Checklist:**
- ✅ Maintain nick → User* map with **normalized** (lowercase) keys
- ✅ Check uniqueness before setting nick
- ✅ Update map on nick change
- ✅ Remove from map on disconnect
- ✅ Return `433 ERR_NICKNAMEINUSE` on collision

```cpp
std::string newNickLower = normalize(newNick);  // lowercase
User* existing = server->getUser(newNickLower);
if (existing && existing != user) {
    user->sendError(433, newNick, "Nickname is already in use");
    return;
}
```

---

### ❌ Nick change not propagated

**Trap:**  
Changing nick without notifying other users in shared channels.

**Why it fails:**  
Other users keep seeing old nick → inconsistent state.

**Checklist:**
- ✅ Broadcast NICK change to all users in shared channels
- ✅ Update channel membership maps with new nick

---

## 6. Channel & Membership Errors

### ❌ Not removing users on disconnect

**Trap:**  
User disconnects but remains in channel member maps.

**Evaluator test:**
1. User A and B in #test
2. User B disconnects (close terminal)
3. User A sends message to #test
4. Server crashes or sends to ghost

**Checklist:**
- ✅ Central `disconnectUser()` function handles everything
- ✅ On disconnect:
  - Broadcast QUIT to all shared channels
  - Remove from all channel member maps
  - Remove from channel operator maps
  - Delete empty channels
  - Remove from `_usersFd` and `_usersNick` maps
  - Close fd
  - Delete User object

---

### ❌ Allowing non-members to talk in channel

**Trap:**  
Allowing `PRIVMSG #channel` from users who haven't joined.

**Checklist:**
- ✅ Check membership before allowing channel message
- ✅ Return `442 ERR_NOTONCHANNEL`

```cpp
if (!channel->is_user_member(user)) {
    user->sendError(442, channelName, "You're not on that channel");
    return;
}
```

---

### ❌ Not making channel creator an operator

**Trap:**  
First user joins channel but doesn't get operator status.

**Checklist:**
- ✅ First joiner automatically gets `+o`
- ✅ Track whether channel was just created

```cpp
bool wasCreated = false;
Channel* channel = server->getOrCreateChannel(channelName, user, &wasCreated);
// ... add user to channel ...
if (wasCreated)
    channel->make_user_operator(user);
```

---

## 7. MODE Command (HIGH FAILURE RATE)

### ❌ MODE parsing too naive

**Trap:**  
Assuming one mode at a time or not handling combined modes.

**Evaluator tests:**
```
MODE #chan +itkl secret 10
MODE #chan +o-o nick1 nick2
MODE #chan -k
MODE #chan +it
```

**Checklist:**
- ✅ Track current sign (`+` or `-`)
- ✅ Handle multiple modes in one command
- ✅ Consume arguments in correct order:
  - `+k` needs key argument
  - `-k` no argument
  - `+l` needs limit argument
  - `-l` no argument
  - `+o` and `-o` both need nick argument
- ✅ Return `461` for missing required arguments

---

### ❌ Not enforcing mode behavior

**Trap:**  
Setting mode flags but not actually enforcing them.

**Examples:**
- `+i` set but JOIN still allowed without invite
- `+k secret` set but any key accepted
- `+l 2` set but unlimited users can join
- `+t` set but anyone can change topic

**Checklist:**
- ✅ Every mode must affect behavior
- ✅ `can_user_join()` checks ALL modes
- ✅ Evaluator **WILL** test each mode

```cpp
bool Channel::can_user_join(User* user, const std::string& key,
                            JoinResult& result) const
{
    // Check invite-only
    if (_invite_only && !is_invited(user->getNicknameLower())) {
        result = JOIN_INVITE_ONLY;
        return false;
    }
    // Check key
    if (!_channel_key.empty() && key != _channel_key) {
        result = JOIN_BAD_KEY;
        return false;
    }
    // Check limit
    if (_user_limit > 0 && get_connected_user_number() >= _user_limit) {
        result = JOIN_FULL;
        return false;
    }
    return true;
}
```

---

## 8. Operator Privileges

### ❌ Not checking operator status

**Trap:**  
Letting any channel member use:
- KICK
- INVITE
- MODE (modifications)
- TOPIC (when +t is set)

**Checklist:**
- ✅ Check `is_user_operator()` before allowing these commands
- ✅ Return `482 ERR_CHANOPRIVSNEEDED`

```cpp
if (!channel->is_user_operator(user)) {
    user->sendError(482, channel->get_name(), "You're not channel operator");
    return false;
}
```

---

### ❌ MODE +o/-o mistakes

**Trap:**
- Granting op to non-member
- Not validating target exists
- Not checking if target is in channel

**Checklist:**
- ✅ Target user must exist → `401`
- ✅ Target must be channel member → `441`
- ✅ Then grant/revoke op status

---

## 9. TOPIC & INVITE Edge Cases

### ❌ TOPIC ignoring +t mode

**Trap:**  
Allowing anyone to set topic when `+t` is active.

**Checklist:**
- ✅ If `+t` mode: only operators can set topic
- ✅ Non-op gets `482`

```cpp
if (channel->has_topic_protection() && !channel->is_user_operator(user)) {
    user->sendError(482, channel->get_name(), "You're not channel operator");
    return false;
}
```

---

### ❌ INVITE not stored

**Trap:**  
Sending invite notification but not recording it.

**Why it fails:**  
User tries to JOIN +i channel, still gets rejected.

**Checklist:**
- ✅ Store invited nick in channel's invitation set
- ✅ Check invitation list in `can_user_join()`
- ✅ Optionally: remove from list after successful JOIN

```cpp
// In INVITE handler
channel->add_invite(target->getNicknameLower());

// In can_user_join()
if (_invite_only && !is_invited(user->getNicknameLower())) {
    result = JOIN_INVITE_ONLY;
    return false;
}
```

---

## 10. IRC Formatting Issues

### ❌ Missing CRLF

**Trap:**  
Using `\n` instead of `\r\n`.

**Why it fails:**  
Many IRC clients silently ignore messages without proper line ending.

**Checklist:**
- ✅ Every server message ends with `\r\n`
- ✅ Check all `sendServerMsg()`, `sendError()`, etc.

```cpp
void User::sendServerMsg(const std::string& message) {
    _outputBuffer += ":" + _server->getServerName() + " " + message + "\r\n";
}
```

---

### ❌ Incorrect prefix format

**Trap:**  
Missing or malformed message prefix.

**Checklist:**
- ✅ User messages: `:<nick>!<user>@<host> COMMAND ...`
- ✅ Server messages: `:<servername> <numeric> <target> :<message>`

```cpp
std::string User::buildHostmask() const {
    return _nickname + "!" + _username + "@" + _host;
}

// User message
":" + user->buildHostmask() + " PRIVMSG #channel :Hello\r\n"

// Server message  
":" + serverName + " 001 " + nick + " :Welcome\r\n"
```

---

## 11. Cleanup & Stability

### ❌ Crashing on disconnect

**Trap:**  
Use-after-free, invalid iterator, double erase when user disconnects.

**Common causes:**
- Iterating map while erasing
- Accessing user after deletion
- Not checking for NULL pointers

**Checklist:**
- ✅ Central `disconnectUser(fd)` function
- ✅ Increment iterator before processing (or use post-increment)
- ✅ Check pointers before use

```cpp
// Safe iteration while potentially removing
std::map<int, User*>::iterator it = _usersFd.begin();
while (it != _usersFd.end()) {
    int fd = it->first;
    ++it;  // Increment BEFORE potential removal
    
    if (shouldDisconnect(fd))
        disconnectUser(fd);
}
```

---

### ❌ Memory leaks

**Trap:**  
Forgetting to delete dynamically allocated objects.

**Checklist:**
- ✅ Delete User objects when disconnecting
- ✅ Delete Channel objects when empty
- ✅ Test with Valgrind on Linux:
  ```sh
  valgrind --leak-check=full ./ircserv 6667 password
  ```

---

## 12. Cross-Platform Issues

### ❌ Linux-only code on macOS (or vice versa)

**Trap:**  
Using `SOCK_NONBLOCK` on macOS (doesn't exist).

**Checklist:**
- ✅ Use preprocessor for platform-specific code
- ✅ Test on both Linux and macOS if possible

```cpp
#if defined(LINUX_OS)
    _fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
#elif defined(MACOS_OS)
    _fd = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(_fd, F_SETFL, O_NONBLOCK);
#endif
```

In Makefile:
```makefile
OS := $(shell uname -s)
ifeq ($(OS),Darwin)
    CPPFLAGS += -DMACOS_OS
else ifeq ($(OS),Linux)
    CPPFLAGS += -DLINUX_OS
endif
```

---

## 13. Evaluation & Defense Traps

### ❌ "It works on my machine"

**Trap:**  
Cannot explain design decisions during defense.

**Evaluator questions to prepare for:**
- Why single select/poll?
- How do you handle partial packets?
- Why buffer output instead of sending directly?
- What happens when a user disconnects?
- How does MODE parsing work?
- How do you handle the +i, +k, +l modes?

**Checklist:**
- ✅ Be able to explain WITHOUT looking at code
- ✅ Draw the event loop flow
- ✅ Explain buffer logic with examples
- ✅ Trace through a disconnect scenario

---

### ❌ Overengineering

**Trap:**  
Complex abstractions that are hard to explain.

**Why it fails:**  
Hard to explain = hard to debug = hard to defend.

**Checklist:**
- ✅ Simple containers (`std::map`, `std::set`, `std::vector`)
- ✅ Explicit logic (no template metaprogramming)
- ✅ Clear ownership (who owns what memory)
- ✅ Static Command class is simpler than Dispatcher pattern

---

## Final Pre-Defense Checklist

### I/O & Networking
- [ ] Single `select()` or `poll()` only
- [ ] All sockets non-blocking (listen + all accepted)
- [ ] No read/write outside readiness check
- [ ] Works on both Linux and macOS

### Buffers
- [ ] Input buffer accumulates until `\n`
- [ ] Handles partial packets
- [ ] Handles multiple commands per packet
- [ ] Output buffer used for all sending
- [ ] `send()` only in main loop

### Registration
- [ ] PASS/NICK/USER work in any order
- [ ] Commands blocked until registered (except PASS/NICK/USER/QUIT)
- [ ] Welcome message (001) sent on registration
- [ ] Duplicate nick rejected (433)

### Channels
- [ ] JOIN creates channel, first joiner is op
- [ ] PART removes user, empty channel deleted
- [ ] PRIVMSG/NOTICE work to channels and nicks
- [ ] Non-member cannot message channel (442)

### Modes
- [ ] +i blocks non-invited users (473)
- [ ] +k requires correct key (475)
- [ ] +l enforces user limit (471)
- [ ] +t restricts topic to ops (482)
- [ ] +o/-o grants/revokes operator

### Operator Commands
- [ ] KICK requires op (482)
- [ ] INVITE requires op (482)
- [ ] MODE changes require op (482)
- [ ] TOPIC respects +t mode

### Cleanup
- [ ] Disconnect removes from all channels
- [ ] Disconnect broadcasts QUIT
- [ ] Empty channels deleted
- [ ] No memory leaks
- [ ] No crashes on disconnect

### Formatting
- [ ] All messages end with `\r\n`
- [ ] Correct prefix format
- [ ] Correct numeric format

### Defense
- [ ] Can explain event loop
- [ ] Can explain buffer logic
- [ ] Can explain MODE parsing
- [ ] Can explain disconnect cleanup

---

**If you pass all items above, your ft_irc is very hard to fail.**
