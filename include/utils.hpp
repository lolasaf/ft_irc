/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dodordev <dodordev@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 08:33:00 by wel-safa          #+#    #+#             */
/*   Updated: 2026/01/28 11:37:41 by dodordev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>
#include <cctype>

// ============================================
// String Manipulation
// ============================================

/*
    * Convert string to uppercase (C++98 compatible)
    * @param str String to convert
    * @return Uppercase version of string
*/
std::string toUpper(const std::string& str);

/*
    * Convert string to lowercase (C++98 compatible)
    * @param str String to convert
    * @return Lowercase version of string
*/
std::string toLower(const std::string& str);


/*
    * Trim leading and trailing whitespace
    * @param str String to trim
    * @return Trimmed string
*/
std::string trim(const std::string& str);

// Parsing Utilities
/*
    * Split comma-separated list into vector
    * Used for JOIN #chan1,#chan2 or MODE params
    * @param list Comma-separated string
    * @return Vector of strings
*/
std::vector<std::string> splitCommaList(const std::string& list);

/*
    * Split string by delimiter
    * @param str String to split
    * @param delimiter Delimiter character
    * @return Vector of strings
*/
std::vector<std::string> split(const std::string& str, char delimiter);

// ============================================
// Case-Insensitive Comparison
// ============================================

/*
    * Case-insensitive string comparison
    * @param str1 First string
    * @param str2 Second string
    * @return true if equal (case-insensitive), false otherwise
*/
bool caseInsensitiveCompare(const std::string& str1, const std::string& str2);

/*
    * Case-insensitive string less-than comparison
    * For use in std::map with case-insensitive keys
    * @param str1 First string
    * @param str2 Second string
    * @return true if str1 < str2 (case-insensitive)
*/
bool caseInsensitiveLess(const std::string& str1, const std::string& str2);

#endif
