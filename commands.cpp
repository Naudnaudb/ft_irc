#include "IrcServer.hpp"

void IrcServer::mode_command(int client_socket, const std::vector<std::string> &tokens, user &current_user)
{
	if (tokens.size() < 3)
		return (send_response(client_socket, "461", "This command needs at least 2 parameters : channel and the mode to change"));
	channel channel_t_w_change = tokens[1];
	std::string mode = tokens[2];
	if (mode[0] == '+')
	{
		if (mode[1] == 'i')
			channel_t_w_change.mode['i'] = true;
		else if (mode[1] == 't')
			channel_t_w_change.mode['t'] = true;
		else if (mode[1] == 'k')
		{
			if (tokens.size() != 4)
				return (send_response(client_socket, "461", "This option needs a parameter"));
			channel_t_w_change.mode['k'] = true;
			channel_t_w_change.key = tokens[3];
		}
		else if (mode[1] == 'o')
		{
			if (tokens.size() != 4)
				return (send_response(client_socket, "461", "This option needs a parameter"));
			channel_t_w_change.mode['o'] = true;
			current_user.channels[channel_t_w_change.name] = true;
		}
		else if (mode[1] == 'l')
		{
			if (tokens.size() != 4)
				return (send_response(client_socket, "461", "This option needs a parameter"));

			channel_t_w_change.mode['l'] = true;
			int i = atoi(tokens[3].c_str());
			if (i > 1)
				channel_t_w_change.user_limit = i;
			else
				return (send_response(client_socket, "461", "Last parameter must be > 1"));
		}
	}
	else if (mode[0] == '-')
	{
		if (mode[1] == 'i')
			channel_t_w_change.mode['i'] = false;
		else if (mode[1] == 't')
			channel_t_w_change.mode['t'] = false;
		else if (mode[1] == 'k')
		{
			if (tokens.size() != 4)
				return (send_response(client_socket, "461", "This option needs a parameter"));
			channel_t_w_change.mode['k'] = false;
			channel_t_w_change.key = "";
		}
		else if (mode[1] == 'o')
		{
			if (tokens.size() != 4)
				return (send_response(client_socket, "461", "This option needs a parameter"));
			channel_t_w_change.mode['o'] = false;
			current_user.channels[channel_t_w_change.name] = false;
		}
		else if (mode[1] == 'l')
		{
			channel_t_w_change.mode['l'] = false;
			channel_t_w_change.user_limit = __INT_MAX__;
		}
	}
	else
	{
		send_response(client_socket, "472", "Unknown mode");
		return;
	}
}

void IrcServer::whois_command(int client_socket, const std::string &nickname)
{
	nickname.empty();
	std::string message = "You are in invisible mode";
	send_response(client_socket, "WHOIS", message);
}

void IrcServer::join_command(user &current_user, const std::string &channel_name)
{
	// Vérifier si le canal existe déjà
	std::map<std::string, channel>::iterator it = channels_list.find(channel_name);
	if (it == channels_list.end())
	{
		// Si le canal n'existe pas, le créer
		channel new_channel;
		new_channel.name = channel_name;
		new_channel.users.push_back(current_user.nickname);
		channels_list[channel_name] = new_channel;
		std::string formatted_message = current_user.nickname + " " + channel_name + " :" + new_channel.topic;
		send_response(current_user.socket, "332", formatted_message);
	}
	else
	{
		// Si le canal existe, ajouter l'utilisateur à la liste des utilisateurs du canal
		it->second.users.push_back(current_user.nickname);
	}

	// Envoyer un message de bienvenue à l'utilisateur
	std::string formatted_message = ":" + current_user.username + "!" + current_user.nickname + "@" + SERVER_NAME + " JOIN :" + channel_name;
	send_message_to_channel(channel_name, formatted_message);

	// Envoyer la liste des utilisateurs du canal à l'utilisateur
	std::map<std::string, channel>::iterator it2 = channels_list.find(channel_name);
	if (it2 != channels_list.end())
	{
		formatted_message = ":" + std::string(SERVER_NAME) + " 353 " + current_user.nickname + " = " + channel_name + " :";
		for (std::vector<std::string>::iterator user_it = it2->second.users.begin(); user_it != it2->second.users.end(); ++user_it)
		{
			formatted_message += *user_it + " ";
		}
		send_response(current_user.socket, "353", formatted_message);
	}

	// Envoyer un message de fin de liste des utilisateurs du canal à l'utilisateur
	formatted_message = ":" + std::string(SERVER_NAME) + " 366 " + current_user.nickname + " " + channel_name + " :End of /NAMES list";
	send_response(current_user.socket, "366", formatted_message);
}

