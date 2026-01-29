/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bot.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dodordev <dodordev@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/29 00:00:00 by                   #+#    #+#             */
/*   Updated: 2026/01/29 15:05:02 by dodordev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef BOT_HPP
#define BOT_HPP

#include <string>
#include <vector>
#include <ctime>

class Bot {
private:
    int _socket;
    std::string _server;
    int _port;
    std::string _password;
    std::string _nickname;
    std::string _username;
    std::string _realname;
    std::vector<std::string> _channels;
    std::string _commandPrefix;
    std::string _inputBuffer;
    bool _running;
    bool _registered;
    time_t _lastCommandTime;
    static const int RATE_LIMIT_SECONDS = 1; // Min seconds between command responses

    // Connection
    bool connectToServer();
    void disconnect();
    bool sendRaw(const std::string& message);
    
    // IRC Protocol
    void sendPass();
    void sendNick();
    void sendUser();
    void joinChannels();
    void sendPrivmsg(const std::string& target, const std::string& message);
    void sendPong(const std::string& token);
    
    // Message handling
    void processLine(const std::string& line);
    void handlePrivmsg(const std::string& prefix, const std::string& target, const std::string& message);
    void handlePing(const std::string& token);
    
    // Bot commands
    bool isCommand(const std::string& message);
    void handleCommand(const std::string& sender, const std::string& target, const std::string& command);
    void cmdHelp(const std::string& replyTo);
    void cmdTime(const std::string& replyTo);
    void cmdPing(const std::string& replyTo);
    void cmdRoll(const std::string& replyTo, const std::string& args);
    void cmdUsers(const std::string& replyTo, const std::string& channel);
    
    // Utility
    std::string extractNick(const std::string& prefix);
    bool rateLimitOk();

public:
    Bot(const std::string& server, int port, const std::string& password);
    ~Bot();
    
    // Configuration
    void setNickname(const std::string& nick);
    void setUsername(const std::string& user);
    void setRealname(const std::string& real);
    void setCommandPrefix(const std::string& prefix);
    void addChannel(const std::string& channel);
    
    // Main loop
    bool start();
    void run();
    void stop();
};

#endif
