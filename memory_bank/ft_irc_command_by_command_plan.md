# ft_irc — Command-by-Command Implementation Plan (C++98)

This plan follows the architecture in `ft_irc_architecture_cxx98.md` and provides detailed implementation steps for each IRC command.

The goal is to implement commands **incrementally** so you always have a runnable server.

---

## Ground Rules (Do This First)

### A. Non-blocking + single select/poll discipline

- Set **O_NONBLOCK** on:
  - listening socket
  - every accepted user socket
- Event loop:
  - Only `accept()` when listen_fd is ready for read
  - Only `recv()` when user fd is ready for read
  - Only `send()` when user fd is ready for write
- **Never** "try to read/write just to see" outside readiness check

### B. Buffer aggregation (required)

```cpp
// In User class
std::string _inputBuffer;
std::string _outputBuffer;

// Append received data
_inputBuffer.append(buffer, bytesRead);

// Extract complete messages
while ((pos = _inputBuffer.find('\n')) != std::string::npos) {
    std::string msg = _inputBuffer.substr(0, pos);
    _inputBuffer.erase(0, pos + 1);
    if (!msg.empty() && msg[msg.size()-1] == '\r')
        msg.erase(msg.size() - 1);
    // Process msg...
}
```

Test with `nc -C` and fragmented input.

### C. Shared helpers (in CommandUtils.cpp)

```cpp
// Check if user is registered
static bool checkRegistered(User* user, const std::string& command) {
    if (!user->isRegistered()) {
        user->sendError(451, command, "You have not registered");
        return false;
    }
    return true;
}

// Split comma-separated list
static std::vector<std::string> splitCommaList(const std::string& list) {
    std::vector<std::string> result;
    std::string::size_type start = 0, end;
    while ((end = list.find(',', start)) != std::string::npos) {
        result.push_back(list.substr(start, end - start));
        start = end + 1;
    }
    result.push_back(list.substr(start));
    return result;
}
```

### D. Common numeric error codes

| Code | Name | Usage |
|------|------|-------|
| 401 | ERR_NOSUCHNICK | Target nick doesn't exist |
| 403 | ERR_NOSUCHCHANNEL | Channel doesn't exist |
| 421 | ERR_UNKNOWNCOMMAND | Unknown command |
| 431 | ERR_NONICKNAMEGIVEN | NICK without parameter |
| 432 | ERR_ERRONEUSNICKNAME | Invalid nick characters |
| 433 | ERR_NICKNAMEINUSE | Nick already taken |
| 441 | ERR_USERNOTINCHANNEL | Target not in channel (KICK) |
| 442 | ERR_NOTONCHANNEL | You're not in the channel |
| 443 | ERR_USERONCHANNEL | User already in channel |
| 451 | ERR_NOTREGISTERED | Command before registration |
| 461 | ERR_NEEDMOREPARAMS | Not enough parameters |
| 462 | ERR_ALREADYREGISTRED | Already registered |
| 464 | ERR_PASSWDMISMATCH | Wrong password |
| 471 | ERR_CHANNELISFULL | Channel is full (+l) |
| 473 | ERR_INVITEONLYCHAN | Invite only channel (+i) |
| 475 | ERR_BADCHANNELKEY | Wrong channel key (+k) |
| 476 | ERR_BADCHANMASK | Invalid channel name |
| 482 | ERR_CHANOPRIVSNEEDED | Not channel operator |

---

## Registration Pipeline (PASS → NICK → USER)

### 1) PASS

**Goal**: Validate server password before registration.

**Syntax**: `PASS <password>`

**Implementation** (in `CommandRegistration.cpp`):

```cpp
void Command::handlePass(Server* server, User* user, 
                         const std::vector<std::string>& tokens)
{
    // Already registered?
    if (user->isRegistered()) {
        user->sendError(462, "PASS", "You may not reregister");
        return;
    }
    
    // Need password parameter
    if (tokens.size() < 2) {
        user->sendError(461, "PASS", "Not enough parameters");
        return;
    }
    
    // Check password (remove leading ':' if present)
    std::string password = tokens[1];
    if (!password.empty() && password[0] == ':')
        password = password.substr(1);
    
    if (password != server->getPassword()) {
        user->sendError(464, "PASS", "Password incorrect");
        return;
    }
    
    user->setHasPassed(true);
    user->tryRegister();
}
```

