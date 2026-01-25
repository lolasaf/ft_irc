/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serverMessage.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wel-safa <wel-safa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/17 20:27:30 by wel-safa          #+#    #+#             */
/*   Updated: 2026/01/17 20:27:56 by wel-safa         ###   ########.fr       */
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
	else
	{
		// Unknown command — send error 421
		std::string error = "421 * " + msg.command + " :Unknown command\r\n";
		user->getOutputBuffer() += error;
	}
}