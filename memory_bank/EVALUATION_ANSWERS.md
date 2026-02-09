# ft_irc Evaluation Answers

This document provides detailed answers and testing procedures for evaluators.

---

## Mandatory Part

### Basic Checks

#### 1. Makefile and Compilation

**Question:** Is there a Makefile? Does the project compile correctly in C++? Is the executable called "ircserv"?

**Answer:** Yes.

**How to verify:**
```bash
# Clean and rebuild
make fclean
make

# Verify executable exists and is named correctly
ls -la ircserv

# Verify C++ compilation (check Makefile)
grep -E "c\+\+|g\+\+" Makefile
```

**Expected output:**
- `make` completes without errors
- Executable `ircserv` exists in project root
- Makefile uses `c++` or `g++` compiler

---

#### 2. Single poll() Check

**Question:** How many `poll()` (or equivalent) are present in the code? There must be only one.

**Answer:** There is exactly **ONE** `poll()` call in the entire codebase.

**How to verify:**
```bash
# Count poll() calls in source files
grep -rn "poll(" src/ include/ | grep -v "//" | wc -l

# Show the actual poll() call location
grep -rn "poll(" src/ include/ | grep -v "//"
```

**Expected output:**
```
1
src/server.cpp:XXX:    int ready = poll(...);
```

**Location:** The single `poll()` is in the main event loop in `src/server.cpp`, inside the `run()` method.

**Explanation:** The server uses a single event loop pattern where one `poll()` monitors ALL file descriptors (listening socket + all client sockets) simultaneously. This is the correct design for a non-blocking server.

---

#### 3. poll() Before I/O Operations

**Question:** Is `poll()` called before each `accept`, `read/recv`, `write/send`? Is `errno` used to trigger specific actions?

**Answer:** Yes, `poll()` is always called first. No, `errno` is not used to trigger re-reads.

**How to verify:**
```bash
# Check that accept/recv/send are not in loops checking errno
grep -rn "EAGAIN\|EWOULDBLOCK" src/ include/

# Verify the pattern: poll() → check POLLIN/POLLOUT → then I/O
grep -B5 -A2 "accept\|recv\|send" src/server.cpp | head -50
```

**Explanation of the correct pattern:**

```cpp
// 1. poll() waits for events
int ready = poll(fds, nfds, timeout);

// 2. Check which fd is ready
for (each fd) {
    if (fds[i].revents & POLLIN) {
        // Only NOW do we read - poll() told us data is available
        recv(fd, buffer, size, 0);
    }
    if (fds[i].revents & POLLOUT) {
        // Only NOW do we write - poll() told us socket is writable
        send(fd, buffer, size, 0);
    }
}
```

**What would be WRONG:**
```cpp
// WRONG: Looping on recv until EAGAIN
while (true) {
    int n = recv(fd, buf, size, 0);
    if (n < 0 && errno == EAGAIN) break;  // BAD!
}
```

Our implementation checks `poll()` return values (POLLIN/POLLOUT) to know when I/O is safe, never looping on errno.

---

#### 4. fcntl() Usage

**Question:** Is each call to `fcntl()` done as `fcntl(fd, F_SETFL, O_NONBLOCK)`?

**Answer:** Yes, `fcntl()` is only used to set non-blocking mode.

**How to verify:**
```bash
# Find all fcntl() calls
grep -rn "fcntl" src/ include/

# Verify the exact usage pattern
grep -rn "fcntl" src/ include/ | grep -v "//"
```

**Expected output:**
```
src/server.cpp:XXX:    fcntl(fd, F_SETFL, O_NONBLOCK);
```

**Explanation:** The only allowed use of `fcntl()` is to set sockets to non-blocking mode. We do this for:
1. The listening socket (after creation)
2. Each new client socket (after `accept()`)

No other `fcntl()` operations are used (no `F_GETFL`, no `F_SETFD`, etc.).

---

### Networking

#### 1. Server Starts and Listens

**Question:** Does the server start and listen on all network interfaces on the specified port?

**Answer:** Yes.

**How to verify:**
```bash
# Start server
./ircserv 6667 testpass &

# Check listening socket (should show 0.0.0.0:6667 or :::6667)
ss -tlnp | grep 6667
# OR
netstat -tlnp | grep 6667
```

**Expected output:**
```
LISTEN  0  128  0.0.0.0:6667  0.0.0.0:*  users:(("ircserv",pid=XXXX,fd=3))
```

