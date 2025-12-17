# ft_irc — C++98 Architecture Guide (ircserv)

This document describes a **clean, modular, C++98** architecture for the 42 *ft_irc* project.

It satisfies the subject constraints: **single `select()` (or `poll()`)**, **non-blocking I/O**, **no fork**, **multiple clients**, TCP/IP, and the mandatory IRC features.

---

## Goals and Constraints

### Must have (subject)
- **C++98 only** (`-std=c++98`), no Boost/external libs
- **One** `select()` or `poll()` to drive all I/O; do **not** read/write sockets outside of readiness
- All sockets **non-blocking**
- Handle **partial reads** and reassemble commands from packets
- Implement: PASS/NICK/USER (auth), JOIN, PART, PRIVMSG, NOTICE, channel broadcasting, operators + channel op commands:
  - `KICK`, `INVITE`, `TOPIC`, `MODE` with `i t k o l`
- No server-to-server networking

### Recommended style rules
- Split large classes across multiple `.cpp` files by functionality
- Keep methods short; extract helpers into separate files
- Prefer **composition** over "god classes"
- No complex templates (simple `toString<T>()` is fine)
- Use exceptions only for fatal startup errors
- Centralize protocol formatting in User messaging methods

---

## Directory Layout

```
ircserv/
├── Makefile
├── include/
│   ├── defines.hpp      # Constants, colors, limits
│   ├── Server.hpp       # Main server class
│   ├── User.hpp         # Connection state (called "User" not "Client")
│   ├── Channel.hpp      # IRC channel state
│   ├── Command.hpp      # Static command dispatcher
│   ├── signal.hpp       # Signal handling (g_running)
│   └── utils.hpp        # Helper functions
├── src/
│   ├── main.cpp
│   ├── Server.cpp           # Core server logic, main loop
│   ├── ServerSocket.cpp     # Socket init, fd_set preparation
│   ├── ServerUser.cpp       # User accept, read/write, disconnect
│   ├── ServerChannel.cpp    # Channel creation/deletion
│   ├── User.cpp             # User state management
│   ├── UserMessaging.cpp    # Send methods (welcome, errors, messages)
│   ├── UserRegistration.cpp # Registration state tracking
│   ├── Channel.cpp          # Channel membership, modes
│   ├── Command.cpp          # Main dispatch + tokenizer
│   ├── CommandRegistration.cpp  # NICK, USER, PASS handlers
│   ├── CommandChannel.cpp       # JOIN, PART, INVITE, TOPIC, KICK, LIST
│   ├── CommandModes.cpp         # MODE command
│   ├── CommandMessaging.cpp     # PRIVMSG, NOTICE
│   ├── CommandConnection.cpp    # QUIT
│   ├── CommandUtils.cpp         # Helper functions for commands
│   ├── signal.cpp
│   └── utils.cpp
```

**Key insight**: Split implementation files by functionality, not just one `.cpp` per class. This keeps files readable and makes navigation easier during evaluation.

---

## High-Level Components

### 1) `Server` (application core)

**Owns**: listening socket fd, all Users (by fd map), all Channels (by name map), server config.

**Does not**: parse IRC text directly (delegate to Command), implement command logic (delegate to Command static methods).

**Key members:**
```cpp
class Server {
private:
    const std::string   _name;          // Server name for replies
    const std::string   _password;      // Server password
    const int           _port;
    int                 _fd;            // Listening socket

    std::map<int, User*>            _usersFd;    // Users by file descriptor
    std::map<std::string, User*>    _usersNick;  // Users by normalized nickname
    std::map<std::string, Channel*> _channels;   // Channels by normalized name

public:
    void run();  // Main event loop with select()
    
    // User management
    User*    getUser(int fd) const;
    User*    getUser(const std::string& nickname) const;
    void     disconnectUser(int fd, const std::string& reason);
    
    // Channel management
    Channel* getChannel(const std::string& name) const;
    Channel* getOrCreateChannel(const std::string& name, User* creator, bool* wasCreated);
    void     deleteChannel(const std::string& name, std::string reason);
};
```

