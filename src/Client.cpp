#include "../include/Client.hpp"

Client::Client(int fd) 
    : _fd(fd), _authenticated(false), _registered(false) {
}

Client::~Client() {
}
