/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   channelMembers.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dodordev <dodordev@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/01 17:22:06 by dodordev          #+#    #+#             */
/*   Updated: 2026/02/01 17:33:27 by dodordev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "channel.hpp"
#include "user.hpp"
#include "utils.hpp"

/*
    This function adds a user to the channel's member list.
    It returns true if the user was successfully added, 
    false if the user was already a member.
*/
bool Channel::addMember(User* user)
{
    if (_members.find(user) != _members.end())
    {
        return false;
    }
    _members.insert(user);
    return true;
}

/*
    This function removes a user from the channel's member list.
    It returns true if the user was successfully removed,
    false if the user was not a member.
*/
bool Channel::removeMember(User* user)
{
    if (_members.find(user) == _members.end())
    {
        return false;
    }
    _members.erase(user);
    _operators.erase(user);
    _invited_users.erase(user);  // Clean up invite when leaving
    return true;
}

/*
    This function checks if a user is a member of the channel.
    It returns true if the user is a member, false otherwise.
*/
bool Channel::isMember(User* user) const
{
    return (_members.find(user) != _members.end());
}

/*
    This function returns the number of members in the channel.
*/
size_t Channel::getMemberCount() const
{
    return _members.size();
}

/*
    This function returns the set of members in the channel.
*/
const std::set<User*>& Channel::getMembers() const
{
    return _members;
}

/*
    This function checks if a user can join the channel based on its modes.
    First it checks the user limit, then invite-only status, and finally the channel key.
    It returns a JoinResult enum indicating the result.
*/
JoinResult Channel::canJoin(User* user, const std::string& key) const
{
    if (_user_limit > 0 && _members.size() >= _user_limit)
        return JOIN_FULL;

    if (_invite_only)
    {
        if (!isInvited(user))
            return JOIN_INVITE_ONLY;
    }

    if (!_key.empty() && key != _key)
        return JOIN_BADKEY;
    
    return JOIN_OK;
}

/*
    This function returns a string representing the NAMES list of the channel.
    Operators are prefixed with '@'.
*/
std::string Channel::getNamesList() const
{
    std::string namesList = "";
    for (std::set<User*>::iterator it = _members.begin(); it != _members.end(); it++)
    {
        if (it != _members.begin())
            namesList += " ";
        if (isOperator(*it))
            namesList += "@" + (*it)->getNickname();
        else
            namesList += (*it)->getNickname();
    }
    return namesList;
}

/*
    This function broadcasts a message to all channel members,
    optionally excluding one user.
*/
void Channel::broadcast(const std::string& message, User* exclude) const
{
    for (std::set<User*>::iterator it = _members.begin(); it != _members.end(); it++)
    {
        if (*it != exclude)
            (*it)->getOutputBuffer() += message; 
    }
}