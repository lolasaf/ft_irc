#include "server.hpp"
#include <cerrno>  // For errno

Server::Server(int port, const std::string& password) : port(port), password(password), serverFd(-1) {
	setupSocket();
}

Server::~Server() {
	// Delete all User objects in _users
	for (std::map<int, User*>::iterator it = users.begin(); it != users.end(); ++it) {
		delete it->second;
	}
	// Close server socket if open
	if (serverFd != -1) {
		close(serverFd);
	}
}

/* This function sets up the server socket*/
void Server::setupSocket() {
	// 1. Create socket
	// socket(domain, type, protocol)
	// - domain: AF_INET means IPv4
	// - type: SOCK_STREAM means TCP (reliable, connection-based)
	// - protocol: 0 means auto-select
	serverFd = socket(AF_INET, SOCK_STREAM, 0);

	if (serverFd == -1) {
		throw std::runtime_error("Failed to create socket");
	}

	// 2. Set socket option to reuse address
	// This prevents "Address already in use" errors when restarting server
	// TODO: [YOUR CODE] — Call setsockopt() 
	// Parameters: serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)
	// - SOL_SOCKET and SO_REUSEADDR are constants defined in <sys/socket.h>
	// - opt is an integer set to 1
	// Check if return value == -1 (error)
	int opt = 1;

	if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
		throw std::runtime_error("Failed to set socket options");
	}

	// 3. Set up server address structure
	// - serverAddr.sin_family = ???        (hint: same as socket domain)
	// - serverAddr.sin_addr.s_addr = ???   (hint: INADDR_ANY means any interface)
	// - serverAddr.sin_port = ???          (hint: use htons() to convert port)
	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr)); // Zero out the structure
	serverAddr.sin_family = AF_INET;        // What address family are we using? IPv4
	serverAddr.sin_addr.s_addr = INADDR_ANY;   // What address should we listen on? Listen to all available interfaces
	serverAddr.sin_port = htons(port);          // What port? (hint: needs conversion)

	// 4. Bind socket to address
	// Parameters: serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)
	// Check for error (-1)
	if (bind(serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
		throw std::runtime_error("Failed to bind socket");
	}

	// 5. Set non-blocking mode
	// Parameters: serverFd, F_SETFL, O_NONBLOCK
	// Check for error (-1)
	if (fcntl(serverFd, F_SETFL, O_NONBLOCK) == -1) {
		throw std::runtime_error("Failed to set non-blocking mode");
	}

	// 6. Start listening
	// Parameters: serverFd, SOMAXCONN (max pending connections)
	// Check for error (-1)
	if (listen(serverFd, SOMAXCONN) == -1) {
		throw std::runtime_error("Failed to listen on socket");
	}
}

void Server::run() {
	std::cout << "Server is running on port " << port << std::endl;

	fd_set readFds;
	int maxFd;

	while (true) {
		FD_ZERO(&readFds);
		FD_SET(serverFd, &readFds);
		maxFd = serverFd;

        // TODO: [YOUR CODE] — Loop through all users in the map
        // For each user:
        //   1. Get their fd
        //   2. Add it to readFds with FD_SET
        //   3. Update maxFd if this fd is higher
		for (std::map<int, User*>::iterator it = users.begin(); it != users.end(); ++it) {
			int userFd = it->first;
			FD_SET(userFd, &readFds);
			if (userFd > maxFd) {
				maxFd = userFd;
			}
		}

		int activity = select(maxFd + 1, &readFds, NULL, NULL, NULL);

		if (activity == -1) {
			if (errno == EINTR)
				continue;
			std::cerr << "Select error: " << strerror(errno) << std::endl;
			break;
		}

		if (FD_ISSET(serverFd, &readFds)) {
			acceptNewClient();
		}

		// Loop through all users to check if they have data to read
		// Increment iterator BEFORE handleClientData() to avoid invalidation
		// if the user is removed from the map during handling
		for (std::map<int, User*>::iterator it = users.begin(); it != users.end(); ) {
			int userFd = it->first;
			++it;  // Increment BEFORE potentially erasing
			if (FD_ISSET(userFd, &readFds)) {
				handleClientData(userFd);
			}
		}
	}
}

void Server::acceptNewClient() {
	struct sockaddr_in clientAddr;
	socklen_t addrLen = sizeof(clientAddr);

	// Accept the connection — returns new fd for this client
	int clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &addrLen);

	if (clientFd == -1) {
		std::cerr << "Failed to accept client" << std::endl;
		return;
	}

	// Set the new client socket to non-blocking
	if (fcntl(clientFd, F_SETFL, O_NONBLOCK) == -1) {
		std::cerr << "Failed to set non-blocking mode for client" << std::endl;
		close(clientFd);
		return;
	}

	User* newUser = new User(clientFd);
	users[clientFd] = newUser;
	
	std::cout << "New client connected! fd: " << clientFd << std::endl;

	// TODO: [LATER] — Store client fd in a container for tracking
}

void Server::handleClientData(int clientFd) {
	char buffer[512];  // IRC max message size

	ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

	if (bytesRead > 0) {
		buffer[bytesRead] = '\0';  // Null-terminate
		//std::cout << "Received from fd " << clientFd << ": " << buffer;

		// Get the User object from users map
		std::map<int, User*>::iterator it = users.find(clientFd);
		if (it == users.end()) {
			std::cerr << "User not found for fd" << clientFd << std::endl;
			return;
		}
		User* user = it->second;

		// Append buffer to user's inputBuffer
		user->getInputBuffer() += buffer;

		// Loop to extract all complete messages
		while (true) {
			std::size_t pos = user->getInputBuffer().find('\n');
			if (pos != std::string::npos) {
				std::string message = user->getInputBuffer().substr(0, pos); // Extract message WITHOUT '\n'
				if (!message.empty() && message[message.length() - 1] == '\r') {
					message.erase(message.length() - 1, 1); // Remove trailing '\r' if present
				}

				// TODO: PRINT OR PROCESS THE MESSAGE
				std::cout << "Received from fd " << clientFd << ": " << message << std::endl;
	
				user->getInputBuffer().erase(0, pos + 1);
			}
			else
				break;
		}

		// Pattern:
		//   1. Find position of '\n' in inputBuffer using .find('\n')
		//   2. If found (not std::string::npos):
		//      - Extract substring from start to '\n' (include '\n')
		//      - Remove '\r' from the message if present
		//      - Print/process that message
		//      - Remove it from inputBuffer using .erase()
		//      - Go back to step 1 (loop to check for more messages)
		//   3. If not found: break (wait for more data next recv())
	}
	else if (bytesRead == 0) {
		// Client disconnected
		std::cout << "Client fd " << clientFd << " disconnected" << std::endl;
		close(clientFd);
		std::map<int, User*>::iterator it = users.find(clientFd);
		if (it != users.end()) {
			delete it->second;
			users.erase(it);
		}
	}
	else {
		// bytesRead == -1
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			std::cerr << "Recv error on fd " << clientFd << std::endl;
			close(clientFd);
			std::map<int, User*>::iterator it = users.find(clientFd);
			if (it != users.end()) {
				delete it->second;
				users.erase(it);
			}
		}
	}
}