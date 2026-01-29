/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serverUtils.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dodordev <dodordev@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 08:33:00 by wel-safa          #+#    #+#             */
/*   Updated: 2026/01/29 13:28:06 by dodordev         ###   ########.fr       */
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
		oss << sanitizeIrcText(nick);
	else
		oss << "*";

	for (size_t i = 0; i < params.size(); ++i)
	{
		oss << " " << sanitizeIrcText(params[i]);
	}

	if (!trailing.empty())
	{
		oss << " :" << sanitizeIrcText(trailing);
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