**Tests**:
- Wrong password → `464`
- PASS after registration → `462`
- No parameter → `461`

---

### 2) NICK

**Goal**: Set or change nickname; enforce uniqueness.

**Syntax**: `NICK <nickname>`

**Implementation**:

```cpp
void Command::handleNick(Server* server, User* user,
                         const std::vector<std::string>& tokens)
{
    if (tokens.size() < 2) {
        user->sendError(431, "NICK", "No nickname given");
        return;
    }
    
    std::string newNick = tokens[1];
    if (!newNick.empty() && newNick[0] == ':')
        newNick = newNick.substr(1);
    
    // Validate nickname
    if (!isValidNick(newNick)) {
        user->sendError(432, newNick, "Erroneous nickname");
        return;
    }
    
    std::string newNickLower = normalize(newNick);
    
    // Check uniqueness (allow if it's the same user)
    User* existing = server->getUser(newNickLower);
    if (existing && existing != user) {
        user->sendError(433, newNick, "Nickname is already in use");
        return;
    }
    
    std::string oldNick = user->getNickname();
    
    // If registered and changing nick, notify channels
    if (user->isRegistered() && oldNick != "*") {
        // Broadcast nick change to all shared channels
        std::string msg = ":" + user->buildHostmask() + " NICK :" + newNick;
        // ... broadcast to all channels user is in
    }
    
    user->setNickname(newNick, newNickLower);
    user->tryRegister();
}
```

**Validation** (in `utils.cpp`):

```cpp
bool isValidNick(const std::string& nick)
{
    if (nick.empty() || nick.size() > MAX_NICK_LENGTH)
        return false;
    
    // First char must be letter or special
    char c = nick[0];
    if (!isalpha(c) && !isSpecial(c))
        return false;
    
    // Rest can include digits
    for (size_t i = 1; i < nick.size(); ++i) {
        c = nick[i];
        if (!isalpha(c) && !isdigit(c) && !isSpecial(c))
            return false;
    }
    return true;
}
```

**Tests**:
- Duplicate nick → `433`
- Invalid characters → `432`
- Nick change while in channel → broadcast to shared channels

---

### 3) USER

**Goal**: Set username/realname; finalize registration.

**Syntax**: `USER <username> <mode> <unused> :<realname>`

**Implementation**:

```cpp
void Command::handleUser(User* user, const std::vector<std::string>& tokens)
{
    if (user->isRegistered()) {
        user->sendError(462, "USER", "You may not reregister");
        return;
    }
    
    if (tokens.size() < 5) {
        user->sendError(461, "USER", "Not enough parameters");
        return;
    }
    
    std::string username = tokens[1];
    
    // Realname is the trailing parameter (starts with :)
    std::string realname;
    for (size_t i = 4; i < tokens.size(); ++i) {
        if (i > 4) realname += " ";
        realname += tokens[i];
    }
    if (!realname.empty() && realname[0] == ':')
        realname = realname.substr(1);
    
    user->setUsername(username);
    user->setRealname(realname);
    user->tryRegister();
}
```

**tryRegister()** (in `UserRegistration.cpp`):

```cpp
void User::tryRegister()
{
    if (_isRegistered) return;
    if (!_hasPassed || !_hasNick || !_hasUser) return;
    
    _isRegistered = true;
    sendWelcome();
}
```

**Tests**:
- USER before PASS/NICK → accepted, but not registered until all set
- USER twice after registration → `462`
- Any order of PASS/NICK/USER works

---

## Channel Commands (JOIN, PART, PRIVMSG, NOTICE)

### 4) JOIN

**Goal**: Join or create channels.

**Syntax**: `JOIN <channel>{,<channel>} [<key>{,<key>}]`

**Implementation** (in `CommandChannel.cpp`):

