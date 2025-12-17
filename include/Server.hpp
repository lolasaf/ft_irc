#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>
#include <poll.h>
#include "../include/Client.hpp"
#include "../include/Channel.hpp"
#include "../include/Utils.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <sstream>

class Client;
class Channel;

class Server {
private:
    int _port;
    std::string _password;
    int _serverFd;
    std::vector<struct pollfd> _fds;
    std::map<int, Client*> _clients;
    std::map<std::string, Channel*> _channels;

    void setupSocket();
    void acceptClient();
    void handleClient(int fd);
    void removeClient(int fd);
    void processCommand(Client* client, const std::string& line);

public:
    Server(int port, const std::string& password);
    ~Server();
    
    void run();
    Client* getClient(int fd);
    Client* getClientByNick(const std::string& nick);
    Channel* getChannel(const std::string& name);
    Channel* createChannel(const std::string& name);
    void broadcast(const std::string& msg, Channel* channel, int exceptFd);
    const std::string& getPassword() const { return _password; }
};

#endif
