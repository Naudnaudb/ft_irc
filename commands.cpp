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

// void IrcServer::join_command(user &current_user, const std::string &channel)
// {
// 	// ajouter join
// }

// void IrcServer::privmsg_command(const std::string &recipient, const std::string &message)
// {
// 	// ajouter privmsg
// }

void IrcServer::part_command(user &current_user)
{
	// Vérifier si l'utilisateur est dans un canal
	if (!current_user.channels.empty())
	{
		// Prendre le dernier canal de la liste des canaux du client
		std::string channel = current_user.channels.back();

		// Supprimer le canal de la liste des canaux du client
		current_user.channels.pop_back();

		// Envoyer un message de départ aux autres clients du canal
		std::string goodbye_message = current_user.nickname + " left the channel " + channel;
		send_message_to_channel(channel, goodbye_message);
	}
	else
	{
		// Envoyer un message indiquant que le client n'est pas dans un canal
		std::string error_message = "You're not in any channel";
		send_message_to_client(current_user.socket, error_message);
	}
}

void IrcServer::user_command(user &current_user, const std::string &username)
{
	// Vérifier si le nom d'utilisateur est déjà utilisé
	std::map<int, user>::iterator it;
	for (it = users_list.begin(); it != users_list.end(); ++it)
	{
		if (it->second.username == username)
		{
			send_response(current_user.socket, "433", "Username already in use");
			return;
		}
	}

	// Changer le nom d'utilisateur du client
	current_user.username = username;

	// Maintenant que le nom d'utilisateur a changé, nous devons mettre à jour notre map users_list
	users_list[current_user.socket] = current_user;
}

void IrcServer::nick_command(user &current_user, const std::string &nickname)
{
	// Vérifier si le pseudonyme est déjà utilisé
	std::map<int, user>::iterator it;
	for (it = users_list.begin(); it != users_list.end(); ++it)
	{
		user user_checked = it->second;
		if (user_checked.nickname == nickname && it->first != current_user.socket)
		{
			send_response(current_user.socket, "433", "This nickname is already in use");
			return;
		}
	}

	// Changer le pseudonyme du client
	std::string old_nickname = current_user.nickname;
	current_user.nickname = nickname;

	// Envoyer un message de bienvenue au client avec son nouveau pseudonyme
	if (old_nickname.empty())
		send_message_to_client(current_user.socket, ":" + std::string(SERVER_NAME) + " 001 " + nickname + " :Welcome to the server " + SERVER_NAME + ", " + nickname + " !");
	else
		send_message_to_client(current_user.socket, ":" + std::string(SERVER_NAME) + " 001 " + nickname + " :Your nickname has change from " + old_nickname + " to " + nickname);
}