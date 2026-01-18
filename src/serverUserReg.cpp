/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serverUserReg.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wel-safa <wel-safa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/17 20:25:51 by wel-safa          #+#    #+#             */
/*   Updated: 2026/01/17 20:29:50 by wel-safa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"

void Server::registerUser(User* user)
{
	// Check if user is registered (has set PASS, NICK, and USER)
	if (user->getIsRegistered())
		return; // Already registered

	// Check if PASS, NICK, USER were set
	if (user->isPassOk() && !user->getNickname().empty() && !user->getUsername().empty())
	{
		// Mark user as registered
		user->setIsRegistered(true);
		// Send welcome messages
		sendNumeric(user, RPL_WELCOME, std::vector<std::string>(), "Welcome to the ft_irc server, " + user->getNickname() + "!");
		sendNumeric(user, RPL_YOURHOST, std::vector<std::string>(), "Your host is " + SERVER_NAME);
		sendNumeric(user, RPL_CREATED, std::vector<std::string>(), "This server was created just now");
		sendNumeric(user, RPL_MYINFO, std::vector<std::string>(), SERVER_NAME + " 1.0 irc o o");
		sendNumeric(user, RPL_ISUPPORT, isSupported, "are supported by this server");
	}
}

void Server::handlePass(User* user, const Message& msg)
{
	// PASS: set password for the connection before registration
	if (user->getIsRegistered())
	{
		sendNumeric(user, ERR_ALREADYREGISTRED, std::vector<std::string>(), "You may not reregister");
		return;
	}

	// Expect at least one parameter: the password
	if (msg.params.empty())
	{
		sendNumeric(user, ERR_NEEDMOREPARAMS, std::vector<std::string>(1, "PASS"), "Not enough parameters");
		return;
	}

	// Validate provided password against server password
	std::string provided = msg.params[0]; // Only use first parameter, ignore extras
	if (provided != password)
	{
		// Incorrect password
		sendNumeric(user, ERR_PASSWDMISMATCH, std::vector<std::string>(), "Password incorrect");
		return;
	}

	// Password accepted for this connection
	user->setPassOk(true);
	std::cout << "PASS accepted for fd " << user->getFd() << std::endl;
	
	// Registration completion (welcome) is handled when both NICK and USER are present
	registerUser(user);
}

void Server::handleNick(User* user, const Message& msg)
{
	if (msg.params.empty())
	{
		sendNumeric(user, ERR_NONICKNAMEGIVEN, std::vector<std::string>(), "No nickname given");
		return;
	}
	std::string newNick = msg.params[0]; // use first parameter only
	// Check for erroneous nickname (simple check: only alphanum and _ and size <= 9)
	if (newNick.empty() || newNick.length() > 9)
	{
		sendNumeric(user, ERR_ERRONEUSNICKNAME, std::vector<std::string>(1, newNick), "Erroneous nickname");
		return;
	}
	for (size_t i = 0; i < newNick.length(); i++)
	{
		if (!isalnum(newNick[i]) && newNick[i] != '_')
		{
			sendNumeric(user, ERR_ERRONEUSNICKNAME, std::vector<std::string>(1, newNick), "Erroneous nickname");
			return;
		}
	}
	// Check if nickname is already in use
	for (std::map<int, User*>::iterator it = users.begin(); it != users.end(); it++)
	{
		std::string existingNick = it->second->getNickname();
		if (!existingNick.empty() && strcasecmp(existingNick.c_str(), newNick.c_str()) == 0)
		{
			sendNumeric(user, ERR_NICKNAMEINUSE, std::vector<std::string>(1, newNick), "Nickname is already in use");
			return;
		}
	}
	// Set the new nickname
	user->setNickname(newNick);
	std::cout << "Nickname set to " << newNick << " for fd " << user->getFd() << std::endl;
	registerUser(user); // Check if registration can be completed and send welcome
}

void Server::handleUser(User* user, const Message& msg)
{
	if (user->getIsRegistered())
	{
		sendNumeric(user, ERR_ALREADYREGISTRED, std::vector<std::string>(), "You may not reregister");
		return;
	}
	// Expect at least 4 parameters: username, hostname, servername, realname
	if (msg.params.size() < 4)
	{
		sendNumeric(user, ERR_NEEDMOREPARAMS, std::vector<std::string>(1, "USER"), "Not enough parameters");
		return;
	}
	
	// USER <username> <hostname> <servername> :<realname>
	// ignore hostname and servername // there for legacy reasons
	std::string username = msg.params[0];
	std::string realname = msg.params[3];

	if (username.empty())
	{
		sendNumeric(user, ERR_NEEDMOREPARAMS, std::vector<std::string>(1, "USER"), "Username cannot be empty");
		return;
	}

	// user name validation: only alphanum and _
	for (size_t i = 0; i < username.length(); i++)
	{
		if (!isalnum(username[i]) && username[i] != '_')
		{
			sendNumeric(user, ERR_NEEDMOREPARAMS, std::vector<std::string>(1, username), "Erroneous username");
			return;
		}
	}
	
	// Truncate username and realname to allowed lengths
	if (username.length() > USERLEN)
		username.erase(USERLEN);
	if (realname.length() > REALLEN) 
		realname.erase(REALLEN);
	user->setUsername(username);
	user->setRealname(realname);
	std::cout << "USER set: " << username << " (" << realname << ") for fd " << user->getFd() << std::endl;
	registerUser(user); // Check if registration can be completed and send welcome
}

void Server::sendNumeric(User* user, ReplyCode code, const std::vector<std::string>& params, const std::string& trailing)
{
	std::ostringstream oss;
	oss << ":" 
		<< SERVER_NAME << " " 
		<< std::setw(3) << std::setfill('0') 
		<< static_cast<int>(code) << " ";

	std::string nick = user->getNickname();
	if (!nick.empty())
		oss << nick;
	else
		oss << "*";

	for (size_t i = 0; i < params.size(); ++i)
	{
		oss << " " << params[i];
	}

	if (!trailing.empty())
	{
		oss << " :" << trailing;
	}

	oss << "\r\n";
	user->getOutputBuffer() += oss.str();
}
