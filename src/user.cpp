#include "user.hpp"

User::User(int clientFd) : fd(clientFd), inputBuffer(""), outputBuffer(""), passOk(false), nickname(""), username(""), realname(""), hostname("") {}
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

bool User::isRegistered() const
{
	return !nickname.empty() && !username.empty() && passOk;
}

void User::setPassOk(bool ok)
{
	passOk = ok;
}

std::string User::getNickname() const
{
	return nickname;
}

void User::setNickname(const std::string& nick)
{
	nickname = nick;
}

std::string User::getUsername() const
{
	return username;
}

void User::setUsername(const std::string& user)
{
	username = user;
}

void User::setRealname(const std::string& real)
{
	realname = real;
}

void User::setHostname(const std::string& host)
{
	hostname = host;
}