#!/bin/bash

# Test IRC Server with netcat

echo "Testing IRC server..."
echo ""

{
    sleep 1
    echo "PASS test123"
    sleep 0.5
    echo "NICK testuser"
    sleep 0.5
    echo "USER testuser 0 * :Test User"
    sleep 0.5
    echo "JOIN #test"
    sleep 0.5
    echo "PRIVMSG #test :Hello World"
    sleep 0.5
    echo "TOPIC #test :Test Channel"
    sleep 0.5
    echo "MODE #test +t"
    sleep 0.5
    echo "QUIT :Goodbye"
    sleep 1
} | nc localhost 6667

echo ""
echo "Test complete!"