**Key invariants:**
- *Never* `recv()` / `send()` unless `select()` says the fd is ready
- Remove disconnected users cleanly (from all channels, both maps)
- Delete channels when they become empty

---

### 2) `User` (connection state + per-fd buffers)

**Note**: Called "User" (not "Client") to distinguish from IRC client software.

**Key members:**
```cpp
class User {
private:
    int         _fd;              // Socket fd
    std::string _nickname;        // Display nickname
    std::string _nicknameLower;   // Normalized for lookups
    std::string _username;
    std::string _realname;
    std::string _host;            // IP address from accept()
    
    Server*     _server;          // Back-pointer for server methods
    std::string _inputBuffer;     // Accumulate until \r\n or \n
    std::string _outputBuffer;    // Queue for sending when ready
    
    std::set<std::string> _channels;  // Channels user is in (normalized)
    
    // Registration state
    bool _hasNick;
    bool _hasUser;
    bool _hasPassed;
    bool _isRegistered;

public:
    std::string buildHostmask() const;  // nick!user@host
    
    // Messaging (append to output buffer)
    void sendWelcome();
    void sendError(int code, const std::string& param, const std::string& msg);
    void sendServerMsg(const std::string& message);
    void sendMsgFromUser(const User* sender, const std::string& message);
    
    // Registration
    void tryRegister();
    bool isRegistered() const;
};
```

**Important**: User provides buffer operations and messaging, but **not** business rules. Business logic lives in Command handlers.

---

### 3) `Channel` (room state)

**Key members:**
```cpp
class Channel {
private:
    std::string _channel_name;
    std::string _channel_name_lower;  // Normalized
    std::string _channel_topic;
    std::string _channel_topic_set_by;
    int         _channel_topic_set_at;
    
    std::map<std::string, User*> _channel_members_by_nickname;
    std::map<std::string, User*> _channel_operators_by_nickname;
    std::set<std::string>        _channel_invitation_list;
    
    // Mode flags
    int         _user_limit;       // +l
    bool        _invite_only;      // +i
    bool        _topic_protection; // +t
    std::string _channel_key;      // +k

public:
    void add_user(User* user);
    void remove_user(User* user);
    bool is_user_member(User* user) const;
    
    void make_user_operator(User* user);
    void remove_user_operator_status(User* user);
    bool is_user_operator(const User* user) const;
    
    bool can_user_join(User* user, const std::string& key, JoinResult& result) const;
    
    std::string get_names_list() const;
    std::string get_mode_string(const User* user) const;
};
```

**Design choice**: Store `User*` pointers keyed by normalized nickname. Fast lookups without case sensitivity issues.

---

### 4) `Command` (static dispatcher)

**Key design**: All methods are `static`. This is a pure utility class with no state.

```cpp
class Command {
public:
    static bool handleCommand(Server* server, User* user, 
                              std::vector<std::string>& tokens);
    static void broadcastToChannel(Channel* channel, const std::string& message,
                                   const std::string& excludeNick = "");
    static std::vector<std::string> tokenize(const std::string& message);

private:
    Command();  // Prevent instantiation
    
    enum Cmd { UNKNOWN, NICK, USER, PASS, QUIT, PRIVMSG, NOTICE, 
               JOIN, PART, TOPIC, KICK, INVITE, MODE, LIST };
    
    // Registration commands
    static void handleNick(Server*, User*, const std::vector<std::string>&);
    static void handleUser(User*, const std::vector<std::string>&);
    static void handlePass(Server*, User*, const std::vector<std::string>&);
    
    // Channel commands
    static bool handleJoin(Server*, User*, const std::vector<std::string>&);
    static bool handlePart(Server*, User*, const std::vector<std::string>&);
    static bool handleTopic(Server*, User*, const std::vector<std::string>&);
    static bool handleKick(Server*, User*, const std::vector<std::string>&);
    static bool handleInvite(Server*, User*, const std::vector<std::string>&);
    static bool handleMode(Server*, User*, const std::vector<std::string>&);
    static bool handleList(Server*, User*);
    
    // Messaging
    static void handlePrivmsg(Server*, User*, const std::vector<std::string>&);
    static void handleNotice(Server*, User*, const std::vector<std::string>&);
    
    // Connection
    static void handleQuit(Server*, User*, const std::vector<std::string>&);
    
    // Utilities
    static Cmd getCmd(const std::vector<std::string>& tokens);
    static bool checkRegistered(User* user, const std::string& command);
    static std::vector<std::string> splitCommaList(const std::string& list);
};
```

