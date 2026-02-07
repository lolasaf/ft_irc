/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   botParsingUtils.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dodordev <dodordev@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/02 10:15:27 by dodordev          #+#    #+#             */
/*   Updated: 2026/02/06 13:31:49 by dodordev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "bot.hpp"

/*
	Parse a raw IRC message line into prefix, command, and parameters.
*/
void Bot::parseMessage(const std::string& line, std::string& prefix, std::string& command, std::vector<std::string>& params)
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