void IrcServer::privmsg_command(user &current_user, const std::string &target, const std::string &message)
{
	// Vérifier si la cible est un utilisateur ou un canal
	if (target[0] == '#')
	{
		// Envoyer le message à tous les utilisateurs connectés au canal
		std::string formatted_message = ":" + current_user.nickname + "!" + current_user.username + "@" + SERVER_NAME + " PRIVMSG " + target + " :" + message;
		send_message_to_channel_except(target, formatted_message, current_user.nickname);
	}
	else
	{
		// Vérifier si l'utilisateur existe
		std::map<int, user>::iterator it = users_list.find(current_user.socket);
		if (it == users_list.end())
		{
			return;
		}

		// Envoyer le message à l'utilisateur
		send_message_to_client(it->second.socket, current_user.nickname + ": " + message);
	}
}

void IrcServer::part_command(user &current_user, const std::string &channel_name)
{
	// Vérifier si le canal existe
	std::map<std::string, channel>::iterator it = channels_list.find(channel_name);
	if (it == channels_list.end())
	{
		send_response(current_user.socket, "403", channel_name + " :No such channel");
		return;
	}

	// Vérifier si l'utilisateur est connecté au canal
	bool user_found = false;
	for (std::vector<std::string>::iterator user_it = it->second.users.begin(); user_it != it->second.users.end(); ++user_it)
	{
		if (*user_it == current_user.nickname)
		{
			user_found = true;
			it->second.users.erase(user_it);

			// Envoyer un message de départ à tous les utilisateurs connectés au canal
			std::string formatted_message = ":" + current_user.nickname + "!" + current_user.username + "@" + SERVER_NAME + " PART " + channel_name;
			send_message_to_all(formatted_message);
			break;
		}
	}
	if (!user_found)
	{
		send_response(current_user.socket, "442", channel_name + " :You're not on that channel");
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

int	IrcServer::fix_nickname_collision(user &current_user, std::string nickname)
{
	int i = 0;
	std::stringstream nb;
	for (std::map<int, user>::iterator it = users_list.begin(); it != users_list.end(); ++it)
	{
		if (it->second.nickname == nickname + nb.str() && it->first != current_user.socket)
		{
			if (current_user.status == REGISTERED)
			{
				send_response(current_user.socket, "433", "Nickname is already in use");
				return -1;
			}
			nb.str("");
			nb << ++i;
			it = users_list.begin();
		}
	}
	current_user.nickname = nickname + nb.str();
	return 0;
}

int IrcServer::nick_command(user &current_user, const std::vector<std::string> & tokens)
{
	// Vérifier si le pseudonyme est vide
	if (tokens.size() < 2 || tokens[1].empty())
	{
		send_response(current_user.socket, "431", "No nickname given");
		return -1;
	}
	if (current_user.nickname == tokens[1])
		return 0;
	std::string nickname = tokens[1];
	std::string old_nickname = current_user.nickname;
	// Assign nickname and handle nickname collision
	if (fix_nickname_collision(current_user, nickname) == -1)
		return -1;

	// Send confirmation message to user and channel
	if (current_user.status == REGISTERED)
	{
		send_message_to_client(current_user.socket, ":" + old_nickname + " NICK " + nickname);
		// send_message_to_channel(channel_name, message);
	}
	else
		current_user.status = NICKNAME_SET;
	return 0;
}


void IrcServer::quit_command(user &current_user, const std::string &message)
{
	// Envoyer un message de départ à tous les utilisateurs connectés au canal
	std::string formatted_message = ":" + current_user.username + "!" + current_user.nickname + "@" + SERVER_NAME + " QUIT :" + message;
	send_message_to_all(formatted_message);

	// Fermer la socket de l'utilisateur
	shutdown(current_user.socket, SHUT_RDWR);

	// Supprimer l'utilisateur de la liste des utilisateurs connectés
	users_list.erase(current_user.socket);
}

void IrcServer::who_command(user &current_user, const std::string &channel_name)
{
	// Vérifier si le canal existe
	std::map<std::string, channel>::iterator it = channels_list.find(channel_name);
	if (it == channels_list.end())
	{
		send_response(current_user.socket, "403", channel_name + " :No such channel");
		return;
	}

	// Envoyer la liste des utilisateurs connectés au canal
	for (std::vector<std::string>::iterator user_it = it->second.users.begin(); user_it != it->second.users.end(); ++user_it)
	{
		std::map<int, user>::iterator user_list_it = users_list.find(current_user.socket);
		if (user_list_it != users_list.end())
		{
			std::string formatted_message = ":" + std::string(SERVER_NAME) + " 352 " + current_user.nickname + " " + channel_name + " " + user_list_it->second.username + " " + SERVER_NAME + " " + user_list_it->second.nickname;
			send_response(current_user.socket, "352", formatted_message);
		}
	}
}


void IrcServer::quit_command(user &current_user, const std::string &message)
{
	// Envoyer un message de départ à tous les utilisateurs connectés au canal
	std::string formatted_message = ":" + current_user.username + "!" + current_user.nickname + "@" + SERVER_NAME + " QUIT :" + message;
	send_message_to_all(formatted_message);

	// Fermer la socket de l'utilisateur
	shutdown(current_user.socket, SHUT_RDWR);

	// Supprimer l'utilisateur de la liste des utilisateurs connectés
	users_list.erase(current_user.socket);
}

void IrcServer::who_command(user &current_user, const std::string &channel_name)
{
	// Vérifier si le canal existe
	std::map<std::string, channel>::iterator it = channels_list.find(channel_name);
	if (it == channels_list.end())
	{
		send_response(current_user.socket, "403", channel_name + " :No such channel");
		return;
	}

	// Envoyer la liste des utilisateurs connectés au canal
	for (std::vector<std::string>::iterator user_it = it->second.users.begin(); user_it != it->second.users.end(); ++user_it)
	{
		std::map<int, user>::iterator user_list_it = users_list.find(current_user.socket);
		if (user_list_it != users_list.end())
		{
			std::string formatted_message = ":" + std::string(SERVER_NAME) + " 352 " + current_user.nickname + " " + channel_name + " " + user_list_it->second.username + " " + SERVER_NAME + " " + user_list_it->second.nickname;
			send_response(current_user.socket, "352", formatted_message);
		}
	}
}

void IrcServer::names_command(user &current_user, const std::string &channel_name)
{
	// Vérifier si le canal existe
	std::map<std::string, channel>::iterator it = channels_list.find(channel_name);
	if (it == channels_list.end())
	{
		send_response(current_user.socket, "403", channel_name + " :No such channel");
		return;
	}

	// Envoyer la liste des utilisateurs connectés au canal
	std::string user_list = "";
	for (std::vector<std::string>::iterator user_it = it->second.users.begin(); user_it != it->second.users.end(); ++user_it)
	{
		user_list += *user_it + " ";
	}
	send_response(current_user.socket, "353", "= " + channel_name + " :" + user_list);
	send_response(current_user.socket, "366", channel_name + " :End of /NAMES list");
}