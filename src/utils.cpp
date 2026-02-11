/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dodordev <dodordev@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 08:33:00 by wel-safa          #+#    #+#             */
/*   Updated: 2026/02/11 12:26:11 by dodordev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "utils.hpp"
#include <sstream>

/* String Manipulation Helper Functions */

std::string toUpper(const std::string& str)
{
    std::string result = str;

    for (std::string::size_type i = 0; i < result.size(); ++i)
    {
        result[i] = static_cast<char>(
            std::toupper(static_cast<unsigned char>(result[i]))
        );
    }
    return result;
}

std::string toLower(const std::string& str)
{
    std::string result = str;
    for (size_t i = 0; i < result.length(); ++i)
        result[i] = std::tolower(result[i]);
    return result;
}

std::string trim(const std::string& str)
{
    if (str.empty())
        return str;
    
    size_t start = 0;
    size_t end = str.length() - 1;
    
    // Find first non-whitespace
    while (start < str.length() && std::isspace(str[start]))
        ++start;
    
    // Find last non-whitespace
    while (end > start && std::isspace(str[end]))
        --end;
    
    if (start > end)
        return "";
    
    return str.substr(start, end - start + 1);
}


// This function removes CR and LF characters from the input string, 
// which are used in IRC protocol to denote message boundaries. 
// By stripping these characters, we can prevent malicious users 
// from injecting additional commands or messages into the IRC stream.
std::string sanitizeIrcText(const std::string& str)
{
    std::string result;
    result.reserve(str.size());
    for (size_t i = 0; i < str.size(); ++i)
    {
        char c = str[i];
        // Strip CR (\r) and LF (\n) to prevent IRC protocol injection
        if (c != '\r' && c != '\n')
            result += c;
    }
    return result;
}

// Parsing Utilities

// The splitCommaList function is specifically designed to handle lists of items
// separated by commas, which is a common format in IRC for parameters like 
// channel lists or user lists.
std::vector<std::string> splitCommaList(const std::string& list)
{
    std::vector<std::string> result;
    std::stringstream ss(list);
    std::string item;
    while (std::getline(ss, item, ','))
        result.push_back(item);
    return result;
}

// The split function is a more general utility that can split a string based on 
// any specified delimiter, making it versatile for various parsing needs 
// in the IRC server, such as splitting command parameters.
std::vector<std::string> split(const std::string& str, char delimiter)
{
    std::vector<std::string> result;
    if (str.empty())
        return result;
    
    std::stringstream ss(str);
    std::string item;
    
    while (std::getline(ss, item, delimiter))
        result.push_back(item);
    
    return result;
}

// Case-Insensitive Comparison is essential for IRC commands and parameters, 
// as the protocol specifies that command names are case-insensitive.
bool caseInsensitiveCompare(const std::string& str1, const std::string& str2)
{
    if (str1.length() != str2.length())
        return false;
    for (size_t i = 0; i < str1.length(); ++i)
    {
        unsigned char c1 = static_cast<unsigned char>(str1[i]);
        unsigned char c2 = static_cast<unsigned char>(str2[i]);
        if (std::tolower(c1) != std::tolower(c2))
            return false;
    }
    return true;
}

bool caseInsensitiveLess(const std::string& str1, const std::string& str2)
{
    size_t minLen = (str1.length() < str2.length()) ? str1.length() : str2.length();
    
    for (size_t i = 0; i < minLen; ++i)
    {
        unsigned char c1 = static_cast<unsigned char>(str1[i]);
        unsigned char c2 = static_cast<unsigned char>(str2[i]);
        if (std::tolower(c1) != std::tolower(c2))
            return std::tolower(c1) < std::tolower(c2);
    }
    return str1.length() < str2.length();
}