```cpp
bool Command::handleJoin(Server* server, User* user,
                         const std::vector<std::string>& tokens)
{
    if (!checkRegistered(user, "JOIN"))
        return false;
    
    if (tokens.size() < 2) {
        user->sendError(461, "JOIN", "Not enough parameters");
        return false;
    }
    
    std::vector<std::string> channels = splitCommaList(tokens[1]);
    std::vector<std::string> keys;
    if (tokens.size() >= 3) {
        std::string keyList = tokens[2];
        if (!keyList.empty() && keyList[0] == ':')
            keyList = keyList.substr(1);
        keys = splitCommaList(keyList);
    }
    
    for (size_t i = 0; i < channels.size(); ++i) {
        std::string key = (i < keys.size()) ? keys[i] : "";
        handleSingleJoin(server, user, channels[i], key);
    }
    return true;
}

bool Command::handleSingleJoin(Server* server, User* user,
                               const std::string& channelName,
                               const std::string& key)
{
    // Validate channel name
    if (!isValidChannelName(channelName)) {
        user->sendError(476, channelName, "Bad Channel Mask");
        return false;
    }
    
    // Already in this channel?
    if (user->getChannels().count(normalize(channelName)) > 0) {
        user->sendError(443, channelName, "is already on channel");
        return false;
    }
    
    bool wasCreated = false;
    Channel* channel = server->getOrCreateChannel(channelName, user, &wasCreated);
    
    // Check if user can join
    Channel::JoinResult result;
    if (!channel->can_user_join(user, key, result)) {
        switch (result) {
            case Channel::JOIN_INVITE_ONLY:
                user->sendError(473, channel->get_name(), "Cannot join channel (+i)");
                break;
            case Channel::JOIN_BAD_KEY:
                user->sendError(475, channel->get_name(), "Cannot join channel (+k)");
                break;
            case Channel::JOIN_FULL:
                user->sendError(471, channel->get_name(), "Cannot join channel (+l)");
                break;
        }
        return false;
    }
    
    // Add user to channel
    channel->add_user(user);
    user->addChannel(channelName);
    
    // First user becomes operator
    if (wasCreated)
        channel->make_user_operator(user);
    
    // Broadcast JOIN to all members
    std::string joinMsg = ":" + user->buildHostmask() + " JOIN :" + channel->get_name();
    broadcastToChannel(channel, joinMsg);
    
    // Send topic
    if (channel->get_topic().empty())
        user->sendServerMsg("331 " + user->getNickname() + " " + 
                           channel->get_name() + " :No topic is set");
    else
        user->sendServerMsg("332 " + user->getNickname() + " " +
                           channel->get_name() + " :" + channel->get_topic());
    
    // Send names list
    user->sendServerMsg("353 " + user->getNickname() + " = " +
                       channel->get_name() + " :" + channel->get_names_list());
    user->sendServerMsg("366 " + user->getNickname() + " " +
                       channel->get_name() + " :End of /NAMES list");
    
    return true;
}
```

**Tests**:
- JOIN creates channel, creator is op
- JOIN multiple channels: `JOIN #a,#b`
- `+i` blocks → `473`
- `+k` wrong key → `475`
- `+l` full → `471`

---

### 5) PART

**Goal**: Leave a channel.

**Syntax**: `PART <channel>{,<channel>} [:<message>]`

**Implementation**:

```cpp
bool Command::handlePart(Server* server, User* user,
                         const std::vector<std::string>& tokens)
{
    if (!checkRegistered(user, "PART"))
        return false;
    
    if (tokens.size() < 2) {
        user->sendError(461, "PART", "Not enough parameters");
        return false;
    }
    
    std::vector<std::string> channels = splitCommaList(tokens[1]);
    
    std::string partMessage = "Leaving";
    if (tokens.size() >= 3) {
        partMessage = tokens[2];
        if (!partMessage.empty() && partMessage[0] == ':')
            partMessage = partMessage.substr(1);
    }
    
    for (size_t i = 0; i < channels.size(); ++i)
        handleSinglePart(server, user, channels[i], partMessage);
    
    return true;
}

bool Command::handleSinglePart(Server* server, User* user,
                               const std::string& channelName,
                               const std::string& partMessage)
{
    Channel* channel = server->getChannel(channelName);
    if (!channel) {
        user->sendError(403, channelName, "No such channel");
        return false;
    }
    
    if (!channel->is_user_member(user)) {
        user->sendError(442, channel->get_name(), "You're not on that channel");
        return false;
    }
    
    // Broadcast PART to channel (including the leaving user)
    std::string partMsg = ":" + user->buildHostmask() + " PART " +
                          channel->get_name() + " :" + partMessage;
    broadcastToChannel(channel, partMsg);
    
    // Remove user from channel
    channel->remove_user(user);
    user->removeChannel(channelName);
    
    // Delete empty channel
    if (channel->get_connected_user_number() == 0)
        server->deleteChannel(channelName, "empty");
    
    return true;
}
```

