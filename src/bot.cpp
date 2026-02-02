/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bot.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dodordev <dodordev@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/01 19:32:23 by dodordev          #+#    #+#             */
/*   Updated: 2026/02/02 10:25:00 by dodordev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "bot.hpp"

/* 
	Constructor: initializes bot with server IP, port, password, and channel.
	Also opens log file for appending.
*/
Bot::Bot(const std::string& server, 
			int port, 
			const std::string& password, 
			const std::string& channel) 
			: _socket(-1), 
			server(server), 
			port(port), 
			password(password), 
			channel(channel), 
			registered(false)
{
	logFile.open("bot.log", std::ios::app);
}

/*
	Destructor: closes socket and log file if open.
*/
Bot::~Bot()
{
	if (_socket >= 0)
		close(_socket);
	if (logFile.is_open())
		logFile.close();
}

/*
	Connects to the IRC server.
	returns true on success, false on failure.
	First creates a socket.
	Then sets up server address struct:
		- family: AF_INET
		- port: htons(port)
		- address: inet_pton from server string
	Finally attempts to connect.
*/
bool Bot::connectToServer()
{
	_socket = socket(AF_INET, SOCK_STREAM, 0);

	if (_socket == -1)
	{
		std::cerr << "Failed to create socket" << std::endl;
		return false;
	}

	struct sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	inet_pton(AF_INET, server.c_str(), &serverAddr.sin_addr);

	if (connect(_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1)
	{
		std::cerr << "Failed to connect to server" << std::endl;
		return false;
	}
	
	return true;
}

/*
	Send a raw IRC command to the server.
	Appends \r\n to the line as per IRC protocol.
*/
void Bot::sendRaw(const std::string& line)
{
	std::string msg = line + "\r\n";
	send(_socket, msg.c_str(), msg.size(), 0);
	std::cout << ">> " << line << std::endl;
}

/*
	Register the bot with PASS, NICK and USER commands.
*/
void Bot::registerBot()
{
	sendRaw("PASS " + password);
	sendRaw("NICK " + BOT_NICKNAME);
	sendRaw("USER " + BOT_USERNAME + " 0 * :" + BOT_REALNAME);
}

/*
	Join the specified channel.
*/
void Bot::joinChannel()
{
	sendRaw("JOIN " + channel);
}

/*
	Send a PRIVMSG to the target (channel or user).
*/
void Bot::sendMessage(const std::string& target, const std::string& message)
{
	sendRaw("PRIVMSG " + target + " :" + message);
}

/*
	Bot main loop: connects to server, registers, and processes incoming messages.
*/
void Bot::run()
{
	if (!connectToServer())
	{
		std::cerr << "Failed to connect" << std::endl;
		return;
	}
	
	registerBot();
	
	struct pollfd pfd;
	pfd.fd = _socket;
	pfd.events = POLLIN;
	
	char buffer[512];
	
	while (true)
	{
		int ret = poll(&pfd, 1, -1);
		if (ret < 0)
			break;
		
		if (pfd.revents & POLLIN)
		{
			ssize_t bytes = recv(_socket, buffer, sizeof(buffer) - 1, 0);
			if (bytes <= 0)
				break;
			buffer[bytes] = '\0';
			inputBuffer += buffer;
			std::string::size_type pos;
			while ((pos = inputBuffer.find('\n')) != std::string::npos)
			{
				std::string line = inputBuffer.substr(0, pos);
				if (!line.empty() && line[line.size() - 1] == '\r')
					line.erase(line.size() - 1);
				processLine(line);
				inputBuffer.erase(0, pos + 1);
			}
		}
		
		if (pfd.revents & (POLLERR | POLLHUP))
			break;
	}
}

int main(int argc, char* argv[])
{
    if (argc != 5)
    {
        std::cerr << "Usage: ./ircbot <server> <port> <password> <channel>" << std::endl;
        return 1;
    }
    
    Bot bot(argv[1], std::atoi(argv[2]), argv[3], argv[4]);
    bot.run();
    
    return 0;
}