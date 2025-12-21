#ifndef USER_HPP
#define USER_HPP

#include <string>

class User {
private:
	int				fd;           // Socket file descriptor
	std::string		inputBuffer;  // Data received, waiting to be parsed
	std::string		outputBuffer; // Data waiting to be sent

	// TODO: [LATER] — Add nickname, username, registration state, etc.
	// std::string		nickname;
	// std::string		username;
	// std::string		realname;
	// std::string		hostname;

public:
	// Constructor: takes the client's socket fd
	User(int clientFd);
	// Destructor
	~User();

    // Getters
    // TODO: [YOUR CODE] — Write getter for _fd
	int getFd() const;
    // TODO: [YOUR CODE] — Write getter that returns reference to _inputBuffer
	std::string& getInputBuffer();
    // TODO: [YOUR CODE] — Write getter that returns reference to _outputBuffer
	std::string& getOutputBuffer();
};

#endif