#!/bin/bash

echo "Starting server in background..."
./ircserv 6667 test123 &
SERVER_PID=$!

sleep 2

echo "Connecting client..."
{
    echo "PASS test123"
    sleep 0.2
    echo "NICK alice"
    sleep 0.2
    echo "USER alice 0 * :Alice"
    sleep 0.2
    echo "JOIN #test"
    sleep 0.2
    echo "PRIVMSG #test :Hello"
    sleep 0.2
    echo "QUIT :Bye"
    sleep 0.5
} | nc localhost 6667

sleep 1

echo "Stopping server..."
kill $SERVER_PID 2>/dev/null

echo "Done!"