The `0.0.0.0` means it listens on ALL network interfaces.

---

#### 2. nc Connection Test

**Question:** Can you connect with `nc`, send commands, and get responses?

**Answer:** Yes.

**How to test:**
```bash
# Terminal 1: Start server
./ircserv 6667 testpass

# Terminal 2: Connect with nc
nc localhost 6667
PASS testpass
NICK alice
USER alice 0 * :Alice Smith
JOIN #test
PRIVMSG #test :Hello from nc!
QUIT
```

**Expected responses:**
```
:server 001 alice :Welcome to the IRC server alice!alice@localhost
:server 002 alice :Your host is server, running version 1.0
:server 003 alice :This server was created <date>
:server 004 alice server 1.0 o itkol
:alice!alice@localhost JOIN #test
:server 353 alice = #test :@alice
:server 366 alice #test :End of /NAMES list
```

---

#### 3. Reference IRC Client

**Question:** What is the reference IRC client?

**Answer:** **HexChat** (or irssi/WeeChat).

**How to connect with HexChat:**
1. Open HexChat → Network List
2. Add new network "ft_irc"
3. Edit → Add server: `localhost/6667`
4. Set server password: `testpass`
5. Connect

**How to connect with irssi:**
```bash
irssi -c localhost -p 6667 -w testpass -n myuser
```

**How to connect with WeeChat:**
```
/server add ft_irc localhost/6667 -password=testpass
/connect ft_irc
```

---

#### 4. Multiple Simultaneous Connections

**Question:** Can the server handle multiple connections without blocking?

**Answer:** Yes.

**How to test:**
```bash
# Terminal 1: Server
./ircserv 6667 testpass

# Terminal 2: IRC client (HexChat)
# Connect as user1, join #test

# Terminal 3: nc connection
nc localhost 6667
PASS testpass
NICK ncuser
USER ncuser 0 * :NC User
JOIN #test
PRIVMSG #test :Message from nc

# Terminal 4: Another nc
nc localhost 6667
PASS testpass
NICK ncuser2
USER ncuser2 0 * :NC User 2
JOIN #test
```

**Expected behavior:**
- All clients connect simultaneously
- Messages from any client appear on all others
- No client blocks or waits for another
- Server remains responsive to new connections

---

#### 5. Channel Message Distribution

**Question:** Are messages sent to all channel members?

**Answer:** Yes.

**How to test:**
```bash
# Connect 3 clients (mix of nc and IRC client), all join #test
# From client 1:
PRIVMSG #test :Hello everyone!

# Verify clients 2 and 3 receive:
:client1!user@host PRIVMSG #test :Hello everyone!
```

---

### Networking Specials

#### 1. Partial Commands with nc

**Question:** Does the server handle partial commands correctly?

**Answer:** Yes. The server buffers incomplete commands until `\r\n` is received.

**How to test:**
```bash
# Terminal 1: Server running

# Terminal 2: Send partial command
{
  printf "PASS test"      # No \r\n yet!
  sleep 1
  printf "pass\r\n"       # Complete it
  echo "NICK alice"
  echo "USER alice 0 * :Alice"
  sleep 1
  echo "QUIT"
} | nc localhost 6667

# Terminal 3: While partial is pending, verify other connections work
nc localhost 6667
PASS testpass
NICK bob
USER bob 0 * :Bob
QUIT
```

**Expected behavior:**
- Server waits for complete command (with `\r\n`)
- Other connections work normally during the wait
- Once command is complete, it processes correctly

**Alternative test (byte by byte):**
```bash
for char in P A S S ' ' t e s t p a s s $'\r' $'\n'; do
  printf "%s" "$char"
  sleep 0.1
done | nc localhost 6667
```

---

#### 2. Unexpectedly Kill Client

**Question:** Does the server remain operational when a client is killed?

**Answer:** Yes.

**How to test:**
```bash
# Terminal 1: Server
./ircserv 6667 testpass

# Terminal 2: Client 1 (alice) - stays connected
nc localhost 6667
PASS testpass
NICK alice
USER alice 0 * :Alice
JOIN #test

# Terminal 3: Client 2 (bob)
nc localhost 6667 &
NC_PID=$!
# Register bob, join #test...

# Kill bob's connection abruptly
kill -9 $NC_PID

# Terminal 2: Verify alice is still connected
PRIVMSG #test :Am I still here?
# Should work fine

# Terminal 4: New client can connect
nc localhost 6667
PASS testpass
NICK carol
USER carol 0 * :Carol
```

