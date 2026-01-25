# PRIVMSG Command - Pseudo Code

## IRC Protocol Format

```
PRIVMSG <target>[,<target2>,...] :<message>
```

**Examples:**
- `PRIVMSG #channel :Hello everyone!`
- `PRIVMSG nickname :Hello!`
- `PRIVMSG #chan1,#chan2,nickname :Broadcast message`

---

## Pseudo Code Implementation

```
FUNCTION handlePrivmsg(User* sender, Message msg):
    
    // ============================================
    // 1. REGISTRATION CHECK
    // ============================================
    IF sender is NOT registered:
        sendNumeric(sender, ERR_NOTREGISTERED, [], "You have not registered")
        RETURN
    
    // ============================================
    // 2. PARAMETER VALIDATION
    // ============================================
    IF msg.params.size() < 1:
        sendNumeric(sender, ERR_NORECIPIENT, ["PRIVMSG"], "No recipient given")
        RETURN
    
    IF msg.params.size() < 2:
        sendNumeric(sender, ERR_NOTEXTTOSEND, [], "No text to send")
        RETURN
    
    // Extract targets (first parameter) and message (second parameter, trailing)
    targets = msg.params[0]  // e.g., "#channel" or "nickname" or "#chan1,#chan2,nick"
    message = msg.params[1]  // Trailing parameter (may have leading ':')
    
    // Remove leading ':' from message if present (trailing param format)
    IF message[0] == ':':
        message = message.substr(1)
    
    IF message is empty:
        sendNumeric(sender, ERR_NOTEXTTOSEND, [], "No text to send")
        RETURN
    
    // ============================================
    // 3. BUILD PRIVMSG MESSAGE FORMAT
    // ============================================
    // IRC format: :nick!user@host PRIVMSG <target> :<message>
    hostmask = buildHostmask(sender)
    privmsgFormat = ":" + hostmask + " PRIVMSG " + target + " :" + message + "\r\n"
    
    // ============================================
    // 4. HANDLE MULTIPLE TARGETS (comma-separated)
    // ============================================
    targetList = splitCommaList(targets)
    
    FOR EACH target IN targetList:
        
        // ============================================
        // 5. DETERMINE IF TARGET IS CHANNEL OR USER
        // ============================================
        IF target[0] == '#' OR target[0] == '&':
            // TARGET IS A CHANNEL
            handleChannelPrivmsg(sender, target, privmsgFormat)
        ELSE:
            // TARGET IS A USER (nickname)
            handleUserPrivmsg(sender, target, privmsgFormat)
    
    END FOR
END FUNCTION


// ============================================
// HELPER: Handle PRIVMSG to Channel
// ============================================
FUNCTION handleChannelPrivmsg(User* sender, string channelName, string privmsgFormat):
    
    // Convert channel name to lowercase for lookup
    channelNameLower = toLower(channelName)
    
    // Find channel (case-insensitive)
    channel = findChannel(channelNameLower)
    
    IF channel is NULL:
        // Channel doesn't exist
        sendNumeric(sender, ERR_NOSUCHCHANNEL, [channelName], "No such channel")
        RETURN
    
    // Check if sender is a member of the channel
    IF NOT channel->isMember(sender):
        sendNumeric(sender, ERR_CANNOTSENDTOCHAN, [channelName], "Cannot send to channel")
        RETURN
    
    // Broadcast PRIVMSG to all channel members (excluding sender)
    // Note: privmsgFormat already has the correct target in it
    broadcastToChannel(channel, privmsgFormat, sender)
    
END FUNCTION


// ============================================
// HELPER: Handle PRIVMSG to User
// ============================================
FUNCTION handleUserPrivmsg(User* sender, string nickname, string privmsgFormat):
    
    // Find user by nickname (case-insensitive search)
    targetUser = NULL
    
    FOR EACH (fd, user) IN users map:
        IF caseInsensitiveCompare(user->getNickname(), nickname):
            targetUser = user
            BREAK
        END IF
    END FOR
    
    IF targetUser is NULL:
        // User not found
        sendNumeric(sender, ERR_NOSUCHNICK, [nickname], "No such nick/channel")
        RETURN
    
    // Check if target user is registered (has nickname set)
    IF targetUser->getNickname() is empty:
        sendNumeric(sender, ERR_NOSUCHNICK, [nickname], "No such nick/channel")
        RETURN
    
    // Replace target in privmsgFormat with actual nickname (preserve case)
    actualNickname = targetUser->getNickname()
    // Rebuild message with correct target nickname
    hostmask = buildHostmask(sender)
    message = extract message from privmsgFormat (after second ':')
    privmsgToUser = ":" + hostmask + " PRIVMSG " + actualNickname + " :" + message + "\r\n"
    
    // Send PRIVMSG to target user
    targetUser->getOutputBuffer() += privmsgToUser
    
END FUNCTION
```

