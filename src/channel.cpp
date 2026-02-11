/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wel-safa <wel-safa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 10:00:00 by wel-safa          #+#    #+#             */
/*   Updated: 2026/02/11 22:34:47 by wel-safa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "channel.hpp"
#include "user.hpp"
#include "utils.hpp"

/*
    Constructor: initializes channel with: 
        - name (converted to lowercase),
        - empty topic,
        - no topic setter,
        - topic set time 0,
        - user limit 0,
        - invite-only mode off,
        - topic protection on,
        - empty key,
        - empty invited users list.
*/
Channel::Channel(const std::string& name) : 
    _name(toLower(name)),
    _topic(""),
    _topic_setter(""),
    _topic_set_at(0),
    _user_limit(0),
    _invite_only(false),
    _topic_protection(false),
    _key(""),
    _invited_users()
{}

Channel::~Channel() {}

std::string Channel::getName() const
{
    return _name;
}
