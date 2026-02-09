# ft_irc — Evaluation Cheatsheet

Quick reference for evaluators and defenders. All commands assume server running on port 6667 with password "pass".

---

## Quick Start

```bash
# Build
make

# Run server
./ircserv 6667 pass

# Connect (terminal 1)
nc localhost 6667

# Build & run bot (bonus)
make bonus
./ircbot 127.0.0.1 6667 pass "#bot"
```

---

## 1. Registration Tests

### ✅ Basic Registration
```
PASS pass
NICK alice
USER alice 0 * :Alice
```
**Expected:** Welcome messages (001-005)

### ✅ Wrong Password
```
PASS wrongpassword
NICK bob
USER bob 0 * :Bob
```
**Expected:** `464 :Password incorrect`

### ✅ Duplicate Nickname
Terminal 1:
```
PASS pass
NICK alice
USER alice 0 * :Alice
```
Terminal 2:
```
PASS pass
NICK alice
```
**Expected:** `433 alice :Nickname is already in use`

### ✅ Commands Before Registration
```
PASS pass
JOIN #test
```
**Expected:** `451 :You have not registered`

---

## 2. Channel Tests

### ✅ Create & Join Channel
```
JOIN #test
```
**Expected:** 
- JOIN message echoed
- 353 (names list with @alice - first joiner is op)
- 366 (end of names)

### ✅ Second User Joins
Terminal 2:
```
JOIN #test
```
**Expected:** Both users see JOIN message

### ✅ Channel Message
```
PRIVMSG #test :Hello everyone!
```
**Expected:** Other users receive the message

### ✅ Part Channel
```
PART #test :Goodbye!
```
**Expected:** PART message broadcast, user removed

---

## 3. Mode Tests (+i, +t, +k, +l, +o)

### ✅ MODE +i (invite-only)
Operator:
```
MODE #test +i
```
New user tries:
```
JOIN #test
```
**Expected:** `473 #test :Cannot join channel (+i)`

### ✅ MODE +k (channel key)
Operator:
```
MODE #test +k secret
```
User tries without key:
```
JOIN #test
```
**Expected:** `475 #test :Cannot join channel (+k)`

User with correct key:
```
JOIN #test secret
```
**Expected:** Success

### ✅ MODE +l (user limit)
Operator:
```
MODE #test +l 2
```
With 2 users already in channel, third tries:
```
JOIN #test
```
**Expected:** `471 #test :Cannot join channel (+l)`

### ✅ MODE +t (topic protection)
Operator:
```
MODE #test +t
```
Non-operator tries:
```
TOPIC #test :New topic
```
**Expected:** `482 #test :You're not channel operator`

### ✅ MODE +o (operator status)
Operator:
```
MODE #test +o bob
```
**Expected:** Bob becomes operator, MODE broadcast

```
MODE #test -o bob
```
**Expected:** Bob loses operator status

### ✅ Query Modes
```
MODE #test
```
**Expected:** `324 #test +<modes> [args]`

---

## 4. Operator Commands

### ✅ KICK
Operator:
```
KICK #test bob :Reason here
```
**Expected:** Bob removed, KICK broadcast

Non-operator:
```
KICK #test alice
```
**Expected:** `482 #test :You're not channel operator`

### ✅ INVITE
Operator (with +i mode):
```
INVITE bob #test
```
**Expected:** 
- `341 bob #test` to inviter
- INVITE message to bob
- Bob can now JOIN despite +i

Non-operator:
```
INVITE charlie #test
```
**Expected:** `482 #test :You're not channel operator`

### ✅ TOPIC
View topic:
```
TOPIC #test
```
**Expected:** `332 #test :Current topic` or `331 #test :No topic`

Set topic (as operator):
```
TOPIC #test :New topic here
```
**Expected:** TOPIC broadcast to channel

---

## 5. Private Messages

### ✅ User-to-User
```
PRIVMSG bob :Hello Bob!
```
**Expected:** Bob receives message

### ✅ Non-existent User
```
PRIVMSG nobody :Hello
```
**Expected:** `401 nobody :No such nick/channel`

### ✅ Non-member to Channel
```
PRIVMSG #secret :Hello
```
(without being a member)
**Expected:** `404 #secret :Cannot send to channel`

---

## 6. Critical Edge Cases (EVALUATOR FOCUS)