**Expected behavior:**
- Server detects disconnection (POLLHUP or read returns 0)
- Sends QUIT message to other users in shared channels
- Cleans up resources
- Continues serving other clients
- Accepts new connections

---

#### 3. Kill nc with Half Command

**Question:** Does the server handle an abrupt disconnect mid-command?

**Answer:** Yes.

**How to test:**
```bash
# Terminal 1: Server running

# Terminal 2: Send half command then kill
{
  echo "PASS testpass"
  echo "NICK halfus"
  printf "USER half"      # Incomplete command!
} | nc localhost 6667 &
NC_PID=$!
sleep 0.5
kill -9 $NC_PID

# Terminal 3: Verify server still works
nc localhost 6667
PASS testpass
NICK newuser
USER newuser 0 * :New User
JOIN #test
QUIT
```

**Expected behavior:**
- Server discards incomplete buffer when connection closes
- No crash, no hang, no memory leak
- Server continues operating normally

---

#### 4. Stopped Client Flood Test (^Z / SIGSTOP)

**Question:** Does the server handle a stopped client while being flooded?

**Answer:** Yes. The server uses `poll()` with POLLOUT to avoid blocking on full send buffers.

**How to test:**
```bash
# Terminal 1: Server
./ircserv 6667 testpass

# Terminal 2: Victim client (will be stopped)
nc localhost 6667 > /tmp/victim_output.txt &
VICTIM_PID=$!
echo -e "PASS testpass\r\nNICK victim\r\nUSER v 0 * :V\r\nJOIN #flood\r\n" | nc localhost 6667 -q1

# Stop the victim (simulates Ctrl+Z)
kill -STOP $VICTIM_PID

# Terminal 3: Flooder
{
  echo "PASS testpass"
  echo "NICK flooder"
  echo "USER f 0 * :F"
  echo "JOIN #flood"
  for i in $(seq 1 200); do
    echo "PRIVMSG #flood :Flood message $i"
  done
  echo "QUIT"
} | nc localhost 6667

# Terminal 4: Third client should still work
nc localhost 6667
PASS testpass
NICK observer
USER o 0 * :O
JOIN #flood
PRIVMSG #flood :Can you hear me?
QUIT

# Resume victim
kill -CONT $VICTIM_PID

# Check for memory leaks
valgrind --leak-check=full ./ircserv 6667 testpass
# (repeat flood test while running under valgrind)
```

**Expected behavior:**
- Server doesn't block when victim's buffer fills
- Other clients remain responsive
- Server may drop messages to blocked client (implementation choice)
- No memory leaks
- Valgrind reports 0 definitely/indirectly lost bytes

---

### Client Commands

#### 1. Authentication, Nickname, Username, Join

**Question:** Can you authenticate, set nick/user, and join channels with both nc and IRC client?

**Answer:** Yes (already verified above).

**Quick verification:**
```bash
# nc
nc localhost 6667
PASS testpass
NICK alice
USER alice 0 * :Alice Smith
JOIN #test

# IRC client: Use HexChat/irssi connection steps above
```

---

#### 2. PRIVMSG and NOTICE

**Question:** Do private messages and notices work correctly?

**Answer:** Yes.

**How to test PRIVMSG:**
```bash
# Two clients connected as alice and bob

# Channel message (alice):
PRIVMSG #test :Hello channel!
# bob receives: :alice!alice@host PRIVMSG #test :Hello channel!

# Private message (alice to bob):
PRIVMSG bob :Hello privately!
# bob receives: :alice!alice@host PRIVMSG bob :Hello privately!

# Multiple targets (if supported):
PRIVMSG bob,#test :Hello both!
```

**How to test NOTICE:**
```bash
# Notice to channel (alice):
NOTICE #test :This is a notice
# bob receives: :alice!alice@host NOTICE #test :This is a notice

# Notice to user (alice to bob):
NOTICE bob :Private notice
# bob receives: :alice!alice@host NOTICE bob :Private notice
```

**Key difference between PRIVMSG and NOTICE:**
- PRIVMSG may trigger auto-replies (like away messages)
- NOTICE should NEVER trigger auto-replies (to prevent loops)
- NOTICE to non-existent user should NOT return error (per RFC)

**Test NOTICE no-error behavior:**
```bash
NOTICE nonexistentuser :Hello
# Should produce NO error response (unlike PRIVMSG which returns 401)
```

