#include "server.hpp"

Server::Server(int port, const std::string& password) : port(port), password(password), serverFd(-1) {
	setupSocket();
}

Server::~Server() {
	if (serverFd != -1) {
		close(serverFd);
	}
}

void Server::run() {
	std::cout << "Server is running on port " << port << std::endl;
	// Temporary: keep server alive to test socket
	while (true) {
		// Main server loop will go here
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