### ✅ Partial Commands (nc -C)
```bash
printf "NIC" && sleep 0.1 && printf "K test\r\n"
```
**Expected:** Server reassembles → `NICK test` works

### ✅ Multiple Commands in One Packet
```bash
printf "PASS pass\r\nNICK multi\r\nUSER multi 0 * :Multi\r\n" | nc localhost 6667
```
**Expected:** All three commands processed

### ✅ Client Kill/Disconnect
1. Connect user A and B to #test
2. Kill user B's terminal (Ctrl+C or close)
3. User A sends message to #test
**Expected:** No crash, message delivered (B gone)

### ✅ Signal Handling
```bash
# Start server
./ircserv 6667 pass &
PID=$!
# Send SIGINT
kill -INT $PID
```
**Expected:** Clean shutdown, no leaks

---

## 7. Memory Check (Valgrind)

```bash
valgrind --leak-check=full --show-leak-kinds=all ./ircserv 6667 pass
```

After testing and clean exit:
**Expected:**
```
definitely lost: 0 bytes
indirectly lost: 0 bytes
```

---

## 8. Bonus: Bot

### ✅ Bot Commands
Connect client to #bot channel, then:
```
PRIVMSG #bot :!help
PRIVMSG #bot :!time
PRIVMSG #bot :!weather Paris
```
**Expected:** Bot responds in channel

### ✅ Bot Greeting
New user joins #bot
**Expected:** Bot welcomes the user

### ✅ Bot Logging
Check `bot.log` file
**Expected:** Contains timestamped messages

---

## 9. Bonus: File Transfer (DCC)

### ✅ DCC Relay Test
```bash
python3 << 'EOF'
import socket, time
def connect(nick):
    s = socket.socket()
    s.connect(('127.0.0.1', 6667))
    s.settimeout(3)
    s.send(f"PASS pass\r\nNICK {nick}\r\nUSER {nick} 0 * :{nick}\r\n".encode())
    time.sleep(0.5)
    try: s.recv(4096)
    except: pass
    return s

sender = connect("sender")
receiver = connect("receiver")
time.sleep(0.3)
sender.send(b"PRIVMSG receiver :\x01DCC SEND test.txt 2130706433 12345 1024\x01\r\n")
time.sleep(0.3)
data = receiver.recv(4096)
print("PASS" if b"\x01DCC SEND" in data else "FAIL")
sender.close()
receiver.close()
EOF
```
**Expected:** `PASS` (CTCP markers preserved)

---

## 10. Defense Questions

Be ready to explain:

1. **"Why single poll()?"**  
   Subject requirement. Multiple polls = grade 0.

2. **"How do you handle partial packets?"**  
   Input buffer per user. Append recv() data, extract only complete lines (ending in \n).

3. **"Why buffer output?"**  
   Socket may not be writable. Queue messages, send only when poll() says POLLOUT.

4. **"What happens on disconnect?"**  
   Remove from all channels, broadcast QUIT, delete empty channels, close fd, free memory.

5. **"How does MODE parsing work?"**  
   Track +/- sign, iterate mode chars, consume arguments in order (+k needs key, +o needs nick, etc.)

6. **"How do you enforce +i mode?"**  
   canJoin() checks invite-only flag and invitation list. INVITE stores nick in channel's invite set.

---

## Quick Error Code Reference

| Code | Name | Meaning |
|------|------|---------|
| 401 | ERR_NOSUCHNICK | No such nick/channel |
| 403 | ERR_NOSUCHCHANNEL | No such channel |
| 404 | ERR_CANNOTSENDTOCHAN | Cannot send to channel |
| 433 | ERR_NICKNAMEINUSE | Nickname already in use |
| 441 | ERR_USERNOTINCHANNEL | User not on channel |
| 442 | ERR_NOTONCHANNEL | You're not on that channel |
| 443 | ERR_USERONCHANNEL | User already on channel |
| 451 | ERR_NOTREGISTERED | You have not registered |
| 461 | ERR_NEEDMOREPARAMS | Not enough parameters |
| 464 | ERR_PASSWDMISMATCH | Password incorrect |
| 471 | ERR_CHANNELISFULL | Channel is full (+l) |
| 473 | ERR_INVITEONLYCHAN | Invite only (+i) |
| 475 | ERR_BADCHANNELKEY | Bad channel key (+k) |
| 482 | ERR_CHANOPRIVSNEEDED | You're not operator |

---

**Good luck with your evaluation!**
