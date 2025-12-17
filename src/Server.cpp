#include "../include/Server.hpp"

Server::Server(int port, const std::string& password)
    : _port(port), _password(password), _serverFd(-1) {
    setupSocket();
}

Server::~Server() {
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        close(it->first);
        delete it->second;
    }
    for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
        delete it->second;
    }
    if (_serverFd >= 0)
        close(_serverFd);
}

void Server::setupSocket() {
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd < 0)
        throw std::runtime_error("socket() failed");

    int opt = 1;
    setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    fcntl(_serverFd, F_SETFL, O_NONBLOCK);

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(_port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(_serverFd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("bind() failed");

    if (listen(_serverFd, SOMAXCONN) < 0)
        throw std::runtime_error("listen() failed");

    struct pollfd pfd;
    pfd.fd = _serverFd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    _fds.push_back(pfd);

    std::cout << "Server listening on port " << _port << std::endl;
}

void Server::run() {
    while (true) {
        int ret = poll(&_fds[0], _fds.size(), -1);
        if (ret < 0)
            continue;

        for (size_t i = 0; i < _fds.size(); ++i) {
            if (_fds[i].revents & POLLIN) {
                if (_fds[i].fd == _serverFd) {
                    acceptClient();
                } else {
                    handleClient(_fds[i].fd);
                }
            }
        }
    }
}

void Server::acceptClient() {
    struct sockaddr_in clientAddr;
    socklen_t len = sizeof(clientAddr);
    int clientFd = accept(_serverFd, (struct sockaddr*)&clientAddr, &len);
    
    if (clientFd < 0)
        return;

    fcntl(clientFd, F_SETFL, O_NONBLOCK);

    Client* client = new Client(clientFd);
    _clients[clientFd] = client;

    struct pollfd pfd;
    pfd.fd = clientFd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    _fds.push_back(pfd);

    std::cout << "New client connected: FD " << clientFd << std::endl;
}

void Server::handleClient(int fd) {
    char buffer[1024];
    ssize_t bytes = recv(fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes <= 0) {
        removeClient(fd);
        return;
    }

    buffer[bytes] = '\0';
    Client* client = _clients[fd];
    client->getRecvBuffer() += buffer;

    size_t pos;
    while ((pos = client->getRecvBuffer().find("\r\n")) != std::string::npos) {
        std::string line = client->getRecvBuffer().substr(0, pos);
        client->getRecvBuffer().erase(0, pos + 2);
        
        if (!line.empty()) {
            processCommand(client, line);
        }
    }

    if (!client->getSendBuffer().empty()) {
        send(fd, client->getSendBuffer().c_str(), client->getSendBuffer().size(), 0);
        client->getSendBuffer().clear();
    }
}

void Server::removeClient(int fd) {
    Client* client = _clients[fd];
    if (!client)
        return;

    std::cout << "Client disconnected: FD " << fd;
    if (!client->getNickname().empty())
        std::cout << " (" << client->getNickname() << ")";
    std::cout << std::endl;

    const std::set<std::string>& channels = client->getChannels();
    for (std::set<std::string>::const_iterator it = channels.begin(); it != channels.end(); ++it) {
        Channel* chan = getChannel(*it);
        if (chan) {
            chan->removeMember(fd);
            std::string quitMsg = ":" + client->getNickname() + " QUIT :Client disconnected\r\n";
            broadcast(quitMsg, chan, fd);
        }
    }

    close(fd);
    delete client;
    _clients.erase(fd);

    for (std::vector<struct pollfd>::iterator it = _fds.begin(); it != _fds.end(); ++it) {
        if (it->fd == fd) {
            _fds.erase(it);
            break;
        }
    }
}

Client* Server::getClient(int fd) {
    std::map<int, Client*>::iterator it = _clients.find(fd);
    if (it != _clients.end())
        return it->second;
    return NULL;
}

Client* Server::getClientByNick(const std::string& nick) {
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        if (it->second->getNickname() == nick)
            return it->second;
    }
    return NULL;
}

