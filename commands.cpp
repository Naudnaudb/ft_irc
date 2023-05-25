#include "IrcServer.hpp"

void IrcServer::mode_command(int client_socket, const std::string &nickname)
{
	nickname.empty();
	std::string message = "You are in invisible mode";
	send_response(client_socket, "MODE", message);
}

void IrcServer::whois_command(int client_socket, const std::string &nickname)
{
	nickname.empty();
	std::string message = "You are in invisible mode";
	send_response(client_socket, "WHOIS", message);
}

void IrcServer::join_command(int client_socket, const std::string &channel, const std::string &nickname)
{
	std::set<int> clients;
	std::map<int, std::set<std::string> >::iterator it;
	for (it = client_channels_.begin(); it != client_channels_.end(); ++it)
		if (it->second.count(channel) > 0)
			clients.insert(it->first);

	if (client_channels_[client_socket].count(channel) == 0)
		client_channels_[client_socket].insert(channel);

	std::string welcome_message = "Welcome on the channel " + channel + ", " + nickname + "!";
	std::set<int>::iterator client_it;
	for (client_it = clients.begin(); client_it != clients.end(); ++client_it)
		send_message_to_client(*client_it, welcome_message);
}

void IrcServer::privmsg_command(const std::string &recipient, const std::string &message)
{
	std::cout << "Private message from " << recipient << " : " << message << std::endl;
	std::set<int> clients;
	for (std::map<int, std::set<std::string> >::iterator it = client_channels_.begin(); it != client_channels_.end(); ++it)
		for (std::set<std::string>::iterator channel_it = it->second.begin(); channel_it != it->second.end(); ++channel_it)
			if (*channel_it == recipient)
				clients.insert(it->first);

	for (std::set<int>::iterator client_it = clients.begin(); client_it != clients.end(); ++client_it)
		send_message_to_client(*client_it, message);
}

void IrcServer::part_command(int client_socket, const std::string &channel, const std::string &nickname)
{
	// Supprimer le client de la liste des clients du canal
	std::set<std::string> &channels = client_channels_[client_socket];
	channels.erase(channel);

	// Envoyer un message de départ aux autres clients du canal
	std::string goodbye_message = nickname + " left the channel " + channel;
	send_message_to_channel(channel, goodbye_message);

	// Supprimer le canal de la liste des canaux du client si nécessaire
	if (channels.empty())
		client_channels_.erase(client_socket);
}

void IrcServer::user_command(int client_socket, const std::string &username)
{
	user current_user = users_list[client_socket];
	// Vérifier si le nom d'utilisateur est déjà utilisé
	std::map<int, user>::iterator it;
	for (it = users_list.begin(); it != users_list.end(); ++it)
	{
		user user_checked = it->second;
		if (user_checked.nickname == username && it->first != client_socket)
		{
			send_response(client_socket, "462", "You are already registered");
			return;
		}
	}

	// Changer le nom d'utilisateur du client
	current_user.username = username;
}

void IrcServer::nick_command(int client_socket, const std::string &nickname)
{
	// goes there if it is the first connection of user
	if (!users_list.count(client_socket))
	{
		//nick command for first connection
	}
	user current_user = users_list.at(client_socket);
	// Vérifier si le pseudonyme est déjà utilisé
	std::map<int, user>::iterator it;
	for (it = users_list.begin(); it != users_list.end(); ++it)
	{
		user& user_checked = it->second;
		if (current_user.nickname == user_checked.nickname && it->first != client_socket)
		{
			send_response(client_socket, "433", "This nickname is already in use");
			return;
		}
	}

	// Changer le pseudonyme du client
	std::cout << "your nickname is: " << current_user.nickname << std::endl;
	std::string old_nickname = current_user.nickname;
	current_user.nickname = nickname;

	// Envoyer un message de bienvenue au client avec son nouveau pseudonyme
	if (old_nickname.empty())
		send_message_to_client(client_socket, ":" + std::string(SERVER_NAME) + " 001 " + nickname + " :Welcome to the server " + SERVER_NAME + ", " + nickname + " !");
	else
		send_message_to_client(client_socket, ":" + std::string(SERVER_NAME) + " 001 " + nickname + " :Your nickname has change from " + old_nickname + " to " + nickname);
}