---

## Data Ownership and Containers

### Server storage
- `std::map<int, User*> _usersFd` - Owns User objects, keyed by fd
- `std::map<std::string, User*> _usersNick` - Secondary index, same pointers
- `std::map<std::string, Channel*> _channels` - Owns Channel objects

### Memory management
- Users created with `new` in `acceptNewUser()`, deleted in `deleteUser()`
- Channels created with `new` in `getOrCreateChannel()`, deleted in `deleteChannel()`
- Both maps must be kept in sync

### Channel membership
Store `User*` pointers keyed by normalized nickname:
```cpp
std::map<std::string, User*> _channel_members_by_nickname;
std::map<std::string, User*> _channel_operators_by_nickname;
```

**Why maps instead of sets of fds?** Allows O(1) lookup by nickname without iterating.

---

## Event Loop Design (single select)

### Using `select()`

```cpp
void Server::run()
{
    fd_set readFds, writeFds;
    int maxFd, ready;

    while (g_running)
    {
        maxFd = prepareReadSet(readFds);
        int writeMaxFd = prepareWriteSet(writeFds);
        if (writeMaxFd > maxFd) maxFd = writeMaxFd;

        ready = select(maxFd + 1, &readFds, &writeFds, NULL, NULL);
        
        if (ready == -1) {
            if (errno == EINTR) return;  // Signal, clean shutdown
            throw std::runtime_error("select() failed");
        }

        if (FD_ISSET(_fd, &readFds))
            acceptNewUser();

        handleReadReadyUsers(readFds);
        handleWriteReadyUsers(writeFds);
    }
}
```

### Preparing fd_sets

```cpp
int Server::prepareReadSet(fd_set& readFds)
{
    FD_ZERO(&readFds);
    FD_SET(_fd, &readFds);  // Listen socket
    int maxFd = _fd;

    for (std::map<int, User*>::const_iterator it = _usersFd.begin(); 
         it != _usersFd.end(); ++it)
    {
        FD_SET(it->first, &readFds);
        if (it->first > maxFd) maxFd = it->first;
    }
    return maxFd;
}

int Server::prepareWriteSet(fd_set& writeFds)
{
    FD_ZERO(&writeFds);
    int maxFd = -1;

    for (std::map<int, User*>::const_iterator it = _usersFd.begin(); 
         it != _usersFd.end(); ++it)
    {
        User* user = it->second;
        if (user && !user->getOutputBuffer().empty())
        {
            FD_SET(it->first, &writeFds);
            if (it->first > maxFd) maxFd = it->first;
        }
    }
    return maxFd;
}
```

**Key insight**: Only add fds to write set if they have pending output. Prevents busy-looping.

---

## Input/Output Buffer Handling

### Reading (partial packets)

```cpp
Server::UserInputResult Server::handleUserInput(int fd)
{
    char buffer[MAX_BUFFER_SIZE];
    ssize_t bytesRead = recv(fd, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead == 0)
        return INPUT_DISCONNECTED;

    if (bytesRead < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return INPUT_OK;
        return INPUT_ERROR;
    }

    user->getInputBuffer().append(buffer, bytesRead);
    
    std::vector<std::string> messages = extractMessagesFromBuffer(user);
    
    for (size_t i = 0; i < messages.size(); ++i) {
        std::vector<std::string> tokens = Command::tokenize(messages[i]);
        if (!tokens.empty())
            Command::handleCommand(this, user, tokens);
    }
    return INPUT_OK;
}
```

### Extracting messages from buffer