---

## Error Codes Reference

| Code | Name | When to Send |
|------|------|--------------|
| 401 | ERR_NOSUCHNICK | Target user not found |
| 403 | ERR_NOSUCHCHANNEL | Target channel doesn't exist |
| 404 | ERR_CANNOTSENDTOCHAN | User not in channel (for channel PRIVMSG) |
| 411 | ERR_NORECIPIENT | No target specified |
| 412 | ERR_NOTEXTTOSEND | No message text provided |
| 451 | ERR_NOTREGISTERED | User not registered |

---

## Implementation Notes

### 1. **Case-Insensitive Nickname Lookup**
- Use `caseInsensitiveCompare()` to find users by nickname
- Store actual nickname case in User object
- Use actual nickname case in PRIVMSG format

### 2. **Channel vs User Detection**
- Channel names start with `#` or `&`
- Everything else is treated as a nickname
- Use `isValidChannelName()` if you want stricter validation

### 3. **Message Format**
- **To Channel:** `:sender!user@host PRIVMSG #channel :message`
- **To User:** `:sender!user@host PRIVMSG nickname :message`
- Always include `\r\n` at the end

### 4. **Multiple Targets**
- Support comma-separated targets: `PRIVMSG #chan1,#chan2,nick :message`
- Process each target independently
- Send errors for invalid targets, but continue processing others

### 5. **Channel Membership Check**
- User must be a member of the channel to send PRIVMSG
- Use `channel->isMember(sender)` to check
- Broadcast excludes sender (use `broadcastToChannel(chan, msg, sender)`)

### 6. **User Lookup Optimization**
- Currently O(n) iteration through users map
- Consider creating a nickname → User* map for O(log n) lookup (future optimization)

---

## Example Scenarios

### Scenario 1: PRIVMSG to Channel
```
User sends: PRIVMSG #test :Hello everyone!

1. Check registration ✓
2. Validate parameters ✓
3. Detect channel (starts with '#') ✓
4. Find channel "test" ✓
5. Check sender is member ✓
6. Broadcast to all members except sender ✓
```

### Scenario 2: PRIVMSG to User
```
User sends: PRIVMSG john :Hello!

1. Check registration ✓
2. Validate parameters ✓
3. Detect user (no '#') ✓
4. Find user "john" (case-insensitive) ✓
5. Send message to john's output buffer ✓
```

### Scenario 3: Multiple Targets
```
User sends: PRIVMSG #test,john,#other :Broadcast!

1. Split: ["#test", "john", "#other"]
2. Process #test → broadcast to channel
3. Process john → send to user
4. Process #other → broadcast to channel
```

### Scenario 4: Error Cases
```
PRIVMSG #nonexistent :Hello
→ ERR_NOSUCHCHANNEL (403)

PRIVMSG #test :Hello  (user not in channel)
→ ERR_CANNOTSENDTOCHAN (404)

PRIVMSG unknownuser :Hello
→ ERR_NOSUCHNICK (401)

PRIVMSG #test
→ ERR_NOTEXTTOSEND (412)
```

---

## Integration Points

1. **Add to `processMessage()` in `serverMessage.cpp`:**
   ```cpp
   else if (msg.command == "PRIVMSG")
       handlePrivmsg(user, msg);
   ```

2. **Implement in `serverChannel.cpp`** (or create `serverMessage.cpp` section)

3. **Use existing functions:**
   - `findChannel()` - for channel lookup
   - `broadcastToChannel()` - for channel messaging
   - `buildHostmask()` - for message format
   - `splitCommaList()` - for multiple targets
   - `caseInsensitiveCompare()` - for nickname lookup
