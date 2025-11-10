#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

class Server {
	private:
		int port; // Port number for the server
		std::string password; // Password for server access
		int serverSocket; // Server socket file descriptor
		int serverFd; // Server file descriptor
		// Additional private members can be declared here

		public:
		Server(int port, const std::string& password);
		~Server();
		void run();
		void setupSocket();
		// Additional public members can be declared here
};

#endif