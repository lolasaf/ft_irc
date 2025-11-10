#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>

class Server {
	private:
		int port;
		std::string password;
		int serverSocket;
		// Additional private members can be declared here
	public:
		Server(int port, const std::string& password);
		~Server();
		void run();
		// Additional public members can be declared here
};

#endif