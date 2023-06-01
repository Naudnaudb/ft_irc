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
    std::map<std::string, channel>::iterator it = channels.find(channel_name);
    if (it == channels.end())
    {
        // Si le canal n'existe pas, le créer
        channel new_channel;
        new_channel.name = channel_name;
        new_channel.users.push_back(current_user.nickname);
        channels[channel_name] = new_channel;
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
    std::map<std::string, channel>::iterator it2 = channels.find(channel_name);
    if (it2 != channels.end())
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

void IrcServer::privmsg_command(user &current_user, const std::string &recipient, const std::string &message)
{
    // Vérifier si le destinataire est un canal ou un utilisateur
    bool is_channel = recipient[0] == '#';

    // Si le destinataire est un canal, envoyer le message à tous les utilisateurs connectés au canal
    if (is_channel)
    {
        std::string channel_name = recipient.substr(1);
        std::string formatted_message = ":" + current_user.nickname + "!" + current_user.username + "@" + SERVER_NAME + " PRIVMSG #" + channel_name + " :" + message;
        send_message_to_channel(channel_name, formatted_message, current_user.socket);
    }
    // Sinon, envoyer le message au destinataire spécifié
    else
    {
        bool user_found = false;
        for (std::map<int, user>::iterator it = users_list.begin(); it != users_list.end(); ++it)
        {
            if (it->second.nickname == recipient)
            {
                user_found = true;
                std::string formatted_message = ":" + current_user.nickname + "!" + current_user.username + "@" + SERVER_NAME + " PRIVMSG " + recipient + " :" + message;
                send_message_to_client(it->second.socket, formatted_message);
                break;
            }
        }
        if (!user_found)
        {
            send_response(current_user.socket, "401", recipient + " :No such nick/channel");
        }
    }
}

void IrcServer::part_command(user &current_user)
{
	// add part
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