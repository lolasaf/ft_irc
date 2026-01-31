/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dodordev <dodordev@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/16 16:43:21 by wel-safa          #+#    #+#             */
/*   Updated: 2026/01/31 10:29:59 by dodordev         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"

/* 
	Main entry point for the IRC server.
	First we check if the correct number of arguments is provided.
	Then we validate the port number (valid range is from 1 to 65535) 
	and start the server.
*/
int main(int ac, char **av)
{

	if (ac != 3)
	{
		std::cerr << "Usage: " << av[0] << " <port> <password>" << std::endl;
		return 1;
	}

	int port = atoi(av[1]);
	if (port <= 0 || port > 65535)
	{
		std::cerr << "Error: Invalid port number" << std::endl;
		return 1;
	}
	
	try {
		Server server(port, av[2]);
		server.run();
	} catch (std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	
	return 0;
}