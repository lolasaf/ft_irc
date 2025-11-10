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
	// Main server loop would go here
}

