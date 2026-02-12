/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   botParsingUtils.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dodordev <dodordev@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/02 10:15:27 by dodordev          #+#    #+#             */
/*   Updated: 2026/02/12 18:06:33 by dodordev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "bot.hpp"

/*
	Parse a raw IRC message line into prefix, command, and parameters.
	IRC message format:
		[:prefix] command [params] :trailing
	Parameters:
		- line: the raw message line to parse
		- prefix: output parameter to store the prefix (if present)
		- command: output parameter to store the command
		- params: output vector to store parameters (including trailing)
	Returns:
		- void, but fills the output parameters with parsed data.
*/
void Bot::parseMessage(const std::string& line, std::string& prefix, 
						std::string& command, std::vector<std::string>& params)
{
	std::string::size_type pos = 0;

	if (line.empty() || pos >= line.size())
		return;

	if (line[pos] == ':')
	{
		std::string::size_type prefixEnd = line.find(' ', pos);
		if (prefixEnd == std::string::npos)
			return;
		prefix = line.substr(1, prefixEnd - 1);
		pos = prefixEnd + 1;
	}

	while (pos < line.size() && line[pos] == ' ')
		pos++;
	std::string::size_type cmdEnd = line.find(' ', pos);
	if (cmdEnd == std::string::npos)
	{
		command = line.substr(pos);
		return;
	}
	command = line.substr(pos, cmdEnd - pos);
	pos = cmdEnd + 1;

	while (pos < line.size())
	{
		while (pos < line.size() && line[pos] == ' ')
			pos++;
		if (pos >= line.size())
			break;
		if (line[pos] == ':')
		{
			params.push_back(line.substr(pos + 1));
			break;
		}
		std::string::size_type paramEnd = line.find(' ', pos);
		if (paramEnd == std::string::npos)
		{
			params.push_back(line.substr(pos));
			break;
		}
		params.push_back(line.substr(pos, paramEnd - pos));
		pos = paramEnd + 1;
	}
}

/*
	Extract nickname from IRC prefix (nick!user@host).
*/
std::string Bot::getNickFromPrefix(const std::string& prefix)
{
	std::string::size_type exclamPos = prefix.find('!');
	if (exclamPos == std::string::npos)
		return prefix;
	return prefix.substr(0, exclamPos);
}

/*
	Log a message to the log file with timestamp, channel, nick, and message.
*/
void Bot::logMessage(const std::string& channel, const std::string& nick, const std::string& message)
{
	if (!logFile.is_open())
		return;

	std::time_t now = std::time(NULL);
	std::string timeStr = std::ctime(&now);

	if (!timeStr.empty() && timeStr[timeStr.size() - 1] == '\n')
		timeStr.erase(timeStr.size() - 1);
	logFile << "[" << timeStr << "] " << channel << " <" << nick << "> " << message << std::endl;
}

/*
	Parse a raw IRC message line into prefix, command, and parameters.
*/
void Bot::processLine(const std::string& line)
{
	std::cout << "<< " << line << std::endl;
	
	std::string prefix, command;
	std::vector<std::string> params;
	parseMessage(line, prefix, command, params);

	if (command == "PING")
	{
		if (!params.empty())
			sendRaw("PONG :" + params[0]);
		return;
	}

	if (command == "001")
	{
		registered = true;
		joinChannel();
		return;
	}

	// Handle INVITE - bot auto-joins when invited to a channel
	if (command == "INVITE" && params.size() >= 2)
	{
		std::string invitedTo = params[1];
		std::string inviter = getNickFromPrefix(prefix);
		
		std::cout << "INFO: Invited to " << invitedTo << " by " << inviter << std::endl;
		std::cout << "      Auto-joining..." << std::endl;
		
		sendRaw("JOIN " + invitedTo);
		return;
	}

	// Handle JOIN errors - 473 ERR_INVITEONLYCHAN
	if (command == "473")
	{
		std::string chan = params.size() > 1 ? params[1] : channel;
		std::cerr << std::endl;
		std::cerr << "ERROR: Cannot join " << chan << " - Channel is invite only (+i)" << std::endl;
		std::cerr << "       Ask an operator to invite the bot:" << std::endl;
		std::cerr << "       /invite " << BOT_NICKNAME << " " << chan << std::endl;
		std::cerr << std::endl;
		return;
	}

	// Handle JOIN errors - 475 ERR_BADCHANNELKEY
	if (command == "475")
	{
		std::string chan = params.size() > 1 ? params[1] : channel;
		std::cerr << std::endl;
		std::cerr << "ERROR: Cannot join " << chan << " - Bad/missing channel key (+k)" << std::endl;
		std::cerr << "       Restart bot with correct key:" << std::endl;
		std::cerr << "       ./ircbot <server> <port> <password> " << chan << " <key>" << std::endl;
		std::cerr << std::endl;
		return;
	}

	// Handle JOIN errors - 471 ERR_CHANNELISFULL
	if (command == "471")
	{
		std::string chan = params.size() > 1 ? params[1] : channel;
		std::cerr << std::endl;
		std::cerr << "ERROR: Cannot join " << chan << " - Channel is full (+l)" << std::endl;
		std::cerr << "       Wait for a slot to open or ask operator to increase limit." << std::endl;
		std::cerr << std::endl;
		return;
	}

	// Handle other JOIN errors - 403 ERR_NOSUCHCHANNEL
	if (command == "403")
	{
		std::string chan = params.size() > 1 ? params[1] : channel;
		std::cerr << std::endl;
		std::cerr << "ERROR: Cannot join " << chan << " - No such channel" << std::endl;
		std::cerr << "       Check the channel name and try again." << std::endl;
		std::cerr << std::endl;
		return;
	}

	if (command == "JOIN" && !prefix.empty())
	{
		std::string nick = getNickFromPrefix(prefix);
		std::string chan = params.empty() ? "" : params[0];
		handleUserJoin(nick, chan);
		return;
	}

	if (command == "PRIVMSG" && params.size() >= 2)
	{
		std::string nick = getNickFromPrefix(prefix);
		std::string target = params[0];
		std::string text = params[1];

		if (!target.empty() && target[0] == '#')
			logMessage(target, nick, text);

		std::string responseTarget;
		if (!target.empty() && target[0] == '#')
			responseTarget = target;
		else
			responseTarget = nick;

		if (text.length() >= 5 && text.substr(0, 5) == "!help")
		{
			handleHelp(responseTarget);
		}
		else if (text.length() >= 5 && text.substr(0, 5) == "!time")
		{
			handleTime(responseTarget);
		}
		else if (text.length() >= 8 && text.substr(0, 8) == "!weather")
		{
			std::string city;
			if (text.length() > 9)
				city = text.substr(9);
			handleWeather(responseTarget, city);
		}
	}
}