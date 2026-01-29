/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serverUtilsPreconditions.cpp                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dodordev <dodordev@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/29 13:27:50 by dodordev          #+#    #+#             */
/*   Updated: 2026/01/29 13:28:22 by dodordev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"

// Command Precondition Helpers (return true if check passes, false + error if not)

// Check if user is registered. Returns true if OK, false + sends ERR_NOTREGISTERED
bool Server::requireRegistered(User* user)
{
	if (user->getIsRegistered())
		return true;
	sendNumeric(user, ERR_NOTREGISTERED, std::vector<std::string>(), "You have not registered");
	return false;
}

// Check if enough params. Returns true if OK, false + sends ERR_NEEDMOREPARAMS
bool Server::requireParams(User* user, const Message& msg, size_t minParams, const std::string& cmdName)
{
	if (msg.params.size() >= minParams)
		return true;
	sendNumeric(user, ERR_NEEDMOREPARAMS, std::vector<std::string>(1, cmdName), "Not enough parameters");
	return false;
}

// Find channel or send error. Returns channel ptr or NULL + sends ERR_NOSUCHCHANNEL
Channel* Server::requireChannel(User* user, const std::string& channelName)
{
	Channel* chan = findChannel(channelName);
	if (chan)
		return chan;
	sendNumeric(user, ERR_NOSUCHCHANNEL, std::vector<std::string>(1, channelName), "No such channel");
	return NULL;
}

// Check if user is on channel. Returns true if OK, false + sends ERR_NOTONCHANNEL
bool Server::requireOnChannel(User* user, Channel* chan)
{
	if (chan->isMember(user))
		return true;
	sendNumeric(user, ERR_NOTONCHANNEL, std::vector<std::string>(1, chan->getName()), "You're not on that channel");
	return false;
}

// Check if user is operator. Returns true if OK, false + sends ERR_CHANOPRIVSNEEDED
bool Server::requireOperator(User* user, Channel* chan)
{
	if (chan->isOperator(user))
		return true;
	sendNumeric(user, ERR_CHANOPRIVSNEEDED, std::vector<std::string>(1, chan->getName()), "You're not channel operator");
	return false;
}

// Find user or send error. Returns user ptr or NULL + sends ERR_NOSUCHNICK
User* Server::requireUser(User* sender, const std::string& nickname)
{
	User* target = findUserByNick(nickname);
	if (target)
		return target;
	sendNumeric(sender, ERR_NOSUCHNICK, std::vector<std::string>(1, nickname), "No such nick/channel");
	return NULL;
}