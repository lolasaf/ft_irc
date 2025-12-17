#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

std::vector<std::string> split(const std::string& str, char delim);
std::string trim(const std::string& str);
std::string toUpper(const std::string& str);
int stringToInt(const std::string& str);

#endif