**Tests**:
- PART from channel you're in → broadcast + removal
- PART from channel you're not in → `442`
- PART non-existent channel → `403`

---

### 6) PRIVMSG

**Goal**: Send message to user or channel.

**Syntax**: `PRIVMSG <target> :<message>`

**Implementation** (in `CommandMessaging.cpp`):

```cpp
void Command::handlePrivmsg(Server* server, User* user,
                            const std::vector<std::string>& tokens)
{
    handleMessage(server, user, tokens, "PRIVMSG");
}

void Command::handleMessage(Server* server, User* user,
                            const std::vector<std::string>& tokens,
                            const std::string& commandName)
{
    if (!checkRegistered(user, commandName))
        return;
    
    if (tokens.size() < 2) {
        user->sendError(411, commandName, "No recipient given");
        return;
    }
    if (tokens.size() < 3) {
        user->sendError(412, commandName, "No text to send");
        return;
    }
    
    std::string target = tokens[1];
    std::string text = tokens[2];
    // Handle potential multi-word message
    for (size_t i = 3; i < tokens.size(); ++i)
        text += " " + tokens[i];
    if (!text.empty() && text[0] == ':')
        text = text.substr(1);
    
    std::string msg = ":" + user->buildHostmask() + " " + commandName +
                      " " + target + " :" + text;
    
    if (target[0] == '#') {
        // Channel message
        Channel* channel = server->getChannel(target);
        if (!channel) {
            user->sendError(403, target, "No such channel");
            return;
        }
        if (!channel->is_user_member(user)) {
            user->sendError(442, target, "You're not on that channel");
            return;
        }
        // Broadcast to all except sender
        broadcastToChannel(channel, msg, user->getNicknameLower());
    } else {
        // Direct message to user
        User* recipient = server->getUser(target);
        if (!recipient) {
            user->sendError(401, target, "No such nick/channel");
            return;
        }
        recipient->getOutputBuffer() += msg + "\r\n";
    }
}
```

**Tests**:
- Message to channel → all members except sender receive
- DM to nick → only that user receives
- Non-member to channel → `442`
- Non-existent target → `401` or `403`

---

### 7) NOTICE

**Goal**: Same as PRIVMSG but should not trigger auto-replies.

**Syntax**: `NOTICE <target> :<message>`

**Implementation**:

```cpp
void Command::handleNotice(Server* server, User* user,
                           const std::vector<std::string>& tokens)
{
    handleMessage(server, user, tokens, "NOTICE");
}
```

Same as PRIVMSG, just different command name in the message.

---

## Operator Commands (KICK, INVITE, TOPIC, MODE)

### 8) KICK

**Goal**: Operator removes user from channel.

**Syntax**: `KICK <channel> <nick> [:<reason>]`

**Implementation**:

```cpp
bool Command::handleKick(Server* server, User* user,
                         const std::vector<std::string>& tokens)
{
    if (!checkRegistered(user, "KICK"))
        return false;
    
    if (tokens.size() < 3) {
        user->sendError(461, "KICK", "Not enough parameters");
        return false;
    }
    
    std::string channelName = tokens[1];
    std::string targetNick = tokens[2];
    std::string reason = user->getNickname();  // Default reason
    if (tokens.size() >= 4) {
        reason = tokens[3];
        if (!reason.empty() && reason[0] == ':')
            reason = reason.substr(1);
    }
    
    Channel* channel = server->getChannel(channelName);
    if (!channel) {
        user->sendError(403, channelName, "No such channel");
        return false;
    }
    
    if (!channel->is_user_member(user)) {
        user->sendError(442, channel->get_name(), "You're not on that channel");
        return false;
    }
    
    if (!channel->is_user_operator(user)) {
        user->sendError(482, channel->get_name(), "You're not channel operator");
        return false;
    }
    
    User* target = server->getUser(targetNick);
    if (!target) {
        user->sendError(401, targetNick, "No such nick/channel");
        return false;
    }
    
    if (!channel->is_user_member(target)) {
        user->sendError(441, targetNick, "They aren't on that channel");
        return false;
    }
    
    // Broadcast KICK
    std::string kickMsg = ":" + user->buildHostmask() + " KICK " +
                          channel->get_name() + " " + target->getNickname() +
                          " :" + reason;
    broadcastToChannel(channel, kickMsg);
    
    // Remove target from channel
    channel->remove_user(target);
    target->removeChannel(channelName);
    
    // Delete empty channel
    if (channel->get_connected_user_number() == 0)
        server->deleteChannel(channelName, "empty");
    
    return true;
}
```

