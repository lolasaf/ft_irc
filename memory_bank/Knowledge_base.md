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

2. What is select()?
	select() is a system call that lets you monitor multiple file descriptors at once. 
	It blocks (waits) until one or more fds are "ready" for I/O.

	int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

		Parameter	|	Purpose
		nfds		|	Highest fd number + 1
		readfds		|	Set of fds to watch for reading (incoming data)
		writefds	|	Set of fds to watch for writing (can send data)
		exceptfds	|	Set of fds to watch for errors (we'll use NULL)
		timeout		|	How long to wait (NULL = wait forever)

3. What is fd_set()?
	An fd_set is basically a bit array where each bit represents a file descriptor. 
	You use macros to manipulate it:

		Macro				|	Purpose
		FD_ZERO(&set)		|	Clear all bits (initialize)
		FD_SET(fd, &set)	|	Add fd to the set
		FD_CLR(fd, &set)	|	Remove fd from the set
		FD_ISSET(fd, &set)	|	Check if fd is in the set (returns true/false)

	┌────────────────────────────────────────────────────────────┐
	│                     Main Event Loop                         │
	├────────────────────────────────────────────────────────────┤
	│                                                            │
	│   while (running) {                                        │
	│       1. Prepare fd_sets (which fds to watch)              │
	│       2. Call select() — blocks until something happens    │
	│       3. Check: Is serverFd ready? → accept() new client   │
	│       4. Check: Are client fds ready? → read/write data    │
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
		1. Evaluator Trap: You can only write when select() says the fd is writable. Writing at other times = grade 0.

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
		│  Main Loop (select):                                            │
		│      1. If outputBuffer not empty → add fd to writeFds          │
		│      2. Call select()                                           │
		│      3. If FD_ISSET(fd, writeFds) → NOW we can send!            │
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
