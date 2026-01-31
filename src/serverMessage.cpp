/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serverMessage.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wel-safa <wel-safa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/17 20:27:30 by wel-safa          #+#    #+#             */
/*   Updated: 2026/01/30 16:53:50 by wel-safa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"

std::vector<std::string> Server::extractMessages(User* user)
{
	std::vector<std::string> messages;
	std::string& buf = user->getInputBuffer();

	// Loop extracting complete lines (ends with \n)
	while (true)
	{
		std::size_t pos = buf.find('\n');
		if (pos == std::string::npos)
			break;  // No complete message yet

		// Extract message WITHOUT the '\n'
		std::string message = buf.substr(0, pos);

		// Remove trailing '\r' if present (handle \r\n)
		if (!message.empty() && message[message.length() - 1] == '\r')
			message.erase(message.length() - 1, 1);

		// Remove the processed part from buffer (including \n)
		buf.erase(0, pos + 1);

		// Add to our list of messages
		if (!message.empty())
			messages.push_back(message);
	}

	return messages;
}

void Server::processMessage(User* user, const std::string& line)
{
	// Parse the raw line into a Message struct
	Message msg = parseMessage(line);

	if (msg.command.empty())
		return; // TODO: check what to do with invalid messages
	
	// Capitalize command for consistency
	msg.command = toUpper(msg.command);
	
	// Debug: print what we received
	// TODO: Remove or comment out in production
	std::cout << "Command: " << msg.command;
	for (size_t i = 0; i < msg.params.size(); ++i)
		std::cout << " [" << msg.params[i] << "]";
	std::cout << std::endl;

	// Route to appropriate handler based on command
	// TODO: Add more commands here
	if (msg.command == "PASS")
		handlePass(user, msg);
	else if (msg.command == "NICK")
		handleNick(user, msg);
	else if (msg.command == "USER")
		handleUser(user, msg);
	else if (msg.command == "PING")
		handlePing(user, msg);
	else if (msg.command == "JOIN")
		handleJoin(user, msg);
	else if (msg.command == "PART")
		handlePart(user, msg);
	else if (msg.command == "PRIVMSG")
		handlePrivmsg(user, msg);
	else if (msg.command == "NOTICE")
		handleNotice(user, msg);
	else if (msg.command == "QUIT")
		handleQuit(user, msg);
	else if (msg.command == "MODE")
		handleMode(user, msg);
	else if (msg.command == "TOPIC")
		handleTopic(user, msg);
	else if (msg.command == "INVITE")
		handleInvite(user, msg);
	else if (msg.command == "KICK")
		handleKick(user, msg);
	else
	{
		sendNumeric(user, ERR_UNKNOWNCOMMAND, std::vector<std::string>(1, msg.command), "Unknown command");
	}
}

// LOLA-TODO: Check this again 
void Server::handleMessageCommand(User* user, const Message& msg, const std::string& command)
{
	bool isNotice = command == "NOTICE";
	// 1. Registration check
	if (!user->getIsRegistered())
	{
		if (!isNotice) sendNumeric(user, ERR_NOTREGISTERED, std::vector<std::string>(), "You have not registered");
		return;
	}
	// 2. Parameter validation
	if (msg.params.empty())  // No target
	{
		if (!isNotice) sendNumeric(user, ERR_NORECIPIENT, std::vector<std::string>(1, command), "No recipient given");
		return;
	}
	if (msg.params.size() < 2)  // No message text
	{
		if (!isNotice) sendNumeric(user, ERR_NOTEXTTOSEND, std::vector<std::string>(), "No text to send");
		return;
	}
	// 3. Extract targets and message
	std::vector<std::string> targets = splitCommaList(msg.params[0]);
	std::string message = msg.params[1];

	// 4. Build hostmask for sender
	std::string hostmask = buildHostmask(user);

	// 5. Process each target
	for (size_t i = 0; i < targets.size(); ++i)
	{
		std::string target = targets[i];

		// Guard against empty targets produced by splitCommaList (e.g., consecutive commas)
		if (target.empty())
			continue;
		if (target[0] == '#')  // Channel target
		{
			// Find channel
			Channel* chan = findChannel(target);
			if (chan == NULL)
			{
				if (!isNotice) sendNumeric(user, ERR_NOSUCHCHANNEL, std::vector<std::string>(1, target), "No such channel");
				continue;
			}
			// Check membership
			if (!chan->isMember(user))
			{
				if (!isNotice) sendNumeric(user, ERR_CANNOTSENDTOCHAN, std::vector<std::string>(1, target), "Cannot send to channel");
				continue;
			}
			// Build and broadcast message
			std::string outputMessage = ":" + hostmask + " " + command + " " + chan->getName() + " :" + message + "\r\n";
			broadcastToChannel(chan, outputMessage, user);  // exclude sender
		}
		else  // User target
		{
			// Find user by nickname (case-insensitive) using shared helper
			User* targetUser = findUserByNick(target);
			if (targetUser == NULL)
			{
				if (!isNotice) sendNumeric(user, ERR_NOSUCHNICK, std::vector<std::string>(1, target), "No such nick/channel");
				continue;
			}
			// Build and send message
			std::string outputMessage = ":" + hostmask + " " + command + " " + targetUser->getNickname() + " :" + message + "\r\n";
			targetUser->getOutputBuffer() += outputMessage;
		}
	}
}


// Thin wrappers
void Server::handlePrivmsg(User* user, const Message& msg)
{
	handleMessageCommand(user, msg, "PRIVMSG");
}

void Server::handleNotice(User* user, const Message& msg)
{
	handleMessageCommand(user, msg, "NOTICE");
}