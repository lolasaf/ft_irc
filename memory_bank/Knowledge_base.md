Reference: https://modern.ircdocs.horse/

1. What is a Socket?
	A socket is an endpoint for communication between two machines over a network. Think of it like a phone — you need to:

		1. Get a phone (socket()) — Create the communication endpoint
		2. Get a phone number (bind()) — Associate it with an address/port
		3. Turn it on and listen (listen()) — Start waiting for incoming calls
		4. Answer calls (accept()) — Accept incoming connections

	The functions we'll use:
		socket()	Creates a new socket
		setsockopt()	Sets socket options (like allowing address reuse)
		bind()	Binds socket to an IP address and port
		listen()	Marks socket as passive (ready to accept connections)
		fcntl()	Sets socket to non-blocking mode

	Why non-blocking?
	In blocking mode, when you call recv() on a socket with no data, your program stops and waits. With multiple clients, this would freeze the server.

	In non-blocking mode, recv() returns immediately (with an error code if no data). This lets us use select() to check which sockets are ready before reading.



		sin_port — What port to listen on?
			This is the port number from your Server class (the port member variable).

			But there's a catch! Different computers store numbers differently:

			Little-endian (most CPUs): 0x1234 stored as 34 12
			Big-endian (network standard): 0x1234 stored as 12 34
			Networks always use big-endian (called "network byte order").

			htons() = Host TO Network Short

			It converts your port number from your CPU's format to network format:
			serverAddr.sin_port = htons(port);  // 'port' is your class member

			If your port is 6667:
				Without htons(): might send 0x0B1A (wrong!)
				With htons(): correctly sends 0x1A0B

2. What is poll()?
	poll() is a system call that lets you monitor multiple file descriptors at once.
	It blocks (waits) until one or more fds are "ready" for I/O.

	We use poll() instead of select() because:
		- No fd limit (select limited to ~1024)
		- No need to track maxFd
		- Cleaner separation: events (what you want) vs revents (what happened)

	int poll(struct pollfd *fds, nfds_t nfds, int timeout);

		Parameter	|	Purpose
		fds			|	Array of pollfd structs
		nfds		|	Number of elements in array
		timeout		|	Milliseconds to wait (-1 = wait forever)

3. What is pollfd?
	A pollfd struct holds info about one file descriptor to watch:

		struct pollfd {
		    int   fd;       // File descriptor to watch
		    short events;   // Events we WANT to watch (input)
		    short revents;  // Events that HAPPENED (output, filled by poll)
		};

	Event flags:
		Flag		|	Meaning					|	Use in events	|	In revents
		POLLIN		|	Ready to read			|	✓				|	✓
		POLLOUT		|	Ready to write			|	✓				|	✓
		POLLERR		|	Error occurred			|	(auto)			|	✓
		POLLHUP		|	Hang up (disconnect)	|	(auto)			|	✓
		POLLNVAL	|	Invalid fd				|	(auto)			|	✓

	How to use:
		pfd.events = POLLIN;           // Watch for read
		pfd.events |= POLLOUT;         // Also watch for write
		if (pfd.revents & POLLIN)      // Check if readable
		if (pfd.revents & POLLOUT)     // Check if writable

	┌────────────────────────────────────────────────────────────┐
	│                     Main Event Loop                         │
	├────────────────────────────────────────────────────────────┤
	│                                                            │
	│   while (running) {                                        │
	│       1. Build pollFds array (server + all clients)        │
	│       2. Call poll() — blocks until something happens      │
	│       3. Check: pollFds[0].revents & POLLIN → accept()     │
	│       4. Check: client revents → read/write/error          │
	│   }                                                        │
	│                                                            │
	└────────────────────────────────────────────────────────────┘

4. What is accept()?
	When a client connects to your listening socket, accept() creates a new socket specifically for that client:

	int clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &addrLen);

	serverFd keeps listening for MORE connections
	clientFd is used to talk to THIS specific client

	Before accept():
	┌─────────────┐         ┌─────────────┐
	│   Client    │ ──────► │  serverFd   │  (listening)
	└─────────────┘         └─────────────┘

	After accept():
	┌─────────────┐         ┌─────────────┐
	│   Client    │ ◄─────► │  clientFd   │  (connected to client)
	└─────────────┘         └─────────────┘
							┌─────────────┐
							│  serverFd   │  (still listening)
							└─────────────┘

5. What is the User class?
	The User class represents a connected client. Each time someone connects to your server, you create a User object to track everything about that connection:

	┌─────────────────────────────────────────────────────────────┐
	│                         User                                │
	├─────────────────────────────────────────────────────────────┤
	│  fd            →  Socket file descriptor (to send/recv)     │
	│  inputBuffer   →  Data received but not yet processed       │
	│  outputBuffer  →  Data waiting to be sent                   │
	│  nickname      →  IRC nickname (set by NICK command)        │
	│  username      →  IRC username (set by USER command)        │
	│  isRegistered  →  Has completed PASS/NICK/USER?             │
	│  ...                                                        │
	└─────────────────────────────────────────────────────────────┘

	Why do we need buffers?
	Input Buffer — TCP is a stream protocol. Data can arrive in chunks:
		Client sends: "NICK john\r\n"
		You might receive: "NIC"  then  "K john\r\n"
		Or even: "NICK john\r\nUSER john 0 * :John\r\n" (two commands at once!)

	The input buffer accumulates data until you see a complete line (\n).

	Output Buffer — You can't always send immediately (socket might be busy). Store messages here, send when select() says the fd is writable.

	Storage in Server
	The Server will store Users in a std::map:
		std::map<int, User*> _users;  // fd → User pointer

	Why map<int, User*>?
		Fast lookup by fd (O(log n))
		Easy iteration to add all fds to select()
		Clean ownership (Server owns the User objects)

