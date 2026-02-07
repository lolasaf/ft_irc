/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   channelTopic.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dodordev <dodordev@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/01 17:40:26 by dodordev          #+#    #+#             */
/*   Updated: 2026/02/01 17:43:31 by dodordev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "channel.hpp"
#include "user.hpp"
#include "utils.hpp"

std::string Channel::getTopic() const
{
    return _topic;
}

void Channel::setTopic(const std::string& topic)
{
    _topic = topic;
}

std::string Channel::getTopicSetter() const
{
    return _topic_setter;
}

void Channel::setTopicSetter(const std::string& setter)
{
    _topic_setter = setter;
}

time_t Channel::getTopicSetAt() const
{
    return _topic_set_at;
}

void Channel::setTopicSetAt(time_t timestamp)
{
    _topic_set_at = timestamp;
}