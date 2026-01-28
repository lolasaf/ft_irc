/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   user.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dodordev <dodordev@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/16 16:43:49 by wel-safa          #+#    #+#             */
/*   Updated: 2026/01/27 11:35:46 by dodordev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "user.hpp"

User::User(int clientFd) 
	: fd(clientFd),
	inputBuffer(""),
	outputBuffer(""),
	isRegistered(false),
	passOk(false),
	markedForDisconnection(false),
	nickname(""),
	username(""),
	realname(""),
	hostname(""),
	channels()	{}

// Buffers are automatically empty (std::string default)
User::~User()
{
	// Destructor can be empty for now
}

// Getters
int User::getFd() const
{
	return fd;
}

//Buffer getters
std::string& User::getInputBuffer()
{
	return inputBuffer;
}
std::string& User::getOutputBuffer()
{
	return outputBuffer;
}

bool User::getIsRegistered() const
{
	return isRegistered;
}

void User::setIsRegistered(bool reg)
{
	isRegistered = reg;
}

void User::setPassOk(bool ok)
{
	passOk = ok;
}

std::string User::getNickname() const
{
	return nickname;
}

void User::setNickname(const std::string& nick)
{
	nickname = nick;
}

std::string User::getUsername() const
{
	return username;
}

std::string User::getHostname() const
{
	return hostname;
}

void User::setUsername(const std::string& user)
{
	username = user;
}

void User::setRealname(const std::string& real)
{
	realname = real;
}

void User::setHostname(const std::string& host)
{
	hostname = host;
}

bool User::isPassOk() const
{
	return passOk;
}

void User::addChannel(Channel* channel)
{
	channels.insert(channel);
}

void User::removeChannel(Channel* channel)
{
	channels.erase(channel);
}

std::set<Channel*> User::getChannels() const
{
	return channels;
}

bool User::isMarkedForDisconnection() const
{
	return markedForDisconnection;
}

void User::markForDisconnection(bool mark)
{
	markedForDisconnection = mark;
}