**Tests**:
- Op kicks user → user removed, all notified
- Non-op tries → `482`
- Target not in channel → `441`

---

### 9) INVITE

**Goal**: Invite user to channel (required for +i channels).

**Syntax**: `INVITE <nick> <channel>`

**Implementation**:

```cpp
bool Command::handleInvite(Server* server, User* user,
                           const std::vector<std::string>& tokens)
{
    if (!checkRegistered(user, "INVITE"))
        return false;
    
    if (tokens.size() < 3) {
        user->sendError(461, "INVITE", "Not enough parameters");
        return false;
    }
    
    std::string targetNick = tokens[1];
    std::string channelName = tokens[2];
    
    Channel* channel = server->getChannel(channelName);
    if (!channel) {
        user->sendError(403, channelName, "No such channel");
        return false;
    }
    
    if (!channel->is_user_member(user)) {
        user->sendError(442, channel->get_name(), "You're not on that channel");
        return false;
    }
    
    if (!channel->is_user_operator(user)) {
        user->sendError(482, channel->get_name(), "You're not channel operator");
        return false;
    }
    
    User* target = server->getUser(targetNick);
    if (!target) {
        user->sendError(401, targetNick, "No such nick/channel");
        return false;
    }
    
    if (channel->is_user_member(target)) {
        user->sendError(443, targetNick, "is already on channel");
        return false;
    }
    
    // Add to invite list
    channel->add_invite(target->getNicknameLower());
    
    // Notify inviter (341 RPL_INVITING)
    user->sendServerMsg("341 " + user->getNickname() + " " +
                       target->getNickname() + " " + channel->get_name());
    
    // Notify target
    target->sendMsgFromUser(user, "INVITE " + target->getNickname() +
                           " :" + channel->get_name());
    
    return true;
}
```

**Tests**:
- Invite allows user to JOIN +i channel
- Non-op cannot invite → `482`
- Target already in channel → `443`

---

### 10) TOPIC

**Goal**: View or set channel topic.

**Syntax**: 
- View: `TOPIC <channel>`
- Set: `TOPIC <channel> :<topic>`

**Implementation**:

```cpp
bool Command::handleTopic(Server* server, User* user,
                          const std::vector<std::string>& tokens)
{
    if (!checkRegistered(user, "TOPIC"))
        return false;
    
    if (tokens.size() < 2) {
        user->sendError(461, "TOPIC", "Not enough parameters");
        return false;
    }
    
    std::string channelName = tokens[1];
    Channel* channel = server->getChannel(channelName);
    
    if (!channel) {
        user->sendError(403, channelName, "No such channel");
        return false;
    }
    
    if (!channel->is_user_member(user)) {
        user->sendError(442, channel->get_name(), "You're not on that channel");
        return false;
    }
    
    // View topic
    if (tokens.size() == 2) {
        if (channel->get_topic().empty())
            user->sendServerMsg("331 " + user->getNickname() + " " +
                               channel->get_name() + " :No topic is set");
        else {
            user->sendServerMsg("332 " + user->getNickname() + " " +
                               channel->get_name() + " :" + channel->get_topic());
            user->sendServerMsg("333 " + user->getNickname() + " " +
                               channel->get_name() + " " +
                               channel->get_topic_set_info());
        }
        return true;
    }
    
    // Set topic
    if (channel->has_topic_protection() && !channel->is_user_operator(user)) {
        user->sendError(482, channel->get_name(), "You're not channel operator");
        return false;
    }
    
    std::string newTopic = tokens[2];
    for (size_t i = 3; i < tokens.size(); ++i)
        newTopic += " " + tokens[i];
    if (!newTopic.empty() && newTopic[0] == ':')
        newTopic = newTopic.substr(1);
    
    channel->set_topic(newTopic, user->buildHostmask());
    
    // Broadcast topic change
    std::string topicMsg = ":" + user->buildHostmask() + " TOPIC " +
                           channel->get_name() + " :" + newTopic;
    broadcastToChannel(channel, topicMsg);
    
    return true;
}
```

**Tests**:
- View topic works
- Set topic works
- `+t` mode: non-op cannot set → `482`

---

### 11) MODE

**Goal**: Manage channel modes (+i +t +k +l +o).

