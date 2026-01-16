/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wel-safa <wel-safa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/16 16:42:58 by wel-safa          #+#    #+#             */
/*   Updated: 2026/01/16 20:28:44 by wel-safa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdlib> //For atoi
#include <fcntl.h> // For fcntl(), O_NONBLOCK, F_SETFL
#include <cstring> // For memset() if you want to zero the struct
#include <map> // For storing clients
#include <poll.h> // For poll() if needed
#include <vector> // For std::vector
#include <cerrno>  // For errno
#include "user.hpp"
#include "message.hpp"
#include "replies.hpp"

class Server {
	private:
		int port; // Port number for the server
		std::string password; // Password for server access
		int serverFd; // Server socket file descriptor
		std::map<int, User*> users; //Connected users by fd

	public:
		Server(int port, const std::string& password);
		~Server();
		void run();
		void setupSocket();
		void acceptNewClient();

		// Incoming data handler
		void handleClientData(int clientFd); // Handle incoming data from client
		std::vector<std::string> extractMessages(User* user); // Extract complete messages from user's input buffer
		void processMessage(User* user, const std::string& message); // Process a single IRC message

		// Individual command handlers
		void handlePass(User* user, const Message& msg); // Handle PASS command
		void handleNick(User* user, const Message& msg); // Handle NICK command
		void handleUserCmd(User* user, const Message& msg); // Handle USER command
		void handleUser(User* user, const Message& msg); // Handle USER command
		void handlePing(User* user, const Message& msg); // Handle PING command

		// Outgoing data handler
		void handleClientWrite(int clientFd); // Handle outgoing data to client

		// Helper to send numeric replies (formats code to 3 digits and prefixes with server name)
		void sendNumeric(User* user, ReplyCode code, const std::vector<std::string>& params = std::vector<std::string>(), const std::string& trailing = "");
};

#endif