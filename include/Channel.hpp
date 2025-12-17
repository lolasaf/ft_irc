#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <set>

class Channel {
private:
    std::string _name;
    std::string _topic;
    std::string _key;
    std::set<int> _members;
    std::set<int> _operators;
    std::set<int> _invited;
    bool _inviteOnly;
    bool _topicRestricted;
    int _userLimit;

public:
    Channel(const std::string& name);
    ~Channel();

    const std::string& getName() const { return _name; }
    const std::string& getTopic() const { return _topic; }
    const std::string& getKey() const { return _key; }
    const std::set<int>& getMembers() const { return _members; }
    const std::set<int>& getOperators() const { return _operators; }
    bool isInviteOnly() const { return _inviteOnly; }
    bool isTopicRestricted() const { return _topicRestricted; }
    int getUserLimit() const { return _userLimit; }
    
    void setTopic(const std::string& topic) { _topic = topic; }
    void setKey(const std::string& key) { _key = key; }
    void setInviteOnly(bool val) { _inviteOnly = val; }
    void setTopicRestricted(bool val) { _topicRestricted = val; }
    void setUserLimit(int limit) { _userLimit = limit; }
    
    void addMember(int fd);
    void removeMember(int fd);
    void addOperator(int fd) { _operators.insert(fd); }
    void removeOperator(int fd) { _operators.erase(fd); }
    void addInvited(int fd) { _invited.insert(fd); }
    bool isInvited(int fd) const { return _invited.find(fd) != _invited.end(); }
    bool isMember(int fd) const { return _members.find(fd) != _members.end(); }
    bool isOperator(int fd) const { return _operators.find(fd) != _operators.end(); }
};

#endif
