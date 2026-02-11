/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serverCommandsMode.cpp                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wel-safa <wel-safa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/28 11:04:56 by dodordev          #+#    #+#             */
/*   Updated: 2026/02/11 22:04:51 by wel-safa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"

// Send current channel modes to user (RPL_CHANNELMODEIS 324)
void Server::sendChannelModes(User* user, Channel* chan)
{
	std::string modeStr = "+";
	std::vector<std::string> modeArgs;

	if (chan->isInviteOnly())
		modeStr += "i";
	if (chan->isTopicProtected())
		modeStr += "t";
	if (!chan->getKey().empty())
	{
		modeStr += "k";
		// Mask key value (common IRC behavior - don't expose actual key)
		modeArgs.push_back("*");
	}
	if (chan->getUserLimit() > 0)
	{
		modeStr += "l";
		std::ostringstream oss;
		oss << chan->getUserLimit();
		modeArgs.push_back(oss.str());
	}

	std::vector<std::string> params;
	params.push_back(chan->getName());
	params.push_back(modeStr);
	for (size_t i = 0; i < modeArgs.size(); ++i)
		params.push_back(modeArgs[i]);
	sendNumeric(user, RPL_CHANNELMODEIS, params, "");
}

// Apply a single mode character, returns false if error occurred
bool Server::applySingleMode(User* user, Channel* chan, char mode, bool adding,
								const Message& msg, size_t& argIndex)
{
    switch (mode)
    {
        case 'i':
            // Only change if state is different (idempotent check)
            if (chan->isInviteOnly() == adding)
                return false;  // No change needed, don't broadcast
            chan->setInviteOnly(adding);
            break;
            
        case 't':
            // Only change if state is different (idempotent check)
            if (chan->isTopicProtected() == adding)
                return false;  // No change needed, don't broadcast
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
                // Sanitize key to prevent IRC protocol injection via CR/LF
                std::string key = sanitizeIrcText(msg.params[argIndex++]);
                if (key.empty())
                {
                    sendNumeric(user, ERR_NEEDMOREPARAMS,
                        std::vector<std::string>(1, "MODE"), "Invalid key for +k");
                    return false;
                }
                // Only change if key is different (idempotent check)
                if (chan->getKey() == key)
                    return false;  // Same key, don't broadcast
                chan->setKey(key);
            }
            else
            {
                // Only change if key exists (idempotent check)
                if (chan->getKey().empty())
                    return false;  // No key to remove, don't broadcast
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
                const std::string& limitStr = msg.params[argIndex++];
                // Validate: must be non-empty and contain only digits
                if (limitStr.empty() || limitStr.find_first_not_of("0123456789") != std::string::npos)
                {
                    sendNumeric(user, ERR_NEEDMOREPARAMS,
                        std::vector<std::string>(1, "MODE"), "Invalid limit for +l (must be positive integer)");
                    return false;
                }
                long limit = std::strtol(limitStr.c_str(), NULL, 10);
                // Validate range: must be 1-10000
                if (limit < 1 || limit > 10000)
                {
                    sendNumeric(user, ERR_NEEDMOREPARAMS,
                        std::vector<std::string>(1, "MODE"), "Limit must be between 1 and 10000");
                    return false;
                }
                size_t newLimit = static_cast<size_t>(limit);
                // Only change if limit is different (idempotent check)
                if (chan->getUserLimit() == newLimit)
                    return false;  // Same limit, don't broadcast
                chan->setUserLimit(newLimit);
            }
            else
            {
                // Only change if limit exists (idempotent check)
                if (chan->getUserLimit() == 0)
                    return false;  // No limit to remove, don't broadcast
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
            
            // Check if user exists
            if (!targetUser)
            {
                sendNumeric(user, ERR_NOSUCHNICK,
                    std::vector<std::string>(1, targetNick), "No such nick/channel");
                return false;
            }
            
            // Check if user is on the channel
            if (!chan->isMember(targetUser))
            {
                std::vector<std::string> errParams;
                errParams.push_back(targetNick);
                errParams.push_back(chan->getName());
                sendNumeric(user, ERR_USERNOTINCHANNEL, errParams, "They aren't on that channel");
                return false;
            }
            if (adding)
                return chan->addOperator(targetUser);
            else
                return chan->removeOperator(targetUser);
            break;
        }
            
        default:
            sendNumeric(user, ERR_UNKNOWNMODE,
                std::vector<std::string>(1, std::string(1, mode)), "Unknown MODE flag");
            return false;
    }
    return true;
}

// Parse mode string and apply all modes
// Returns the successfully applied mode string and consumed args via output params
// Continues processing even if some modes fail, to avoid partial state without broadcast
void Server::applyChannelModes(User* user, Channel* chan, const Message& msg, 
                                std::string& appliedModes, std::vector<std::string>& appliedArgs)
{
    std::string modeString = msg.params[1];
    bool adding = true;
    bool lastWasPlus = true;  // Track last direction for output
    size_t argIndex = 2;
    
    appliedModes = "";
    appliedArgs.clear();
    
    for (size_t i = 0; i < modeString.size(); ++i)
    {
        char c = modeString[i];
        
        if (c == '+') { adding = true; continue; }
        if (c == '-') { adding = false; continue; }
        
        // Remember argIndex before applying (to know if an arg was consumed)
        size_t argBefore = argIndex;
        
        if (applySingleMode(user, chan, c, adding, msg, argIndex))
        {
            // Mode succeeded - add to applied list
            // Add +/- prefix if direction changed
            if (appliedModes.empty() || (adding && !lastWasPlus) || (!adding && lastWasPlus))
            {
                appliedModes += (adding ? '+' : '-');
                lastWasPlus = adding;
            }
            appliedModes += c;
            
            // If an argument was consumed, add it to the list
            for (size_t j = argBefore; j < argIndex; ++j)
            {
                // Sanitize arguments before recording them to avoid reintroducing
                // any CR/LF or other unsafe characters into the broadcast.
                appliedArgs.push_back(sanitizeIrcText(msg.params[j]));
            }
        }
        // On failure, error was already sent by applySingleMode, continue with next mode
    }
}

// Main MODE handler
void Server::handleMode(User* user, const Message& msg)
{
    // 1. Registration and params check
    if (!requireRegistered(user)) return;
    if (!requireParams(user, msg, 1, "MODE")) return;
    
    std::string target = msg.params[0];
    
    // 2. Guard against empty target (e.g., "MODE :" produces empty param)
    if (target.empty())
    {
        sendNumeric(user, ERR_NEEDMOREPARAMS, std::vector<std::string>(1, "MODE"), "Not enough parameters");
        return;
    }
    
    // 3. Check if it's a channel (starts with #)
    if (target[0] != '#') // User mode — ignore for ft_irc (silently return)
    {
        sendNumeric(user, ERR_NOSUCHCHANNEL, std::vector<std::string>(1, target), "No such channel");
        return;
    }

    // 4. Find the channel
    Channel* chan = requireChannel(user, target);
    if (!chan) return;
    
    // 5. Query modes if no mode string provided
    if (msg.params.size() == 1)
    {
        // Require membership to query modes (protects channel key from leaking)
        if (!requireOnChannel(user, chan)) return;
        sendChannelModes(user, chan);
        return;
    }
    
    // 6. Must be operator to change modes
    if (!requireOperator(user, chan)) return;
    
    // 7. Apply modes and track what succeeded
    std::string appliedModes;
    std::vector<std::string> appliedArgs;
    applyChannelModes(user, chan, msg, appliedModes, appliedArgs);
    
    // 8. Broadcast only if at least one mode was successfully applied
    if (!appliedModes.empty())
    {
        std::string modeMsg = ":" + buildHostmask(user) + " MODE " + chan->getName() + " " + appliedModes;
        for (size_t i = 0; i < appliedArgs.size(); ++i)
            modeMsg += " " + appliedArgs[i];
        modeMsg += "\r\n";
        broadcastToChannel(chan, modeMsg);
    }
}