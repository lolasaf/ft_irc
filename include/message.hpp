#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
#include <vector>

// Holds a parsed IRC message
struct Message {
	std::string command;              // "NICK", "USER", "PASS", etc.
	std::vector<std::string> params;  // Parameters (including trailing)
};

// Parse a raw IRC line into a Message struct
// Example: "NICK john" -> { command: "NICK", params: ["john"] }
// Example: "USER john 0 * :John Doe" -> { command: "USER", params: ["john", "0", "*", "John Doe"] }
Message parseMessage(const std::string& line);

#endif
