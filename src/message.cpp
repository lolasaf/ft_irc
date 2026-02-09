#include "message.hpp"

/*
 * IRC Message Format:
 *   [:<prefix>] <command> [<params>] [:<trailing>]
 *
 * Examples:
 *   PASS secretpassword
 *   NICK john
 *   USER john 0 * :John Doe
 *   PRIVMSG #channel :Hello world!
 *
 * The trailing parameter (after ':') can contain spaces.
 */

Message parseMessage(const std::string& line)
{
	Message msg;
	std::string::size_type pos = 0;

	// Skip leading spaces (just in case)
	while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t'))
		pos++;

	// 1. Skip optional prefix (starts with ':' at the beginning)
	//    Prefix is the server/user that sent the message
	//    We don't need it for client -> server messages
	if (pos < line.size() && line[pos] == ':')
	{
		pos = line.find(' ', pos);
		if (pos == std::string::npos)
			return msg;
	}

	// Skip any spaces before command
	while (pos < line.size() && line[pos] == ' ')
		pos++;

	// 2. Extract command (everything until next space or end)
	// Find the end of the command
	// If no space found, command goes to end of line
	// Store the command in msg.command (convert to uppercase for consistency)
	std::string::size_type cmdEnd = line.find(' ', pos);
	if (cmdEnd == std::string::npos)
	{
		// No space found — command is the rest of the line
		msg.command = line.substr(pos);
		return msg;  // No parameters
	}
	msg.command = line.substr(pos, cmdEnd - pos);
	pos = cmdEnd;

	// 3. Extract parameters
	while (pos < line.size())
	{
		// Skip spaces between parameters
		while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t'))
			pos++;

		if (pos >= line.size())
			break;

		// Check for trailing parameter (starts with ':')
		// Everything after ':' is ONE parameter (can contain spaces)
		if (line[pos] == ':')
		{
			// Extract trailing parameter
			// Add it to msg.params
			msg.params.push_back(line.substr(pos + 1));
			break;  // Trailing is always last
		}

		// Regular parameter (until next space or end)
		// Extract parameter until space
		// If no space, parameter goes to end of line
		// Add it to msg.params
		std::string::size_type paramEnd = line.find(' ', pos);
		if (paramEnd == std::string::npos)
		{
			// No more spaces — this parameter goes to end
			msg.params.push_back(line.substr(pos));
			break;
		}
		msg.params.push_back(line.substr(pos, paramEnd - pos));
		pos = paramEnd;
	}

	return msg;
}
