/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serverCommands.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wel-safa <wel-safa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/28 10:37:55 by dodordev          #+#    #+#             */
/*   Updated: 2026/01/30 19:32:42 by wel-safa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"
#include <ctime>

// JOIN #channel
// JOIN #channel key
// JOIN #channel1,#channel2 key1,key2
void Server::handleJoin(User* user, const Message& msg) {
    if (!requireRegistered(user)) return;
    if (!requireParams(user, msg, 1, "JOIN")) return;
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
        keys = splitCommaList(msg.params[1]);
    }
    for (size_t i = 0; i < channels.size(); i++) {
        std::string key = (i < keys.size()) ? keys[i] : "";
        joinChannel(user, channels[i], key);
    }
}

void Server::handlePart(User* user, const Message& msg) {
    if (!requireRegistered(user)) return;
    if (!requireParams(user, msg, 1, "PART")) return;
    // Extract optional PART message (second parameter, trailing)
    // Sanitize to prevent IRC protocol injection via CR/LF
    std::string partMessage = "Leaving";  // Default message
    if (msg.params.size() >= 2) {
        partMessage = sanitizeIrcText(msg.params[1]);
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
    // If QUIT message has not been broadcast yet, broadcast it
    // Use stored quit reason if available, otherwise use provided reason
    if (!user->wasQuitBroadcast()) 
        broadcastQuit(user, user->getQuitReason().empty() ? reason : user->getQuitReason());
    // Always clean up channel memberships
    std::set<Channel*> userChannels = user->getChannels();
    for (std::set<Channel*>::iterator it = userChannels.begin(); it != userChannels.end(); ++it)
    {
        Channel* chan = *it;
        if (!chan)
            continue;
        // Remove user from channel (bidirectional cleanup)
        chan->removeMember(user);
        user->removeChannel(chan);
        // Delete channel if empty
        if (chan->getMemberCount() == 0)
            deleteChannel(chan->getName());
    }
    // Clean up any pending invitations for this user across ALL channels
    // (user may have been invited to channels they haven't joined yet)
    for (std::map<std::string, Channel*>::iterator it = channels.begin(); it != channels.end(); ++it)
        it->second->removeInvite(user);
}

void Server::broadcastQuit(User* user, const std::string& reason)
{
    // Build QUIT message
    std::string quitMsg = ":" + buildHostmask(user) + " QUIT :" + reason + "\r\n";
    // Track who we've notified (avoid duplicates)
    std::set<User*> notified;
    // Get user's channels
    std::set<Channel*> userChannels = user->getChannels();
    // Iterate over user's channels
    for (std::set<Channel*>::const_iterator it = userChannels.begin(); it != userChannels.end(); ++it)
    {
        Channel* chan = *it;
        if (!chan)
            continue;
        // Broadcast to all members of this channel
        const std::set<User*>& members = chan->getMembers();
        for (std::set<User*>::const_iterator mit = members.begin(); mit != members.end(); ++mit)
        {
            if (*mit == user) // Don't send to quitting user
                continue;
            if (notified.find(*mit) != notified.end()) // Already notified
                continue;
            (*mit)->getOutputBuffer() += quitMsg; // Add QUIT message to recipient's output buffer
            notified.insert(*mit);
        }
    }
}

void Server::handleQuit(User* user, const Message& msg)
{
    // Build quit reason (use param[0] if provided, else default)
    //    Sanitize to prevent IRC protocol injection via CR/LF
    std::string reason = "Client Quit";
    if (msg.params.size() > 0)
        reason = sanitizeIrcText(msg.params[0]);
    // Broadcast QUIT message to all users sharing channels
    broadcastQuit(user, reason);
    // Mark user for disconnection with reason and broadcast flag
    //    Let the poll loop handle cleanup after sending final data
    user->markForDisconnection(true, reason, true);  // broadcast already done
}

void Server::handleTopic(User* user, const Message& msg)
{
    if (!requireRegistered(user)) return;
    if (!requireParams(user, msg, 1, "TOPIC")) return;
    
    Channel* chan = requireChannel(user, msg.params[0]);
    if (!chan) return;
    if (!requireOnChannel(user, chan)) return;
    
    // Query topic (no second param)
    if (msg.params.size() == 1)
    {
        sendTopicInfo(user, chan);
        return;
    }
    
    // Set topic - check +t mode (topic protection)
    if (chan->isTopicProtected() && !chan->isOperator(user))
    {
        sendNumeric(user, ERR_CHANOPRIVSNEEDED, std::vector<std::string>(1, chan->getName()), 
                    "You're not channel operator");
        return;
    }
    
    // 7. Set the new topic (sanitize to prevent IRC protocol injection)
    std::string newTopic = sanitizeIrcText(msg.params[1]);
    // TODO: Check trim again with tests
    // Trim whitespace - if empty or whitespace-only after trimming, treat as clearing topic
    newTopic = trim(newTopic);
    chan->setTopic(newTopic);
    chan->setTopicSetter(user->getNickname());  // who set it
    chan->setTopicSetAt(std::time(NULL));      // when
    
    // 8. Broadcast TOPIC change to all channel members
    // Format: :nick!user@host TOPIC #channel :new topic
    std::string topicMsg = ":" + buildHostmask(user) + " TOPIC " + chan->getName() + " :" + newTopic + "\r\n";
    broadcastToChannel(chan, topicMsg);
}

void Server::handleInvite(User* user, const Message& msg)
{
    if (!requireRegistered(user)) return;
    if (!requireParams(user, msg, 2, "INVITE")) return;
    
    std::string targetNick = msg.params[0];
    
    User* target = requireUser(user, targetNick);
    if (!target) return;
    
    Channel* chan = requireChannel(user, msg.params[1]);
    if (!chan) return;
    if (!requireOnChannel(user, chan)) return;
    if (!requireOperator(user, chan)) return;
    
    // Check if target is already on the channel
    if (chan->isMember(target))
    {
        std::vector<std::string> userOnChanParams;
        userOnChanParams.push_back(targetNick);
        userOnChanParams.push_back(chan->getName());
        sendNumeric(user, ERR_USERONCHANNEL, userOnChanParams, "is already on channel");
        return;
    }
    
    // 8. Add target to invitation list (for +i bypass)
    //    Store User* pointer for stable identity (survives NICK changes,
    //    automatically invalid on disconnect)
    chan->addInvite(target);
    
    // 9. Send RPL_INVITING (341) to inviter
    // Format: :server 341 <inviter> <target> <#channel>
    std::vector<std::string> params;
    params.push_back(targetNick);  // target nickname
    params.push_back(chan->getName());  // channel name
    sendNumeric(user, RPL_INVITING, params, "");
    
    // 10. Send INVITE notification to target
    // Format: :nick!user@host INVITE <target> <#channel>
    std::string inviteMsg = ":" + buildHostmask(user) + " INVITE " 
                          + targetNick + " " + chan->getName() + "\r\n";
    target->getOutputBuffer() += inviteMsg;
}

void Server::handleKick(User* user, const Message& msg)
{
    if (!requireRegistered(user)) return;
    if (!requireParams(user, msg, 2, "KICK")) return;
    
    std::string targetNick = msg.params[1];
    // Sanitize reason to prevent IRC protocol injection via CR/LF
    std::string reason = (msg.params.size() > 2) ? sanitizeIrcText(msg.params[2]) : user->getNickname();
    
    Channel* chan = requireChannel(user, msg.params[0]);
    if (!chan) return;
    if (!requireOnChannel(user, chan)) return;
    if (!requireOperator(user, chan)) return;
    
    User* target = requireUser(user, targetNick);
    if (!target) return;
    
    // Check if target is on the channel
    if (!chan->isMember(target))
    {
        std::vector<std::string> userNotInChanParams;
        userNotInChanParams.push_back(targetNick);
        userNotInChanParams.push_back(chan->getName());
        sendNumeric(user, ERR_USERNOTINCHANNEL, userNotInChanParams, "is not on channel");
        return;
    }
    
    // 8. Broadcast KICK to channel (target sees it too)
    // Format: :nick!user@host KICK #channel target :reason
    std::string kickMsg = ":" + buildHostmask(user) + " KICK " 
                        + chan->getName() + " " + targetNick 
                        + " :" + reason + "\r\n";
    broadcastToChannel(chan, kickMsg);
    
    // 9. Remove target from channel (bidirectional)
    chan->removeMember(target);
    target->removeChannel(chan);
    
    // 10. Delete channel if empty
    if (chan->getMemberCount() == 0)
    {
        deleteChannel(chan->getName());
    }
}