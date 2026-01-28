/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   user.hpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dodordev <dodordev@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/16 16:43:09 by wel-safa          #+#    #+#             */
/*   Updated: 2026/01/27 11:29:12 by dodordev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef USER_HPP
#define USER_HPP

#include <string>
#include <set>
#include "channel.hpp"

class User {
private:
	int				fd;           // Socket file descriptor
	std::string		inputBuffer;  // Data received, waiting to be parsed
	std::string		outputBuffer; // Data waiting to be sent

	bool			isRegistered;	
	bool			passOk;
	bool			markedForDisconnection;
	std::string		nickname;
	std::string		username;
	std::string		realname;
	std::string		hostname;

	// channels the user is in
	std::set<Channel*>	channels;

public:
	User(int clientFd); // Constructor: takes the client's socket fd
	~User();

	// Getters
	int				getFd() const;
	std::string&	getInputBuffer();
	std::string&	getOutputBuffer();
	std::string		getNickname() const;
	std::string		getUsername() const;
	std::string		getHostname() const;
	bool 			getIsRegistered() const;
	bool			isPassOk() const;
	bool			isMarkedForDisconnection() const;
	
	void		setPassOk(bool ok);
	void		setIsRegistered(bool reg);
	void		setNickname(const std::string& nick);
	void		setUsername(const std::string& user);
	void		setRealname(const std::string& real);
	void		setHostname(const std::string& host);

	// channel management
	void		addChannel(Channel* channel);
	void		removeChannel(Channel* channel);
	std::set<Channel*> getChannels() const;
	void		markForDisconnection(bool mark);

};

#endif