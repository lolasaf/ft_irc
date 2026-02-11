/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wel-safa <wel-safa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/16 16:43:38 by wel-safa          #+#    #+#             */
/*   Updated: 2026/02/11 15:15:49 by wel-safa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"

/*
	Constructor: initializes server with:
										-  port, 
										-  password,
										-  serverFd to -1 (indicating not set up yet),
	It calls initISupport to set up ISUPPORT tokens,
	It calls setupSocket to create and bind the server socket.
*/
Server::Server(int port, const std::string& password) : port(port), password(password), serverFd(-1)
{
	initISupport();
	setupSocket();
}

/* 
	Destructor: cleans up users, channels, and closes server socket.
		First we clean up all users (handleDisconnect will clean up channels).
		Then we clean up any remaining channels (shouldn't be any, but safety check).
		Finally, we close the server socket if it is open.
*/
Server::~Server()
{
	for (std::map<int, User*>::iterator it = users.begin(); it != users.end(); ++it)
	{
		handleDisconnect(it->second, "Server shutting down");
		close(it->first);  // Close client socket fd
		delete it->second; // Delete User object
	}

	for (std::map<std::string, Channel*>::iterator it = channels.begin(); it != channels.end(); ++it)
	{
		delete it->second;
	}
	channels.clear();

	if (serverFd != -1)
	{
		close(serverFd);
	}
}

/* 
	Initialize ISUPPORT tokens.
	ISUPPORT tokens inform clients about server capabilities and limits.
	We use std::ostringstream to build each token string.
*/
void Server::initISupport()
{
	isSupported.clear();
	std::ostringstream oss;
	oss << "USERLEN=" << USERLEN;
	isSupported.push_back(oss.str());
	oss.str(""); oss.clear();
	oss << "NICKLEN=" << NICKLEN;
	isSupported.push_back(oss.str());
	oss.str(""); oss.clear();
	oss << "REALLEN=" << REALLEN;
	isSupported.push_back(oss.str());
	oss.str(""); oss.clear();
	oss << "CHANTYPES=#";
	isSupported.push_back(oss.str());
	oss.str(""); oss.clear();
	oss << "CHANNELLEN=" << CHANNELLEN;
	isSupported.push_back(oss.str());
	oss.str(""); oss.clear();
}

/* 
	This creates a TCP socket, sets socket options, binds to the specified port,
	sets non-blocking mode, and starts listening for incoming connections.

	1. Create socket
		socket(domain, type, protocol)
		Give me a standard IPv4 TCP socket, use the normal protocol for this combination.
		- domain: AF_INET means IPv4
		- type: SOCK_STREAM means TCP (reliable, connection-based) i.e. TCP
		- protocol: 0 means auto-select (The kernel will choose the appropriate protocol i.e. TCP for SOCK_STREAM.)
	
	2. Set socket option to reuse address
		This prevents "Address already in use" errors when restarting server.
		Parameters:
		- SOL_SOCKET is the socket level
		- SO_REUSEADDR allows reuse of local addresses
		- opt is a pointer to the option value
		- sizeof(opt) is the size of the option value

	3. Set up server address structure
		struct sockaddr_in serverAddr;
		- Zero out the structure using memset
		- Set sin_family to AF_INET (IPv4)
		- Set sin_addr.s_addr to INADDR_ANY (listen on all interfaces)
		- Set sin_port to htons(port) (convert port to network byte order)

	4. Bind socket to the specified port and any available network interface.
		Binding socket is necessary before listening for connections because it 
		associates the socket with a specific local address and port.
		Parameters: 
			- serverFd, the socket file descriptor
			- (struct sockaddr*)&serverAddr, the address to bind to
			- sizeof(serverAddr), size of the address structure
		- Check for error (-1)

	5. Set non-blocking mode. 
		Non-blocking mode is important for servers to avoid blocking
		on socket operations, allowing the server to handle multiple 
		clients efficiently.
		Parameters: 
			- serverFd, is the socket file descriptor
			- F_SETFL, is the command to set file status flags
			- O_NONBLOCK, is the flag to enable non-blocking mode
		- Check for error (-1)

	6. Start listening for incoming connections.
		Listening marks the socket as a passive socket that will be used to 
		accept incoming connection requests.
		Parameters: 
			- serverFd, the socket file descriptor
			- SOMAXCONN, specifies the maximum number of pending connections
		- Check for error (-1)
	
*/
void Server::setupSocket() {

	serverFd = socket(AF_INET, SOCK_STREAM, 0);

	if (serverFd == -1)
	{
		throw std::runtime_error("Failed to create socket");
	}

	int opt = 1;

	if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
	{
		throw std::runtime_error("Failed to set socket options");
	}

	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);

	if (bind(serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1)
	{
		throw std::runtime_error("Failed to bind socket");
	}

	if (fcntl(serverFd, F_SETFL, O_NONBLOCK) == -1)
	{
		throw std::runtime_error("Failed to set non-blocking mode");
		// Parameters: serverFd, SOMAXCONN (max pending connections)
	}

	if (listen(serverFd, SOMAXCONN) == -1)
	{
		throw std::runtime_error("Failed to listen on socket");
	}
}