6. Output Buffering and Write Handling

	Problems with direct send():
		1. Evaluator Trap: You can only write when poll() says the fd is writable. Writing at other times = grade 0.

		2. Partial writes: send() might not send everything! If you try to send 100 bytes, it might only send 50. You need to track what's left.

		3. Blocking risk: If the client's receive buffer is full, send() would block (or return EAGAIN in non-blocking mode).

	The solution - Output Buffer Pattern
		┌─────────────────────────────────────────────────────────────────┐
		│                    Output Buffering Flow                        │
		├─────────────────────────────────────────────────────────────────┤
		│                                                                 │
		│  Command Handler:                                               │
		│      "Send welcome message"                                     │
		│            │                                                    │
		│            ▼                                                    │
		│      outputBuffer += "001 nick :Welcome!\r\n"                   │
		│      (just append, DON'T send yet!)                             │
		│                                                                 │
		│  ─────────────────────────────────────────────────────────────  │
		│                                                                 │
		│  Main Loop (poll):                                              │
		│      1. If outputBuffer not empty → events |= POLLOUT           │
		│      2. Call poll()                                             │
		│      3. If revents & POLLOUT → NOW we can send!                 │
		│            │                                                    │
		│            ▼                                                    │
		│      bytes = send(fd, outputBuffer.c_str(), ...)                │
		│      outputBuffer.erase(0, bytes)  // Remove sent portion       │
		│                                                                 │
		└─────────────────────────────────────────────────────────────────┘

7. What is send()
	ssize_t send(int sockfd, const void *buf, size_t len, int flags);


	Parameter		|	Purpose
	sockfd			|	The client's file descriptor
	buf				|	Pointer to data to send
	len				|	Number of bytes to send
	flags			|	Usually 0

	Return values:
	Return	|	Meaning							|	Action
	> 0		|	Number of bytes actually sent	|	Erase those bytes from buffer
	0		|	Connection closed				|	Disconnect client
	-1		|	Error							|	Check errno (EAGAIN = try later)

8. What is the Message struct and parseMessage()?
	IRC messages follow this format:
		[:<prefix>] <command> [<params>] [:<trailing>]

	Examples:
		PASS secretpassword
		NICK john
		USER john 0 * :John Doe
		PRIVMSG #channel :Hello world!

	The Message struct holds parsed data:
		struct Message {
		    std::string command;              // "NICK", "USER", etc.
		    std::vector<std::string> params;  // Parameters
		};

	parseMessage() converts a raw string into a Message:
		"USER john 0 * :John Doe"
		    ↓ parseMessage()
		{ command: "USER", params: ["john", "0", "*", "John Doe"] }

	Key parsing rules:
		- Prefix (optional): starts with ':', skip it
		- Command: first word after prefix
		- Params: space-separated words
		- Trailing: everything after ':' is ONE param (can contain spaces)

9. IRC Registration Flow
	Before a client can use most commands, they must register:

		┌─────────────────────────────────────────────────────────────────┐
		│                    Registration Flow                            │
		├─────────────────────────────────────────────────────────────────┤
		│                                                                 │
		│  Client connects                                                │
		│       │                                                         │
		│       ▼                                                         │
		│  PASS <password>     → Server checks password                   │
		│       │                                                         │
		│       ▼                                                         │
		│  NICK <nickname>     → Server validates, checks uniqueness      │
		│       │                                                         │
		│       ▼                                                         │
		│  USER <user> 0 * :<realname>  → Server stores user info         │
		│       │                                                         │
		│       ▼                                                         │
		│  All three done? → Send RPL_WELCOME (001)                       │
		│       │                                                         │
		│       ▼                                                         │
		│  User is now REGISTERED — can use JOIN, PRIVMSG, etc.           │
		│                                                                 │
		└─────────────────────────────────────────────────────────────────┘

	User registration state:
		bool passOk;           // Has correct PASS been sent?
		std::string nickname;  // Set by NICK
		std::string username;  // Set by USER
		std::string realname;  // Set by USER (trailing param)
		std::string hostname;  // Set by USER (usually "*" or client IP)

		bool isRegistered() { return passOk && !nickname.empty() && !username.empty(); }

10. User Class - Complete Registration Implementation

	The User class stores all client state including registration:

		┌─────────────────────────────────────────────────────────────────┐
		│                    User Class Members                           │
		├─────────────────────────────────────────────────────────────────┤
		│  Connection:                                                    │
		│    int fd              - Socket file descriptor                 │
		│    std::string inputBuffer   - Received data (not parsed yet)   │
		│    std::string outputBuffer  - Data waiting to send             │
		│                                                                 │
		│  Registration:                                                  │
		│    bool passOk         - Has client sent correct PASS?          │
		│    std::string nickname - Set by NICK command                   │
		│    std::string username - Set by USER command                   │
		│    std::string realname - Set by USER (trailing param)          │
		│    std::string hostname - Set by USER (usually "*")             │
		│                                                                 │
		│  Methods:                                                       │
		│    isRegistered() → true if passOk && nickname && username      │
		│    setPassOk(bool) / getPassOk()                                │
		│    setNickname(str) / getNickname()                             │
		│    setUsername(str) / getUsername()                             │
		│    setRealname(str) / setHostname(str)                          │
		└─────────────────────────────────────────────────────────────────┘

	IMPORTANT: Initialize passOk to false in constructor!
		User::User(int fd) : fd(fd), passOk(false) { ... }

	Without initialization, passOk could be true/false randomly (undefined behavior).

11. IRC Message Format - Complete Reference for ft_irc

IRC MESSAGE FORMATS AND REPLIES — GROUPED BY COMMAND

---

### PASS
**Client → Server:**
	PASS <password>
	Example: PASS secret123

**Server → Client:**
	- 461 * PASS :Not enough parameters (missing password)
	- 464 * :Password incorrect (wrong password)
	- 462 <nick> :You may not reregister (already registered)

---

### NICK
**Client → Server:**
	NICK <nickname>
	Example: NICK john

**Server → Client:**
	- 431 * :No nickname given (no param)
	- 432 * <badnick> :Erroneous nickname (invalid chars)
	- 433 * <nick> :Nickname is already in use
	- 462 <nick> :You may not reregister (already registered)

---

### USER
**Client → Server:**
	USER <username> 0 * :<realname>
	Example: USER jdoe 0 * :John Doe

**Server → Client:**
	- 461 * USER :Not enough parameters
	- 462 <nick> :You may not reregister

---

### Registration Success
**Server → Client:**
	- 001 <nick> :Welcome to the IRC Network <nick>!<user>@<host>
	- 002 <nick> :Your host is ircserv, running version 1.0
	- 003 <nick> :This server was created <date>
	- 004 <nick> ircserv 1.0 o itkol

---

### JOIN
**Client → Server:**
	JOIN <#channel> [key]
	Example: JOIN #general

**Server → Client:**
	- :<nick>!<user>@<host> JOIN #channel (to all in channel)
	- 353 <nick> = #channel :@nick user1 user2 (names list)
	- 366 <nick> #channel :End of /NAMES list
	- 403 <nick> #badchan :No such channel
	- 471 <nick> #channel :Cannot join channel (+l)
	- 473 <nick> #channel :Cannot join channel (+i)
	- 475 <nick> #channel :Cannot join channel (+k)
	- 476 <nick> #bad :Bad Channel Mask

---

### PART
**Client → Server:**
	PART <#channel> [:<message>]
	Example: PART #general :Leaving now

**Server → Client:**
	- :<nick>!<user>@<host> PART #channel :<message> (to all in channel)
	- 442 <nick> #channel :You're not on that channel

---

### TOPIC
**Client → Server:**
	TOPIC <#channel> [:<new topic>]
	Example: TOPIC #general :New topic

**Server → Client:**
	- 331 <nick> #channel :No topic is set
	- 332 <nick> #channel :<topic>
	- :<nick>!<user>@<host> TOPIC #channel :<topic> (to all in channel)

---

### KICK
**Client → Server:**
	KICK <#channel> <user> [:<reason>]
	Example: KICK #general baduser :Spamming

**Server → Client:**
	- :<nick>!<user>@<host> KICK #channel <user> :<reason> (to all in channel)
	- 482 <nick> #channel :You're not channel operator

---

### INVITE
**Client → Server:**
	INVITE <nick> <#channel>
	Example: INVITE john #secret

**Server → Client:**
	- 341 <nick> <target> #channel (confirm to inviter)
	- :<nick>!<user>@<host> INVITE <target> #channel (to invitee)
	- 443 <nick> <target> #channel :is already on channel

---

### MODE
**Client → Server:**
	MODE <target> [<+/-modes>] [params]
	Example: MODE #general +o john

**Server → Client:**
	- :<nick>!<user>@<host> MODE #channel +o <user> (to all in channel)
	- 324 <nick> #channel +nt (reply to MODE query)
	- 472 <nick> x :is unknown mode char to me

---

### PRIVMSG / NOTICE
**Client → Server:**
	PRIVMSG <target> :<message>
	Example: PRIVMSG #general :Hello!

**Server → Client:**
	- :<nick>!<user>@<host> PRIVMSG <target> :<message> (to target user/channel)
	- 401 <nick> <target> :No such nick/channel
	- 404 <nick> #channel :Cannot send to channel
	- 411 <nick> :No recipient given
	- 412 <nick> :No text to send

---

### PING / PONG
**Client → Server:**
	PING <token>
	Example: PING 12345

**Server → Client:**
	- PONG ircserv :<token>

---

### QUIT
**Client → Server:**
	QUIT [:<message>]
	Example: QUIT :Client quit

**Server → Client:**
	- :<nick>!<user>@<host> QUIT :<message> (to all users sharing a channel)

---

### GENERAL ERRORS / UNKNOWN COMMANDS
	- 421 <nick> <command> :Unknown command
	- 451 * :You have not registered

---

### KEY PATTERNS TO REMEMBER
	- User prefix: :nick!user@host (e.g., :john!jdoe@127.0.0.1)
	- Server prefix: :servername (e.g., :ircserv)
	- Numeric target: Use * if user has no nickname yet
	- All lines end with \r\n (CRLF)
	- Trailing param: Anything after : in params is ONE param with spaces
	- Valid nick chars: Start with letter, then letters/digits/-_[]{}|\`, max 9 chars
	- Max message length: 512 bytes (including \r\n)

### PARSING EDGE CASES
	- PRIVMSG #chan :               → Empty trailing (valid, empty message)
	- PRIVMSG #chan ::text          → Trailing starts with colon (message is ":text")
	- NICK :john                    → Some clients send NICK with trailing
	- NICK john                     → Leading spaces (skip them)
	- NICK  john                    → Multiple spaces between (skip them)
	- nick JOHN                     → Commands are case-insensitive
		:john!jdoe@127.0.0.1 QUIT :Client quit


IRC MESSAGE FORMATS AND REPLIES — GROUPED BY COMMAND

--- FULL OVERVIEW

	Every IRC message follows this pattern:
		[:<prefix>] <command> <params> [:<trailing>]

	Where:
		- prefix (optional): Identifies message origin, starts with ':'
		- command: A word (NICK, JOIN) or 3-digit numeric (001, 433)
		- params: Up to 15 space-separated parameters
		- trailing: Starts with ':', can contain spaces, always last

	All messages end with \r\n (CRLF)

	═══════════════════════════════════════════════════════════════════
	                   CLIENT → SERVER (What You Receive)
	═══════════════════════════════════════════════════════════════════

	REGISTRATION COMMANDS:
	┌─────────────┬─────────────────────────────────────┬─────────────────────────────────┐
	│ Command     │ Format                              │ Example                         │
	├─────────────┼─────────────────────────────────────┼─────────────────────────────────┤
	│ PASS        │ PASS <password>                     │ PASS secret123                  │
	│ NICK        │ NICK <nickname>                     │ NICK john                       │
	│ USER        │ USER <user> <unused> <unused> :real │ USER jdoe 0 * :John Doe         │
	│ QUIT        │ QUIT [:<message>]                   │ QUIT :Goodbye!                  │
	└─────────────┴─────────────────────────────────────┴─────────────────────────────────┘

	USER command params explained:
		USER jdoe 0 * :John Doe
		     │    │ │  └─────── realname (trailing, can have spaces)
		     │    │ └────────── unused (historically: server name, just use *)
		     │    └──────────── unused (historically: mode mask, just use 0)
		     └─────────────────  username (no spaces allowed)

	CHANNEL COMMANDS:
	┌─────────────┬─────────────────────────────────────┬─────────────────────────────────┐
	│ Command     │ Format                              │ Example                         │
	├─────────────┼─────────────────────────────────────┼─────────────────────────────────┤
	│ JOIN        │ JOIN <#channel> [key]               │ JOIN #general                   │
	│             │                                     │ JOIN #secret mykey              │
	│ PART        │ PART <#channel> [:<message>]        │ PART #general :Leaving now      │
	│ TOPIC       │ TOPIC <#channel> [:<new topic>]     │ TOPIC #general                  │
	│             │                                     │ TOPIC #general :New topic       │
	│ KICK        │ KICK <#chan> <user> [:<reason>]     │ KICK #general baduser :Spam     │
	│ INVITE      │ INVITE <nickname> <#channel>        │ INVITE john #secret             │
	│ MODE        │ MODE <target> [<+/-modes>] [params] │ MODE #general +o john           │
	└─────────────┴─────────────────────────────────────┴─────────────────────────────────┘

	MESSAGING COMMANDS:
	┌─────────────┬─────────────────────────────────────┬─────────────────────────────────┐
	│ Command     │ Format                              │ Example                         │
	├─────────────┼─────────────────────────────────────┼─────────────────────────────────┤
	│ PRIVMSG     │ PRIVMSG <target> :<message>         │ PRIVMSG #general :Hello!        │
	│             │                                     │ PRIVMSG john :Hey there         │
	│ NOTICE      │ NOTICE <target> :<message>          │ NOTICE john :Server notice      │
	└─────────────┴─────────────────────────────────────┴─────────────────────────────────┘

	UTILITY COMMANDS:
	┌─────────────┬─────────────────────────────────────┬─────────────────────────────────┐
	│ Command     │ Format                              │ Example                         │
	├─────────────┼─────────────────────────────────────┼─────────────────────────────────┤
	│ PING        │ PING <token>                        │ PING 12345                      │
	│ PONG        │ PONG <token>                        │ PONG 12345                      │
	└─────────────┴─────────────────────────────────────┴─────────────────────────────────┘

	═══════════════════════════════════════════════════════════════════
	                   SERVER → CLIENT (What You Send)
	═══════════════════════════════════════════════════════════════════

	SERVER MESSAGE FORMAT:
		:<servername> <numeric> <target> <params> :<trailing>

	Where:
		- servername = your server's name (e.g., "ircserv")
		- numeric = 3-digit reply code
		- target = recipient's nickname (use "*" if not registered yet)

	WELCOME REPLIES (after successful registration):
		:ircserv 001 john :Welcome to the IRC Network john!jdoe@127.0.0.1
		:ircserv 002 john :Your host is ircserv, running version 1.0
		:ircserv 003 john :This server was created <date>
		:ircserv 004 john ircserv 1.0 o itkol

	REGISTRATION ERRORS:
	┌─────────┬─────────────────────────────────────────────────────────────────────────┐
	│ Numeric │ Reply                                                                   │
	├─────────┼─────────────────────────────────────────────────────────────────────────┤
	│ 431     │ :ircserv 431 * :No nickname given                                       │
	│ 432     │ :ircserv 432 * badnick :Erroneous nickname                              │
	│ 433     │ :ircserv 433 * john :Nickname is already in use                         │
	│ 461     │ :ircserv 461 * PASS :Not enough parameters                              │
	│ 462     │ :ircserv 462 john :You may not reregister                               │
	│ 464     │ :ircserv 464 * :Password incorrect                                      │
	└─────────┴─────────────────────────────────────────────────────────────────────────┘

	CHANNEL ERRORS:
	┌─────────┬─────────────────────────────────────────────────────────────────────────┐
	│ Numeric │ Reply                                                                   │
	├─────────┼─────────────────────────────────────────────────────────────────────────┤
	│ 403     │ :ircserv 403 john #badchan :No such channel                             │
	│ 442     │ :ircserv 442 john #general :You're not on that channel                  │
	│ 443     │ :ircserv 443 john target #general :is already on channel                │
	│ 471     │ :ircserv 471 john #general :Cannot join channel (+l)                    │
	│ 473     │ :ircserv 473 john #general :Cannot join channel (+i)                    │
	│ 475     │ :ircserv 475 john #general :Cannot join channel (+k)                    │
	│ 476     │ :ircserv 476 john #bad :Bad Channel Mask                                │
	│ 482     │ :ircserv 482 john #general :You're not channel operator                 │
	└─────────┴─────────────────────────────────────────────────────────────────────────┘

	MESSAGING ERRORS:
	┌─────────┬─────────────────────────────────────────────────────────────────────────┐
	│ Numeric │ Reply                                                                   │
	├─────────┼─────────────────────────────────────────────────────────────────────────┤
	│ 401     │ :ircserv 401 john badnick :No such nick/channel                         │
	│ 404     │ :ircserv 404 john #general :Cannot send to channel                      │
	│ 411     │ :ircserv 411 john :No recipient given                                   │
	│ 412     │ :ircserv 412 john :No text to send                                      │
	└─────────┴─────────────────────────────────────────────────────────────────────────┘

	GENERAL ERRORS:
	┌─────────┬─────────────────────────────────────────────────────────────────────────┐
	│ Numeric │ Reply                                                                   │
	├─────────┼─────────────────────────────────────────────────────────────────────────┤
	│ 421     │ :ircserv 421 john BADCMD :Unknown command                               │
	│ 451     │ :ircserv 451 * :You have not registered                                 │
	└─────────┴─────────────────────────────────────────────────────────────────────────┘

	CHANNEL REPLIES:

	JOIN success (sent to everyone in channel including joiner):
		:john!jdoe@127.0.0.1 JOIN #general
		:ircserv 353 john = #general :@john alice bob      (@ = operator)
		:ircserv 366 john #general :End of /NAMES list

	PART (sent to everyone in channel including leaver):
		:john!jdoe@127.0.0.1 PART #general :Goodbye

	TOPIC:
		:ircserv 331 john #general :No topic is set        (no topic exists)
		:ircserv 332 john #general :The current topic      (topic exists)
		:john!jdoe@127.0.0.1 TOPIC #general :New topic     (topic changed, to all)

	KICK (sent to everyone in channel):
		:john!jdoe@127.0.0.1 KICK #general baduser :Reason

	INVITE:
		:ircserv 341 john target #secret                   (confirm to inviter)
		:john!jdoe@127.0.0.1 INVITE target #secret         (sent to invitee)

	MODE DETAILS:
	┌──────┬────────┬───────────────────────────────┬─────────────────────────────┐
	│ Mode │ Param? │ Meaning                       │ Example                     │
	├──────┼────────┼───────────────────────────────┼─────────────────────────────┤
	│ +i   │ No     │ Invite-only channel           │ MODE #chan +i               │
	│ -i   │ No     │ Remove invite-only            │ MODE #chan -i               │
	│ +t   │ No     │ Topic restricted to ops       │ MODE #chan +t               │
	│ +k   │ Yes    │ Set channel key (password)    │ MODE #chan +k secretkey     │
	│ -k   │ Yes    │ Remove key                    │ MODE #chan -k *             │
	│ +o   │ Yes    │ Give operator status          │ MODE #chan +o john          │
	│ -o   │ Yes    │ Remove operator status        │ MODE #chan -o john          │
	│ +l   │ Yes    │ Set user limit                │ MODE #chan +l 10            │
	│ -l   │ No     │ Remove user limit             │ MODE #chan -l               │
	└──────┴────────┴───────────────────────────────┴─────────────────────────────┘

	MODE replies:
		:john!jdoe@127.0.0.1 MODE #general +o alice        (broadcast to channel)
		:ircserv 324 john #general +nt                     (reply to MODE query)
		:ircserv 472 john x :is unknown mode char to me    (unknown mode)

	PRIVMSG RELAY:

	Channel message (john sends "PRIVMSG #general :Hello"):
		Everyone else in #general receives:
		:john!jdoe@127.0.0.1 PRIVMSG #general :Hello

	Private message (john sends "PRIVMSG alice :Hey"):
		Alice receives:
		:john!jdoe@127.0.0.1 PRIVMSG alice :Hey

	PING/PONG:
		Client sends: PING 12345
		Server responds: PONG ircserv :12345

	QUIT BROADCAST (to all users who share a channel with quitter):
		:john!jdoe@127.0.0.1 QUIT :Client quit

	═══════════════════════════════════════════════════════════════════
	                   KEY PATTERNS TO REMEMBER
	═══════════════════════════════════════════════════════════════════

	1. User prefix format:      :nick!user@host
	   Example:                 :john!jdoe@127.0.0.1

	2. Server prefix format:    :servername
	   Example:                 :ircserv

	3. Numeric target:          Use * if user has no nickname yet
	   Example:                 :ircserv 464 * :Password incorrect

	4. Line endings:            All lines end with \r\n (CRLF)

	5. Trailing parameter:      Anything after : in params is ONE param with spaces

	6. Valid nickname chars:    Start with letter, then letters/digits/-_[]{}|\`
	   Max length:              9 characters

	7. Max message length:      512 bytes (including \r\n)

	═══════════════════════════════════════════════════════════════════
	                   PARSING EDGE CASES TO HANDLE
	═══════════════════════════════════════════════════════════════════

	PRIVMSG #chan :               → Empty trailing (valid, empty message)
	PRIVMSG #chan ::text          → Trailing starts with colon (message is ":text")
	NICK :john                    → Some clients send NICK with trailing
	   NICK john                  → Leading spaces (skip them)
	NICK  john                    → Multiple spaces between (skip them)
	nick JOHN                     → Commands are case-insensitive

12. sendNumeric() Helper Function

	The sendNumeric() function formats and queues IRC numeric replies consistently:

	void sendNumeric(User* user, ReplyCode code, 
	                 const std::vector<std::string>& params,
	                 const std::string& trailing);

	Format produced:
		:<servername> <3-digit-code> <nick|*> [params...] :<trailing>\r\n

	Examples:
		sendNumeric(user, RPL_WELCOME, {}, "Welcome to ft_irc!")
		→ :SugarDaddyFinderIRC 001 john :Welcome to ft_irc!\r\n

		sendNumeric(user, ERR_NEEDMOREPARAMS, {"PASS"}, "Not enough parameters")
		→ :SugarDaddyFinderIRC 461 * PASS :Not enough parameters\r\n

		sendNumeric(user, ERR_NICKNAMEINUSE, {"john"}, "Nickname is already in use")
		→ :SugarDaddyFinderIRC 433 * john :Nickname is already in use\r\n

	Key features:
		- Uses std::setw(3) and std::setfill('0') for 3-digit codes
		- Uses "*" if user has no nickname yet
		- Queues to outputBuffer (never sends directly)

13. ReplyCode Enum (replies.hpp)

	Centralized enum for all IRC numeric reply codes:

	Registration:
		RPL_WELCOME      = 1     // 001 - Welcome after successful registration
		RPL_YOURHOST     = 2     // 002 - Your host information
		RPL_CREATED      = 3     // 003 - Server creation date
		RPL_MYINFO       = 4     // 004 - Server version info
		RPL_ISUPPORT     = 5     // 005 - Server supported features

	Registration Errors:
		ERR_NOTREGISTERED    = 451  // You have not registered
		ERR_NEEDMOREPARAMS   = 461  // Not enough parameters
		ERR_ALREADYREGISTRED = 462  // You may not reregister
		ERR_PASSWDMISMATCH   = 464  // Password incorrect

	NICK Errors:
		ERR_NONICKNAMEGIVEN  = 431  // No nickname given
		ERR_ERRONEUSNICKNAME = 432  // Erroneous nickname
		ERR_NICKNAMEINUSE    = 433  // Nickname already in use

	Channel Errors:
		ERR_NOSUCHCHANNEL    = 403  // No such channel
		ERR_CHANNELISFULL    = 471  // Cannot join (+l limit)
		ERR_INVITEONLYCHAN   = 473  // Cannot join (+i invite only)
		ERR_BADCHANNELKEY    = 475  // Cannot join (+k key wrong)

14. ISUPPORT (005 Numeric)

	RPL_ISUPPORT tells clients what features the server supports:

	Format:
		:servername 005 nick TOKEN1 TOKEN2=value TOKEN3 :are supported by this server

	Our tokens:
		USERLEN=18   - Maximum username length
		NICKLEN=9    - Maximum nickname length  
		REALLEN=50   - Maximum realname length

	Implementation:
		std::vector<std::string> isSupported;  // Stored in Server class
		
		void initISupport() {
		    isSupported.push_back("USERLEN=18");
		    isSupported.push_back("NICKLEN=9");
		    isSupported.push_back("REALLEN=50");
		}

	Sent during registration after RPL_MYINFO (004).

15. Channel Class

	The Channel class represents an IRC channel and manages members, operators, modes, and topics:

		┌─────────────────────────────────────────────────────────────────┐
		│                      Channel Class                              │
		├─────────────────────────────────────────────────────────────────┤
		│  Identity:                                                      │
		│    _name            - Channel name (stored in lowercase)        │
		│    _topic           - Current topic                             │
		│    _topic_setter    - Nickname who set the topic                │
		│    _topic_set_at    - Unix timestamp when topic was set         │
		│                                                                 │
		│  Members:                                                       │
		│    _members         - std::set<User*> of all members            │
		│    _operators       - std::set<User*> of operators              │
		│    _invitation_list - std::set<std::string> for +i mode         │
		│                                                                 │
		│  Modes:                                                         │
		│    _invite_only     - bool (+i)                                 │
		│    _topic_protection- bool (+t)                                 │
		│    _key             - std::string (+k)                          │
		│    _user_limit      - size_t (+l, 0 = no limit)                 │
		│                                                                 │
		│  Key Methods:                                                   │
		│    canJoin(user, key) → JoinResult (OK, INVITE_ONLY, BADKEY, FULL)│
		│    addMember(user) / removeMember(user)                         │
		│    addOperator(user) / removeOperator(user)                     │
		│    isMember(user) / isOperator(user)                            │
		│    getNamesList() → "@op1 user1 user2" (for RPL_NAMREPLY)       │
		│    broadcast(msg, exclude) → send to all members                │
		└─────────────────────────────────────────────────────────────────┘

	Channel Name Storage:
		- All channel names stored in LOWERCASE
		- Map key in Server: lowercase name
		- Channel._name: lowercase name
		- This enables O(log n) case-insensitive lookups

	JoinResult enum:
		JOIN_OK         - User can join
		JOIN_INVITE_ONLY - Channel is +i and user not invited
		JOIN_BADKEY     - Channel is +k and wrong/missing key
		JOIN_FULL       - Channel is +l and at user limit

	canJoin() check order (important!):
		1. Check user limit (+l) FIRST - applies to everyone
		2. Check invite-only (+i) - but invited users pass
		3. Check key (+k) - must match if set

16. Server Channel Management

	Server class manages channels with these helper methods:

		findChannel(name)   → Returns Channel* or NULL (lowercase lookup)
		createChannel(name) → Creates new channel, stores lowercase
		deleteChannel(name) → Removes channel from map, deletes object

	Channel storage:
		std::map<std::string, Channel*> channels;  // lowercase keys

	Example lookup flow:
		1. User sends: JOIN #General
		2. toLower("#General") → "#general"
		3. channels.find("#general") → O(log n) lookup
		4. Found? Join existing. Not found? Create new.

17. User-Channel Bidirectional Relationship

	Users and Channels track each other for proper cleanup:

		User side:
			std::set<Channel*> channels;  // Channels user is in
			addChannel(chan) / removeChannel(chan) / getChannels()

		Channel side:
			std::set<User*> _members;     // Users in channel
			addMember(user) / removeMember(user) / isMember(user)

	Why bidirectional?
		- When user disconnects: need to remove from all channels
		- When channel broadcasts: need to know all members
		- Prevents dangling pointers

	Cleanup pattern (handleDisconnect):
		1. Get copy of user's channels (iterator safety)
		2. For each channel:
		   a. Broadcast QUIT message (exclude disconnecting user)
		   b. chan->removeMember(user)
		   c. user->removeChannel(chan)
		   d. If channel empty, delete it
		3. Delete user object

18. Utility Functions (utils.hpp/utils.cpp)

	String manipulation:
		toUpper(str)    → Convert to uppercase
		toLower(str)    → Convert to lowercase
		trim(str)       → Remove leading/trailing whitespace

	Parsing:
		splitCommaList(str) → Split "a,b,c" into vector ["a","b","c"]
		split(str, delim)   → Split by any delimiter

	Comparison:
		caseInsensitiveCompare(a, b) → true if equal (case-insensitive)
		caseInsensitiveLess(a, b)    → For std::map ordering

19. buildHostmask() Helper

	Builds IRC hostmask format for message prefixes:

		std::string buildHostmask(User* user);
		// Returns: "nick!username@hostname"
		// Example: "john!jdoe@127.0.0.1"

	Used in:
		- JOIN broadcasts: :john!jdoe@127.0.0.1 JOIN #channel
		- PART broadcasts: :john!jdoe@127.0.0.1 PART #channel :message
		- QUIT broadcasts: :john!jdoe@127.0.0.1 QUIT :reason
		- PRIVMSG relays:  :john!jdoe@127.0.0.1 PRIVMSG #channel :text

20. handleDisconnect() Cleanup Function

	Centralized cleanup when a user disconnects:

		void Server::handleDisconnect(User* user, const std::string& reason);

	Steps:
		1. Get copy of user's channels (std::set<Channel*>)
		2. For each channel:
		   - Broadcast QUIT message to other members
		   - Remove user from channel
		   - Remove channel from user
		   - Delete channel if empty
		3. (Caller handles closing fd and deleting User object)

	Called from:
		- POLLERR/POLLHUP/POLLNVAL detection
		- recv() returns 0 (graceful disconnect)
		- recv() returns -1 (non-EAGAIN error)
		- send() returns -1 (non-EAGAIN error)
		- Server destructor

21. handlePrivmsg() Implementation

	PRIVMSG sends messages to channels or users:

		PRIVMSG <target>[,<target2>,...] :<message>

	Implementation flow:

		┌─────────────────────────────────────────────────────────────────┐
		│                    handlePrivmsg() Flow                         │
		├─────────────────────────────────────────────────────────────────┤
		│                                                                 │
		│  1. Registration check                                          │
		│     └─ If not registered → ERR_NOTREGISTERED (451)              │
		│                                                                 │
		│  2. Parameter validation                                        │
		│     └─ No target → ERR_NORECIPIENT (411)                        │
		│     └─ No message → ERR_NOTEXTTOSEND (412)                      │
		│                                                                 │
		│  3. Split targets (comma-separated)                             │
		│     └─ splitCommaList(msg.params[0])                            │
		│                                                                 │
		│  4. Build hostmask                                              │
		│     └─ ":nick!user@host"                                        │
		│                                                                 │
		│  5. For each target:                                            │
		│     ├─ If starts with '#' → Channel message                     │
		│     │   ├─ findChannel() → ERR_NOSUCHCHANNEL (403)              │
		│     │   ├─ isMember() → ERR_CANNOTSENDTOCHAN (404)              │
		│     │   └─ broadcastToChannel(chan, msg, sender)                │
		│     │                                                           │
		│     └─ Else → Private message                                   │
		│         ├─ Find user (case-insensitive loop)                    │
		│         ├─ Not found → ERR_NOSUCHNICK (401)                     │
		│         └─ targetUser->getOutputBuffer() += msg                 │
		│                                                                 │
		└─────────────────────────────────────────────────────────────────┘

	Message format sent:
		:sender!user@host PRIVMSG <target> :<message>\r\n

	Channel vs User detection:
		- Channel names start with '#'
		- Everything else is treated as nickname

	Error handling with multiple targets:
		- Use `continue` on errors (don't abort entire command)
		- Process remaining valid targets

22. handleMessageCommand() - Shared PRIVMSG/NOTICE Handler

	Refactored to avoid code duplication between PRIVMSG and NOTICE:

		void handleMessageCommand(User* user, const Message& msg, const std::string& command);

	The `command` parameter ("PRIVMSG" or "NOTICE") is used in:
		- Error messages: `sendNumeric(user, ERR_NORECIPIENT, {command}, ...)`
		- Output format: `":" + hostmask + " " + command + " " + target + " :" + message`

	Thin wrappers:
		void handlePrivmsg(User* user, const Message& msg) {
		    handleMessageCommand(user, msg, "PRIVMSG");
		}

		void handleNotice(User* user, const Message& msg) {
		    handleMessageCommand(user, msg, "NOTICE");
		}

	PRIVMSG vs NOTICE difference:
		- PRIVMSG: Clients/bots MAY auto-reply
		- NOTICE: Clients/bots must NEVER auto-reply (prevents loops)
		- Server implementation is identical for both

23. handleQuit() - Graceful Disconnect

	When a user sends QUIT, we need to:
	1. Broadcast QUIT message to all users sharing channels
	2. Clean up channels
	3. Close connection

	Why the "flag approach"?
		When handleQuit() is called, we're inside:
		poll loop → handleClientData() → processMessage() → handleQuit()

		If we delete the user immediately, we could:
		- Invalidate iterators
		- Leave dangling pointers
		- Crash when code after processMessage() tries to use deleted user

		Solution: Set a flag, let the poll loop do cleanup AFTER returning.

	Implementation:

		void Server::handleQuit(User* user, const Message& msg)
		{
		    // 1. Extract reason (default: "Client Quit")
		    std::string reason = "Client Quit";
		    if (msg.params.size() > 0)
		        reason = msg.params[0];

		    // 2. Build QUIT message
		    std::string quitMsg = ":" + buildHostmask(user) + " QUIT :" + reason + "\r\n";

		    // 3. Track who we've notified (user might be in multiple channels)
		    std::set<User*> notified;

		    // 4. For each channel user is in, broadcast to members
		    for (map iterator over channels)
		    {
		        if (!chan->isMember(user))
		            continue;

		        const std::set<User*>& members = chan->getMembers();
		        for (each member)
		        {
		            if (member == user) continue;        // Don't send to self
		            if (notified.count(member)) continue; // Already sent

		            member->getOutputBuffer() += quitMsg;
		            notified.insert(member);
		        }
		    }

		    // 5. Mark for cleanup (poll loop handles the rest)
		    user->markForDisconnection(true);
		}

	Poll loop cleanup (in handleClientData, after processMessage):

		if (user->isMarkedForDisconnection()) {
		    handleDisconnect(user, "Client Quit");  // Channel cleanup
		    close(clientFd);
		    delete user;
		    users.erase(it);
		    return;  // Exit handleClientData entirely
		}

	Why use std::set<User*> for tracking?
		- User might be in channels #a, #b, #c
		- User X might also be in #a and #c
		- Without tracking, X would receive QUIT twice
		- std::set guarantees each user notified exactly once

	User class additions:
		private:
		    bool markedForDisconnection;

		public:
		    bool isMarkedForDisconnection() const;
		    void markForDisconnection(bool mark);

	Channel class addition:
		const std::set<User*>& getMembers() const { return _members; }

24. handleMode() - Channel Mode Command

	The MODE command is the most complex IRC command. It handles:
	- Querying current modes: MODE #channel
	- Setting modes: MODE #channel +itl 10
	- Multiple modes at once: MODE #channel +o-o john bob

	Supported modes for ft_irc:
		Mode | Meaning              | Argument?
		-----|----------------------|----------
		+i   | Invite-only          | No
		+t   | Topic protection     | No (ops only can set topic)
		+k   | Channel key          | Yes (key when setting)
		+l   | User limit           | Yes (number when setting)
		+o   | Operator status      | Yes (nickname)

	Implementation split into helper functions:

		// Query current modes
		void Server::sendChannelModes(User* user, Channel* chan);

		// Apply a single mode character
		bool Server::applySingleMode(User* user, Channel* chan, char mode, 
		                             bool adding, const Message& msg, size_t& argIndex);

		// Parse mode string and apply all modes
		bool Server::applyChannelModes(User* user, Channel* chan, 
		                               const Message& msg, size_t& argIndex);

		// Main handler
		void Server::handleMode(User* user, const Message& msg);

	Mode parsing algorithm:
		Input: MODE #chan +otk-l john secret
		       ├── target: #chan
		       ├── modestring: +otk-l
		       └── args: [john, secret]

		Parse modestring char by char:
		  '+' → sign = ADD
		  'o' → needs arg → consume "john" → +o john
		  't' → no arg → +t
		  'k' → needs arg → consume "secret" → +k secret
		  '-' → sign = REMOVE
		  'l' → removing, no arg needed → -l

	Error codes:
		- 324 RPL_CHANNELMODEIS — mode query response
		- 403 ERR_NOSUCHCHANNEL — channel doesn't exist
		- 441 ERR_USERNOTINCHANNEL — target user not in channel (+o)
		- 472 ERR_UNKNOWNMODE — unknown mode character
		- 482 ERR_CHANOPRIVSNEEDED — not operator

25. findUserByNick() - Utility Function

	Reusable utility to find a user by their nickname:

		User* Server::findUserByNick(const std::string& nick)
		{
		    for (std::map<int, User*>::iterator it = users.begin(); 
		         it != users.end(); ++it)
		    {
		        if (it->second->getNickname() == nick)
		            return it->second;
		    }
		    return NULL;
		}

	Used by: MODE +o/-o, KICK, INVITE, PRIVMSG (user targets)
	Located in: serverUtils.cpp

26. sendTopicInfo() - Utility Function

	Sends topic information to a user (used by JOIN and TOPIC query):

		void Server::sendTopicInfo(User* user, Channel* chan)
		{
		    std::string topic = chan->getTopic();
		    if (topic.empty()) {
		        sendNumeric(user, RPL_NOTOPIC, {chan->getName()}, "No topic is set");
		        return;
		    }
		    sendNumeric(user, RPL_TOPIC, {chan->getName()}, topic);
		    // Also send RPL_TOPICWHOTIME (333)
		    std::vector<std::string> params;
		    params.push_back(chan->getName());
		    params.push_back(chan->getTopicSetter());
		    params.push_back(std::to_string(chan->getTopicSetAt()));
		    sendNumeric(user, RPL_TOPICWHOTIME, params, "");
		}

	Used by:
		- joinChannel() — sends topic info after JOIN
		- handleTopic() — when querying topic (no new topic provided)

	Replies sent:
		- RPL_NOTOPIC (331) — if channel has no topic
		- RPL_TOPIC (332) — current topic text
		- RPL_TOPICWHOTIME (333) — who set it and when (Unix timestamp)

	Located in: serverUtils.cpp

27. handleTopic() - Topic Command

	TOPIC command allows users to view or set channel topics:

		TOPIC #channel          — Query current topic
		TOPIC #channel :text    — Set new topic

	Implementation flow:

		┌─────────────────────────────────────────────────────────────────┐
		│                    handleTopic() Flow                           │
		├─────────────────────────────────────────────────────────────────┤
		│  1. Registration check → ERR_NOTREGISTERED (451)                │
		│  2. Parameter check → ERR_NEEDMOREPARAMS (461)                  │
		│  3. Find channel → ERR_NOSUCHCHANNEL (403)                      │
		│  4. Member check → ERR_NOTONCHANNEL (442)                       │
		│  5. If query (no new topic):                                    │
		│     └─ sendTopicInfo(user, chan) and return                     │
		│  6. If setting topic:                                           │
		│     └─ If +t mode AND not op → ERR_CHANOPRIVSNEEDED (482)       │
		│  7. Set topic, setter, timestamp                                │
		│  8. Broadcast: :nick!user@host TOPIC #chan :new topic           │
		└─────────────────────────────────────────────────────────────────┘

	+t mode behavior:
		- Anyone can VIEW the topic (query)
		- Only operators can SET the topic when +t is enabled
		- If +t is NOT set, any channel member can set the topic

	Channel methods used:
		- getTopic() / setTopic(text)
		- getTopicSetter() / setTopicSetter(nick)
		- getTopicSetAt() / setTopicSetAt(timestamp)
		- isTopicProtected() — returns true if +t mode is set

	Located in: serverCommands.cpp

28. handleInvite() - Invite Command

	INVITE allows a channel operator to invite a user to a channel:

		INVITE <nickname> <#channel>

	This is especially important for +i (invite-only) channels — invited
	users can bypass the invite-only restriction.

	Implementation flow:

		┌─────────────────────────────────────────────────────────────────┐
		│                    handleInvite() Flow                          │
		├─────────────────────────────────────────────────────────────────┤
		│  1. Registration check → ERR_NOTREGISTERED (451)                │
		│  2. Params check (need 2) → ERR_NEEDMOREPARAMS (461)            │
		│  3. Find target user → ERR_NOSUCHNICK (401)                     │
		│  4. Find channel → ERR_NOSUCHCHANNEL (403)                      │
		│  5. Inviter on channel? → ERR_NOTONCHANNEL (442)                │
		│  6. Inviter is operator? → ERR_CHANOPRIVSNEEDED (482)           │
		│  7. Target already on channel? → ERR_USERONCHANNEL (443)        │
		│  8. Add target to channel's invite list (addInvite)             │
		│  9. Send RPL_INVITING (341) to inviter                          │
		│ 10. Send INVITE notification to target                          │
		└─────────────────────────────────────────────────────────────────┘

	Replies sent:
		- RPL_INVITING (341): :server 341 <inviter> <target> <#channel>
		- INVITE to target: :inviter!user@host INVITE <target> <#channel>

	Channel methods used:
		- isMember(user) — check if inviter is on channel
		- isOperator(user) — check if inviter has op status
		- addInvite(nickname) — store nickname in invitation list

	The invitation list is checked in canJoin() when +i mode is set.
	After successful join, the invite is removed from the list.

	Located in: serverCommands.cpp

29. handleKick() - Kick Command

	KICK allows a channel operator to forcibly remove a user from a channel:

		KICK <#channel> <nickname> [:<reason>]

	The reason is optional — if omitted, defaults to the kicker's nickname.

	Implementation flow (10 steps):

		┌─────────────────────────────────────────────────────────────────┐
		│                     handleKick() Flow                           │
		├─────────────────────────────────────────────────────────────────┤
		│  1. Registration check → ERR_NOTREGISTERED (451)                │
		│  2. Params check (need 2+) → ERR_NEEDMOREPARAMS (461)           │
		│  3. Find channel → ERR_NOSUCHCHANNEL (403)                      │
		│  4. Kicker on channel? → ERR_NOTONCHANNEL (442)                 │
		│  5. Kicker is operator? → ERR_CHANOPRIVSNEEDED (482)            │
		│  6. Find target user → ERR_NOSUCHNICK (401)                     │
		│  7. Target on channel? → ERR_USERNOTINCHANNEL (441)             │
		│  8. Broadcast KICK to all channel members                       │
		│  9. Remove target from channel (bidirectional cleanup)          │
		│ 10. Delete channel if now empty                                 │
		└─────────────────────────────────────────────────────────────────┘

	KICK message format (broadcast to all members including target):
		:kicker!user@host KICK <#channel> <target> :<reason>

	Key implementation details:
		- Check kicker's op status BEFORE looking up target
		- Broadcast BEFORE removing target (so target receives message)
		- Use leaveChannel() for bidirectional cleanup
		- Delete empty channel after kick

	Channel methods used:
		- findChannel() — find channel by name
		- isMember(user) — check membership
		- isOperator(user) — check op status
		- leaveChannel(user) — remove target + cleanup

	Located in: serverCommands.cpp
30. Helper Functions (require* pattern)

	To reduce code duplication across command handlers, we extracted common
	validation patterns into reusable helper functions:

	┌─────────────────────────────────────────────────────────────────┐
	│                    Helper Function Pattern                      │
	├─────────────────────────────────────────────────────────────────┤
	│  bool requireX(User* user, ...) {                               │
	│      if (condition_not_met) {                                   │
	│          sendNumeric(user, ERR_CODE, ...);                      │
	│          return false;                                          │
	│      }                                                          │
	│      return true;  // or return found object                    │
	│  }                                                              │
	│                                                                 │
	│  Usage in handlers:                                             │
	│      if (!requireRegistered(user)) return;                      │
	│      if (!requireParams(user, msg, 2, "CMD")) return;           │
	│      Channel* chan = requireChannel(user, name);                │
	│      if (!chan) return;                                         │
	└─────────────────────────────────────────────────────────────────┘

	Available helpers (serverUtils.cpp):

	1. requireRegistered(user)
	   - Checks user->getIsRegistered()
	   - Sends ERR_NOTREGISTERED (451) if false
	   - Returns: bool

	2. requireParams(user, msg, count, cmdName)
	   - Checks msg.params.size() >= count
	   - Sends ERR_NEEDMOREPARAMS (461) if insufficient
	   - Returns: bool

	3. requireChannel(user, channelName)
	   - Calls findChannel() for lookup
	   - Sends ERR_NOSUCHCHANNEL (403) if not found
	   - Returns: Channel* (or NULL)

	4. requireOnChannel(user, channel)
	   - Checks channel->isMember(user)
	   - Sends ERR_NOTONCHANNEL (442) if not member
	   - Returns: bool

	5. requireOperator(user, channel)
	   - Checks channel->isOperator(user)
	   - Sends ERR_CHANOPRIVSNEEDED (482) if not op
	   - Returns: bool

	6. requireUser(user, nickname)
	   - Calls findUserByNick() for lookup
	   - Sends ERR_NOSUCHNICK (401) if not found
	   - Returns: User* (or NULL)

	Benefits:
	- ~66 lines of code saved across command handlers
	- Consistent error messages
	- Single point of change for validation logic
	- Cleaner, more readable command handlers

31. QUIT Handling (Two-Phase Cleanup)

	QUIT requires careful handling to avoid double-broadcasts and ensure
	the client's quit reason is preserved.

	Problem: When user sends QUIT:
	1. handleQuit() broadcasts QUIT to peers
	2. User marked for disconnection
	3. Poll loop calls handleDisconnect()
	4. handleDisconnect() would broadcast QUIT again!

	Solution: Track broadcast state on User object:

	┌─────────────────────────────────────────────────────────────────┐
	│                  User QUIT Tracking Fields                      │
	├─────────────────────────────────────────────────────────────────┤
	│  bool        markedForDisconnection;  // Should disconnect      │
	│  bool        quitBroadcast;           // QUIT already sent?     │
	│  std::string quitReason;              // "Goodbye" etc.         │
	└─────────────────────────────────────────────────────────────────┘

	markForDisconnection(bool mark, string reason, bool broadcast):
	- Sets all three fields at once
	- Called by handleQuit(): markForDisconnection(true, reason, true)
	- Default for unexpected disconnect: markForDisconnection(true, "...", false)

	handleDisconnect() behavior:
	- Checks wasQuitBroadcast() first
	- If true: skips broadcast, only cleans up channel memberships
	- If false: broadcasts QUIT with stored/provided reason
	- Uses std::set<User*> notified to deduplicate recipients

	QUIT Flow:

	┌──────────────────────────────────────────────────────────────────┐
	│   User sends QUIT         │  Connection error/close              │
	├───────────────────────────┼──────────────────────────────────────┤
	│  handleQuit():            │  (No QUIT message)                   │
	│   - Build QUIT msg        │                                      │
	│   - Broadcast to peers    │                                      │
	│   - markFor...(true,      │  markFor...(true,                    │
	│       reason, TRUE)       │      "Connection closed", FALSE)     │
	├───────────────────────────┴──────────────────────────────────────┤
	│                        Poll loop detects flag                    │
	├──────────────────────────────────────────────────────────────────┤
	│                      handleDisconnect():                         │
	│   - wasQuitBroadcast()?                                          │
	│     - YES: Skip broadcast (already done)                         │
	│     - NO:  Broadcast QUIT now                                    │
	│   - Remove from all channels (always)                            │
	│   - Delete empty channels (always)                               │
	└──────────────────────────────────────────────────────────────────┘

32. findUserByNick() Case Sensitivity Fix

	IRC nicknames are case-insensitive. "Alice", "alice", and "ALICE" all
	refer to the same user. This must be consistent everywhere.

	Where case-insensitivity is used:
	- NICK uniqueness check (can't register "alice" if "Alice" exists)
	- PRIVMSG target lookup (sending to "alice" finds "Alice")
	- findUserByNick() - used by INVITE, KICK, MODE +o/-o

	Bug found: findUserByNick() used == comparison (case-sensitive):
	
	    if (it->second->getNickname() == nick)  // BUG!

	Fixed: Now uses caseInsensitiveCompare():

	    if (caseInsensitiveCompare(it->second->getNickname(), nick))

	The caseInsensitiveCompare() function (utils.cpp):
	- Converts both strings to lowercase
	- Compares the lowercase versions
	- Returns true if equal (ignoring case)

	This ensures INVITE/KICK/MODE +o work regardless of nick casing.

33. MODE +l Limit Validation Security Fix

	Bug: The +l (user limit) mode used atoi() without validation:

	    size_t limit = static_cast<size_t>(std::atoi(arg.c_str()));

	Problems with atoi():
	- Non-numeric input → returns 0 (channel allows 0 users = no one can join)
	- Negative input → returns negative → cast to size_t → huge number
	- No error indication

	Fix: Validate before parsing:

	    // 1. Check all characters are digits
	    for (size_t i = 0; i < arg.size(); ++i) {
	        if (!std::isdigit(arg[i])) {
	            // ERR_INVALIDMODEPARAM
	            return;
	        }
	    }
	    
	    // 2. Parse and check range
	    long limit = std::atol(arg.c_str());
	    if (limit < 1 || limit > 10000) {
	        // ERR_INVALIDMODEPARAM  
	        return;
	    }

	Validation rules:
	- Must be digits only (no letters, no minus sign)
	- Must be in range 1-10000 (reasonable for IRC channel)
	- Empty string rejected (no digits)

34. MODE Query Security Fix

	Bug: Anyone could query MODE to see channel modes, including the key:

	    MODE #secret  → :server 324 user #secret +k secretpassword

	Non-members could discover channel keys by querying MODE!

	Fix: Require channel membership for mode queries:

	    // Mode query (no mode string provided)
	    if (msg.params.size() == 1) {
	        // Check membership first
	        if (!chan->isMember(user)) {
	            sendNumeric(user, ERR_NOTONCHANNEL, ...);
	            return;
	        }
	        sendChannelModes(user, chan);
	        return;
	    }

	Now non-members get ERR_NOTONCHANNEL (442) when querying modes.
	Mode CHANGES still require operator status (checked later).

35. MODE Empty Target Guard

	Bug: handleMode() accessed target[0] without checking if target is empty:

	    std::string target = msg.params[0];
	    if (target[0] != '#')  // UB if target is empty!

	How to trigger:
	- Send "MODE :" (colon with no following text)
	- Parser produces msg.params[0] = "" (empty string)
	- Accessing [0] on empty string = undefined behavior

	Fix: Check empty() before accessing [0]:

	    // 3. Guard against empty target (e.g., "MODE :" produces empty param)
	    if (target.empty())
	    {
	        sendNumeric(user, ERR_NEEDMOREPARAMS, 
	            std::vector<std::string>(1, "MODE"), "Not enough parameters");
	        return;
	    }
	    
	    // 4. Now safe to check target[0]
	    if (target[0] != '#')
	    {
	        // User mode — ignore for ft_irc
	        return;
	    }

	Lesson: Always validate string is non-empty before indexing into it.
	This pattern applies anywhere you access str[0] or str.at(0).

36. const_iterator for Const Container References

	Bug: Iterating over a const reference with non-const iterator:

	    const std::set<User*>& members = chan->getMembers();
	    for (std::set<User*>::iterator mit = members.begin(); ...)  // Won't compile!

	The compiler may allow this in some cases but it's incorrect.
	Const containers require const_iterator.

	Fix:

	    const std::set<User*>& members = chan->getMembers();
	    for (std::set<User*>::const_iterator mit = members.begin(); mit != members.end(); ++mit)

	Rule: When iterating over a `const T&`, always use `T::const_iterator`.

37. Channel Key Masking (Defense-in-Depth)

	Even though MODE queries now require membership, the key should still
	be masked in responses. This is standard IRC behavior and provides
	defense-in-depth.

	Before:
	    :server 324 user #chan +k actualpassword

	After:
	    :server 324 user #chan +k *

	Implementation in sendChannelModes():

	    if (!chan->getKey().empty())
	    {
	        modeStr += "k";
	        modeArgs += " *";  // Mask, don't expose actual key
	    }

	Users who need the key should remember it from when they joined or set it.

38. Separate Error Codes for "No Such User" vs "Not On Channel"

	MODE +o/-o needs to distinguish between:
	- User doesn't exist at all → ERR_NOSUCHNICK (401)
	- User exists but isn't on channel → ERR_USERNOTINCHANNEL (441)

	Wrong (combined check):

	    if (!targetUser || !chan->isMember(targetUser))
	        sendNumeric(user, ERR_USERNOTINCHANNEL, ...);

	Correct (separate checks):

	    if (!targetUser)
	    {
	        sendNumeric(user, ERR_NOSUCHNICK, ...);
	        return false;
	    }
	    if (!chan->isMember(targetUser))
	    {
	        sendNumeric(user, ERR_USERNOTINCHANNEL, ...);
	        return false;
	    }

	This provides accurate error feedback to the operator.

39. MODE Partial Application and Broadcast Consistency

	Problem: When processing `MODE #ch +itl abc` (invalid limit):
	- +i applied to channel state ✓
	- +t applied to channel state ✓
	- +l validation fails → return false → no broadcast!
	
	Result: Channel has +it but clients never see MODE message.

	Solution: Track successful modes separately, broadcast what worked:

	    void applyChannelModes(User* user, Channel* chan, const Message& msg,
	                           std::string& appliedModes, 
	                           std::vector<std::string>& appliedArgs)
	    {
	        for each mode:
	            if (applySingleMode(...))  // Returns true on success
	            {
	                // Add to appliedModes string
	                // Add consumed args to appliedArgs
	            }
	            // On failure: error sent, but continue processing
	    }

	    // In handleMode():
	    if (!appliedModes.empty())
	    {
	        broadcast(":user MODE #ch " + appliedModes + args);
	    }

	Now `MODE #ch +itl abc` broadcasts `MODE #ch +it` for the successful modes,
	and sends error for +l. State always matches what clients see.

40. Server Destructor File Descriptor Leak

	Bug: Server destructor deleted Users without closing their socket fds:

	    Server::~Server()
	    {
	        for (...)
	        {
	            handleDisconnect(it->second, "Server shutting down");
	            delete it->second;  // fd still open!
	        }
	    }

	Unlike normal disconnect paths (which call `close(clientFd)`), this
	leaks file descriptors if the Server is destroyed while the process
	continues.

	Fix: Close socket before deleting User:

	    for (...)
	    {
	        handleDisconnect(it->second, "Server shutting down");
	        close(it->first);  // Close client socket fd
	        delete it->second;
	    }

	Rule: Every socket opened with accept() must be closed with close().

41. handleQuit() Channel Iteration Optimization

	Inefficient approach (O(total_channels)):

	    for (map<string, Channel*>::iterator it = channels.begin(); ...)
	    {
	        if (!chan->isMember(user))  // Check membership on EVERY channel
	            continue;
	        // broadcast...
	    }

	With 1000 channels and user in 3, this checks 1000 memberships.

	Efficient approach (O(user's_channels)):

	    const std::set<Channel*>& userChannels = user->getChannels();
	    for (std::set<Channel*>::const_iterator it = userChannels.begin(); ...)
	    {
	        // No membership check needed - we know user is in these
	        // broadcast...
	    }

	With 1000 channels and user in 3, this only iterates 3 channels.

	This is consistent with how handleDisconnect() already works.
	Always prefer iterating the smaller set when you have a choice.

42. Disconnect Path Must Use Stored Quit Reason

	When handleQuit() marks a user for disconnection, it stores the
	quit reason via markForDisconnection(true, reason, true).

	But the disconnect path in handleClientData() was:

	    if (user->isMarkedForDisconnection()) {
	        handleDisconnect(user, "Client Quit");  // Wrong! Ignores stored reason
	        ...
	    }

	Fix: Retrieve the actual stored reason:

	    if (user->isMarkedForDisconnection()) {
	        std::string reason = user->getQuitReason();
	        if (reason.empty())
	            reason = "Client Quit";
	        handleDisconnect(user, reason);
	        ...
	    }

	This ensures the client's actual quit message is preserved through
	the entire cleanup path.

43. sendChannelModes() Param Vector Structure

	Wrong approach - embedding spaces inside one param:

	    std::string modeArgs = "";
	    modeArgs += " *";      // key
	    modeArgs += " 50";     // limit
	    params.push_back(modeStr + modeArgs);  // "+tkl * 50" as one element

	This breaks IRC protocol semantics - params should be separate tokens.

	Correct approach - separate params:

	    std::vector<std::string> modeArgs;
	    modeArgs.push_back("*");   // key (masked)
	    modeArgs.push_back("50");  // limit
	    
	    params.push_back(chan->getName());
	    params.push_back(modeStr);  // "+tkl"
	    for (size_t i = 0; i < modeArgs.size(); ++i)
	        params.push_back(modeArgs[i]);

	Result: params = ["#chan", "+tkl", "*", "50"]
	sendNumeric joins with spaces → ":server 324 nick #chan +tkl * 50"

44. IRC Protocol Injection via CR/LF in User Text

	Vulnerability: User-controlled text embedded directly in IRC lines.

	Attack vector:
	    Client sends: QUIT :bye\r\nPRIVMSG #admin :hacked
	    
	    Server builds: ":nick!user@host QUIT :bye\r\nPRIVMSG #admin :hacked\r\n"
	    
	    Peer receives two lines:
	        Line 1: ":nick!user@host QUIT :bye"
	        Line 2: "PRIVMSG #admin :hacked"  ← Injected command!

	Affected fields: QUIT reason, KICK reason, PART message, TOPIC text.

	Fix: Sanitize all user-controlled text before embedding:

	    std::string sanitizeIrcText(const std::string& str)
	    {
	        std::string result;
	        result.reserve(str.size());
	        for (size_t i = 0; i < str.size(); ++i)
	        {
	            char c = str[i];
	            if (c != '\r' && c != '\n')
	                result += c;
	        }
	        return result;
	    }

	Usage:
	    std::string reason = sanitizeIrcText(msg.params[0]);
	    std::string quitMsg = ":" + hostmask + " QUIT :" + reason + "\r\n";

	Rule: Never trust client input. Sanitize before embedding in protocol.

45. MODE +k Key Must Be Sanitized

	The channel key from MODE +k is stored and later broadcast to other
	clients. Like QUIT/TOPIC, it needs sanitization.

	Vulnerable code:

	    chan->setKey(msg.params[argIndex++]);  // Unsanitized!

	Fix:

	    std::string key = sanitizeIrcText(msg.params[argIndex++]);
	    if (key.empty())
	    {
	        sendNumeric(user, ERR_NEEDMOREPARAMS, ..., "Invalid key for +k");
	        return false;
	    }
	    chan->setKey(key);

	The empty check catches keys that were only CR/LF characters.

46. Makefile: Clean Stray Object Files

	Problem: Object files in src/ (from manual compilation) weren't
	cleaned by `make fclean` because it only removed obj/.

	Symptom:
	    $ make fclean
	    $ find . -name "*.o"
	    ./src/main.o
	    ./src/server.o

	Fix: Add safety net to clean rule:

	    clean:
	        rm -rf $(OBJ_DIR)
	        rm -f $(SRCS_DIR)/*.o    # Catch stray files

47. Refactoring: Use Shared require* Helpers

	Before (handleMode with inline checks):

	    if (!user->getIsRegistered())
	    {
	        sendNumeric(user, ERR_NOTREGISTERED, ...);
	        return;
	    }
	    if (msg.params.size() < 1)
	    {
	        sendNumeric(user, ERR_NEEDMOREPARAMS, ...);
	        return;
	    }
	    Channel* chan = findChannel(target);
	    if (!chan)
	    {
	        sendNumeric(user, ERR_NOSUCHCHANNEL, ...);
	        return;
	    }
	    // ... more checks

	After (using helpers):

	    if (!requireRegistered(user)) return;
	    if (!requireParams(user, msg, 1, "MODE")) return;
	    Channel* chan = requireChannel(user, target);
	    if (!chan) return;
	    if (!requireOnChannel(user, chan)) return;
	    if (!requireOperator(user, chan)) return;

	Benefits:
	- Reduces code duplication (~20 lines saved in MODE alone)
	- Consistent error messages across all commands
	- Single point of change for error handling

48. MODE Broadcast Arguments Must Be Sanitized

	When MODE changes are broadcast, arguments are echoed to all channel
	members. Even though applySingleMode() sanitizes specific cases (like
	+k key), the broadcast path should independently sanitize for defense
	in depth.

	Vulnerable code:

	    for (size_t j = argBefore; j < argIndex; ++j)
	    {
	        appliedArgs.push_back(msg.params[j]);  // Raw param!
	    }

	Fix:

	    for (size_t j = argBefore; j < argIndex; ++j)
	    {
	        // Sanitize arguments before recording them to avoid reintroducing
	        // any CR/LF or other unsafe characters into the broadcast.
	        appliedArgs.push_back(sanitizeIrcText(msg.params[j]));
	    }

	This ensures that even if a new mode type is added later without
	proper sanitization, the broadcast path remains safe.        proper sanitization, the broadcast path remains safe.

49. Invite Tracking: Use User* Not Nickname Strings

        Problem: Storing invites by nickname string has security issues:
        
        1. Transferable invites: If alice is invited to #secret, disconnects,
           and bob takes the nick "alice", bob can now join #secret
        2. NICK changes break invites: If alice changes nick to alice2 before
           joining, the invite is lost

        Vulnerable approach:

            std::set<std::string> _invitation_list;  // Stores nicknames
            
            void addInvite(const std::string& nick) {
                _invitation_list.insert(nick);
            }
            
            // In canJoin():
            if (_invitation_list.find(user->getNickname()) == _invitation_list.end())
                return JOIN_INVITE_ONLY;

        Fixed approach:

            std::set<User*> _invited_users;  // Stores User pointers
            
            void addInvite(User* user) {
                _invited_users.insert(user);
            }
            
            bool isInvited(User* user) const {
                return _invited_users.find(user) != _invited_users.end();
            }

        Cleanup on disconnect (in handleDisconnect):

            // Clean up invites from ALL channels (not just ones user is in)
            for (map<string, Channel*>::iterator it = channels.begin(); ...)
            {
                it->second->removeInvite(user);
            }

        User* tracks identity, not name. Survives NICK changes, invalid on disconnect.

50. Consolidate Nickname Lookups to findUserByNick()

        DRY principle: Don't Repeat Yourself.

        Before (duplicated in handleMessageCommand):

            User* targetUser = NULL;
            for (std::map<int, User*>::iterator it = users.begin(); it != users.end(); ++it)
            {
                if (caseInsensitiveCompare(it->second->getNickname(), target))
                {
                    targetUser = it->second;
                    break;
                }
            }

        After (using shared helper):

            User* targetUser = findUserByNick(target);

        Benefits:
        - Single source of truth for nick lookups
        - Consistent case-insensitive behavior everywhere
        - If lookup logic changes, only one place to update
        - Less code, fewer bugs

51. Channel Topic Protection (+t) Should Default to True

        Most IRC networks enable +t by default on new channels. This means:
        - Only operators can change the topic
        - Prevents topic vandalism in new channels

        Code fix:

            Channel::Channel(const std::string& name) : 
                ...
                _topic_protection(true),  // Was: false
                ...

        This matches:
        - IRC convention (most servers default to +t)
        - Test expectations (Test 4.3 assumes +t by default)
        - MODE query output (new channels show +t)

52. Use time_t for Timestamps, Not int

        The Problem:
        - std::time(NULL) returns time_t
        - Storing in int causes implicit narrowing conversion
        - On 32-bit systems with 32-bit time_t, Y2038 overflow occurs
        - Even on 64-bit systems, int truncates the value

        Bad:
            int _topic_set_at;
            void setTopicSetAt(int timestamp);
            chan->setTopicSetAt(std::time(NULL));  // Implicit narrowing!

        Good:
            time_t _topic_set_at;  
            void setTopicSetAt(time_t timestamp);
            chan->setTopicSetAt(std::time(NULL));  // Type-safe

        Required include:
            #include <ctime>  // For time_t

        General rule: When working with timestamps from the C time library,
        always use time_t to avoid data loss and maintain Y2038 compatibility.

53. Sanitize All Outputs, Not Just Commands

        Problem: Even if you sanitize user input in command handlers (QUIT, KICK, etc.),
        there are other paths where user-controlled data reaches the output buffer:
        - Numeric replies (ERR_NOSUCHNICK with attacker's nick)
        - Error messages reflecting user input
        - Channel names in JOIN/PART confirmations

        The sendNumeric() function is a central output point for numeric replies.
        By sanitizing at this choke point, ALL numeric replies become safe:

        Before:
            oss << nick;
            oss << " " << params[i];
            oss << " :" << trailing;

        After:
            oss << sanitizeIrcText(nick);
            oss << " " << sanitizeIrcText(params[i]);
            oss << " :" << sanitizeIrcText(trailing);

        Defense in depth principle:
        - Sanitize at input (when storing user data)
        - Sanitize at output (when writing to protocol stream)
        - Sanitize at chokepoints (central output functions)

        This way, even if one layer fails, others catch the injection.

54. Always Use Centralized Output Functions

        Problem: When you have a centralized function like sendNumeric() that handles:
        - Formatting (server prefix, 3-digit code, nick)
        - Sanitization (stripping CR/LF)
        - Buffer management

        ...but then bypass it with raw string building:

        Bad:
            std::string error = "421 * " + msg.command + " :Unknown command\r\n";
            user->getOutputBuffer() += error;

        Issues:
        1. Missing server prefix (:servername)
        2. Uses "*" instead of user's actual nick
        3. Bypasses sanitizeIrcText() - injection possible
        4. Inconsistent format across codebase

        Good:
            sendNumeric(user, ERR_UNKNOWNCOMMAND, 
                std::vector<std::string>(1, msg.command), "Unknown command");

        Output: :SugarDaddyFinderIRC 421 tester BADCMD :Unknown command

        Rule: If you have a centralized output helper, USE IT EVERYWHERE.
        Don't build IRC lines manually - that's what the helper is for.
