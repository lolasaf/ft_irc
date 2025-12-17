#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <set>

class Client {
private:
    int _fd;
    std::string _nickname;
    std::string _username;
    std::string _realname;
    std::string _recvBuffer;
    std::string _sendBuffer;
    bool _authenticated;
    bool _registered;
    std::set<std::string> _channels;

public:
    Client(int fd);
    ~Client();

    int getFd() const { return _fd; }
    const std::string& getNickname() const { return _nickname; }
    const std::string& getUsername() const { return _username; }
    const std::string& getRealname() const { return _realname; }
    std::string& getRecvBuffer() { return _recvBuffer; }
    std::string& getSendBuffer() { return _sendBuffer; }
    bool isAuthenticated() const { return _authenticated; }
    bool isRegistered() const { return _registered; }
    
    void setNickname(const std::string& nick) { _nickname = nick; }
    void setUsername(const std::string& user) { _username = user; }
    void setRealname(const std::string& real) { _realname = real; }
    void setAuthenticated(bool auth) { _authenticated = auth; }
    void setRegistered(bool reg) { _registered = reg; }
    
    void addChannel(const std::string& channel) { _channels.insert(channel); }
    void removeChannel(const std::string& channel) { _channels.erase(channel); }
    const std::set<std::string>& getChannels() const { return _channels; }
};

#endif
