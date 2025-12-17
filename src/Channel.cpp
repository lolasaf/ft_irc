#include "../include/Channel.hpp"

Channel::Channel(const std::string& name)
    : _name(name), _inviteOnly(false), _topicRestricted(false), _userLimit(-1) {
}

Channel::~Channel() {
}

void Channel::addMember(int fd) {
    _members.insert(fd);
    if (_members.size() == 1) {
        _operators.insert(fd);
    }
}

void Channel::removeMember(int fd) {
    _members.erase(fd);
    _operators.erase(fd);
}
