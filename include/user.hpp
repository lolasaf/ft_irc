/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   user.hpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wel-safa <wel-safa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/16 16:43:09 by wel-safa          #+#    #+#             */
/*   Updated: 2026/01/17 17:15:11 by wel-safa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef USER_HPP
#define USER_HPP

#include <string>

class User {
private:
	int				fd;           // Socket file descriptor
	std::string		inputBuffer;  // Data received, waiting to be parsed
	std::string		outputBuffer; // Data waiting to be sent

	bool			isRegistered;	
	bool			passOk;
	std::string		nickname;
	std::string		username;
	std::string		realname;
	std::string		hostname;

public:
	User(int clientFd); // Constructor: takes the client's socket fd
	~User();

	// Getters
	int				getFd() const;
	std::string&	getInputBuffer();
	std::string&	getOutputBuffer();
	std::string		getNickname() const;
	std::string		getUsername() const;
	bool 			getIsRegistered() const;
	bool			isPassOk() const;
	
	
	void		setPassOk(bool ok);
	void		setIsRegistered(bool reg);
	void		setNickname(const std::string& nick);
	void		setUsername(const std::string& user);
	void		setRealname(const std::string& real);
	void		setHostname(const std::string& host);
};

#endif