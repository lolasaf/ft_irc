#include "user.hpp"

User::User(int clientFd) : fd(clientFd), inputBuffer(""), outputBuffer("") {}
// Buffers are automatically empty (std::string default)
User::~User()
{
	// Destructor can be empty for now
}

// Getters
int User::getFd() const
{
	return fd;
}

//Buffer getters
std::string& User::getInputBuffer()
{
	return inputBuffer;
}
std::string& User::getOutputBuffer()
{
	return outputBuffer;
}