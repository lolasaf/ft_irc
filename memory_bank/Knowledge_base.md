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
