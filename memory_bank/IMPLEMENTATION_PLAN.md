# ft_irc — Implementation Plan

## Phase 1: Basic Infrastructure
1. Makefile
2. Server class with socket setup
3. poll() event loop
4. Accept connections

## Phase 2: Client Management
1. Client class
2. Client tracking (map by FD)
3. Receive/send buffers
4. Message parsing

## Phase 3: Authentication
1. PASS command
2. NICK command
3. USER command
4. Registration flow

## Phase 4: Messaging
1. PRIVMSG to user
2. PRIVMSG to channel
3. PING/PONG

## Phase 5: Channels
1. Channel class
2. JOIN command
3. PART command
4. TOPIC command

## Phase 6: Operators
1. MODE command (i, t, k, o, l)
2. KICK command
3. INVITE command

## Phase 7: Polish
1. Error handling
2. Edge cases
3. Valgrind clean
4. Test with irssi

---

**Start:** Phase 1, Step 1 (Makefile)