/*
	Main server loop: uses poll() to monitor server and client sockets for activity.
	1. Rebuild pollFds vector each iteration:
		- Add server socket (index 0) to monitor for new connections (POLLIN)
		- Add all client sockets:
			- Always monitor for POLLIN (incoming data)
			- Also monitor for POLLOUT if user's outputBuffer is not empty (data to send)
	2. Call poll() to wait for activity on any socket.
	3. Check server socket (index 0) for POLLIN to accept new clients.
	4. Check each client socket for:
		- Errors/hangup (POLLERR, POLLHUP, POLLNVAL): disconnect client
		- POLLIN: call handleClientData(clientFd) to read incoming data
		- POLLOUT: call handleClientWrite(clientFd) to send outgoing data
*/
void Server::run()
{
	std::cout << "Server is running on port " << port << std::endl;

	std::vector<struct pollfd> pollFds;

	while (true)
	{
		// Clear and rebuild pollFds each iteration
		pollFds.clear();

		// 1. Add server socket (listening for new connections)
		struct pollfd serverPfd;
		serverPfd.fd = serverFd;
		serverPfd.events = POLLIN;  // Watch for incoming connections
		serverPfd.revents = 0;
		pollFds.push_back(serverPfd);

		// 2. Add all client sockets
		for (std::map<int, User*>::iterator it = users.begin(); it != users.end(); ++it)
		{
			struct pollfd clientPfd;
			clientPfd.fd = it->first;

			//Set events to POLLIN (always watch for data)
			// Then, if outputBuffer is NOT empty, also add POLLOUT
			clientPfd.events = POLLIN;
			User* user = it->second;
			if (!user->getOutputBuffer().empty())
			{
				clientPfd.events |= POLLOUT;
			}
			clientPfd.revents = 0;
			pollFds.push_back(clientPfd);
		}

		// 3. Call poll() — wait for activity
		// Call poll() with pollFds array
		// Parameters: &pollFds[0], pollFds.size(), -1
		int activity = poll(&pollFds[0], pollFds.size(), -1);

		if (activity == -1)
		{
			if (errno == EINTR)
				continue;
			std::cerr << "Poll error: " << strerror(errno) << std::endl;
			break;
		}

		// 4. Check server socket (index 0) for new connections
		// Check if pollFds[0].revents has POLLIN set
		// Hint: use & (bitwise AND) to check: (revents & POLLIN)
		if (pollFds[0].revents & POLLIN)
		{
			acceptNewClient();
		}

		// 5. Check client sockets (starting from index 1)
		for (size_t i = 1; i < pollFds.size(); ++i)
		{
			int clientFd = pollFds[i].fd;

			// Check for errors/hangup first
			// If revents has POLLERR, POLLHUP, or POLLNVAL, disconnect client
			if (pollFds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
			{
				std::cout << "Client fd " << clientFd << " disconnected (error/hangup)" << std::endl;
				std::map<int, User*>::iterator it = users.find(clientFd);
				if (it != users.end())
				{
					handleDisconnect(it->second, "Connection closed");
					delete it->second;
					users.erase(it);
				}
				close(clientFd);
				continue; // Move to next client
			}
			// Check if ready to read (POLLIN)
			// If so, call handleClientData(clientFd)
			if (pollFds[i].revents & POLLIN)
			{
				handleClientData(clientFd);
			}

			// Check if ready to write (POLLOUT)  
			// If so AND user still exists, call handleClientWrite(clientFd)
			if (pollFds[i].revents & POLLOUT)
			{
				// Check if user still exists before writing
				if (users.find(clientFd) != users.end())
				{
					handleClientWrite(clientFd);
				}
			}
		}
	}
}

void Server::acceptNewClient()
{
	struct sockaddr_in clientAddr;
	socklen_t addrLen = sizeof(clientAddr);

	// Accept the connection — returns new fd for this client
	int clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &addrLen);

	if (clientFd == -1)
	{
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

	// Test output buffering: queue a welcome message
	// This will be sent on the next select() cycle when fd is writable
	newUser->getOutputBuffer() += "Welcome to ft_irc server!\r\n"; // TODO: I think we can remove this line later
}

void Server::handleClientData(int clientFd)
{
	char buffer[512];  // IRC max message size

	ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

	if (bytesRead > 0)
	{
		buffer[bytesRead] = '\0';  // Null-terminate

		// Get the User object from users map
		std::map<int, User*>::iterator it = users.find(clientFd);
		if (it == users.end())
		{
			std::cerr << "User not found for fd " << clientFd << std::endl;
			return;
		}
		User* user = it->second;

		// Append buffer to user's inputBuffer
		user->getInputBuffer() += buffer;

		// Extract complete messages and process each one
		std::vector<std::string> messages = extractMessages(user);
		for (size_t i = 0; i < messages.size(); ++i)
		{
			processMessage(user, messages[i]);
			if (user->isMarkedForDisconnection()) {
				// Use stored quit reason (set by handleQuit), fallback to default
				std::string reason = user->getQuitReason();
				if (reason.empty())
					reason = "Client Quit";
				users.erase(it); // Erase from map first - better iterator handling
				handleDisconnect(user, reason);
				close(clientFd);
				delete user;
				return;  // Exit handleClientData entirely
			}
		}
	}
	else if (bytesRead == 0)
	{
		// Client disconnected
		std::cout << "Client fd " << clientFd << " disconnected" << std::endl;
		std::map<int, User*>::iterator it = users.find(clientFd);
		if (it != users.end())
		{
			User* user = it->second;
			users.erase(it); // Erase from map first - better iterator handling
			handleDisconnect(user, "Client disconnected");
			delete user;
		}
		close(clientFd);
	}
	else
	{
		// bytesRead == -1
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			std::cerr << "Recv error on fd " << clientFd << std::endl;
			std::map<int, User*>::iterator it = users.find(clientFd);
			if (it != users.end())
			{
				User* user = it->second;
				users.erase(it);
				handleDisconnect(user, "Connection error");
				delete user;				
			}
			close(clientFd);
		}
	}
}

