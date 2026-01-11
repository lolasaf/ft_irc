#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdlib> //For atoi
#include <fcntl.h> // For fcntl(), O_NONBLOCK, F_SETFL
#include <cstring> // For memset() if you want to zero the struct
#include <map> // For storing clients
#include <poll.h> // For poll() if needed
#include <vector> // For std::vector
#include "user.hpp"

class Server {
	private:
		int port; // Port number for the server
		std::string password; // Password for server access
		int serverFd; // Server socket file descriptor
		std::map<int, User*> users; //Connected users by fd

		public:
		Server(int port, const std::string& password);
		~Server();
		void run();
		void setupSocket();
		void acceptNewClient();
		void handleClientData(int clientFd);
		void handleClientWrite(int clientFd);
};

#endif