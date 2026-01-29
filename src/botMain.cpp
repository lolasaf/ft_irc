/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   botMain.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dodordev <dodordev@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/29 00:00:00 by                   #+#    #+#             */
/*   Updated: 2026/01/29 15:26:25 by dodordev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "bot.hpp"
#include <iostream>
#include <cstdlib>
#include <csignal>

static Bot* g_bot = NULL;

void signalHandler(int sig)
{
    if (sig == SIGINT || sig == SIGTERM)
    {
        std::cout << "\nBot: Shutting down..." << std::endl;
        if (g_bot)
            g_bot->stop();
    }
}

void printUsage(const char* progname)
{
    std::cout << "Usage: " << progname << " <server> <port> <password> [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -n <nick>     Set bot nickname (default: ircbot)" << std::endl;
    std::cout << "  -c <channel>  Join channel (can be used multiple times)" << std::endl;
    std::cout << "  -p <prefix>   Set command prefix (default: !)" << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl;
    std::cout << "  " << progname << " localhost 6667 mypassword -n MyBot -c general -c help" << std::endl;
}

int main(int argc, char** argv)
{
    if (argc < 4)
    {
        printUsage(argv[0]);
        return 1;
    }

    std::string server = argv[1];
    int port = std::atoi(argv[2]);
    std::string password = argv[3];

    if (port <= 0 || port > 65535)
    {
        std::cerr << "Error: Invalid port number" << std::endl;
        return 1;
    }

    Bot bot(server, port, password);

    // Parse optional arguments
    for (int i = 4; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "-n" && i + 1 < argc)
        {
            bot.setNickname(argv[++i]);
        }
        else if (arg == "-c" && i + 1 < argc)
        {
            bot.addChannel(argv[++i]);
        }
        else if (arg == "-p" && i + 1 < argc)
        {
            bot.setCommandPrefix(argv[++i]);
        }
        else if (arg == "-h" || arg == "--help")
        {
            printUsage(argv[0]);
            return 0;
        }
    }

    // Set up signal handlers
    g_bot = &bot;
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::cout << "Starting IRC Bot..." << std::endl;
    std::cout << "Server: " << server << ":" << port << std::endl;

    if (!bot.start())
    {
        std::cerr << "Failed to start bot" << std::endl;
        return 1;
    }

    bot.run();

    std::cout << "Bot terminated." << std::endl;
    return 0;
}
