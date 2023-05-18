#include "IrcServer.hpp"

void IrcServer::handle_command(int client_socket, const std::vector<std::string> &tokens)
{

	std::string command = tokens[0];
	// std::cout << "Commande reçue : " << command << std::endl;
	if (command == "JOIN")
	{
		std::string channel = "#test";
		std::string nickname = users_list[client_socket].nickname;
		handle_join_command(client_socket, channel, nickname);
	}
	else if (command == "NICK")
	{
		handle_nick_command(client_socket, tokens[1]);
	}
	else if (command == "PRIVMSG")
	{
		std::string recipient = "#test";
		std::string message = "test";
		handle_privmsg_command(recipient, message);
	}
	else if (command == "PING")
	{
		std::string message = "PONG";
		send_message_to_client(client_socket, message);
	}
	else if (command == "MODE")
	{
		std::string nickname = users_list[client_socket].nickname;
		handle_mode_command(client_socket, nickname);
	}
	else if (command == "WHOIS")
	{
		std::string nickname = users_list[client_socket].nickname;
		handle_whois_command(client_socket, nickname);
	}
	else
		std::cout << "Unknown command" << std::endl;
}

int IrcServer::handle_client_reply(int client_socket, std::string message)
{
	// if the message is empty it means client disconnected
	if (message.empty())
		return -1;
	std::vector<std::string> tokens = tokenize(message);
	return handle_command(client_socket, tokens);
}

int IrcServer::handle_client_connection(int client_socket)
{
	struct sockaddr_in client_address_;

	socklen_t client_address_length = sizeof(client_address_);
	getpeername(client_socket, (struct sockaddr *)&client_address_, &client_address_length);
	std::cout << "New incoming connection : " << inet_ntoa(client_address_.sin_addr) << std::endl;

	char buffer[4096];
	int bytes_received = recv(client_socket, buffer, 4096, 0);
	if (bytes_received < 0)
	{
		perror("Error: recv() failed");
		exit(EXIT_FAILURE);
	}
	else if (bytes_received == 0)
		return -1;
	std::string message(buffer, bytes_received);
	return handle_client_reply(client_socket, message);
}
