#include "IrcServer.hpp"

int IrcServer::handle_command(int client_socket, const std::vector<std::string> &tokens)
{
	user current_user = users_list[client_socket];
	std::string nickname = users_list[client_socket].nickname;
	std::string command = tokens[0];
	// std::cout << "Commande reçue : " << command << std::endl;
	// if (command == "JOIN")
	// {
	// 	std::string channel = tokens[1];
	// 	join_command(current_user, channel);
	// }
	if (command == "NICK")
		nick_command(current_user, tokens[1]);
	// else if (command == "PRIVMSG")
	// 	privmsg_command(recipient, message);
	else if (command == "PING")
		send_message_to_client(client_socket, "PONG");
	else if (command == "MODE")
		mode_command(client_socket, tokens, current_user);
	else if (command == "WHOIS")
		whois_command(client_socket, nickname);
	else if (command == "PART")
		part_command(current_user);
	else
		std::cout << "Unknown command" << std::endl;
	return 0;
}

bool IrcServer::check_password(const std::string & password, user & current_user)
{
	if (password != password_)
	{
		send_response(current_user.socket, "464", ":Incorrect Password");
		return false;
	}
	if (current_user.authentified == true)
	{
		send_response(current_user.socket, "462", ":Already registered");
		return false;
	}
	current_user.authentified = true;
	return true;
}

int	IrcServer::handle_client_first_connection(int client_socket, std::vector<std::string> tokens)
{
	user current_user(client_socket);

	if (tokens.empty() || tokens.size() < 6) // send an error message
		return -1;
	if (tokens[0] == "CAP" && tokens[1] == "LS")
		tokens.erase(tokens.begin(), tokens.begin() + 2);
	if (tokens[0] == "PASS" && check_password(tokens[1], current_user))
	{
		tokens.erase(tokens.begin(), tokens.begin() + 2);
		if (tokens[0] != "NICK")// send an error message or change behaviour
			return -1;
		nick_command(current_user, tokens[1]);
		tokens.erase(tokens.begin(), tokens.begin() + 2);
		if (tokens[0] != "USER")// send an error message
			return -1;
		user_command(current_user, tokens[1]);
		users_list.insert(std::pair<int, user>(client_socket, current_user));
		return 0;
	}
	return -1;
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
	// afficher tout les tokens
	for (std::vector<std::string>::iterator it = tokens.begin(); it != tokens.end(); ++it)
		std::cout << *it << std::endl;
	
	// check if the client is new
	if (users_list.find(client_socket) == users_list.end())
		return handle_client_first_connection(client_socket, tokens);
	else
		return handle_command(client_socket, tokens);
}
