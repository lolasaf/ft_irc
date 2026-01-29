/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bot.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dodordev <dodordev@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/29 00:00:00 by                   #+#    #+#             */
/*   Updated: 2026/01/29 15:05:28 by dodordev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "bot.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include <iostream>
#include <cerrno>

// ============================================================================
// Constructor / Destructor
// ============================================================================

Bot::Bot(const std::string& server, int port, const std::string& password)
    : _socket(-1),
      _server(server),
      _port(port),
      _password(password),
      _nickname("ircbot"),
      _username("ircbot"),
      _realname("ft_irc Bot"),
      _commandPrefix("!"),
      _inputBuffer(""),
      _running(false),
      _registered(false),
      _lastCommandTime(0)
{
    std::srand(static_cast<unsigned>(std::time(NULL)));
}

Bot::~Bot()
{
    disconnect();
}

// ============================================================================
// Configuration
// ============================================================================

void Bot::setNickname(const std::string& nick)
{
    _nickname = nick;
}

void Bot::setUsername(const std::string& user)
{
    _username = user;
}

void Bot::setRealname(const std::string& real)
{
    _realname = real;
}

void Bot::setCommandPrefix(const std::string& prefix)
{
    _commandPrefix = prefix;
}

void Bot::addChannel(const std::string& channel)
{
    std::string chan = channel;
    if (!chan.empty() && chan[0] != '#')
        chan = "#" + chan;
    _channels.push_back(chan);
}

// ============================================================================
// Connection Management
// ============================================================================

bool Bot::connectToServer()
{
    struct addrinfo hints, *res, *p;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    std::ostringstream portStr;
    portStr << _port;

    int status = getaddrinfo(_server.c_str(), portStr.str().c_str(), &hints, &res);
    if (status != 0)
    {
        std::cerr << "Bot: getaddrinfo error: " << gai_strerror(status) << std::endl;
        return false;
    }

    for (p = res; p != NULL; p = p->ai_next)
    {
        _socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (_socket < 0)
            continue;

        if (connect(_socket, p->ai_addr, p->ai_addrlen) == 0)
            break;

        close(_socket);
        _socket = -1;
    }

    freeaddrinfo(res);

    if (_socket < 0)
    {
        std::cerr << "Bot: Failed to connect to " << _server << ":" << _port << std::endl;
        return false;
    }

    std::cout << "Bot: Connected to " << _server << ":" << _port << std::endl;
    return true;
}

void Bot::disconnect()
{
    if (_socket >= 0)
    {
        close(_socket);
        _socket = -1;
    }
    _running = false;
    _registered = false;
}

bool Bot::sendRaw(const std::string& message)
{
    if (_socket < 0)
        return false;

    std::string msg = message + "\r\n";
    ssize_t sent = send(_socket, msg.c_str(), msg.size(), 0);
    if (sent < 0)
    {
        std::cerr << "Bot: Send error: " << strerror(errno) << std::endl;
        return false;
    }
    std::cout << "Bot >> " << message << std::endl;
    return true;
}

// ============================================================================
// IRC Protocol
// ============================================================================

void Bot::sendPass()
{
    if (!_password.empty())
        sendRaw("PASS " + _password);
}

void Bot::sendNick()
{
    sendRaw("NICK " + _nickname);
}

void Bot::sendUser()
{
    sendRaw("USER " + _username + " 0 * :" + _realname);
}

void Bot::joinChannels()
{
    for (size_t i = 0; i < _channels.size(); ++i)
    {
        sendRaw("JOIN " + _channels[i]);
    }
}

void Bot::sendPrivmsg(const std::string& target, const std::string& message)
{
    sendRaw("PRIVMSG " + target + " :" + message);
}

void Bot::sendPong(const std::string& token)
{
    sendRaw("PONG :" + token);
}

// ============================================================================
// Main Loop
// ============================================================================

bool Bot::start()
{
    if (!connectToServer())
        return false;

    // Register with server
    sendPass();
    sendNick();
    sendUser();

    _running = true;
    return true;
}

void Bot::run()
{
    struct pollfd pfd;
    pfd.fd = _socket;
    pfd.events = POLLIN;

    char buffer[512];

    while (_running)
    {
        int ret = poll(&pfd, 1, 1000); // 1 second timeout
        if (ret < 0)
        {
            if (errno == EINTR)
                continue;
            std::cerr << "Bot: poll error: " << strerror(errno) << std::endl;
            break;
        }

        if (ret == 0)
            continue; // Timeout, just loop

        if (pfd.revents & POLLIN)
        {
            ssize_t bytes = recv(_socket, buffer, sizeof(buffer) - 1, 0);
            if (bytes <= 0)
            {
                if (bytes == 0)
                    std::cout << "Bot: Server closed connection" << std::endl;
                else
                    std::cerr << "Bot: recv error: " << strerror(errno) << std::endl;
                break;
            }

            buffer[bytes] = '\0';
            _inputBuffer += buffer;

            // Process complete lines
            size_t pos;
            while ((pos = _inputBuffer.find("\r\n")) != std::string::npos)
            {
                std::string line = _inputBuffer.substr(0, pos);
                _inputBuffer.erase(0, pos + 2);
                if (!line.empty())
                    processLine(line);
            }
            // Also handle \n only (some servers/clients)
            while ((pos = _inputBuffer.find("\n")) != std::string::npos)
            {
                std::string line = _inputBuffer.substr(0, pos);
                _inputBuffer.erase(0, pos + 1);
                // Remove trailing \r if present
                if (!line.empty() && line[line.size() - 1] == '\r')
                    line.erase(line.size() - 1);
                if (!line.empty())
                    processLine(line);
            }
        }

        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
        {
            std::cerr << "Bot: Socket error" << std::endl;
            break;
        }
    }

    disconnect();
}

void Bot::stop()
{
    _running = false;
}

// ============================================================================
// Message Processing
// ============================================================================

void Bot::processLine(const std::string& line)
{
    std::cout << "Bot << " << line << std::endl;

    // Parse IRC message: [:prefix] command [params] [:trailing]
    std::string prefix, command, params, trailing;
    std::string rest = line;

    // Extract prefix
    if (!rest.empty() && rest[0] == ':')
    {
        size_t space = rest.find(' ');
        if (space != std::string::npos)
        {
            prefix = rest.substr(1, space - 1);
            rest = rest.substr(space + 1);
        }
    }

    // Extract command
    size_t space = rest.find(' ');
    if (space != std::string::npos)
    {
        command = rest.substr(0, space);
        rest = rest.substr(space + 1);
    }
    else
    {
        command = rest;
        rest = "";
    }

    // Extract trailing (after " :")
    size_t colonPos = rest.find(" :");
    if (colonPos != std::string::npos)
    {
        params = rest.substr(0, colonPos);
        trailing = rest.substr(colonPos + 2);
    }
    else
    {
        params = rest;
    }

    // Handle PING
    if (command == "PING")
    {
        handlePing(trailing.empty() ? params : trailing);
        return;
    }

    // Handle welcome (001) - we're registered
    if (command == "001")
    {
        _registered = true;
        std::cout << "Bot: Registered successfully!" << std::endl;
        joinChannels();
        return;
    }

    // Handle PRIVMSG
    if (command == "PRIVMSG")
    {
        // params contains target, trailing contains message
        handlePrivmsg(prefix, params, trailing);
        return;
    }

    // Handle nickname in use (433)
    if (command == "433")
    {
        _nickname += "_";
        sendNick();
        return;
    }
}

void Bot::handlePing(const std::string& token)
{
    sendPong(token);
}

void Bot::handlePrivmsg(const std::string& prefix, const std::string& target, const std::string& message)
{
    std::string sender = extractNick(prefix);
    
    // Determine reply target: if sent to channel, reply to channel; if PM, reply to sender
    std::string replyTo = target;
    if (!target.empty() && target[0] != '#')
    {
        // It's a private message to us, reply to sender
        replyTo = sender;
    }

    // Check if it's a command
    if (isCommand(message))
    {
        handleCommand(sender, replyTo, message);
    }
}

// ============================================================================
// Bot Commands
// ============================================================================

bool Bot::isCommand(const std::string& message)
{
    return message.size() > _commandPrefix.size() &&
           message.substr(0, _commandPrefix.size()) == _commandPrefix;
}

bool Bot::rateLimitOk()
{
    time_t now = std::time(NULL);
    if (now - _lastCommandTime < RATE_LIMIT_SECONDS)
        return false;
    _lastCommandTime = now;
    return true;
}

void Bot::handleCommand(const std::string& sender, const std::string& target, const std::string& message)
{
    if (!rateLimitOk())
        return;

    // Extract command and args
    std::string cmdLine = message.substr(_commandPrefix.size());
    std::string cmd, args;
    
    size_t space = cmdLine.find(' ');
    if (space != std::string::npos)
    {
        cmd = cmdLine.substr(0, space);
        args = cmdLine.substr(space + 1);
    }
    else
    {
        cmd = cmdLine;
    }

    // Convert command to lowercase for comparison
    for (size_t i = 0; i < cmd.size(); ++i)
        cmd[i] = std::tolower(cmd[i]);

    if (cmd == "help")
        cmdHelp(target);
    else if (cmd == "time")
        cmdTime(target);
    else if (cmd == "ping")
        cmdPing(target);
    else if (cmd == "roll")
        cmdRoll(target, args);
    else if (cmd == "users")
        cmdUsers(target, target);
    else
    {
        (void)sender; // Unused for now
        sendPrivmsg(target, "Unknown command. Type " + _commandPrefix + "help for available commands.");
    }
}

void Bot::cmdHelp(const std::string& replyTo)
{
    sendPrivmsg(replyTo, "Available commands:");
    sendPrivmsg(replyTo, _commandPrefix + "help   - Show this help message");
    sendPrivmsg(replyTo, _commandPrefix + "time   - Show current server time");
    sendPrivmsg(replyTo, _commandPrefix + "ping   - Check if bot is alive");
    sendPrivmsg(replyTo, _commandPrefix + "roll N - Roll a random number 1-N (default 6)");
    sendPrivmsg(replyTo, _commandPrefix + "users  - List users in current channel");
}

void Bot::cmdTime(const std::string& replyTo)
{
    time_t now = std::time(NULL);
    char timeStr[64];
    std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S UTC", std::gmtime(&now));
    sendPrivmsg(replyTo, std::string("Current time: ") + timeStr);
}

void Bot::cmdPing(const std::string& replyTo)
{
    sendPrivmsg(replyTo, "pong!");
}

void Bot::cmdRoll(const std::string& replyTo, const std::string& args)
{
    int max = 6;
    if (!args.empty())
    {
        int n = std::atoi(args.c_str());
        if (n > 0 && n <= 1000000)
            max = n;
    }
    int result = (std::rand() % max) + 1;
    std::ostringstream oss;
    oss << "Rolling 1-" << max << ": " << result;
    sendPrivmsg(replyTo, oss.str());
}

void Bot::cmdUsers(const std::string& replyTo, const std::string& channel)
{
    if (channel.empty() || channel[0] != '#')
    {
        sendPrivmsg(replyTo, "This command only works in channels.");
        return;
    }
    // Send NAMES query - the response will come from server
    // For simplicity, just inform the user
    sendPrivmsg(replyTo, "Use /NAMES " + channel + " to see channel users.");
}

// ============================================================================
// Utility
// ============================================================================

std::string Bot::extractNick(const std::string& prefix)
{
    // prefix is nick!user@host
    size_t bang = prefix.find('!');
    if (bang != std::string::npos)
        return prefix.substr(0, bang);
    return prefix;
}
