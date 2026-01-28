/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dodordev <dodordev@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/16 16:42:58 by wel-safa          #+#    #+#             */
/*   Updated: 2026/01/27 11:13:21 by dodordev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
# define SERVER_HPP

# include <iostream>
# include <string>
# include <sys/socket.h>
# include <netinet/in.h>
# include <unistd.h>
# include <cstdlib> //For atoi
# include <fcntl.h> // For fcntl(), O_NONBLOCK, F_SETFL
# include <cstring> // For memset() if you want to zero the struct
# include <map> // For storing clients
# include <poll.h> // For poll() if needed
# include <vector> // For std::vector
# include <cerrno>  // For errno
# include <sstream>
# include <iomanip>
# include "user.hpp"
# include "message.hpp"
# include "replies.hpp"
# include "channel.hpp"
# include "utils.hpp"

# define USERLEN 18
# define NICKLEN 9
# define REALLEN 50
# define CHANNELLEN 50

const std::string SERVER_NAME = "SugarDaddyFinderIRC";

class Server {
	private:
		int port; // Port number for the server
		std::string password; // Password for server access
		int serverFd; // Server socket file descriptor
		std::map<int, User*> users; //Connected users by fd
		std::vector<std::string> isSupported; // List of supported ISUPPORT tokens
		std::map<std::string, Channel*> channels; // Channels by lowercase name (Channel object also stores lowercase)
		
		void initISupport(); // Initialize ISUPPORT tokens

		Channel* createChannel(const std::string& name); // Create a new channel
		void deleteChannel(const std::string& name); // Delete a channel
		Channel* findChannel(const std::string& name); // Find channel (case-insensitive)
		void leaveChannel(User* user, Channel* chan, const std::string& partMessage = "Leaving"); // Leave a channel
		void broadcastToChannel(Channel* chan, const std::string& msg, User* exclude = 0);

		void setupSocket();
		void acceptNewClient();

		// Incoming data handler
		void handleClientData(int clientFd); // Handle incoming data from client
		std::vector<std::string> extractMessages(User* user); // Extract complete messages from user's input buffer
		void processMessage(User* user, const std::string& message); // Process a single IRC message

		// Individual command handlers
		void handlePass(User* user, const Message& msg); // Handle PASS command
		void handleNick(User* user, const Message& msg); // Handle NICK command
		//void handleUserCmd(User* user, const Message& msg); // Handle USER command
		void handleUser(User* user, const Message& msg); // Handle USER command
		void handlePing(User* user, const Message& msg); // Handle PING command

		// Channel command handlers
		void handleJoin(User* user, const Message& msg); // Handle JOIN command
		void joinChannel(User* user, const std::string& channel, const std::string& key); // Join a single channel
		void handlePart(User* user, const Message& msg); // Handle PART command
		void handleMessageCommand(User* user, const Message& msg, const std::string& command); // Handle PRIVMSG/NOTICE commands
		void handlePrivmsg(User* user, const Message& msg); // Handle PRIVMSG command
		void handleNotice(User* user, const Message& msg); // Handle NOTICE command
		void handleTopic(User* user, const Message& msg); // Handle TOPIC command
		void handleInvite(User* user, const Message& msg); // Handle INVITE command
		void handleKick(User* user, const Message& msg); // Handle KICK command
		void handleMode(User* user, const Message& msg); // Handle MODE command
		
		// Outgoing data handler
		void handleClientWrite(int clientFd); // Handle outgoing data to client

		// Server Utility functions
		// Helper to send numeric replies (formats code to 3 digits and prefixes with server name)
		void sendNumeric(User* user, ReplyCode code, const std::vector<std::string>& params = std::vector<std::string>(), const std::string& trailing = "");
		void registerUser(User* user); // Check and complete user registration
		bool isValidChannelName(const std::string& name); // Check if a channel name is valid
		std::string buildHostmask(User* user); // Build IRC hostmask format: nick!user@host
		void disconnectUser(User* user, const std::string& reason = "Client disconnected"); // Clean up user from all channels before deletion
		void handleQuit(User* user, const Message& msg); // Handle QUIT command
		
	public:
		Server(int port, const std::string& password);
		~Server();
		void run();

};

#endif