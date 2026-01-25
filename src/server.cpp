/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wel-safa <wel-safa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/16 16:43:38 by wel-safa          #+#    #+#             */
/*   Updated: 2026/01/17 20:29:58 by wel-safa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"

Server::Server(int port, const std::string& password) : port(port), password(password), serverFd(-1)
{
	initISupport();
	setupSocket();
}

Server::~Server()
{
	// Clean up all users (disconnectUser will clean up channels)
	for (std::map<int, User*>::iterator it = users.begin(); it != users.end(); ++it)
	{
		disconnectUser(it->second, "Server shutting down");
		delete it->second;
	}
	// Clean up any remaining channels (shouldn't be any, but safety check)
	for (std::map<std::string, Channel*>::iterator it = channels.begin(); it != channels.end(); ++it)
	{
		delete it->second;
	}
	channels.clear();
	// Close server socket if open
	if (serverFd != -1)
	{
		close(serverFd);
	}
}

/* Initialize ISUPPORT tokens */
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
	// Add more ISUPPORT tokens as needed
}

/* This function sets up the server socket*/
void Server::setupSocket() {
	// 1. Create socket
	// socket(domain, type, protocol)
	// - domain: AF_INET means IPv4
	// - type: SOCK_STREAM means TCP (reliable, connection-based)
	// - protocol: 0 means auto-select
	serverFd = socket(AF_INET, SOCK_STREAM, 0);

	if (serverFd == -1)
	{
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

	if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
	{
		throw std::runtime_error("Failed to set socket options");
	}

	// 3. Set up server address structure
	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));  // Zero out the structure
	serverAddr.sin_family = AF_INET;  // What address family are we using? IPv4
	serverAddr.sin_addr.s_addr = INADDR_ANY; // What address should we listen on? Listen to all available interfaces
	serverAddr.sin_port = htons(port);  // What port? (hint: needs conversion)

	// 4. Bind socket to address
	// Parameters: serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)
	// Check for error (-1)
	if (bind(serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1)
	{
		throw std::runtime_error("Failed to bind socket");
	}

	// 5. Set non-blocking mode
	// Parameters: serverFd, F_SETFL, O_NONBLOCK
	// Check for error (-1)
	if (fcntl(serverFd, F_SETFL, O_NONBLOCK) == -1)
	{
		throw std::runtime_error("Failed to set non-blocking mode");
		// Parameters: serverFd, SOMAXCONN (max pending connections)
	}

	// 6. Start listening
	// Check for error (-1)
	if (listen(serverFd, SOMAXCONN) == -1)
	{
		throw std::runtime_error("Failed to listen on socket");
	}
}

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
					disconnectUser(it->second, "Connection closed");
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
		}
	}
	else if (bytesRead == 0)
	{
		// Client disconnected
		std::cout << "Client fd " << clientFd << " disconnected" << std::endl;
		std::map<int, User*>::iterator it = users.find(clientFd);
		if (it != users.end())
		{
			disconnectUser(it->second, "Client disconnected");
			delete it->second;
			users.erase(it);
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
				disconnectUser(it->second, "Connection error");
				delete it->second;
				users.erase(it);
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
			disconnectUser(user, "Send error");
			delete user;
			users.erase(it);
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