// This function handles sending data to the client
void Server::handleClientWrite(int clientFd)
{
	std::map<int, User*>::iterator it = users.find(clientFd);
	if (it == users.end())
		return;

	User* user = it->second;
	std::string& outBuf = user->getOutputBuffer();

	if (outBuf.empty())
		return;

	// Send data from outputBuffer
	ssize_t bytesSent = send(clientFd, outBuf.c_str(), outBuf.size(), 0);

	// Handle the result:
	// If bytesSent > 0: erase the sent bytes from outBuf using .erase(0, bytesSent)
	// If bytesSent == -1 AND errno is NOT EAGAIN/EWOULDBLOCK: disconnect client
	if (bytesSent > 0) {
		outBuf.erase(0, bytesSent);
	} else if (bytesSent == -1) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			std::cerr << "Send error on fd " << clientFd << std::endl;
			users.erase(it);
			handleDisconnect(user, "Send error");
			delete user;
			close(clientFd);
		}
	}
}

void Server::handlePing(User* user, const Message& msg)
{
    if (msg.params.empty())
    {
        sendNumeric(user, ERR_NEEDMOREPARAMS, std::vector<std::string>(1, "PING"), "Not enough parameters");
        return;
    }
    std::string pong = "PONG :" + msg.params[0] + "\r\n";
    user->getOutputBuffer() += pong;
}