Channel* Server::getChannel(const std::string& name) {
    std::map<std::string, Channel*>::iterator it = _channels.find(name);
    if (it != _channels.end())
        return it->second;
    return NULL;
}

Channel* Server::createChannel(const std::string& name) {
    Channel* chan = new Channel(name);
    _channels[name] = chan;
    return chan;
}

void Server::broadcast(const std::string& msg, Channel* channel, int exceptFd) {
    const std::set<int>& members = channel->getMembers();
    for (std::set<int>::const_iterator it = members.begin(); it != members.end(); ++it) {
        if (*it != exceptFd) {
            Client* client = getClient(*it);
            if (client) {
                client->getSendBuffer() += msg;
            }
        }
    }
}

void Server::processCommand(Client* client, const std::string& line) {
    std::cout << "Received: " << line << std::endl;
    
    std::vector<std::string> tokens;
    std::string current;
    bool inTrailing = false;
    
    for (size_t i = 0; i < line.length(); ++i) {
        if (line[i] == ':' && i == 0) {
            continue;
        }
        if (line[i] == ':' && !inTrailing && (i == 0 || line[i-1] == ' ')) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            inTrailing = true;
            continue;
        }
        if (line[i] == ' ' && !inTrailing) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current += line[i];
        }
    }
    if (!current.empty())
        tokens.push_back(current);
    
    if (tokens.empty())
        return;
    
    std::string cmd = toUpper(tokens[0]);
    
    // PASS command
    if (cmd == "PASS") {
        if (tokens.size() < 2) {
            client->getSendBuffer() += "ERROR :No password given\r\n";
            return;
        }
        if (tokens[1] == _password) {
            client->setAuthenticated(true);
        } else {
            client->getSendBuffer() += "ERROR :Wrong password\r\n";
            removeClient(client->getFd());
        }
    }
    // NICK command
    else if (cmd == "NICK") {
        if (tokens.size() < 2) {
            client->getSendBuffer() += ":server 431 * :No nickname given\r\n";
            return;
        }
        if (getClientByNick(tokens[1])) {
            client->getSendBuffer() += ":server 433 * " + tokens[1] + " :Nickname is already in use\r\n";
            return;
        }
        client->setNickname(tokens[1]);
        if (!client->getUsername().empty() && !client->isRegistered()) {
            client->setRegistered(true);
            client->getSendBuffer() += ":server 001 " + client->getNickname() + " :Welcome to the IRC server\r\n";
        }
    }
    // USER command
    else if (cmd == "USER") {
        if (tokens.size() < 5) {
            client->getSendBuffer() += ":server 461 * USER :Not enough parameters\r\n";
            return;
        }
        client->setUsername(tokens[1]);
        client->setRealname(tokens[4]);
        if (!client->getNickname().empty() && !client->isRegistered()) {
            client->setRegistered(true);
            client->getSendBuffer() += ":server 001 " + client->getNickname() + " :Welcome to the IRC server\r\n";
        }
    }
    // JOIN command
    else if (cmd == "JOIN") {
        if (!client->isAuthenticated() || !client->isRegistered())
            return;
        if (tokens.size() < 2) {
            client->getSendBuffer() += ":server 461 " + client->getNickname() + " JOIN :Not enough parameters\r\n";
            return;
        }
        
        std::string chanName = tokens[1];
        if (chanName[0] != '#')
            chanName = "#" + chanName;
        
        Channel* chan = getChannel(chanName);
        if (!chan) {
            chan = createChannel(chanName);
        }
        
        if (chan->isInviteOnly() && !chan->isInvited(client->getFd()) && !chan->isMember(client->getFd())) {
            client->getSendBuffer() += ":server 473 " + client->getNickname() + " " + chanName + " :Cannot join channel (+i)\r\n";
            return;
        }
        
        if (chan->getUserLimit() > 0 && (int)chan->getMembers().size() >= chan->getUserLimit()) {
            client->getSendBuffer() += ":server 471 " + client->getNickname() + " " + chanName + " :Cannot join channel (+l)\r\n";
            return;
        }
        
        if (!chan->getKey().empty() && (tokens.size() < 3 || tokens[2] != chan->getKey())) {
            client->getSendBuffer() += ":server 475 " + client->getNickname() + " " + chanName + " :Cannot join channel (+k)\r\n";
            return;
        }
        
        chan->addMember(client->getFd());
        client->addChannel(chanName);
        
        std::string joinMsg = ":" + client->getNickname() + " JOIN " + chanName + "\r\n";
        client->getSendBuffer() += joinMsg;
        broadcast(joinMsg, chan, client->getFd());
        
        if (!chan->getTopic().empty()) {
            client->getSendBuffer() += ":server 332 " + client->getNickname() + " " + chanName + " :" + chan->getTopic() + "\r\n";
        }
    }
    // PRIVMSG command
    else if (cmd == "PRIVMSG") {
        if (!client->isAuthenticated() || !client->isRegistered())
            return;
        if (tokens.size() < 3) {
            client->getSendBuffer() += ":server 411 " + client->getNickname() + " :No recipient given\r\n";
            return;
        }
        
        std::string target = tokens[1];
        std::string message = tokens[2];
        
        if (target[0] == '#') {
            Channel* chan = getChannel(target);
            if (!chan) {
                client->getSendBuffer() += ":server 403 " + client->getNickname() + " " + target + " :No such channel\r\n";
                return;
            }
            if (!chan->isMember(client->getFd())) {
                client->getSendBuffer() += ":server 442 " + client->getNickname() + " " + target + " :You're not on that channel\r\n";
                return;
            }
            std::string msg = ":" + client->getNickname() + " PRIVMSG " + target + " :" + message + "\r\n";
            broadcast(msg, chan, client->getFd());
        } else {
            Client* targetClient = getClientByNick(target);
            if (!targetClient) {
                client->getSendBuffer() += ":server 401 " + client->getNickname() + " " + target + " :No such nick\r\n";
                return;
            }
            std::string msg = ":" + client->getNickname() + " PRIVMSG " + target + " :" + message + "\r\n";
            targetClient->getSendBuffer() += msg;
        }
    }
    // PING command
    else if (cmd == "PING") {
        if (tokens.size() < 2)
            return;
        client->getSendBuffer() += ":server PONG server :" + tokens[1] + "\r\n";
    }
    // PART command
    else if (cmd == "PART") {
        if (!client->isAuthenticated() || !client->isRegistered())
            return;
        if (tokens.size() < 2) {
            client->getSendBuffer() += ":server 461 " + client->getNickname() + " PART :Not enough parameters\r\n";
            return;
        }
        
        std::string chanName = tokens[1];
        if (chanName[0] != '#')
            chanName = "#" + chanName;
        
        Channel* chan = getChannel(chanName);
        if (!chan || !chan->isMember(client->getFd())) {
            client->getSendBuffer() += ":server 442 " + client->getNickname() + " " + chanName + " :You're not on that channel\r\n";
            return;
        }
        
        std::string reason = tokens.size() > 2 ? tokens[2] : "Leaving";
        std::string partMsg = ":" + client->getNickname() + " PART " + chanName + " :" + reason + "\r\n";
        
        client->getSendBuffer() += partMsg;
        broadcast(partMsg, chan, client->getFd());
        
        chan->removeMember(client->getFd());
        client->removeChannel(chanName);
        
        if (chan->getMembers().empty()) {
            delete chan;
            _channels.erase(chanName);
        }
    }
    // TOPIC command
    else if (cmd == "TOPIC") {
        if (!client->isAuthenticated() || !client->isRegistered())
            return;
        if (tokens.size() < 2) {
            client->getSendBuffer() += ":server 461 " + client->getNickname() + " TOPIC :Not enough parameters\r\n";
            return;
        }
        
        std::string chanName = tokens[1];
        if (chanName[0] != '#')
            chanName = "#" + chanName;
        
        Channel* chan = getChannel(chanName);
        if (!chan) {
            client->getSendBuffer() += ":server 403 " + client->getNickname() + " " + chanName + " :No such channel\r\n";
            return;
        }
        
        if (!chan->isMember(client->getFd())) {
            client->getSendBuffer() += ":server 442 " + client->getNickname() + " " + chanName + " :You're not on that channel\r\n";
            return;
        }
        
        if (tokens.size() == 2) {
            if (chan->getTopic().empty()) {
                client->getSendBuffer() += ":server 331 " + client->getNickname() + " " + chanName + " :No topic is set\r\n";
            } else {
                client->getSendBuffer() += ":server 332 " + client->getNickname() + " " + chanName + " :" + chan->getTopic() + "\r\n";
            }
        } else {
            if (chan->isTopicRestricted() && !chan->isOperator(client->getFd())) {
                client->getSendBuffer() += ":server 482 " + client->getNickname() + " " + chanName + " :You're not channel operator\r\n";
                return;
            }
            
            chan->setTopic(tokens[2]);
            std::string topicMsg = ":" + client->getNickname() + " TOPIC " + chanName + " :" + tokens[2] + "\r\n";
            client->getSendBuffer() += topicMsg;
            broadcast(topicMsg, chan, client->getFd());
        }
    }
    // KICK command
    else if (cmd == "KICK") {
        if (!client->isAuthenticated() || !client->isRegistered())
            return;
        if (tokens.size() < 3) {
            client->getSendBuffer() += ":server 461 " + client->getNickname() + " KICK :Not enough parameters\r\n";
            return;
        }
        
        std::string chanName = tokens[1];
        if (chanName[0] != '#')
            chanName = "#" + chanName;
        
        Channel* chan = getChannel(chanName);
        if (!chan) {
            client->getSendBuffer() += ":server 403 " + client->getNickname() + " " + chanName + " :No such channel\r\n";
            return;
        }
        
        if (!chan->isOperator(client->getFd())) {
            client->getSendBuffer() += ":server 482 " + client->getNickname() + " " + chanName + " :You're not channel operator\r\n";
            return;
        }
        
        Client* target = getClientByNick(tokens[2]);
        if (!target || !chan->isMember(target->getFd())) {
            client->getSendBuffer() += ":server 441 " + client->getNickname() + " " + tokens[2] + " " + chanName + " :They aren't on that channel\r\n";
            return;
        }
        
        std::string reason = tokens.size() > 3 ? tokens[3] : client->getNickname();
        std::string kickMsg = ":" + client->getNickname() + " KICK " + chanName + " " + tokens[2] + " :" + reason + "\r\n";
        
        target->getSendBuffer() += kickMsg;
        broadcast(kickMsg, chan, target->getFd());
        
        chan->removeMember(target->getFd());
        target->removeChannel(chanName);
    }
    // INVITE command
    else if (cmd == "INVITE") {
        if (!client->isAuthenticated() || !client->isRegistered())
            return;
        if (tokens.size() < 3) {
            client->getSendBuffer() += ":server 461 " + client->getNickname() + " INVITE :Not enough parameters\r\n";
            return;
        }
        
        std::string chanName = tokens[2];
        if (chanName[0] != '#')
            chanName = "#" + chanName;
        
        Channel* chan = getChannel(chanName);
        if (!chan) {
            client->getSendBuffer() += ":server 403 " + client->getNickname() + " " + chanName + " :No such channel\r\n";
            return;
        }
        
        if (!chan->isMember(client->getFd())) {
            client->getSendBuffer() += ":server 442 " + client->getNickname() + " " + chanName + " :You're not on that channel\r\n";
            return;
        }
        
        if (chan->isInviteOnly() && !chan->isOperator(client->getFd())) {
            client->getSendBuffer() += ":server 482 " + client->getNickname() + " " + chanName + " :You're not channel operator\r\n";
            return;
        }
        
        Client* target = getClientByNick(tokens[1]);
        if (!target) {
            client->getSendBuffer() += ":server 401 " + client->getNickname() + " " + tokens[1] + " :No such nick\r\n";
            return;
        }
        
        if (chan->isMember(target->getFd())) {
            client->getSendBuffer() += ":server 443 " + client->getNickname() + " " + tokens[1] + " " + chanName + " :is already on channel\r\n";
            return;
        }
        
        chan->addInvited(target->getFd());
        client->getSendBuffer() += ":server 341 " + client->getNickname() + " " + tokens[1] + " " + chanName + "\r\n";
        target->getSendBuffer() += ":" + client->getNickname() + " INVITE " + tokens[1] + " " + chanName + "\r\n";
    }
    // MODE command
    else if (cmd == "MODE") {
        if (!client->isAuthenticated() || !client->isRegistered())
            return;
        if (tokens.size() < 2) {
            client->getSendBuffer() += ":server 461 " + client->getNickname() + " MODE :Not enough parameters\r\n";
            return;
        }
        
        std::string chanName = tokens[1];
        if (chanName[0] != '#')
            chanName = "#" + chanName;
        
        Channel* chan = getChannel(chanName);
        if (!chan) {
            client->getSendBuffer() += ":server 403 " + client->getNickname() + " " + chanName + " :No such channel\r\n";
            return;
        }
        
        if (tokens.size() == 2) {
            std::string modes = "+";
            if (chan->isInviteOnly()) modes += "i";
            if (chan->isTopicRestricted()) modes += "t";
            if (!chan->getKey().empty()) modes += "k";
            if (chan->getUserLimit() > 0) modes += "l";
            client->getSendBuffer() += ":server 324 " + client->getNickname() + " " + chanName + " " + modes + "\r\n";
            return;
        }
        
        if (!chan->isOperator(client->getFd())) {
            client->getSendBuffer() += ":server 482 " + client->getNickname() + " " + chanName + " :You're not channel operator\r\n";
            return;
        }
        
        std::string modeStr = tokens[2];
        bool adding = true;
        size_t paramIdx = 3;
        
        for (size_t i = 0; i < modeStr.length(); ++i) {
            char mode = modeStr[i];
            
            if (mode == '+') {
                adding = true;
            } else if (mode == '-') {
                adding = false;
            } else if (mode == 'i') {
                chan->setInviteOnly(adding);
            } else if (mode == 't') {
                chan->setTopicRestricted(adding);
            } else if (mode == 'k') {
                if (adding && paramIdx < tokens.size()) {
                    chan->setKey(tokens[paramIdx++]);
                } else {
                    chan->setKey("");
                }
            } else if (mode == 'l') {
                if (adding && paramIdx < tokens.size()) {
                    chan->setUserLimit(stringToInt(tokens[paramIdx++]));
                } else {
                    chan->setUserLimit(-1);
                }
            } else if (mode == 'o') {
                if (paramIdx < tokens.size()) {
                    Client* target = getClientByNick(tokens[paramIdx++]);
                    if (target && chan->isMember(target->getFd())) {
                        if (adding) {
                            chan->addOperator(target->getFd());
                        } else {
                            chan->removeOperator(target->getFd());
                        }
                    }
                }
            }
        }
        
        std::string modeMsg = ":" + client->getNickname() + " MODE " + chanName + " " + tokens[2];
        for (size_t i = 3; i < tokens.size(); ++i) {
            modeMsg += " " + tokens[i];
        }
        modeMsg += "\r\n";
        
        client->getSendBuffer() += modeMsg;
        broadcast(modeMsg, chan, client->getFd());
    }
    // QUIT command
    else if (cmd == "QUIT") {
        std::string reason = tokens.size() > 1 ? tokens[1] : "Client quit";
        
        const std::set<std::string>& channels = client->getChannels();
        for (std::set<std::string>::const_iterator it = channels.begin(); it != channels.end(); ++it) {
            Channel* chan = getChannel(*it);
            if (chan) {
                std::string quitMsg = ":" + client->getNickname() + " QUIT :" + reason + "\r\n";
                broadcast(quitMsg, chan, client->getFd());
            }
        }
        
        client->getSendBuffer() += "ERROR :Closing connection\r\n";
        removeClient(client->getFd());
    }
}
