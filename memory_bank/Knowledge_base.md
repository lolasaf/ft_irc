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

