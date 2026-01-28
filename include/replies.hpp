/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   replies.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dodordev <dodordev@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/16 19:55:26 by wel-safa          #+#    #+#             */
/*   Updated: 2026/01/28 12:06:53 by dodordev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REPLIES_HPP
#define REPLIES_HPP

// Decimal values; format to 3 digits when sending (e.g. 1 -> "001").
enum ReplyCode {

	// USER
	// Registration replies
    RPL_WELCOME = 1,            // 001: Welcome after successful PASS+NICK+USER
    RPL_YOURHOST = 2,           // 002: Your host information
    RPL_CREATED = 3,            // 003: Server creation date/info
    RPL_MYINFO = 4,             // 004: Server version / info
	RPL_ISUPPORT = 5,        // 005: Server supported features
	// Registration errors
	ERR_NOTREGISTERED = 451,    // 451: You have not registered (command before registration)
    ERR_NEEDMOREPARAMS = 461,   // 461: Not enough parameters
    ERR_ALREADYREGISTRED = 462, // 462: You may not reregister
    ERR_PASSWDMISMATCH = 464,   // 464: Password incorrect
	// NICK errors
	ERR_NONICKNAMEGIVEN = 431,  // 431: No nickname given (NICK missing)
    ERR_ERRONEUSNICKNAME = 432, // 432: Erroneous nickname (invalid chars)
    ERR_NICKNAMEINUSE = 433,    // 433: Nickname already in use
	// PRIVMSG/NOTICE errors
    ERR_NOSUCHNICK = 401,       // 401: No such nick/channel (PRIVMSG target missing)
    ERR_CANNOTSENDTOCHAN = 404, // 404: Cannot send to channel (e.g., not a member)
    ERR_NORECIPIENT = 411,      // 411: No recipient given (PRIVMSG with no target)
    ERR_NOTEXTTOSEND = 412,     // 412: No text to send (PRIVMSG with no message)

	// CHANNEL 
	// names list replies
	RPL_NAMREPLY = 353,         // 353: Names reply (list users in a channel)
    RPL_ENDOFNAMES = 366,        // 366: End of /NAMES list
	// Channel errors
	ERR_NOSUCHCHANNEL = 403,    // 403: No such channel (JOIN/CHANNEL refer to missing channel)
	ERR_TOOMANYCHANNELS = 405,  // 405: You have joined too many channels
	ERR_NOTONCHANNEL = 442,     // 442: You're not on that channel (PART)
	// JOIN errors
	ERR_CHANNELISFULL = 471,     // 471: Cannot join channel (+l limit reached)
	ERR_INVITEONLYCHAN = 473,   // 473: Cannot join channel (+i invite only)
	ERR_BADCHANNELKEY = 475,    // 475: Cannot join channel (+k key incorrect)
	ERR_BADCHANMASK = 476,      // 476: Bad Channel Mask
	//TOPIC replies
	RPL_NOTOPIC = 331,          // 331: No topic is set
	RPL_TOPIC = 332,            // 332: Topic for channel
	RPL_TOPICWHOTIME = 333,      // 333: Topic set by <nick> <timestamp>
	// TOPIC errors
	ERR_CHANOPRIVSNEEDED = 482,  // 482: You're not channel operator
	// INVITE replies/errors
	RPL_INVITING = 341,         // 341: Invite confirmation to inviter
	ERR_USERNOTINCHANNEL = 441, // 441: User not in channel
	ERR_USERONCHANNEL = 443,    // 443: Already on channel

	// OPERATOR
	ERR_UMODEUNKNOWNFLAG = 501, // 501: Unknown MODE flag
	ERR_USERSDONTMATCH = 502,   // 502: Cannot change mode for other users
	// MODE replies/errors
	RPL_CHANNELMODEIS = 324,    // 324: Channel mode is ...
	ERR_NOSUCHMODE = 472,      // 472: Unknown MODE flag for channel

	// Other errors
    ERR_UNKNOWNCOMMAND = 421,   // 421: Unknown command
	
	// Generic errors
	ERR_UNKNOWNERROR = 400,
	ERR_NOSUCHSERVER = 402,
};

#endif
