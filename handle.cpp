#include "IrcServer.hpp"

int IrcServer::handle_command(int client_socket, const std::vector<std::string> &tokens)
{
	user & current_user = users_list[client_socket];
	std::string nickname = users_list[client_socket].nickname;
	std::string command = tokens[0];
	std::cout << "Commande reçue : " << command << std::endl;
	if (command == "JOIN")
		join_command(current_user, tokens);
	else if (command == "NICK")
		nick_command(current_user, tokens);
	else if (command == "PRIVMSG" || command == "NOTICE")
		privmsg_command(current_user, tokens);
	else if (command == "PING")
		send_message_to_client(client_socket, "PONG");
	else if (command == "MODE")
		mode_command(tokens, current_user);
	else if (command == "WHOIS")
		whois_command(client_socket, nickname);
	else if (command == "PART")
		part_command(current_user, tokens[1]);
	else if (command == "QUIT")
		quit_command(current_user, tokens[1]);
	else if (command == "WHO")
		who_command(current_user, tokens[1]);
	else if (command == "NAMES")
		names_command(current_user, tokens[1]);
	else if (command == "TOPIC")
		topic_command(current_user, tokens);
	else if (command == "INVITE")
		invite_command(current_user, tokens);
	else
		std::cout << "Unknown command" << std::endl;
	return current_user.status == DISCONNECTED ? -1 : 0;
}

bool IrcServer::check_password(const std::string & password, user & current_user)
{
	if (password != password_)
	{
		send_response(current_user.socket, "464", ":Incorrect Password");
		return false;
	}
	if (current_user.status >= AUTHENTIFIED)
	{
		send_response(current_user.socket, "462", ":Already registered");
		return false;
	}
	current_user.status = AUTHENTIFIED;
	unregistered_users_list.insert(std::pair<int, user>(current_user.socket, current_user));
	return true;
}

int	IrcServer::handle_client_first_connection(user & current_user, std::vector<std::string> tokens)
{
	if (tokens.empty()) // send an error message
		return -1;
	if (tokens.size() > 1 && tokens[0] == "CAP" && tokens[1] == "LS")
		tokens.erase(tokens.begin(), tokens.begin() + 2);
	if (!tokens.empty() && tokens[0] == "PASS")
	{
		if (!check_password(tokens[1], current_user))
			return -1;
		tokens.erase(tokens.begin(), tokens.begin() + 2);
	}

	if (!tokens.empty() && tokens[0] == "NICK" && current_user.status == AUTHENTIFIED)// send an error message or change behaviour
	{
		if (nick_command(current_user, tokens) == -1)
			return -1;
		tokens.erase(tokens.begin(), tokens.begin() + 2);
	}
	if (!tokens.empty() && tokens[0] == "USER")// send an error message
	{
		if (user_command(current_user, tokens) == -1)
			return -1;
		users_list.insert(std::pair<int, user>(current_user.socket, current_user));
		unregistered_users_list.erase(current_user.socket);
	}
	return 0;
}

int IrcServer::handle_client_connection(int client_socket)
{
	user & current_user = users_list.find(client_socket) == users_list.end() ? unregistered_users_list[client_socket] : users_list[client_socket];
	current_user.socket = client_socket;

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
	int res = 0;
	while (message.size() > 0 && res == 0)
	{
		std::vector<std::string> tokens = tokenize(message);
		if (tokens.empty())
			return -1;
		// check if the client is new
		if (users_list.find(client_socket) == users_list.end())
			res = handle_client_first_connection(current_user, tokens);
		else
			res = handle_command(client_socket, tokens);
	}
	return res;
}
