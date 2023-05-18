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

int	IrcServer::handle_client_first_connection(int client_socket, std::vector<std::string> tokens)
{
	user current_user;

	if (tokens.empty())
		return -1;
	
	for (size_t i = 0; i < tokens.size(); i++)
	{
		std::string command = tokens[i];
		if (command == "CAP" || command == "LS")
			continue;
		else if (command == "PASS")
		{
			std::string password = tokens[i + 1];
			if (password == password_)
			else
			{
				send_response(client_socket, "464", "Mot de passe incorrect");
				return -1;
			}
			i++;
		}
		else if (!authenticated)
		{
			send_response(client_socket, "464", "Mot de passe absent");
			return -1;
		}
		else if (command == "NICK" && tokens.size() > 1 && nickname_set == false)
		{
			current_user.nickname = tokens[i + 1];
			handle_nick_command(client_socket, current_user.nickname);
			nickname_set = true;
			i++;
		}
		else if (command == "USER" && tokens.size() > 1 && username_set == false)
		{
			current_user.username = tokens[i + 5] + tokens[i + 6];
			handle_user_command(client_socket, current_user.username);
			username_set = true;
			i += 6;
		}
		else if (nickname_set && username_set)
			handle_command(client_socket, tokens);
	}
	users_list.insert(std::pair<int, user>(client_socket, current_user));
	return 0;
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
	// if the message is empty it means client disconnected
	if (message.empty())
		return -1;
	std::vector<std::string> tokens = tokenize(message);
	// check if the client is new
	if (users_list.find(client_socket) == users_list.end())
		return handle_client_first_connection(client_socket, tokens);
	else
		return handle_client_reply(client_socket, tokens);
}