**Syntax**:
- View: `MODE <channel>`
- Set: `MODE <channel> <modes> [<params>]`

**Implementation** (in `CommandModes.cpp`):

```cpp
bool Command::handleMode(Server* server, User* user,
                         const std::vector<std::string>& tokens)
{
    if (!checkRegistered(user, "MODE"))
        return false;
    
    if (tokens.size() < 2) {
        user->sendError(461, "MODE", "Not enough parameters");
        return false;
    }
    
    std::string target = tokens[1];
    
    // Only channel modes supported
    if (target[0] != '#') {
        // User modes - minimal implementation
        user->sendServerMsg("221 " + user->getNickname() + " +");
        return true;
    }
    
    Channel* channel = server->getChannel(target);
    if (!channel) {
        user->sendError(403, target, "No such channel");
        return false;
    }
    
    // View modes
    if (tokens.size() == 2) {
        std::string modes, params, paramsLog;
        formatChannelModes(channel, user, modes, params, paramsLog);
        sendModeReply(user, channel->get_name(), modes, params, paramsLog);
        return true;
    }
    
    // Modify modes - must be operator
    if (!channel->is_user_operator(user)) {
        user->sendError(482, channel->get_name(), "You're not channel operator");
        return false;
    }
    
    return handleModeChanges(server, user, channel, tokens);
}

bool Command::handleModeChanges(Server* server, User* user, Channel* channel,
                                const std::vector<std::string>& tokens)
{
    std::string modeStr = tokens[2];
    bool adding = true;
    size_t paramIndex = 3;
    std::string appliedModes;
    std::string appliedParams;
    
    for (size_t i = 0; i < modeStr.size(); ++i) {
        char c = modeStr[i];
        
        if (c == '+') {
            adding = true;
            continue;
        }
        if (c == '-') {
            adding = false;
            continue;
        }
        
        std::string modeParams;
        bool validMode = false;
        
        switch (c) {
            case 'i':  // Invite-only
                channel->set_invite_only(adding);
                validMode = true;
                break;
                
            case 't':  // Topic protection
                channel->set_topic_protection(adding);
                validMode = true;
                break;
                
            case 'k':  // Channel key
                if (adding) {
                    if (paramIndex >= tokens.size()) {
                        user->sendError(461, "MODE", "Not enough parameters");
                        continue;
                    }
                    std::string key = tokens[paramIndex++];
                    channel->set_password(key);
                    modeParams = key;
                } else {
                    channel->set_password("");
                }
                validMode = true;
                break;
                
            case 'l':  // User limit
                if (adding) {
                    if (paramIndex >= tokens.size()) {
                        user->sendError(461, "MODE", "Not enough parameters");
                        continue;
                    }
                    int limit = atoi(tokens[paramIndex++].c_str());
                    if (limit > 0) {
                        channel->set_user_limit(limit);
                        modeParams = toString(limit);
                    }
                } else {
                    channel->set_user_limit(0);
                }
                validMode = true;
                break;
                
            case 'o':  // Operator status
                if (paramIndex >= tokens.size()) {
                    user->sendError(461, "MODE", "Not enough parameters");
                    continue;
                }
                {
                    std::string targetNick = tokens[paramIndex++];
                    User* targetUser = server->getUser(targetNick);
                    if (!targetUser) {
                        user->sendError(401, targetNick, "No such nick/channel");
                        continue;
                    }
                    if (!channel->is_user_member(targetUser)) {
                        user->sendError(441, targetNick, "They aren't on that channel");
                        continue;
                    }
                    if (adding)
                        channel->make_user_operator(targetUser);
                    else
                        channel->remove_user_operator_status(targetUser);
                    modeParams = targetUser->getNickname();
                    validMode = true;
                }
                break;
                
            default:
                user->sendError(472, std::string(1, c), "is unknown mode char to me");
                continue;
        }
        
        if (validMode) {
            if (appliedModes.empty() || appliedModes[appliedModes.size()-1] != (adding ? '+' : '-'))
                appliedModes += (adding ? '+' : '-');
            appliedModes += c;
            if (!modeParams.empty())
                appliedParams += " " + modeParams;
        }
    }
    
    // Broadcast mode changes
    if (!appliedModes.empty()) {
        std::string modeMsg = ":" + user->buildHostmask() + " MODE " +
                              channel->get_name() + " " + appliedModes + appliedParams;
        broadcastToChannel(channel, modeMsg);
    }
    
    return true;
}
```

