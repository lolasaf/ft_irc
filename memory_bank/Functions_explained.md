setupSocket()

	This function prepares server to accept network connections. 
	Think of it as setting up a reception desk at a building entrance.

		┌─────────────────────────────────────────────────────────────────┐
		│                        setupSocket()                            │
		├─────────────────────────────────────────────────────────────────┤
		│                                                                 │
		│  1. socket()      →  Create a "phone" (communication endpoint)  │
		│                      Returns a file descriptor (just a number)  │
		│                                                                 │
		│  2. setsockopt()  →  Configure the phone settings               │
		│                      SO_REUSEADDR = can reuse number immediately│
		│                                                                 │
		│  3. sockaddr_in   →  Fill out the "address card"                │
		│                      - What protocol? (IPv4)                    │
		│                      - What interfaces? (all of them)           │
		│                      - What port? (e.g., 6667)                  │
		│                                                                 │
		│  4. bind()        →  Register your phone number                 │
		│                      Associates socket with the address/port    │
		│                                                                 │
		│  5. fcntl()       →  Set to "don't wait" mode                   │
		│                      Non-blocking = return immediately          │
		│                                                                 │
		│  6. listen()      →  Start accepting calls                      │
		│                      Socket is now ready for connections        │
		│                                                                 │
		└─────────────────────────────────────────────────────────────────┘

		Step-by-Step Breakdown:

			1. socket(AF_INET, SOCK_STREAM, 0)

				serverFd = socket(AF_INET, SOCK_STREAM, 0);
				
				Parameter	|	Value		|	Meaning
				domain		|	AF_INET		|	Use IPv4 addresses
				type		|	SOCK_STREAM	|	TCP (reliable, ordered, connection-based)
				protocol	|	0			|	Let the system choose (TCP for SOCK_STREAM)
				
				Returns: A file descriptor (integer) — just a number that represents this socket. 
				-1 means error.

			
			2. setsockopt(...SO_REUSEADDR...)

				int opt = 1;setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

				Problem it solves: When you stop your server and restart it quickly, you might get "Address already in use" error. The OS keeps the port reserved for ~60 seconds.

				Solution: SO_REUSEADDR tells the OS "let me reuse this address immediately".


			3. sockaddr_in — The Address Structure

				struct sockaddr_in serverAddr;
				memset(&serverAddr, 0, sizeof(serverAddr));  // Clear to zeros
				serverAddr.sin_family = AF_INET;             // IPv4
				serverAddr.sin_addr.s_addr = INADDR_ANY;     // Any network interface
				serverAddr.sin_port = htons(port);           // Port number
				
				This structure tells bind() WHERE to listen:

				Field			|	Your Value		|	Meaning
				sin_family		|	AF_INET			|	IPv4 protocol
				sin_addr.s_addr	|	INADDR_ANY		|	Accept connections on ANY network interface (localhost, ethernet, wifi, etc.)
				sin_port		|	htons(port)		|	The port number, converted to network byte order
				
				Why htons()? Networks use big-endian byte order. 
				Your CPU might use little-endian. htons() converts to ensure the port number is understood correctly.


			4. bind()

				bind(serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
			
				What it does: Associates your socket with the address you specified.

				After this, the port (e.g., 6667) is reserved for your server.


			5. fcntl(...O_NONBLOCK)

				fcntl(serverFd, F_SETFL, O_NONBLOCK);
			
				Blocking (default):
				recv() called → No data? → WAIT... → WAIT... → Data arrives → Return                          (server frozen!)
			
				Non-blocking:
				recv() called → No data? → Return immediately with EAGAIN error                          (server continues running!)
			
				This is critical for ft_irc — you can't have the server freeze waiting for one client!


			6. listen()

				listen(serverFd, SOMAXCONN);

				Parameter	|	Value		|	Meaning
				socket		|	serverFd	|	Your socket
				backlog		|	SOMAXCONN	|	Max number of pending connections in queue
				
				Before listen(): Socket exists but isn't accepting connections.

				After listen(): Socket is in "passive" mode — ready to accept incoming connections!

			The Complete Flow of the setupSocket():

				Your Code                          Operating System
				─────────                          ────────────────
				socket()         ─────────────►    Creates socket, returns fd=3

				setsockopt()     ─────────────►    Marks socket as reusable

				bind()           ─────────────►    "Port 6667 belongs to fd=3"

				fcntl()          ─────────────►    "fd=3 is non-blocking"

				listen()         ─────────────►    "fd=3 is now accepting connections"

												┌─────────────────┐
												│  Port 6667      │
												│  LISTENING...   │◄──── Clients can
												│  (your server)  │      now connect!
												└─────────────────┘

