/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   botHandlers.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dodordev <dodordev@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/02 10:07:41 by dodordev          #+#    #+#             */
/*   Updated: 2026/02/02 10:08:50 by dodordev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "bot.hpp"

void Bot::handleHelp(const std::string& target)
{
	sendMessage(target, "Available commands:");
	sendMessage(target, "  !help    - Show this help message");
	sendMessage(target, "  !time    - Show current server time");
	sendMessage(target, "  !weather <city> - Get weather for a city");
}

void Bot::handleTime(const std::string& target)
{
	std::time_t now = std::time(NULL);
	std::string timeStr = std::ctime(&now);
	timeStr.erase(timeStr.find_last_not_of("\n") + 1);
	sendMessage(target, "Current server time: " + timeStr);
}

void Bot::handleWeather(const std::string& target, const std::string& city)
{
	if (city.empty())
	{
		sendMessage(target, "Usage: !weather <city>");
		return;
	}

	int temperature = rand() % 35;
	std::string conditions = (rand() % 2 == 0) ? "Sunny" : "Cloudy";

	std::ostringstream oss;
	oss << temperature;
	sendMessage(target, "Weather in " + city + ": " + conditions + ", " + oss.str() + " C");

}

void Bot::handleUserJoin(const std::string& nick, const std::string& channel)
{
	if (nick == BOT_NICKNAME)
		return;

	sendMessage(channel, "Welcome to " + channel + ", " + nick + "!");
}