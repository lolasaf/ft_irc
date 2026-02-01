/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   channelModes.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dodordev <dodordev@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/01 17:34:15 by dodordev          #+#    #+#             */
/*   Updated: 2026/02/01 17:39:16 by dodordev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "channel.hpp"
#include "user.hpp"
#include "utils.hpp"

/*
	This function checks if the channel is in invite-only mode (+i).
	@return True if the channel is invite-only, false otherwise.
*/
bool Channel::isInviteOnly() const
{
    return _invite_only;
}

/*
	This function sets the invite-only mode (+i) for the channel.
	@param value True to enable invite-only mode, false to disable it.
*/
void Channel::setInviteOnly(bool value)
{
    _invite_only = value;
}

/*
	This function checks if the channel's topic is protected (+t).
	@return True if the topic is protected, false otherwise.
*/
bool Channel::isTopicProtected() const
{
    return _topic_protection;
}

/*
	This function sets the topic protection mode (+t) for the channel.
	@param value True to enable topic protection, false to disable it.
*/
void Channel::setTopicProtected(bool value)
{
    _topic_protection = value;
}

/*
	This function gets the channel key (+k).
	@return The channel key as a string.
*/
std::string Channel::getKey() const
{
    return _key;
}

/*
	This function sets the channel key (+k).
	@param key The key to set for the channel.
*/
void Channel::setKey(const std::string& key)
{
    _key = key;
}

/*
	This function gets the user limit (+l) for the channel.
	@return The user limit as a size_t.
*/
size_t Channel::getUserLimit() const
{
    return _user_limit;
}

/*
	This function sets the user limit (+l) for the channel.
	@param limit The user limit to set.
*/
void Channel::setUserLimit(size_t limit)
{
    _user_limit = limit;
}