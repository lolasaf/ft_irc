# ft_irc Test Cases

This file contains all test commands used to verify the IRC server functionality.

**Prerequisites:**
- Server compiled: `make`
- Server running: `./ircserv <port> <password>`

---

## Table of Contents

1. [Basic Registration](#1-basic-registration)
2. [JOIN Command](#2-join-command)
3. [PART Command](#3-part-command)
4. [TOPIC Command](#4-topic-command)
5. [INVITE Command](#5-invite-command)
6. [KICK Command](#6-kick-command)
7. [MODE Command](#7-mode-command)
8. [PRIVMSG Command](#8-privmsg-command)
9. [NOTICE Command](#9-notice-command)
10. [QUIT Command](#10-quit-command)
11. [Error Cases](#11-error-cases)
12. [Memory Leak Testing](#12-memory-leak-testing)
13. [Multi-User Scenarios](#13-multi-user-scenarios)
14. [Edge Cases (Evaluator Focus)](#14-edge-cases-evaluator-focus)
15. [IRC Client Compatibility](#15-irc-client-compatibility)

---

## 1. Basic Registration

### Test 1.1: Successful Registration
**Description:** Test complete registration flow with PASS, NICK, USER commands.  
**Expected Result:** User receives welcome messages (001-005 numerics).

```bash
./ircserv 6667 pass &
sleep 0.5

{
  echo "PASS pass"
  echo "NICK alice"
  echo "USER alice 0 * :Alice Smith"
} | nc localhost 6667
```

**Expected Output:**
```
:SugarDaddyFinderIRC 001 alice :Welcome to the ft_irc server, alice!
:SugarDaddyFinderIRC 002 alice :Your host is SugarDaddyFinderIRC
:SugarDaddyFinderIRC 003 alice :This server was created just now
:SugarDaddyFinderIRC 004 alice :SugarDaddyFinderIRC 1.0 irc o o
:SugarDaddyFinderIRC 005 alice USERLEN=18 NICKLEN=9 REALLEN=50 CHANTYPES=# CHANNELLEN=50 :are supported by this server
```

### Test 1.2: Wrong Password
**Description:** Test registration with incorrect password.  
**Expected Result:** ERR_PASSWDMISMATCH (464).

```bash
{
  echo "PASS wrongpassword"
  echo "NICK alice"
} | nc localhost 6667
```

### Test 1.3: Duplicate Nickname
**Description:** Test registering with a nickname already in use.  
**Expected Result:** ERR_NICKNAMEINUSE (433).

```bash
# First user
{ echo "PASS pass"; echo "NICK alice"; echo "USER alice 0 * :Alice"; sleep 1; } | nc localhost 6667 &
sleep 0.5

# Second user tries same nick
{ echo "PASS pass"; echo "NICK alice"; } | nc localhost 6667
```

**Expected Output:**
```
:SugarDaddyFinderIRC 433 * alice :Nickname is already in use
```

---

## 2. JOIN Command

### Test 2.1: Create and Join Channel
**Description:** First user creates a channel and becomes operator.  
**Expected Result:** User joins, receives topic info (331/332), names list (353/366), and is marked as operator (@).

```bash
./ircserv 6667 pass &
sleep 0.5

{
  echo "PASS pass"
  echo "NICK alice"
  echo "USER alice 0 * :Alice"
  echo "JOIN #test"
} | nc localhost 6667
```

**Expected Output:**
```
:alice!alice@* JOIN #test
:SugarDaddyFinderIRC 331 alice #test :No topic is set
:SugarDaddyFinderIRC 353 alice = #test :@alice
:SugarDaddyFinderIRC 366 alice #test :End of /NAMES list
```

### Test 2.2: Second User Joins Existing Channel
**Description:** Second user joins an existing channel.  
**Expected Result:** User joins, sees existing members in names list.

```bash
# Alice creates channel
{ echo "PASS pass"; echo "NICK alice"; echo "USER alice 0 * :Alice"; echo "JOIN #test"; sleep 2; } | nc localhost 6667 &
sleep 0.5

# Bob joins
{
  echo "PASS pass"
  echo "NICK bob"
  echo "USER bob 0 * :Bob"
  echo "JOIN #test"
} | nc localhost 6667
```

**Expected Output (bob sees):**
```
:bob!bob@* JOIN #test
:SugarDaddyFinderIRC 353 bob = #test :@alice bob
:SugarDaddyFinderIRC 366 bob #test :End of /NAMES list
```

### Test 2.3: Join with Key (+k mode)
**Description:** Join a channel that has a key set.  
**Expected Result:** Must provide correct key to join.

```bash
# Alice creates channel with key
{ 
  echo "PASS pass"; echo "NICK alice"; echo "USER alice 0 * :Alice"
  echo "JOIN #secret"
  echo "MODE #secret +k mykey"
  sleep 2
} | nc localhost 6667 &
sleep 1

# Bob tries without key (should fail)
{ echo "PASS pass"; echo "NICK bob"; echo "USER bob 0 * :Bob"; echo "JOIN #secret"; } | nc localhost 6667

# Carol joins with correct key (should succeed)
{ echo "PASS pass"; echo "NICK carol"; echo "USER carol 0 * :Carol"; echo "JOIN #secret mykey"; } | nc localhost 6667
```

---

## 3. PART Command

### Test 3.1: Leave Channel
**Description:** User leaves a channel with a message.  
**Expected Result:** PART message broadcast to channel members.

```bash
./ircserv 6667 pass &
sleep 0.5

{
  echo "PASS pass"
  echo "NICK alice"
  echo "USER alice 0 * :Alice"
  echo "JOIN #test"
  sleep 0.2
  echo "PART #test :Goodbye everyone!"
} | nc localhost 6667
```

**Expected Output:**
```
:alice!alice@* PART #test :Goodbye everyone!
```

### Test 3.2: Part Non-Existent Channel
**Description:** Try to leave a channel that doesn't exist.  
**Expected Result:** ERR_NOSUCHCHANNEL (403).

```bash
{
  echo "PASS pass"
  echo "NICK alice"
  echo "USER alice 0 * :Alice"
  echo "PART #nonexistent"
} | nc localhost 6667
```

**Expected Output:**
```
:SugarDaddyFinderIRC 403 alice #nonexistent :No such channel
```

---

## 4. TOPIC Command

### Test 4.1: Set Topic (as operator)
**Description:** Channel operator sets a topic.  
**Expected Result:** TOPIC message broadcast to all channel members.

```bash
./ircserv 6667 pass &
sleep 0.5

{
  echo "PASS pass"
  echo "NICK alice"
  echo "USER alice 0 * :Alice"
  echo "JOIN #test"
  sleep 0.2
  echo "TOPIC #test :Welcome to the test channel!"
} | nc localhost 6667
```

**Expected Output:**
```
:alice!alice@* TOPIC #test :Welcome to the test channel!
```

### Test 4.2: Query Topic
**Description:** Query the current topic of a channel.  
**Expected Result:** RPL_TOPIC (332) with topic text, RPL_TOPICWHOTIME (333) with setter info.

```bash
# Alice sets topic
{ echo "PASS pass"; echo "NICK alice"; echo "USER alice 0 * :Alice"; echo "JOIN #test"; echo "TOPIC #test :Hello"; sleep 2; } | nc localhost 6667 &
sleep 1

# Bob queries topic
{
  echo "PASS pass"
  echo "NICK bob"
  echo "USER bob 0 * :Bob"
  echo "JOIN #test"
  sleep 0.2
  echo "TOPIC #test"
} | nc localhost 6667
```

**Expected Output:**
```
:SugarDaddyFinderIRC 332 bob #test :Hello
:SugarDaddyFinderIRC 333 bob #test alice <timestamp>
```

### Test 4.3: Set Topic with +t Mode (non-operator fails)
**Description:** Non-operator tries to set topic when +t mode is enabled.  
**Expected Result:** ERR_CHANOPRIVSNEEDED (482).

```bash
# Alice creates channel (has +t by default)
{ echo "PASS pass"; echo "NICK alice"; echo "USER alice 0 * :Alice"; echo "JOIN #test"; sleep 2; } | nc localhost 6667 &
sleep 0.5

# Bob tries to set topic (not an operator)
{
  echo "PASS pass"
  echo "NICK bob"
  echo "USER bob 0 * :Bob"
  echo "JOIN #test"
  sleep 0.2
  echo "TOPIC #test :My new topic"
} | nc localhost 6667
```

**Expected Output:**
```
:SugarDaddyFinderIRC 482 bob #test :You're not channel operator
```

---

## 5. INVITE Command

### Test 5.1: Invite User to Channel
**Description:** Operator invites a user to a channel.  
**Expected Result:** RPL_INVITING (341) sent to inviter, INVITE notification sent to invitee.

```bash
./ircserv 6667 pass &
sleep 0.5

# Alice creates channel
{ echo "PASS pass"; echo "NICK alice"; echo "USER alice 0 * :Alice"; echo "JOIN #test"; sleep 0.5; echo "INVITE bob #test"; sleep 1; } | nc localhost 6667 &
sleep 0.3

# Bob connects (receives invite)
{ echo "PASS pass"; echo "NICK bob"; echo "USER bob 0 * :Bob"; sleep 2; } | nc localhost 6667
```

**Expected Output (alice):**
```
:SugarDaddyFinderIRC 341 alice bob #test
```

**Expected Output (bob):**
```
:alice!alice@* INVITE bob #test
```

### Test 5.2: Invite Bypasses +i Mode
**Description:** Invited user can join invite-only channel.  
**Expected Result:** User successfully joins after being invited.

```bash
./ircserv 6667 pass &
sleep 0.5

# Alice creates invite-only channel
{ 
  echo "PASS pass"; echo "NICK alice"; echo "USER alice 0 * :Alice"
  echo "JOIN #private"
  echo "MODE #private +i"
  sleep 0.5
  echo "INVITE bob #private"
  sleep 2
} | nc localhost 6667 &
sleep 1

# Bob joins after invite
{
  echo "PASS pass"
  echo "NICK bob"
  echo "USER bob 0 * :Bob"
  sleep 0.5
  echo "JOIN #private"
} | nc localhost 6667
```

**Expected Output (bob successfully joins):**
```
:bob!bob@* JOIN #private
```

### Test 5.3: Non-Operator Cannot Invite
**Description:** Non-operator tries to invite.  
**Expected Result:** ERR_CHANOPRIVSNEEDED (482).

```bash
# Alice creates channel
{ echo "PASS pass"; echo "NICK alice"; echo "USER alice 0 * :Alice"; echo "JOIN #test"; sleep 2; } | nc localhost 6667 &
sleep 0.5

# Bob joins and tries to invite carol
{
  echo "PASS pass"
  echo "NICK bob"
  echo "USER bob 0 * :Bob"
  echo "JOIN #test"
  sleep 0.2
  echo "INVITE carol #test"
} | nc localhost 6667
```

**Expected Output:**
```
:SugarDaddyFinderIRC 482 bob #test :You're not channel operator
```

### Test 5.4: Case-Insensitive Nick Lookup for INVITE
**Description:** INVITE finds user regardless of nick case (IRC nicknames are case-insensitive).  
**Expected Result:** RPL_INVITING (341) - user found despite case mismatch.

```bash
./ircserv 6667 pass > /dev/null 2>&1 &
sleep 0.5

# Bob connects with uppercase nick
{
  echo "PASS pass"
  echo "NICK BOB"
  echo "USER bob 0 * :Bob"
  sleep 0.5
} | nc localhost 6667 > /dev/null 2>&1 &
sleep 0.3

# Alice invites "bob" (lowercase) - should find "BOB"
{
  echo "PASS pass"
  echo "NICK alice"
  echo "USER alice 0 * :Alice"
  echo "JOIN #room"
  sleep 0.1
  echo "INVITE bob #room"   # lowercase "bob" finds "BOB"
  echo "QUIT"
} | nc localhost 6667 2>&1 | grep -E "(341|401)"
```

**Expected Output (341 = success, not 401 = not found):**
```
:SugarDaddyFinderIRC 341 alice bob #room
```

---

## 6. KICK Command

### Test 6.1: Operator Kicks User
**Description:** Channel operator kicks another user from the channel.  
**Expected Result:** KICK message broadcast to all channel members, kicked user is removed.

```bash
./ircserv 6667 pass &
sleep 0.5

# Alice creates channel
{ echo "PASS pass"; echo "NICK alice"; echo "USER alice 0 * :Alice"; echo "JOIN #test"; sleep 1; echo "KICK #test bob :Goodbye!"; sleep 1; } | nc localhost 6667 &
sleep 0.3

# Bob joins
{ echo "PASS pass"; echo "NICK bob"; echo "USER bob 0 * :Bob"; echo "JOIN #test"; sleep 2; } | nc localhost 6667
```

**Expected Output:**
```
:alice!alice@* KICK #test bob :Goodbye!
```

### Test 6.2: Non-Operator Cannot Kick
**Description:** Non-operator tries to kick another user.  
**Expected Result:** ERR_CHANOPRIVSNEEDED (482).

```bash
./ircserv 6667 pass &
sleep 0.5

# Alice creates channel
{ echo "PASS pass"; echo "NICK alice"; echo "USER alice 0 * :Alice"; echo "JOIN #test"; sleep 2; } | nc localhost 6667 &
sleep 0.5

# Bob joins and tries to kick alice
{
  echo "PASS pass"
  echo "NICK bob"
  echo "USER bob 0 * :Bob"
  echo "JOIN #test"
  sleep 0.2
  echo "KICK #test alice :bye"
} | nc localhost 6667
```

**Expected Output:**
```
:SugarDaddyFinderIRC 482 bob #test :You're not channel operator
```

### Test 6.3: Kick User Not on Channel
**Description:** Try to kick a user who isn't on the channel.  
**Expected Result:** ERR_USERNOTINCHANNEL (441).

```bash
{
  echo "PASS pass"
  echo "NICK alice"
  echo "USER alice 0 * :Alice"
  echo "JOIN #test"
  sleep 0.2
  echo "KICK #test nonexistent :bye"
} | nc localhost 6667
```

**Expected Output:**
```
:SugarDaddyFinderIRC 401 alice nonexistent :No such nick/channel
```

### Test 6.4: Case-Insensitive Nick Lookup for KICK
**Description:** KICK finds user regardless of nick case (IRC nicknames are case-insensitive).  
**Expected Result:** KICK succeeds despite case mismatch.

```bash
./ircserv 6667 pass > /dev/null 2>&1 &
sleep 0.5

# Bob joins with uppercase nick
{
  echo "PASS pass"
  echo "NICK BOB"
  echo "USER bob 0 * :Bob"
  echo "JOIN #test"
  sleep 1
} | nc localhost 6667 > /dev/null 2>&1 &
sleep 0.3

# Alice kicks "bob" (lowercase) - should find and kick "BOB"
{
  echo "PASS pass"
  echo "NICK alice"
  echo "USER alice 0 * :Alice"
  echo "JOIN #test"
  sleep 0.2
  echo "KICK #test bob :Goodbye"   # lowercase "bob" kicks "BOB"
} | nc localhost 6667 2>&1 | grep -E "(KICK|401|441)"
```

**Expected Output:**
```
:alice!alice@* KICK #test bob :Goodbye
```

---

## 7. MODE Command

### Test 7.1: Query Channel Modes
**Description:** Query current channel modes.  
**Expected Result:** RPL_CHANNELMODEIS (324) with current modes.

```bash
{
  echo "PASS pass"
  echo "NICK alice"
  echo "USER alice 0 * :Alice"
  echo "JOIN #test"
  sleep 0.2
  echo "MODE #test"
} | nc localhost 6667
```

**Expected Output:**
```
:SugarDaddyFinderIRC 324 alice #test +t
```

### Test 7.2: Set Multiple Modes
**Description:** Set invite-only and topic protection modes.  
**Expected Result:** MODE change broadcast to channel.

```bash
{
  echo "PASS pass"
  echo "NICK alice"
  echo "USER alice 0 * :Alice"
  echo "JOIN #test"
  sleep 0.2
  echo "MODE #test +it"
} | nc localhost 6667
```

**Expected Output:**
```
:alice!alice@* MODE #test +it
```

### Test 7.3: Set User Limit (+l)
**Description:** Set user limit on channel.  
**Expected Result:** MODE change with limit parameter.

```bash
{
  echo "PASS pass"
  echo "NICK alice"
  echo "USER alice 0 * :Alice"
  echo "JOIN #test"
  sleep 0.2
  echo "MODE #test +l 10"
} | nc localhost 6667
```

**Expected Output:**
```
:alice!alice@* MODE #test +l 10
```

### Test 7.3b: Invalid User Limit Values (+l validation)
**Description:** Test that invalid limit values are rejected.  
**Expected Result:** ERR_NEEDMOREPARAMS (461) for invalid values; valid limit is applied.

```bash
./ircserv 6667 pass > /dev/null 2>&1 &
sleep 0.5

{
  echo "PASS pass"
  echo "NICK alice"
  echo "USER alice 0 * :Alice"
  echo "JOIN #test"
  sleep 0.1
  echo "MODE #test +l -5"      # Negative - rejected
  echo "MODE #test +l abc"     # Non-numeric - rejected
  echo "MODE #test +l 99999"   # Out of range - rejected
  echo "MODE #test +l 50"      # Valid - accepted
  echo "MODE #test"            # Query - should show +l 50
  echo "QUIT"
} | nc localhost 6667 2>&1 | grep -E "(461|324|MODE \+l)"
```

**Expected Output:**
```
:SugarDaddyFinderIRC 461 alice MODE :Invalid limit for +l (must be positive integer)
:SugarDaddyFinderIRC 461 alice MODE :Invalid limit for +l (must be positive integer)
:SugarDaddyFinderIRC 461 alice MODE :Limit must be between 1 and 10000
:alice!alice@* MODE #test +l 50
:SugarDaddyFinderIRC 324 alice #test +l 50
```

### Test 7.4: Give/Take Operator (+o/-o)
**Description:** Give and remove operator status.  
**Expected Result:** MODE change, user gains/loses @ prefix.

```bash
# Alice creates channel
{ echo "PASS pass"; echo "NICK alice"; echo "USER alice 0 * :Alice"; echo "JOIN #test"; sleep 1; echo "MODE #test +o bob"; sleep 1; } | nc localhost 6667 &
sleep 0.5

# Bob joins
{ echo "PASS pass"; echo "NICK bob"; echo "USER bob 0 * :Bob"; echo "JOIN #test"; sleep 3; } | nc localhost 6667
```

**Expected Output:**
```
:alice!alice@* MODE #test +o bob
```

### Test 7.4b: Case-Insensitive Nick Lookup for MODE +o
**Description:** MODE +o finds user regardless of nick case.  
**Expected Result:** MODE change succeeds despite case mismatch.

```bash
./ircserv 6667 pass > /dev/null 2>&1 &
sleep 0.5

# Bob joins with uppercase nick
{
  echo "PASS pass"
  echo "NICK BOB"
  echo "USER bob 0 * :Bob"
  echo "JOIN #test"
  sleep 1
} | nc localhost 6667 > /dev/null 2>&1 &
sleep 0.3

# Alice gives op to "bob" (lowercase) - should find "BOB"
{
  echo "PASS pass"
  echo "NICK alice"
  echo "USER alice 0 * :Alice"
  echo "JOIN #test"
  sleep 0.2
  echo "MODE #test +o bob"   # lowercase "bob" grants op to "BOB"
} | nc localhost 6667 2>&1 | grep -E "(MODE.*\+o|401|441)"
```

**Expected Output:**
```
:alice!alice@* MODE #test +o bob
```

### Test 7.4c: MODE +o with Non-Existent User (ERR_NOSUCHNICK 401)
**Description:** MODE +o with a nickname that doesn't exist returns 401, not 441.  
**Expected Result:** ERR_NOSUCHNICK (401).

```bash
./ircserv 6667 pass > /dev/null 2>&1 &
sleep 0.5

{
  echo "PASS pass"
  echo "NICK alice"
  echo "USER alice 0 * :Alice"
  echo "JOIN #test"
  sleep 0.1
  echo "MODE #test +o nonexistent"   # User doesn't exist at all
  echo "QUIT"
} | nc localhost 6667 2>&1 | grep -E "(401|441)"
```

**Expected Output (401 not 441):**
```
:SugarDaddyFinderIRC 401 alice nonexistent :No such nick/channel
```

### Test 7.5: MODE Query Security (non-member blocked)
**Description:** Non-member cannot query channel modes (protects channel key from leaking).  
**Expected Result:** ERR_NOTONCHANNEL (442).

```bash
./ircserv 6667 pass > /dev/null 2>&1 &
sleep 0.5

# Owner creates channel with key
{
  echo "PASS pass"
  echo "NICK owner"
  echo "USER owner 0 * :Owner"
  echo "JOIN #secret"
  echo "MODE #secret +k mykey"
  sleep 0.5
} | nc localhost 6667 > /dev/null 2>&1 &
sleep 0.3

# Non-member tries to query modes
{
  echo "PASS pass"
  echo "NICK spy"
  echo "USER spy 0 * :Spy"
  sleep 0.1
  echo "MODE #secret"   # Should fail - not on channel
  echo "QUIT"
} | nc localhost 6667 2>&1 | grep -E "(442|324)"
```

**Expected Output:**
```
:SugarDaddyFinderIRC 442 spy #secret :You're not on that channel
```

### Test 7.5b: MODE Query Key Masking
**Description:** When querying modes, channel key is shown as `*`, not the actual value.  
**Expected Result:** Key appears as `+k *` in 324 response.

```bash
./ircserv 6667 pass > /dev/null 2>&1 &
sleep 0.5

{
  echo "PASS pass"
  echo "NICK alice"
  echo "USER alice 0 * :Alice"
  echo "JOIN #test"
  sleep 0.1
  echo "MODE #test +k secretpassword"
  echo "MODE #test"   # Query modes - key should be masked
  echo "QUIT"
} | nc localhost 6667 2>&1 | grep "324"
```

**Expected Output (key masked as *):**
```
:SugarDaddyFinderIRC 324 alice #test +tk *
```

### Test 7.6: MODE Partial Application (valid modes broadcast despite later errors)
**Description:** When some modes succeed and others fail, successful modes are applied and broadcast.  
**Expected Result:** `+it` applied and broadcast, error for invalid mode `X`.

```bash
./ircserv 6667 pass > /dev/null 2>&1 &
sleep 0.5

{
  echo "PASS pass"
  echo "NICK alice"
  echo "USER alice 0 * :Alice"
  echo "JOIN #test"
  sleep 0.1
  echo "MODE #test +itX"   # +i and +t valid, X invalid
  echo "MODE #test"        # Query to verify modes applied
  echo "QUIT"
} | nc localhost 6667 2>&1 | grep -E "(MODE|324|472)"
```

**Expected Output:**
```
:alice!alice@* MODE #test +it
:SugarDaddyFinderIRC 472 alice X :Unknown MODE flag
:SugarDaddyFinderIRC 324 alice #test +it
```

---

## 8. PRIVMSG Command

### Test 8.1: Channel Message
**Description:** Send message to a channel.  
**Expected Result:** Message delivered to all channel members except sender.

```bash
# Alice in channel
{ echo "PASS pass"; echo "NICK alice"; echo "USER alice 0 * :Alice"; echo "JOIN #test"; sleep 2; } | nc localhost 6667 &
sleep 0.5

# Bob sends message
{
  echo "PASS pass"
  echo "NICK bob"
  echo "USER bob 0 * :Bob"
  echo "JOIN #test"
  sleep 0.2
  echo "PRIVMSG #test :Hello everyone!"
} | nc localhost 6667
```

**Expected Output (alice receives):**
```
:bob!bob@* PRIVMSG #test :Hello everyone!
```

### Test 8.2: Private Message
**Description:** Send direct message to another user.  
**Expected Result:** Message delivered only to target user.

```bash
# Alice online
{ echo "PASS pass"; echo "NICK alice"; echo "USER alice 0 * :Alice"; sleep 2; } | nc localhost 6667 &
sleep 0.5

# Bob sends PM
{
  echo "PASS pass"
  echo "NICK bob"
  echo "USER bob 0 * :Bob"
  sleep 0.2
  echo "PRIVMSG alice :Hello Alice!"
} | nc localhost 6667
```

**Expected Output (alice receives):**
```
:bob!bob@* PRIVMSG alice :Hello Alice!
```

---

## 9. NOTICE Command

### Test 9.1: Channel Notice
**Description:** Send NOTICE to a channel (similar to PRIVMSG but should not trigger auto-replies).  
**Expected Result:** NOTICE delivered to all channel members except sender.

```bash
./ircserv 6667 pass &
sleep 0.5

# Alice in channel
{ echo "PASS pass"; echo "NICK alice"; echo "USER alice 0 * :Alice"; echo "JOIN #test"; sleep 2; } | nc localhost 6667 &
sleep 0.5

# Bob sends notice
{
  echo "PASS pass"
  echo "NICK bob"
  echo "USER bob 0 * :Bob"
  echo "JOIN #test"
  sleep 0.2
  echo "NOTICE #test :Server maintenance in 5 minutes"
} | nc localhost 6667
```

**Expected Output (alice receives):**
```
:bob!bob@* NOTICE #test :Server maintenance in 5 minutes
```

### Test 9.2: Private Notice
**Description:** Send NOTICE directly to another user.  
**Expected Result:** NOTICE delivered to target user.

```bash
# Alice online
{ echo "PASS pass"; echo "NICK alice"; echo "USER alice 0 * :Alice"; sleep 2; } | nc localhost 6667 &
sleep 0.5

# Bob sends private notice
{
  echo "PASS pass"
  echo "NICK bob"
  echo "USER bob 0 * :Bob"
  sleep 0.2
  echo "NOTICE alice :Hey, check your messages!"
} | nc localhost 6667
```

**Expected Output (alice receives):**
```
:bob!bob@* NOTICE alice :Hey, check your messages!
```

### Test 9.3: NOTICE to Non-Existent User (no error reply)
**Description:** NOTICE to non-existent user should NOT return an error (per IRC protocol).  
**Expected Result:** No ERR_NOSUCHNICK reply (unlike PRIVMSG which does send errors).

```bash
{
  echo "PASS pass"
  echo "NICK bob"
  echo "USER bob 0 * :Bob"
  sleep 0.1
  echo "NOTICE nonexistent :Hello?"
  echo "QUIT"
} | nc localhost 6667 2>&1 | grep -c "401"
```

**Expected Output:** `0` (no 401 error returned)

---

## 10. QUIT Command

### Test 10.1: Graceful Quit
**Description:** User quits with a message.  
**Expected Result:** QUIT message broadcast to all users sharing channels.

```bash
# Alice in channel
{ echo "PASS pass"; echo "NICK alice"; echo "USER alice 0 * :Alice"; echo "JOIN #test"; sleep 2; } | nc localhost 6667 &
sleep 0.5

# Bob joins and quits
{
  echo "PASS pass"
  echo "NICK bob"
  echo "USER bob 0 * :Bob"
  echo "JOIN #test"
  sleep 0.2
  echo "QUIT :Goodbye cruel world!"
} | nc localhost 6667
```

**Expected Output (alice receives):**
```
:bob!bob@* QUIT :Goodbye cruel world!
```

---

## 11. Error Cases

### Test 10.1: Command Before Registration
**Description:** Try to use commands before completing registration.  
**Expected Result:** ERR_NOTREGISTERED (451).

```bash
{
  echo "JOIN #test"
} | nc localhost 6667
```

**Expected Output:**
```
:SugarDaddyFinderIRC 451 * :You have not registered
```

### Test 10.2: Not Enough Parameters
**Description:** Send command without required parameters.  
**Expected Result:** ERR_NEEDMOREPARAMS (461).

```bash
{
  echo "PASS pass"
  echo "NICK alice"
  echo "USER alice 0 * :Alice"
  echo "KICK"
} | nc localhost 6667
```

**Expected Output:**
```
:SugarDaddyFinderIRC 461 alice KICK :Not enough parameters
```

### Test 10.3: No Such Channel
**Description:** Try to join/message non-existent channel.  
**Expected Result:** ERR_NOSUCHCHANNEL (403).

```bash
{
  echo "PASS pass"
  echo "NICK alice"
  echo "USER alice 0 * :Alice"
  echo "PART #nonexistent"
} | nc localhost 6667
```

---

## 12. Memory Leak Testing

### Test 11.1: Valgrind Basic Test
**Description:** Run server under valgrind, perform operations, check for leaks.  
**Expected Result:** 0 definitely lost, 0 indirectly lost, 0 possibly lost bytes.

```bash
pkill -9 ircserv 2>/dev/null; sleep 1

valgrind --leak-check=full --show-leak-kinds=all ./ircserv 6668 pass &
VPID=$!
sleep 2

# Test operations
{
  echo "PASS pass"
  echo "NICK alice"
  echo "USER alice 0 * :Alice"
  echo "JOIN #test"
  echo "TOPIC #test :Hello"
  echo "PART #test"
  echo "QUIT :Bye"
} | nc localhost 6668

sleep 1
kill -INT $VPID
wait $VPID 2>/dev/null
```

**Expected Output:**
```
==XXXXX== LEAK SUMMARY:
==XXXXX==    definitely lost: 0 bytes in 0 blocks
==XXXXX==    indirectly lost: 0 bytes in 0 blocks
==XXXXX==      possibly lost: 0 bytes in 0 blocks
==XXXXX==    still reachable: ~75,000 bytes in 12 blocks (normal - static/global)
==XXXXX== ERROR SUMMARY: 0 errors from 0 contexts
```

### Test 11.2: Multi-User Stress Test
**Description:** Multiple users joining, messaging, leaving under valgrind.  
**Expected Result:** No memory leaks after all users disconnect.

```bash
valgrind --leak-check=full ./ircserv 6668 pass &
sleep 2

# Multiple users
for i in 1 2 3 4 5; do
  { echo "PASS pass"; echo "NICK user$i"; echo "USER user$i 0 * :User $i"; echo "JOIN #test"; sleep 0.5; echo "QUIT :done"; } | nc localhost 6668 &
done

sleep 3
pkill -INT ircserv
```

---

## 13. Multi-User Scenarios

### Test 12.1: Full Channel Lifecycle
**Description:** Complete test of channel creation, joins, topic, kick, part.  
**Expected Result:** All operations work correctly with proper broadcasts.

```bash
./ircserv 6667 pass &
sleep 0.5

# Alice creates channel
{
  echo "PASS pass"
  echo "NICK alice"
  echo "USER alice 0 * :Alice"
  echo "JOIN #test"
  sleep 2
  echo "KICK #test bob :You're out!"
  sleep 1
} | nc localhost 6667 &
sleep 0.3

# Bob joins
{
  echo "PASS pass"
  echo "NICK bob"
  echo "USER bob 0 * :Bob"
  echo "JOIN #test"
  sleep 4
} | nc localhost 6667 &
sleep 0.3

# Carol joins
{
  echo "PASS pass"
  echo "NICK carol"
  echo "USER carol 0 * :Carol"
  echo "JOIN #test"
  echo "PRIVMSG #test :Hello everyone!"
  sleep 3
  echo "PART #test :Leaving"
} | nc localhost 6667

sleep 1
pkill ircserv
```

---

## 14. Edge Cases (Evaluator Focus)

These tests are specifically mentioned in the evaluation criteria as potential traps.

### Test 14.1: Partial Commands via nc
**Description:** Send commands character by character with delays to test input buffering.  
**Expected Result:** Server correctly assembles partial data and processes complete commands.

```bash
./ircserv 6667 pass &
sleep 0.5

# Send registration one character at a time (simulates very slow network)
{
  for char in P A S S ' ' p a s s $'\r' $'\n'; do
    printf "%s" "$char"
    sleep 0.1
  done
  for char in N I C K ' ' a l i c e $'\r' $'\n'; do
    printf "%s" "$char"
    sleep 0.1
  done
  echo "USER alice 0 * :Alice"
  sleep 0.5
  echo "QUIT"
} | nc localhost 6667
```

**Expected Output:** Welcome messages (001-005), indicating registration succeeded despite byte-by-byte delivery.

### Test 14.2: Half-Sent Commands (No Newline)
**Description:** Send partial command without terminating `\r\n`, then complete it.  
**Expected Result:** Server buffers partial data, processes when complete.

```bash
./ircserv 6667 pass &
sleep 0.5

{
  printf "PASS pa"           # Partial command
  sleep 0.3
  printf "ss\r\n"            # Complete with rest + newline
  echo "NICK alice"
  echo "USER alice 0 * :Alice"
  sleep 0.3
  echo "QUIT"
} | nc localhost 6667
```

**Expected Output:** Registration completes successfully (001 welcome message).

### Test 14.3: Multiple Commands in Single Packet
**Description:** Send multiple IRC commands concatenated in a single TCP packet.  
**Expected Result:** Server correctly parses and processes all commands.

```bash
./ircserv 6667 pass &
sleep 0.5

# Send everything as one burst
printf "PASS pass\r\nNICK alice\r\nUSER alice 0 * :Alice\r\nJOIN #test\r\nQUIT\r\n" | nc localhost 6667
```

**Expected Output:** All commands processed - welcome messages, JOIN confirmation, then disconnect.

### Test 14.4: Unexpected Client Termination (Kill nc)
**Description:** Kill client connection abruptly mid-session.  
**Expected Result:** Server handles disconnect gracefully, no crash, no leak.

```bash
./ircserv 6667 pass &
sleep 0.5

# Alice stays connected
{ echo "PASS pass"; echo "NICK alice"; echo "USER alice 0 * :Alice"; echo "JOIN #test"; sleep 5; } | nc localhost 6667 &
ALICE_PID=$!
sleep 0.5

# Bob connects, joins, then gets killed
{
  echo "PASS pass"
  echo "NICK bob"
  echo "USER bob 0 * :Bob"
  echo "JOIN #test"
  sleep 0.3
} | nc localhost 6667 &
BOB_NC_PID=$!
sleep 0.5

# Kill bob's nc abruptly (simulates network drop)
kill -9 $BOB_NC_PID 2>/dev/null

# Give server time to detect and clean up
sleep 1

# Verify server still running and alice still connected
kill -0 $(pgrep ircserv) && echo "Server still running: OK"

# Cleanup
kill $ALICE_PID 2>/dev/null
pkill ircserv
```

**Expected Output:** "Server still running: OK", Alice should see Bob's QUIT message.

### Test 14.5: Suspended Client Flood Test (SIGSTOP)
**Description:** Suspend client to cause output buffer to fill up, then resume.  
**Expected Result:** Server handles blocked writes gracefully via poll(), no hang.

```bash
./ircserv 6667 pass &
SERVER_PID=$!
sleep 0.5

# Start receiver that will be suspended
nc localhost 6667 > /tmp/flood_output.txt &
NC_PID=$!
sleep 0.3

# Register the suspended client
echo -e "PASS pass\r\nNICK victim\r\nUSER victim 0 * :Victim\r\nJOIN #flood\r\n" > /proc/$NC_PID/fd/0 2>/dev/null || {
  # Alternative: use a named pipe
  echo "Using direct approach..."
}

# Actually, simpler approach - use background nc with input
{
  echo "PASS pass"
  echo "NICK victim"
  echo "USER victim 0 * :Victim"
  echo "JOIN #flood"
  sleep 10
} | nc localhost 6667 > /tmp/flood_output.txt &
VICTIM_PID=$!
sleep 0.5

# Suspend the victim's nc (simulates network congestion)
kill -STOP $VICTIM_PID

# Start flooder that sends many messages
{
  echo "PASS pass"
  echo "NICK flooder"
  echo "USER flooder 0 * :Flooder"
  echo "JOIN #flood"
  sleep 0.2
  # Send 100 messages rapidly
  for i in $(seq 1 100); do
    echo "PRIVMSG #flood :Flood message number $i"
  done
  sleep 0.5
  echo "QUIT"
} | nc localhost 6667 &
FLOODER_PID=$!

# Wait for flooder to finish
sleep 2

# Resume victim
kill -CONT $VICTIM_PID 2>/dev/null

# Check server didn't hang
sleep 1
if kill -0 $SERVER_PID 2>/dev/null; then
  echo "Server survived flood: OK"
else
  echo "Server crashed: FAIL"
fi

# Cleanup
kill $VICTIM_PID 2>/dev/null
pkill ircserv
```

**Expected Output:** "Server survived flood: OK"

### Test 14.6: Empty/Whitespace Commands
**Description:** Send empty lines and whitespace-only commands.  
**Expected Result:** Server ignores empty commands without crashing.

```bash
./ircserv 6667 pass &
sleep 0.5

{
  echo ""                    # Empty line
  echo "   "                 # Whitespace only
  echo "PASS pass"
  echo ""                    # Empty line mid-registration
  echo "NICK alice"
  echo "USER alice 0 * :Alice"
  echo ""
  echo "QUIT"
} | nc localhost 6667
```

**Expected Output:** Registration succeeds, empty lines ignored.

### Test 14.7: Very Long Input Line
**Description:** Send a line exceeding typical IRC message limits (512 bytes).  
**Expected Result:** Server handles gracefully (truncate or reject), no crash.

```bash
./ircserv 6667 pass &
sleep 0.5

{
  echo "PASS pass"
  echo "NICK alice"
  echo "USER alice 0 * :Alice"
  echo "JOIN #test"
  # Send a very long message (1000+ chars)
  printf "PRIVMSG #test :"
  for i in $(seq 1 200); do printf "AAAAA"; done
  printf "\r\n"
  echo "QUIT"
} | nc localhost 6667
```

**Expected Output:** Server processes or truncates message, continues running.

---

## 15. IRC Client Compatibility

### Test 15.1: HexChat Connection
**Description:** Test with HexChat IRC client.  
**Steps:**
1. Start server: `./ircserv 6667 testpass`
2. Open HexChat
3. Add new network:
   - Server: `localhost/6667`
   - Server Password: `testpass`
4. Connect and verify:
   - [x] Welcome message received
   - [x] Can join channels
   - [x] Can send/receive messages
   - [x] Can see user list
   - [x] Can use /kick, /topic, /mode as operator

### Test 15.2: irssi Connection
**Description:** Test with irssi terminal IRC client.

```bash
# Start server in one terminal
./ircserv 6667 testpass

# In another terminal, connect with irssi
irssi -c localhost -p 6667 -w testpass -n testuser
```

**In irssi:**
```
/join #test
/topic #test Hello World
/msg #test This is a test message
/query someuser
/quit
```

### Test 15.3: WeeChat Connection
**Description:** Test with WeeChat IRC client.

```bash
# Start server
./ircserv 6667 testpass

# In WeeChat
/server add ft_irc localhost/6667 -password=testpass
/connect ft_irc
/join #test
```

### Test 15.4: netcat Raw Protocol Test
**Description:** Manual IRC protocol testing with netcat.

```bash
# Connect and type commands manually
nc localhost 6667

# Type these commands:
PASS testpass
NICK manualtester
USER manualtester 0 * :Manual Tester
JOIN #test
PRIVMSG #test :Hello from netcat!
MODE #test
QUIT :Testing complete
```

---

## Quick Test Script

Save this as `run_tests.sh` for quick testing:

```bash
#!/bin/bash

PORT=6667
PASS=pass

# Start server
pkill -9 ircserv 2>/dev/null
sleep 0.5
./ircserv $PORT $PASS &
SERVER_PID=$!
sleep 0.5

echo "=== Running ft_irc tests ==="

# Basic registration test
echo -e "\n[TEST] Basic Registration..."
{ echo "PASS $PASS"; echo "NICK testuser"; echo "USER testuser 0 * :Test"; echo "QUIT"; } | nc localhost $PORT

# Channel test
echo -e "\n[TEST] Channel Operations..."
{ echo "PASS $PASS"; echo "NICK alice"; echo "USER alice 0 * :Alice"; echo "JOIN #test"; echo "TOPIC #test :Hello"; echo "QUIT"; } | nc localhost $PORT

# Partial command test
echo -e "\n[TEST] Partial Commands..."
{ printf "PASS pa"; sleep 0.1; printf "ss\r\n"; echo "NICK partial"; echo "USER partial 0 * :Test"; echo "QUIT"; } | nc localhost $PORT

echo -e "\n=== Tests complete ==="

# Cleanup
kill $SERVER_PID 2>/dev/null
```

---

## Evaluation Checklist

Use this checklist during peer evaluation:

### Build & Run
- [ ] `make` compiles without errors
- [ ] Executable named `ircserv`
- [ ] Server starts with `./ircserv <port> <password>`

### Core Functionality
- [ ] nc can connect and register (PASS, NICK, USER)
- [ ] Multiple clients can connect simultaneously
- [ ] Channels work (JOIN, PART, TOPIC, KICK, INVITE)
- [ ] Messaging works (PRIVMSG, NOTICE)
- [ ] Modes work (+i, +t, +k, +l, +o)
- [ ] QUIT works with proper broadcast

### Edge Cases (Critical!)
- [ ] Partial commands handled correctly (Test 14.1, 14.2)
- [ ] Multiple commands in one packet (Test 14.3)
- [ ] Client kill doesn't crash server (Test 14.4)
- [ ] Suspended client doesn't hang server (Test 14.5)

### No Crashes
- [ ] No segfaults
- [ ] No unexpected termination
- [ ] Clean shutdown on Ctrl+C

### Memory
- [ ] Valgrind shows 0 definitely/indirectly lost bytes

---

## Notes

- All tests assume server is running on localhost
- Default port: 6667, password: "pass"
- Use `pkill ircserv` to stop server between tests
- Some tests require timing with `sleep` to ensure proper ordering
- For real client testing, use HexChat, irssi, or WeeChat
