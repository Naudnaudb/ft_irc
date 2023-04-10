#include "IrcServer.hpp"

#define MAX_CONNECTIONS 10

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		std::cout << "Usage: ./ircserv [port] [password]" << std::endl;
		return (1);
	}
	int PORT = atoi(argv[1]);
	std::string PASSWORD = argv[2];

	IrcServer ircServer(PORT, PASSWORD);
	ircServer.run();

	return 0;
}