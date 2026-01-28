/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serverCommandsMode.cpp                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dodordev <dodordev@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/28 11:04:56 by dodordev          #+#    #+#             */
/*   Updated: 2026/01/28 11:16:29 by dodordev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"

// Send current channel modes to user (RPL_CHANNELMODEIS 324)
void Server::sendChannelModes(User* user, Channel* chan)
{
	std::string modeStr = "+";
	std::string modeArgs = "";

	if (chan->isInviteOnly())
		modeStr += "i";
	if (chan->isTopicProtected())
		modeStr += "t";
	if (!chan->getKey().empty())
	{
		modeStr += "k";
		modeArgs += " " + chan->getKey();
	}
	if (chan->getUserLimit() > 0)
	{
		modeStr += "l";
		std::ostringstream oss;
		oss << chan->getUserLimit();
		modeArgs += " " + oss.str();
	}

	std::vector<std::string> params;
	params.push_back(chan->getName());
	params.push_back(modeStr + modeArgs);
	sendNumeric(user, RPL_CHANNELMODEIS, params, "");
}

// Apply a single mode character, returns false if error occurred
bool Server::applySingleMode(User* user, Channel* chan, char mode, bool adding,
								const Message& msg, size_t& argIndex)
{
    switch (mode)
    {
        case 'i':
            chan->setInviteOnly(adding);
            break;
            
        case 't':
            chan->setTopicProtected(adding);
            break;
            
        case 'k':
            if (adding)
            {
                if (argIndex >= msg.params.size())
                {
                    sendNumeric(user, ERR_NEEDMOREPARAMS,
                        std::vector<std::string>(1, "MODE"), "Not enough parameters for +k");
                    return false;
                }
                chan->setKey(msg.params[argIndex++]);
            }
            else
            {
                chan->setKey("");
            }
            break;
            
        case 'l':
            if (adding)
            {
                if (argIndex >= msg.params.size())
                {
                    sendNumeric(user, ERR_NEEDMOREPARAMS,
                        std::vector<std::string>(1, "MODE"), "Not enough parameters for +l");
                    return false;
                }
                size_t limit = static_cast<size_t>(atoi(msg.params[argIndex++].c_str()));
                chan->setUserLimit(limit);
            }
            else
            {
                chan->setUserLimit(0);
            }
            break;
            
        case 'o':
        {
            if (argIndex >= msg.params.size())
            {
                sendNumeric(user, ERR_NEEDMOREPARAMS,
                    std::vector<std::string>(1, "MODE"), "Not enough parameters for +o/-o");
                return false;
            }
            std::string targetNick = msg.params[argIndex++];
            User* targetUser = findUserByNick(targetNick);
            
            if (!targetUser || !chan->isMember(targetUser))
            {
                std::vector<std::string> errParams;
                errParams.push_back(targetNick);
                errParams.push_back(chan->getName());
                sendNumeric(user, ERR_USERNOTINCHANNEL, errParams, "They aren't on that channel");
                return false;
            }
            if (adding)
                chan->addOperator(targetUser);
            else
                chan->removeOperator(targetUser);
            break;
        }
            
        default:
            sendNumeric(user, ERR_NOSUCHMODE,
                std::vector<std::string>(1, std::string(1, mode)), "Unknown MODE flag");
            return false;
    }
    return true;
}

// Parse mode string and apply all modes
bool Server::applyChannelModes(User* user, Channel* chan, const Message& msg, size_t& argIndex)
{
    std::string modeString = msg.params[1];
    bool adding = true;
    
    for (size_t i = 0; i < modeString.size(); ++i)
    {
        char c = modeString[i];
        
        if (c == '+') { adding = true; continue; }
        if (c == '-') { adding = false; continue; }
        
        if (!applySingleMode(user, chan, c, adding, msg, argIndex))
            return false;
    }
    return true;
}

// Main MODE handler
void Server::handleMode(User* user, const Message& msg)
{
    // 1. Registration check
    if (!user->getIsRegistered())
    {
        sendNumeric(user, ERR_NOTREGISTERED, std::vector<std::string>(), "You have not registered");
        return;
    }
    
    // 2. Need at least target (channel name)
    if (msg.params.size() < 1)
    {
        sendNumeric(user, ERR_NEEDMOREPARAMS, std::vector<std::string>(1, "MODE"), "Not enough parameters");
        return;
    }
    
    std::string target = msg.params[0];
    
    // 3. Check if it's a channel (starts with #)
    if (target[0] != '#')
    {
        // User mode — ignore for ft_irc
        return;
    }
    
    // 4. Find the channel
    Channel* chan = findChannel(target);
    if (!chan)
    {
        sendNumeric(user, ERR_NOSUCHCHANNEL, std::vector<std::string>(1, target), "No such channel");
        return;
    }
    
    // 5. Query modes if no mode string provided
    if (msg.params.size() == 1)
    {
        sendChannelModes(user, chan);
        return;
    }
    
    // 6. Must be operator to change modes
    if (!chan->isOperator(user))
    {
        sendNumeric(user, ERR_CHANOPRIVSNEEDED, std::vector<std::string>(1, chan->getName()), "You're not channel operator");
        return;
    }
    
    // 7. Apply modes
    size_t argIndex = 2;
    if (!applyChannelModes(user, chan, msg, argIndex))
        return;
    
    // 8. Broadcast MODE change to channel
    std::string modeMsg = ":" + buildHostmask(user) + " MODE " + chan->getName() + " " + msg.params[1];
    for (size_t i = 2; i < argIndex; ++i)
        modeMsg += " " + msg.params[i];
    modeMsg += "\r\n";
    broadcastToChannel(chan, modeMsg);
}