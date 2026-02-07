/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bot.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dodordev <dodordev@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/01 19:32:43 by dodordev          #+#    #+#             */
/*   Updated: 2026/02/02 09:32:38 by dodordev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef BOT_HPP
#define BOT_HPP

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstring>
#include <ctime>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

const std::string BOT_NICKNAME = "BotDaddy";
const std::string BOT_USERNAME = "bot";
const std::string BOT_REALNAME = "IRC Helper Bot";

class Bot 
{
	private:
		int				_socket;
		std::string		server;
		int				port;
		std::string		password;
		std::string		channel;
		std::string		inputBuffer;
		std::ofstream	logFile;
		bool			registered;

	public:
		Bot(const std::string& ip, int port, const std::string& pass, const std::string& chan);
		~Bot();

		bool		connectToServer();
		void		sendRaw(const std::string& line);
		void		registerBot();
		void		joinChannel();
		void		sendMessage(const std::string& target, const std::string& message);
		void		parseMessage(const std::string& line, std::string& prefix, 
								std::string& command, std::vector<std::string>& params);
		std::string getNickFromPrefix(const std::string& prefix);
		void		logMessage(const std::string& channel, const std::string& nick,
									const std::string& message);
		void		handleHelp(const std::string& target);
		void		handleTime(const std::string& target);
		void		handleWeather(const std::string& target, const std::string& location);
		void		handleUserJoin(const std::string& nick, const std::string& channel);
		void		processLine(const std::string& line);
		void		run();
};

#endif