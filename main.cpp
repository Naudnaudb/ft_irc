#include "IrcServer.hpp"

bool check_args(int argc, char **argv, int &PORT, std::string &PASSWORD)
{
	char *endptr;

	if (argc != 3)
	{
		std::cout << "Usage: ./ircserv [port] [password]" << std::endl;
		return (false);
	}
	PORT = strtol(argv[1], &endptr, 10);
	if (*endptr != '\0'|| PORT < 4 || PORT > 65535)
	{
		std::cout << "Invalid port" << std::endl;
		return (false);
	}
	PASSWORD = argv[2];
	if (PASSWORD.empty())
	{
		std::cout << "Invalid password" << std::endl;
		return (false);
	}
	return (true);
}

int main(int argc, char **argv)
{
	int PORT = 0;
	std::string PASSWORD;

	if(check_args(argc, argv, PORT, PASSWORD) == false)
		return (EXIT_FAILURE);

	IrcServer ircServer(PORT, PASSWORD);
	ircServer.poll_client_connections();

	return 0;
}