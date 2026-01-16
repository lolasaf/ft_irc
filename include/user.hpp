/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   user.hpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wel-safa <wel-safa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/16 16:43:09 by wel-safa          #+#    #+#             */
/*   Updated: 2026/01/16 16:43:13 by wel-safa         ###   ########.fr       */
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

	// TODO: [LATER] — Add nickname, username, registration state, etc.
	bool			passOk;
	std::string		nickname;
	std::string		username;
	std::string		realname;
	std::string		hostname;

public:
	User(int clientFd); // Constructor: takes the client's socket fd
	~User();

	// Getters
	int getFd() const;
	std::string& getInputBuffer();
	std::string& getOutputBuffer();

	bool isRegistered() const;
	void setPassOk(bool ok);
	std::string getNickname() const;
	void setNickname(const std::string& nick);
	std::string getUsername() const;
	void setUsername(const std::string& user);
	void setRealname(const std::string& real);
	void setHostname(const std::string& host);
};

#endif