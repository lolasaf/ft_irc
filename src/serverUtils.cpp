/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serverUtils.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dodordev <dodordev@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 08:33:00 by wel-safa          #+#    #+#             */
/*   Updated: 2026/01/29 10:04:58 by dodordev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"

/*
 * This function sends a numeric reply to the user
 * @param user The User object to send the reply to
 * @param code The ReplyCode to send
 * @param params The vector of strings to send as parameters
 * @param trailing The string to send as the trailing part of the reply
 */
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

/*
 * This function checks if a channel name is valid
 * @param name The string to check if it is a valid channel name
 * @return True if the string is a valid channel name, false otherwise
 * must start with '#', not be empty, be less than 50 characters, and must not contain space, comma, colon
 */
bool Server::isValidChannelName(const std::string& name)
{
	return name.length() > 1 && name[0] == '#' && name.length() <= CHANNELLEN && name.find_first_of(" ,:") == std::string::npos;
}

/*
    * Build IRC hostmask format: nick!user@host
    * @param user User object
    * @return Hostmask string (e.g., "john!jdoe@127.0.0.1")
*/
std::string Server::buildHostmask(User* user)
{
    if (!user)
        return "";
    
    std::string nick = user->getNickname();
    std::string username = user->getUsername();
    std::string hostname = user->getHostname();
    
    // Use defaults if empty
    if (nick.empty())
        nick = "*";
    if (username.empty())
        username = "*";
    if (hostname.empty())
        hostname = "*";
    
    return nick + "!" + username + "@" + hostname;
}

// Utility: Find user by nickname (case-insensitive, reusable by KICK, INVITE, etc.)
User* Server::findUserByNick(const std::string& nick)
{
	for (std::map<int, User*>::iterator it = users.begin(); it != users.end(); ++it)
	{
		if (caseInsensitiveCompare(it->second->getNickname(), nick))
			return it->second;
	}
	return NULL;
}

// Utility: Send topic info to user (used by JOIN and TOPIC query)
void Server::sendTopicInfo(User* user, Channel* chan)
{
	std::string topic = chan->getTopic();
	if (topic.empty())
	{
		sendNumeric(user, RPL_NOTOPIC, std::vector<std::string>(1, chan->getName()), "No topic is set");
		return;
	}
	sendNumeric(user, RPL_TOPIC, std::vector<std::string>(1, chan->getName()), topic);
	// Also send RPL_TOPICWHOTIME (333)
	std::vector<std::string> params;
	params.push_back(chan->getName());
	params.push_back(chan->getTopicSetter());
	std::ostringstream oss;
	oss << chan->getTopicSetAt();
	params.push_back(oss.str());
	sendNumeric(user, RPL_TOPICWHOTIME, params, "");
}

// ============================================================================
// Command Precondition Helpers (return true if check passes, false + error if not)
// ============================================================================

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