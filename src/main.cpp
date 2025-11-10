#include "server.hpp"

int main(int ac, char **av) {

	if (ac != 3) {
		std::cerr << "Usage: " << av[0] << " <port> <password>" << std::endl;
		return 1;
	}
	
	// Validate port number (range from 1 to 65535)
	int port = std::atoi(av[1]);
	if (port <= 0 || port > 65535) {
		std::cerr << "Error: Invalid port number" << std::endl;
		return 1;
	}
	
	try {
		Server server(port, av[2]);
		server.run();
	} catch (std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	
	return 0;
}