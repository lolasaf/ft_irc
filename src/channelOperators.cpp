/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   channelOperators.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dodordev <dodordev@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/01 17:44:26 by dodordev          #+#    #+#             */
/*   Updated: 2026/02/01 17:46:23 by dodordev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "channel.hpp"
#include "user.hpp"
#include "utils.hpp"

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