```cpp
std::vector<std::string> Server::extractMessagesFromBuffer(User* user)
{
    std::string& buffer = user->getInputBuffer();
    std::vector<std::string> messages;
    size_t newlinePos;

    // Accept both \r\n and \n (Postel's Law)
    while ((newlinePos = buffer.find('\n')) != std::string::npos)
    {
        std::string msg = buffer.substr(0, newlinePos);
        buffer.erase(0, newlinePos + 1);

        // Handle optional \r
        if (!msg.empty() && msg[msg.size() - 1] == '\r')
            msg.erase(msg.size() - 1);

        if (msg.size() <= MAX_BUFFER_SIZE - 2)
            messages.push_back(msg);
    }
    return messages;
}
```

### Writing (output buffer)

```cpp
void Server::handleWriteReadyUsers(fd_set& writeFds)
{
    std::map<int, User*>::iterator it = _usersFd.begin();
    
    while (it != _usersFd.end())
    {
        int userFd = it->first;
        User* user = it->second;
        ++it;  // Increment before potential disconnect
        
        if (FD_ISSET(userFd, &writeFds) && !user->getOutputBuffer().empty())
        {
            std::string& outbuf = user->getOutputBuffer();
            ssize_t sent = send(userFd, outbuf.c_str(), outbuf.length(), 0);
            
            if (sent > 0)
                outbuf.erase(0, sent);
            else if (sent == -1 && (errno == EPIPE || errno == ECONNRESET))
                disconnectUser(userFd, "Write error");
        }
    }
}
```

---

## IRC Message Tokenizer

Handle the IRC protocol's trailing parameter (`:` prefix):

```cpp
std::vector<std::string> Command::tokenize(const std::string& message)
{
    std::vector<std::string> tokens;
    std::string::size_type pos = 0;

    while (pos < message.size())
    {
        while (pos < message.size() && message[pos] == ' ')
            ++pos;

        if (pos >= message.size())
            break;

        // Colon = trailing parameter (rest of line is one token)
        if (message[pos] == ':') {
            if (pos + 1 < message.size())
                tokens.push_back(message.substr(pos));
            break;
        }

        std::string::size_type end = message.find(' ', pos);
        if (end == std::string::npos)
            end = message.size();
        
        tokens.push_back(message.substr(pos, end - pos));
        pos = end + 1;
    }
    return tokens;
}
```

**Example**: `"USER max 0 * :Max Power the Third"` becomes:
`["USER", "max", "0", "*", ":Max Power the Third"]`

---

## Registration Flow

### State tracking in User

```cpp
bool _hasNick;
bool _hasUser;
bool _hasPassed;
bool _isRegistered;

void User::tryRegister()
{
    if (_isRegistered) return;
    if (!_hasPassed || !_hasNick || !_hasUser) return;
    
    _isRegistered = true;
    sendWelcome();
}
```

### Order-independent registration

Accept NICK, USER, PASS in any order. Registration completes when:
1. PASS matches (or server has no password)
2. NICK is set and unique
3. USER command received

Each handler calls `tryRegister()` at the end.

---

## User Disconnect Cleanup

Central function handles all cleanup:

```cpp
void Server::disconnectUser(int fd, const std::string& reason)
{
    User* user = getUser(fd);
    if (!user) return;

    // 1. Collect all users who need to see QUIT message
    std::set<User*> recipients;
    const std::set<std::string> channels = user->getChannels();
    
    for (std::set<std::string>::const_iterator it = channels.begin(); 
         it != channels.end(); ++it)
    {
        Channel* channel = getChannel(*it);
        if (channel) {
            const std::map<std::string, User*>& members = channel->get_members();
            for (std::map<std::string, User*>::const_iterator m = members.begin();
                 m != members.end(); ++m)
            {
                if (m->second) recipients.insert(m->second);
            }
        }
    }
    recipients.erase(user);

    // 2. Broadcast QUIT to all recipients
    for (std::set<User*>::iterator it = recipients.begin(); 
         it != recipients.end(); ++it)
        (*it)->sendMsgFromUser(user, "QUIT :" + reason);

    // 3. Remove from all channels
    for (std::set<std::string>::const_iterator it = channels.begin(); 
         it != channels.end(); ++it)
    {
        Channel* channel = getChannel(*it);
        if (channel) channel->remove_user(user);
    }

    // 4. Delete empty channels
    for (std::set<std::string>::const_iterator it = channels.begin(); 
         it != channels.end(); ++it)
    {
        Channel* channel = getChannel(*it);
        if (channel && channel->get_connected_user_number() == 0)
            deleteChannel(*it, "no connected users");
    }

    // 5. Delete user
    deleteUser(fd, "disconnected: " + reason);
}
```

