#include "Server.h"
#include <iostream>

int main()
{
	Server server;

	unsigned int maxConnections;
	std::cout << "Max connections: ";
	std::cin >> maxConnections;
	server.SetMaxConnections(maxConnections);

	server.Run();

	return 0;
}