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

---

			7. select() and fd_set

				select() is a system call that lets you monitor multiple file descriptors at once.
				It blocks (waits) until one or more fds are "ready" for I/O.

					┌─────────────────────────────────────────────────────────────────┐
					│                         select() magic                          │
					├─────────────────────────────────────────────────────────────────┤
					│                                                                 │
					│   You: "Hey OS, watch these fds: [3, 5, 7, 9]"                  │
					│                                                                 │
					│   OS:  *waits*                                                  │
					│        *waits*                                                  │
					│        "fd 3 has a new connection!"                             │
					│        "fd 7 has data to read!"                                 │
					│                                                                 │
					│   You: *handle fd 3 and 7*                                      │
					│        *loop back to select()*                                  │
					│                                                                 │
					└─────────────────────────────────────────────────────────────────┘

				Function signature:
					int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

					Parameter	|	Purpose
					nfds		|	Highest fd number + 1
					readfds		|	Set of fds to watch for reading (incoming data)
					writefds	|	Set of fds to watch for writing (can send data)
					exceptfds	|	Set of fds to watch for errors (we use NULL)
					timeout		|	How long to wait (NULL = wait forever)

				What is fd_set?
					An fd_set is basically a bit array where each bit represents a file descriptor.
					You use macros to manipulate it:

					Macro				|	Purpose
					FD_ZERO(&set)		|	Clear all bits (initialize)
					FD_SET(fd, &set)	|	Add fd to the set
					FD_CLR(fd, &set)	|	Remove fd from the set
					FD_ISSET(fd, &set)	|	Check if fd is in the set (returns true/false)

				The Event Loop Pattern:
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

				Error Handling (errno):
					When select() returns -1, check errno:

					errno		|	Meaning				|	Action
					EINTR		|	Signal interrupted	|	Continue (or check shutdown flag)
					EBADF		|	Bad file descriptor	|	Bug in your code — fix it
					Other		|	Serious error		|	Log and exit

			---

			8. accept()

				When a client connects to your listening socket, accept() creates a NEW socket
				specifically for that client.

					int clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &addrLen);

					- serverFd keeps listening for MORE connections
					- clientFd is used to talk to THIS specific client

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

				Important: Set clientFd to non-blocking immediately after accept():
					fcntl(clientFd, F_SETFL, O_NONBLOCK);

			---

User Class

	The User class represents a connected client. Each time someone connects,
	we create a User object to track everything about that connection.

		┌─────────────────────────────────────────────────────────────┐
		│                         User                                │
		├─────────────────────────────────────────────────────────────┤
		│  fd            →  Socket file descriptor (to send/recv)    │
		│  inputBuffer   →  Data received but not yet processed      │
		│  outputBuffer  →  Data waiting to be sent                  │
		│  nickname      →  IRC nickname (set by NICK command)       │
		│  username      →  IRC username (set by USER command)       │
		│  isRegistered  →  Has completed PASS/NICK/USER?            │
		└─────────────────────────────────────────────────────────────┘

	Why do we need buffers?

		Input Buffer:
			TCP is a *stream* protocol. Data can arrive in chunks:
			
			Client sends: "NICK john\r\n"
			You might receive: "NIC"  then  "K john\r\n"
			Or even: "NICK john\r\nUSER john 0 * :John\r\n" (two commands at once!)
			
			The input buffer accumulates data until you see a complete line (\n).

		Output Buffer:
			You can't always send immediately (socket might be busy).
			Store messages here, send when select() says the fd is writable.

	Storage in Server:
		std::map<int, User*> users;  // fd → User pointer

		Why map<int, User*>?
		- Fast lookup by fd (O(log n))
		- Easy iteration to add all fds to select()
		- Clean ownership (Server owns the User objects)

---

			9. recv()

				Receives data from a connected socket. This is how you read what clients send.

					ssize_t recv(int sockfd, void *buf, size_t len, int flags);

					Parameter	|	Purpose
					sockfd		|	The client's file descriptor
					buf			|	Buffer to store received data
					len			|	Maximum bytes to read
					flags		|	Usually 0 (no special flags)

				Return values (CRITICAL to understand):

					Return Value	|	Meaning					|	Action
					> 0				|	Number of bytes read	|	Process the data
					0				|	Client disconnected		|	Clean up and remove client
					-1				|	Error occurred			|	Check errno

				Error handling with errno:

					errno		|	Meaning							|	Action
					EAGAIN		|	No data available (non-blocking)|	Normal, try again later
					EWOULDBLOCK	|	Same as EAGAIN					|	Normal, try again later
					Other		|	Real error						|	Disconnect client

				Example usage:

					char buffer[512];
					ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

					if (bytesRead > 0) {
						buffer[bytesRead] = '\0';  // Null-terminate for string use
						// Process data...
					} else if (bytesRead == 0) {
						// Client closed connection
						disconnectClient(clientFd);
					} else {
						// bytesRead == -1
						if (errno != EAGAIN && errno != EWOULDBLOCK) {
							// Real error, disconnect
							disconnectClient(clientFd);
						}
						// If EAGAIN, just continue — no data yet
					}

				Why sizeof(buffer) - 1?
					Leave room for the null terminator when treating buffer as a string.