---

## Cross-Platform Socket Creation

Handle Linux vs macOS differences:

```cpp
void Server::createSocket()
{
#if defined(LINUX_OS)
    // Linux: SOCK_NONBLOCK flag available
    _fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (_fd == -1)
        throw std::runtime_error("Failed to create socket");

#elif defined(MACOS_OS)
    // macOS: must use fcntl
    _fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_fd == -1)
        throw std::runtime_error("Failed to create socket");
    
    if (fcntl(_fd, F_SETFL, O_NONBLOCK) == -1) {
        close(_fd);
        throw std::runtime_error("Failed to set non-blocking");
    }
#endif
}
```

Set in Makefile:
```makefile
ifeq ($(OS),Darwin)
    CPPFLAGS += -DMACOS_OS
else ifeq ($(OS),Linux)
    CPPFLAGS += -DLINUX_OS
endif
```

---

## Helper Functions (utils.hpp)

```cpp
// Convert any type to string
template <typename T>
std::string toString(const T& value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

// Normalize strings (lowercase for case-insensitive comparison)
std::string normalize(const std::string& name);

// Validation
bool isValidNick(const std::string& nick);
bool isValidChannelName(const std::string& channelName);

// Logging
void logUserAction(const std::string& nick, int fd, const std::string& msg);
void logServerMessage(const std::string& message);
```

---

## Constants (defines.hpp)

```cpp
#define SERVER_NAME       "ircserv"
#define VERSION           "1.0"
#define NETWORK           "42 IRC"

#define MAX_BUFFER_SIZE   512   // RFC 1459
#define MAX_NICK_LENGTH   9     // RFC 1459
#define MAX_CHANNEL_LENGTH 24
#define MAX_CHANNELS      10    // Per user

#define C_MODES  "itkol"  // Supported channel modes
#define U_MODES  "-"      // No user modes implemented
```

---

## Command Implementation Pattern

Each command handler follows this pattern:

```cpp
bool Command::handleSomeCommand(Server* server, User* user, 
                                const std::vector<std::string>& tokens)
{
    // 1. Check registration
    if (!checkRegistered(user, "SOMECOMMAND"))
        return false;

    // 2. Validate parameters
    if (tokens.size() < 2) {
        user->sendError(461, "SOMECOMMAND", "Not enough parameters");
        return false;
    }

    // 3. Business logic
    // ...

    // 4. Send responses (via User methods)
    user->sendServerMsg("...");
    
    // 5. Broadcast if needed
    broadcastToChannel(channel, message, user->getNicknameLower());
    
    return true;
}
```

---

## Suggested Implementation Order

1. **Networking base**
   - Socket/bind/listen/non-blocking
   - select() loop
   - Accept users
   - Per-user buffers

2. **Line reassembly**
   - Input buffer + CRLF parsing
   - Tokenizer

3. **Registration**
   - PASS/NICK/USER
   - Welcome messages

4. **Basic channels**
   - JOIN + broadcast
   - PART
   - PRIVMSG to channel + nick
   - NOTICE

5. **Operators**
   - Channel operator tracking
   - MODE +o/-o

6. **Remaining modes**
   - +i, +t, +k, +l

7. **Operator commands**
   - KICK/INVITE/TOPIC
   - LIST

8. **Hardening**
   - Disconnect cleanup
   - Partial packet testing
   - Edge cases

---

## Testing Notes

- Use `nc -C` to send partial commands (test buffer reassembly)
- Test with real IRC client (HexChat, irssi, weechat)
- Test:
  - Disconnect while in channel
  - Nick collisions
  - MODE combinations (+ik, etc.)
  - Invalid parameters
