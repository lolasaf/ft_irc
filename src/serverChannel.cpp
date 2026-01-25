/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serverChannel.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wel-safa <wel-safa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 10:00:00 by wel-safa          #+#    #+#             */
/*   Updated: 2026/01/22 10:00:00 by wel-safa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"

Channel* Server::findChannel(const std::string& name)
{
    // Convert to lowercase
    std::string nameLower = toLower(name);
    std::map<std::string, Channel*>::iterator it = channels.find(nameLower);
    if (it != channels.end())
        return it->second;
    return NULL;
}

Channel* Server::createChannel(const std::string& name)
{
    // Convert to lowercase for map key
    std::string nameLower = toLower(name);
    
    // Check if channel already exists
    std::map<std::string, Channel*>::iterator it = channels.find(nameLower);
    if (it != channels.end())
        return it->second;
    
    // Create new channel (name will be converted to lowercase in constructor)
    Channel* chan = new Channel(name);
    channels[nameLower] = chan;  // Store with lowercase key
    return chan;
}

void Server::deleteChannel(const std::string& name)
{
    // Convert to lowercase for map lookup
    std::string nameLower = toLower(name);
    std::map<std::string, Channel*>::iterator it = channels.find(nameLower);
    if (it != channels.end())
    {
        delete it->second;
        channels.erase(it);
    }
}

void Server::joinChannel(User* user, const std::string& channel, const std::string& key) {
    if (!isValidChannelName(channel)) {
        sendNumeric(user, ERR_BADCHANMASK, std::vector<std::string>(1, channel), "Bad Channel Mask");
        return;
    }
    Channel* chan = findChannel(channel);
    if (chan == NULL) {
        // Create channel if it doesn't exist (name stored in lowercase)
        chan = createChannel(channel);
        chan->addMember(user); // Add user to channel
        chan->addOperator(user); // First user becomes operator
        user->addChannel(chan);
    }
    else {
        // Channel found (case-insensitive match, stored in lowercase)
        // check if user is already in channel
        if (chan->isMember(user)) {
            sendNumeric(user, ERR_USERONCHANNEL, std::vector<std::string>(1, channel), "is already on channel");
            return;
        }
        // Check if user can join
        JoinResult res = chan->canJoin(user, key);
        if (res != JOIN_OK) {
            if (res == JOIN_INVITE_ONLY) {
                sendNumeric(user, ERR_INVITEONLYCHAN, std::vector<std::string>(1, channel), "Cannot join channel (+i)");
            }
            else if (res == JOIN_BADKEY) {
                sendNumeric(user, ERR_BADCHANNELKEY, std::vector<std::string>(1, channel), "Cannot join channel (+k)");
            }
            else if (res == JOIN_FULL) {
                sendNumeric(user, ERR_CHANNELISFULL, std::vector<std::string>(1, channel), "Cannot join channel (+l)");
            }
            return;
        }
        chan->addMember(user); // Add user to channel
        user->addChannel(chan);
        if (chan->isInviteOnly()) {
            chan->removeInvite(user->getNickname()); // Remove user from invite list
        }
    }
    // Broadcast JOIN to all members (including the joiner)
    // IRC format: :nick!user@host JOIN #channel
    std::string joinMsg = ":" + buildHostmask(user) + " JOIN " + chan->getName() + "\r\n";
    broadcastToChannel(chan, joinMsg);
    // Send topic
    // :server 331 <nick> <channel> :No topic is set
    // :server 332 <nick> <channel> :<topic>
    std::string topic = chan->getTopic();
    if (topic.empty())
        sendNumeric(user, RPL_NOTOPIC, std::vector<std::string>(1, chan->getName()), "No topic is set");
    else {
        sendNumeric(user, RPL_TOPIC, std::vector<std::string>(1, chan->getName()), topic);
        std::vector<std::string> p;
        p.push_back(chan->getName());
        p.push_back(chan->getTopicSetter());
        std::ostringstream ts;
        ts << chan->getTopicSetAt();
        p.push_back(ts.str());
        sendNumeric(user, RPL_TOPICWHOTIME, p, "");
    }
    // Send names list
    // :server 353 <nick> <symbol> <channel> :<names>
    std::vector<std::string> params;
    params.push_back("=");        // channel type; "=" is fine for ft_irc
    params.push_back(chan->getName());
    std::string namesList = chan->getNamesList();
    sendNumeric(user, RPL_NAMREPLY, params, namesList);
    // Send end of names list
    // :server 366 <nick> <channel> :End of /NAMES list
    sendNumeric(user, RPL_ENDOFNAMES, std::vector<std::string>(1, chan->getName()), "End of /NAMES list");
}

void Server::leaveChannel(User* user, Channel* chan, const std::string& partMessage)
{
    if (!chan)
        return;
    if (!chan->isMember(user)) {
        sendNumeric(user, ERR_NOTONCHANNEL, std::vector<std::string>(1, chan->getName()), "You are not on that channel");
        return;
    }
    // Broadcast PART (channel name is stored in lowercase)
    // IRC format: :nick!user@host PART #channel :message
    std::string partMsg = ":" + buildHostmask(user) + " PART " + chan->getName() + " :" + partMessage + "\r\n";
    broadcastToChannel(chan, partMsg);

    // Update both sides
    chan->removeMember(user);
    user->removeChannel(chan);

    // Delete channel if empty
    if (chan->getMemberCount() == 0) {
        deleteChannel(chan->getName());
    }
}

void Server::broadcastToChannel(Channel* chan, const std::string& msg, User* exclude)
{
	chan->broadcast(msg, exclude);
    // TODO: Check again
}

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

void Server::disconnectUser(User* user, const std::string& reason)
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