---

			10. Safe Map Iteration (When Removing Elements)

				When iterating over a map and potentially removing elements, you must be careful
				not to invalidate your iterator.

				THE PROBLEM:

					// WRONG — will crash or skip elements!
					for (it = users.begin(); it != users.end(); ++it) {
						if (shouldDisconnect(it->first)) {
							users.erase(it);  // ❌ Iterator is now INVALID!
							// ++it in the for loop will dereference invalid iterator
						}
					}

				THE SOLUTION — Save fd before incrementing:

					// CORRECT — increment BEFORE potential erase
					for (it = users.begin(); it != users.end(); ) {
						int fd = it->first;     // Save the fd
						++it;                   // Advance iterator FIRST
						
						if (FD_ISSET(fd, &readSet)) {
							handleClientData(fd);  // This might erase the entry
						}
					}

				Why this works:

					┌──────────────────────────────────────────────────────────┐
					│   Before:  it → [fd=4, User*]  →  [fd=5, User*]  → end  │
					│                                                          │
					│   Step 1:  fd = 4                                        │
					│   Step 2:  ++it  (now points to fd=5)                    │
					│   Step 3:  handleClientData(4) erases fd=4               │
					│                                                          │
					│   After:   [fd=5, User*] → end                           │
					│            ↑                                             │
					│            it (still valid!)                             │
					└──────────────────────────────────────────────────────────┘

				Alternative pattern — post-increment in erase:

					for (it = users.begin(); it != users.end(); ) {
						if (shouldRemove(it->first)) {
							users.erase(it++);  // Increment THEN erase old position
						} else {
							++it;
						}
					}

				Key takeaway: 
					Never modify a container while iterating without protecting your iterator!

---

			12. send() — Output Buffering & Write Handling

				Sends data to a connected socket. But NEVER call it directly in command handlers!

					ssize_t send(int sockfd, const void *buf, size_t len, int flags);

					Parameter	|	Purpose
					sockfd		|	The client's file descriptor
					buf			|	Pointer to data to send
					len			|	Number of bytes to send
					flags		|	Usually 0 (no special flags)

				Return values:

					Return Value	|	Meaning					|	Action
					> 0				|	Bytes actually sent		|	Erase those bytes from buffer
					0				|	Connection closed		|	Disconnect client
					-1				|	Error occurred			|	Check errno

				Why can't we call send() directly?

					1. EVALUATOR TRAP: Writing when fd isn't ready = grade 0
					   You can only write when select() says the fd is writable.

					2. PARTIAL WRITES: send() might not send everything!
					   If you try to send 100 bytes, it might only send 50.

					3. BLOCKING RISK: If client's buffer is full, send() blocks.

				THE SOLUTION — Output Buffer Pattern:

					┌─────────────────────────────────────────────────────────────────┐
					│                    Output Buffering Flow                        │
					├─────────────────────────────────────────────────────────────────┤
					│                                                                 │
					│  Command Handler:                                               │
					│      user->getOutputBuffer() += "001 nick :Welcome!\r\n";       │
					│      (just append, DON'T send yet!)                             │
					│                                                                 │
					│  Main Loop (select):                                            │
					│      1. If outputBuffer not empty → add fd to writeFds          │
					│      2. Call select(maxFd+1, &readFds, &writeFds, NULL, NULL)   │
					│      3. If FD_ISSET(fd, writeFds) → call handleClientWrite()    │
					│                                                                 │
					│  handleClientWrite():                                           │
					│      bytesSent = send(fd, outBuf.c_str(), outBuf.size(), 0)     │
					│      outBuf.erase(0, bytesSent)  // Remove ONLY sent portion    │
					│                                                                 │
					└─────────────────────────────────────────────────────────────────┘

				Partial write handling:

					outBuf = "Hello World!"  (12 bytes)
					bytesSent = send(...)    // Returns 5 (partial!)
					outBuf.erase(0, 5)       // outBuf = "World!" (7 bytes remain)
					// Next select() cycle will send the rest

				Error handling:

					if (bytesSent == -1) {
					    if (errno == EAGAIN || errno == EWOULDBLOCK) {
					        // Normal for non-blocking — try again later
					    } else {
					        // Real error — disconnect client
					    }
					}

				Key takeaway:
					NEVER call send() directly. Always:
					1. Append to outputBuffer
					2. Let select() detect writability
					3. handleClientWrite() does the actual send()

