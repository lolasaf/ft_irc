Mandatory part

Basic checks

- There is a Makefile, the project compiles correctly, is written in C++, and the executable is called "ircserv".

- Ask and check how many poll() (or eqivalent) are present in the code. There must be only one.

- Verify that the poll() (or equivalent) is called every time before each accept, read/recv, write/send. After these calls, errno should not be used to trigger specific action (e.g. like reading again after errno == EAGAIN).
/* Answer */
In Server::run():
Each loop iteration:
Rebuild pollFds and call poll(&pollFds[0], pollFds.size(), -1); once.
then:
- Call acceptNewClient() only if pollFds[0].revents & POLLIN.
- Call handleClientData(clientFd) only if pollFds[i].revents & POLLIN.
- Call handleClientWrite(clientFd) only if pollFds[i].revents & POLLOUT.
Inside those handlers:
- handleClientData() does one recv(); if it hits EAGAIN/EWOULDBLOCK, it just returns and waits for the next poll().
- handleClientWrite() does one send(); if it hits EAGAIN/EWOULDBLOCK, it just returns and waits for the next poll().
So: Every accept / recv / send happens only after a poll() has indicated readiness. It never loops on errno to re-read/re-write; it always waits for the next poll() instead.

- Verify that each call to fcntl() is done as follows:
	fcntl(fd, F_SETFL, O_NOBLOCK);
  Any other use of fcntl() is forbiden.
A system call in UNIX-like systems that performs various control operations on open fd's. fcntl = file control. Paramters: fd, cmd, optional arg or flag depending on command.
We use it to set the file status flag ex. O_NONBLOCK, O_APPEND; in our code we use flag F_SETFL wich is the set file status flag, and O_NONBLOCK which sets this fd to O_NOBLOCK.

poll() handles when you should try I/O.
Non‑blocking mode handles what happens if the I/O can’t complete:
Instead of blocking, you get EAGAIN, and your code returns to the loop.

- If any of these points is wrong, the evaluation ends now and the final mark is 0.

Networking

Check the following requirements:
- The server starts and listens on all network interfaces on the port given from the command line.
netstat -tulnp | grep 6667
Should give the connection information on port 6667 and IP should be 0.0.0.0 which means any address not just 127.0.0.0 i.e. localhost

- Using the 'nc' tool, you can connect to the server, send commands, and the server answers you back.

- Ask the team what is their reference IRC client.

- Using this IRC client, you can connect to the server.
- The server can handle multiple connections at the same time. The server should not block. It should be able to answer all demands. Do some tests with the IRC client and 'nc' at the same time.
- Join a channel thanks to te appropriate command. Ensure that all messages from one client on that channel are sent to all other clients that joined the channel.


Networking specials

Network communications can be disturbed by many strange situations.
- Just like in the subject, using nc, try to send partial commands. Check tat the server answers correctly. With a partial command sent, ensure that other connections still run fine.
{
  for char in P A S S ' ' p a s s $'\r' $'\n'; do
    printf "%s" "$char"
    sleep 0.1
  done
  for char in N I C K ' ' a l i c e $'\r' $'\n'; do
    printf "%s" "$char"
    sleep 0.1
  done
  echo "USER alice 0 * :Alice"
  sleep 0.5
  echo "QUIT"
} | nc localhost 6667

- Unexpectedly kill a client. Then check that the server is still operational for the other connections and for any new incoming client.
nc localhost 6667 &
NCPID=$!
# then
kill -9 $NCPID

- Unexpectedly kill a nc with just half of command sent. Check again that the server is not in an odd state or blocked.
(printf 'NICK alice'; sleep 999) | nc localhost 6667 
# partial line without /r/n saved in buffer
# either cntl + C or
# in another terminal
pgrep -f "nc localhost 6667"
kill -9 <last_nc_PID>

- Stop a client (^-Z) connected on a channel. Then flood the channel using another client. The server should be processed normally. Also, check for memory leaks during this operation.
valgrind --leak-check=full --show-leak-kinds=all ./ircserv 6667 password
# connect a terminal normally with nc
# then press cntrl+Z to stop the client, still not disconnected from server
# in another terminal
(printf 'PASS password\r\nNICK bob\r\nUSER bob 0 * :Bob\r\nJOIN #test\r\n'; for i in $(seq 1 500); do printf 'PRIVMSG #test :flood %d\r\n' "$i"; done) | nc localhost 6667

Client Commands

- Withf both nc and a regular IRC client, check that you can authenticate, set a nickname, a username, join a channel. This should be ok (you should have already done this previously).
- Verify that private messages (PRIVMSG) and (NOTICE) are fully functional with different parameters.
- Check that a regular user does not have privileges to do operator actions. Then test with and operator. All the channel operation commands should be tested (remove one point for each feature that is not working).


File transfer
File transfer works with the reference IRC client.

A small bot
There's an IRC bot.