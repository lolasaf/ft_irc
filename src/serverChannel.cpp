/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serverChannel.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dodordev <dodordev@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 10:00:00 by wel-safa          #+#    #+#             */
/*   Updated: 2026/01/28 10:38:26 by dodordev         ###   ########.fr       */
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