**Tests**:
- `MODE #chan` → shows current modes
- `MODE #chan +i` → sets invite-only
- `MODE #chan +k secret` → sets key
- `MODE #chan +l 10` → sets limit
- `MODE #chan +o nick` → gives op
- `MODE #chan +it` → sets both modes
- `MODE #chan -k` → removes key
- Non-op → `482`

---

## Connection Commands (QUIT)

### 12) QUIT

**Goal**: Clean disconnect with message broadcast.

**Syntax**: `QUIT [:<message>]`

**Implementation** (in `CommandConnection.cpp`):

```cpp
void Command::handleQuit(Server* server, User* user,
                         const std::vector<std::string>& tokens)
{
    std::string reason = "Client Quit";
    if (tokens.size() >= 2) {
        reason = tokens[1];
        if (!reason.empty() && reason[0] == ':')
            reason = reason.substr(1);
    }
    
    server->disconnectUser(user->getFd(), reason);
}
```

The actual cleanup happens in `Server::disconnectUser()`.

---

## Utility Commands (LIST)

### 13) LIST

**Goal**: Show available channels.

**Syntax**: `LIST`

**Implementation**:

```cpp
bool Command::handleList(Server* server, User* user)
{
    if (!checkRegistered(user, "LIST"))
        return false;
    
    // 321 RPL_LISTSTART
    user->sendServerMsg("321 " + user->getNickname() + " Channel :Users  Name");
    
    std::map<std::string, Channel*>& channels = server->getAllChannels();
    for (std::map<std::string, Channel*>::iterator it = channels.begin();
         it != channels.end(); ++it)
    {
        Channel* channel = it->second;
        // 322 RPL_LIST
        user->sendServerMsg("322 " + user->getNickname() + " " +
                           channel->get_name() + " " +
                           toString(channel->get_connected_user_number()) +
                           " :" + channel->get_topic());
    }
    
    // 323 RPL_LISTEND
    user->sendServerMsg("323 " + user->getNickname() + " :End of /LIST");
    
    return true;
}
```

---

## Command Dispatch (Command.cpp)

```cpp
bool Command::handleCommand(Server* server, User* user,
                            std::vector<std::string>& tokens)
{
    if (tokens.empty())
        return false;
    
    Cmd cmdType = getCmd(tokens);
    
    switch (cmdType) {
        case NICK:    handleNick(server, user, tokens); break;
        case USER:    handleUser(user, tokens); break;
        case PASS:    handlePass(server, user, tokens); break;
        case JOIN:    handleJoin(server, user, tokens); break;
        case PART:    handlePart(server, user, tokens); break;
        case QUIT:    handleQuit(server, user, tokens); break;
        case PRIVMSG: handlePrivmsg(server, user, tokens); break;
        case NOTICE:  handleNotice(server, user, tokens); break;
        case TOPIC:   handleTopic(server, user, tokens); break;
        case KICK:    handleKick(server, user, tokens); break;
        case INVITE:  handleInvite(server, user, tokens); break;
        case MODE:    handleMode(server, user, tokens); break;
        case LIST:    handleList(server, user); break;
        default:
            return false;  // Unknown command
    }
    return true;
}
```

---

## Test Checklist

### Registration
- [ ] Wrong password → `464`
- [ ] Duplicate nick → `433`
- [ ] Any order of PASS/NICK/USER
- [ ] Re-registration attempts → `462`

### Channels
- [ ] JOIN creates channel, creator is op
- [ ] JOIN multiple: `JOIN #a,#b`
- [ ] PART removes user, deletes empty channel
- [ ] `+i` blocks non-invited → `473`
- [ ] `+k` wrong key → `475`
- [ ] `+l` full → `471`

### Messaging
- [ ] PRIVMSG to channel → all members except sender
- [ ] PRIVMSG to nick → only recipient
- [ ] Non-member to channel → `442`
- [ ] Non-existent target → `401`/`403`

### Operators
- [ ] First joiner is op
- [ ] Non-op KICK/MODE/INVITE → `482`
- [ ] `+t` blocks non-op TOPIC → `482`
- [ ] `+o`/`-o` grants/removes op

### Modes
- [ ] MODE view shows current modes
- [ ] Each mode enforces behavior
- [ ] Combined modes: `+itk secret`
- [ ] Missing params → `461`