---

#### 3. Operator Privileges

**Question:** Do regular users lack operator privileges? Do operators have them?

**Answer:** Yes. Channel operators are marked with `@` prefix.

**How to test regular user restrictions:**
```bash
# alice creates channel (becomes operator automatically)
# bob joins same channel (regular user)

# Bob (non-operator) tries operator commands:
# From bob's connection:

KICK #test alice :bye
# Response: :server 482 bob #test :You're not channel operator

TOPIC #test :New topic
# Response: :server 482 bob #test :You're not channel operator
# (if channel has +t mode set)

MODE #test +i
# Response: :server 482 bob #test :You're not channel operator

INVITE someuser #test
# Response: :server 482 bob #test :You're not channel operator
# (if channel has +i mode set)
```

**How to test operator commands:**
```bash
# alice is operator of #test (created the channel)

# KICK - Remove user from channel
MODE #test +o bob           # Give bob operator status
KICK #test bob :Goodbye     # Kick bob (works because alice is op)
# bob receives: :alice!alice@host KICK #test bob :Goodbye

# INVITE - Invite user to invite-only channel
MODE #test +i               # Set invite-only
INVITE carol #test          # Invite carol
# carol receives: :alice!alice@host INVITE carol #test

# TOPIC - Set channel topic (with +t mode)
MODE #test +t               # Only ops can change topic
TOPIC #test :New Topic Here
# All in channel receive: :alice!alice@host TOPIC #test :New Topic Here

# MODE - All channel modes
MODE #test +k secretkey     # Set channel key
MODE #test +l 10            # Set user limit
MODE #test +o bob           # Give operator to bob
MODE #test -o bob           # Remove operator from bob
MODE #test +i               # Set invite-only
MODE #test -itk             # Remove multiple modes
```

**Operator Commands Checklist:**

| Command | Non-Op Behavior | Operator Behavior |
|---------|-----------------|-------------------|
| `KICK` | 482 error | User removed from channel |
| `INVITE` (on +i channel) | 482 error | User invited |
| `TOPIC` (on +t channel) | 482 error | Topic changed |
| `MODE +i` | 482 error | Channel set invite-only |
| `MODE +t` | 482 error | Topic restricted to ops |
| `MODE +k` | 482 error | Channel key set |
| `MODE +l` | 482 error | User limit set |
| `MODE +o` | 482 error | Operator granted |
| `MODE -o` | 482 error | Operator revoked |

---

## Summary Checklist

### Basic Checks
- [ ] `make` works, produces `ircserv`
- [ ] Only ONE `poll()` call in code
- [ ] `poll()` called before all I/O operations
- [ ] No errno-based read loops
- [ ] `fcntl()` only used as `fcntl(fd, F_SETFL, O_NONBLOCK)`

### Networking
- [ ] Server listens on 0.0.0.0:port
- [ ] nc can connect and exchange messages
- [ ] Reference IRC client connects successfully
- [ ] Multiple simultaneous connections work
- [ ] Channel messages reach all members

### Networking Specials
- [ ] Partial commands buffered correctly
- [ ] Other connections work while partial pending
- [ ] Killing client doesn't crash server
- [ ] Killing nc mid-command doesn't crash server
- [ ] Stopped client doesn't block server
- [ ] No memory leaks during flood test

### Client Commands
- [ ] PASS/NICK/USER/JOIN work
- [ ] PRIVMSG to channel works
- [ ] PRIVMSG to user works
- [ ] NOTICE to channel works
- [ ] NOTICE to user works
- [ ] NOTICE to nonexistent user = no error
- [ ] Non-operator gets 482 on op commands
- [ ] KICK works for operators
- [ ] INVITE works for operators
- [ ] TOPIC (+t) works for operators
- [ ] MODE +i works for operators
- [ ] MODE +t works for operators
- [ ] MODE +k works for operators
- [ ] MODE +l works for operators
- [ ] MODE +o/-o works for operators

---

## Quick Commands Reference

```bash
# Start server
./ircserv 6667 testpass

# Count poll() calls
grep -rn "poll(" src/ | grep -v "//" | wc -l

# Check fcntl usage
grep -rn "fcntl" src/

# Test partial command
printf "PASS test" | nc localhost 6667 &
sleep 1
kill $!

# Memory leak test
valgrind --leak-check=full ./ircserv 6667 testpass
```
