/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wel-safa <wel-safa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 10:00:00 by wel-safa          #+#    #+#             */
/*   Updated: 2026/01/30 19:03:45 by wel-safa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "channel.hpp"
#include "user.hpp"
#include "utils.hpp"

Channel::Channel(const std::string& name) : 
    _name(toLower(name)),
    _topic(""),
    _topic_setter(""),
    _topic_set_at(0),
    _user_limit(0),
    _invite_only(false),
    _topic_protection(true),
    _key(""),
    _invited_users()
{}

Channel::~Channel() {}

std::string Channel::getName() const
{
    return _name;
}

std::string Channel::getTopic() const
{
    return _topic;
}

std::string Channel::getTopicSetter() const
{
    return _topic_setter;
}

time_t Channel::getTopicSetAt() const
{
    return _topic_set_at;
}

void Channel::setTopic(const std::string& topic)
{
    _topic = topic;
}

void Channel::setTopicSetter(const std::string& setter)
{
    _topic_setter = setter;
}

void Channel::setTopicSetAt(time_t timestamp)
{
    _topic_set_at = timestamp;
}

size_t Channel::getMemberCount() const
{
    return _members.size();
}

// Mode getters/setters
bool Channel::isInviteOnly() const
{
    return _invite_only;
}

bool Channel::isTopicProtected() const
{
    return _topic_protection;
}

std::string Channel::getKey() const
{
    return _key;
}

size_t Channel::getUserLimit() const
{
    return _user_limit;
}

void Channel::setInviteOnly(bool value)
{
    _invite_only = value;
}

void Channel::setTopicProtected(bool value)
{
    _topic_protection = value;
}

void Channel::setKey(const std::string& key)
{
    _key = key;
}

void Channel::setUserLimit(size_t limit)
{
    _user_limit = limit;
}

void Channel::addInvite(User* user)
{
    _invited_users.insert(user);
}

void Channel::removeInvite(User* user)
{
    _invited_users.erase(user);
}

bool Channel::isInvited(User* user) const
{
    return _invited_users.find(user) != _invited_users.end();
}

bool Channel::addMember(User* user)
{
    if (_members.find(user) != _members.end())
    {
        return false;
    }
    _members.insert(user);
    return true;
}

bool Channel::removeMember(User* user)
{
    if (_members.find(user) == _members.end())
    {
        return false;
    }
    _members.erase(user);
    _operators.erase(user);
    _invited_users.erase(user);  // Clean up invite when leaving
    // TODO: User class should track channels, and in server class we should remove channel from user class
    return true;
}

bool Channel::isMember(User* user) const
{
    return (_members.find(user) != _members.end());
}

JoinResult Channel::canJoin(User* user, const std::string& key) const
{
    // Check user limit FIRST (applies to everyone, even invited users)
    if (_user_limit > 0 && _members.size() >= _user_limit)
        return JOIN_FULL;
    
    // Check invite-only mode
    if (_invite_only)
    {
        if (!isInvited(user))
            return JOIN_INVITE_ONLY;
        // User is invited, continue to check key
    }
    
    // Check channel key (+k)
    if (!_key.empty() && key != _key)
        return JOIN_BADKEY;
    
    return JOIN_OK;
}

const std::set<User*>& Channel::getMembers() const
{
    return _members;
}

bool Channel::addOperator(User* user)
{
    return _operators.insert(user).second;
}

bool Channel::removeOperator(User* user)
{
    return _operators.erase(user) > 0;
}

bool Channel::isOperator(User* user) const
{
    return _operators.find(user) != _operators.end();
}

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

void Channel::broadcast(const std::string& message, User* exclude) const
{
    for (std::set<User*>::iterator it = _members.begin(); it != _members.end(); it++)
    {
        if (*it != exclude)
            (*it)->getOutputBuffer() += message; 
    }
}
