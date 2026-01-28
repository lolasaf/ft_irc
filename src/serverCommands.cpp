/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serverCommands.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dodordev <dodordev@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/28 10:37:55 by dodordev          #+#    #+#             */
/*   Updated: 2026/01/28 11:19:53 by dodordev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"

// JOIN #channel
// JOIN #channel key
// JOIN #channel1,#channel2 key1,key2
void Server::handleJoin(User* user, const Message& msg) {
    if (!user->getIsRegistered()) {
        sendNumeric(user, ERR_NOTREGISTERED, std::vector<std::string>(), "You have not registered");
        return;
    }
    if (msg.params.size() < 1) {
        sendNumeric(user, ERR_NEEDMOREPARAMS, std::vector<std::string>(1, "JOIN"), "Not enough parameters");
        return;
    }
    if (msg.params[0] == "0") {
        // PART all channels
        std::set<Channel*> chans = user->getChannels(); // copy
        for (std::set<Channel*>::iterator it = chans.begin(); it != chans.end(); ++it)
            leaveChannel(user, *it);
        return;
    }
    std::vector<std::string> channels = splitCommaList(msg.params[0]);
    std::vector<std::string> keys;
    if (msg.params.size() > 1) {
        // // TODO: Check if we need to check ':' before the key list, I think it is handled in parsing but okay to keep it here?
        // std::string keyList = msg.params[1];
        // if (!keyList.empty() && keyList[0] == ':')
        //     keyList = keyList.substr(1);
        keys = splitCommaList(msg.params[1]);
    }
    for (size_t i = 0; i < channels.size(); i++) {
        std::string key = (i < keys.size()) ? keys[i] : "";
        joinChannel(user, channels[i], key);
    }
}

void Server::handlePart(User* user, const Message& msg) {
    if (!user->getIsRegistered()) {
        sendNumeric(user, ERR_NOTREGISTERED, std::vector<std::string>(), "You have not registered");
        return;
    }
    if (msg.params.size() < 1) {
        sendNumeric(user, ERR_NEEDMOREPARAMS, std::vector<std::string>(1, "PART"), "Not enough parameters");
        return;
    }
    // Extract optional PART message (second parameter, trailing)
    std::string partMessage = "Leaving";  // Default message
    if (msg.params.size() >= 2) {
        partMessage = msg.params[1];
    }
    // Handle multiple channels (comma-separated)
    std::vector<std::string> channels = splitCommaList(msg.params[0]);
    for (size_t i = 0; i < channels.size(); i++) {
        Channel* chan = findChannel(channels[i]);
        if (!chan) {
            // Channel doesn't exist
            sendNumeric(user, ERR_NOSUCHCHANNEL, std::vector<std::string>(1, channels[i]), "No such channel");
            continue;  // Try next channel if multiple
        }
        leaveChannel(user, chan, partMessage);
    }
}

void Server::handleDisconnect(User* user, const std::string& reason)
{
    if (!user)
        return;
    
    // Get all channels user is in (make a copy since we'll be modifying)
    std::set<Channel*> userChannels = user->getChannels();
    
    // For each channel, remove user and broadcast QUIT
    for (std::set<Channel*>::iterator it = userChannels.begin(); it != userChannels.end(); ++it)
    {
        Channel* chan = *it;
        if (!chan)
            continue;
        
        // Broadcast QUIT message to channel members
        // IRC format: :nick!user@host QUIT :reason
        std::string quitMsg = ":" + buildHostmask(user) + " QUIT :" + reason + "\r\n";
        broadcastToChannel(chan, quitMsg, user);
        
        // Remove user from channel (bidirectional cleanup)
        chan->removeMember(user);
        user->removeChannel(chan);
        
        // Delete channel if empty
        if (chan->getMemberCount() == 0)
        {
            deleteChannel(chan->getName());
        }
    }
}

void Server::handleQuit(User* user, const Message& msg)
{
    // 1. Build quit reason (use param[0] if provided, else default)
    std::string reason = "Client Quit";
    if (msg.params.size() > 0)
        reason = msg.params[0];
    
    // 2. Build the QUIT message to broadcast
    std::string quitMsg = ":" + buildHostmask(user) + " QUIT :" + reason + "\r\n";
    
    // 3. Collect all users who need to see this quit
    //    (anyone sharing a channel with this user)
    std::set<User*> notified;  // Track who we've notified (avoid duplicates)
    
    // 4. For each channel the user is in:
    for (std::map<std::string, Channel*>::iterator it = channels.begin(); it != channels.end(); ++it)
    {
        Channel* chan = it->second;
        
        if (!chan->isMember(user))
            continue;
        
        // Broadcast to all members of this channel
        const std::set<User*>& members = chan->getMembers();
        for (std::set<User*>::iterator mit = members.begin(); mit != members.end(); ++mit)
        {
            if (*mit == user)  // Don't send to quitting user
                continue;
            if (notified.find(*mit) != notified.end())  // Already notified
                continue;
            
            (*mit)->getOutputBuffer() += quitMsg;
            notified.insert(*mit);
        }
    }
    
    // 5. Mark user for disconnection (don't call handleDisconnect yet!)
    //    Let the poll loop handle cleanup after sending final data
    user->markForDisconnection(true);  // You might need to add this flag
}