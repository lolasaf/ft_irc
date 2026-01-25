/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serverUtils.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wel-safa <wel-safa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 08:33:00 by wel-safa          #+#    #+#             */
/*   Updated: 2026/01/23 08:33:00 by wel-safa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"
/**
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

/**
 * This function checks if a channel name is valid
 * @param name The string to check if it is a valid channel name
 * @return True if the string is a valid channel name, false otherwise
 * must start with '#', not be empty, be less than 50 characters, and must not contain space, comma, colon
 */
bool Server::isValidChannelName(const std::string& name)
{
	return name.length() > 1 && name[0] == '#' && name.length() <= CHANNELLEN && name.find_first_of(" ,:") == std::string::npos;
}

/**
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
