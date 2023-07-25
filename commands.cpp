#include "IrcServer.hpp"

//check if user exists
int		IrcServer::user_exists(const std::string & nick)
{
	for (std::map<int, user>::iterator it = users_list.begin(); it != users_list.end(); ++it)
	{
		if (it->second.nickname == nick)
			return 1;
	}
	return 0;
}

void IrcServer::add_chan_operator(const int current_user_socket, channel & current_chan, const std::string & user_to_promote)
{
	// check if user_to_promote is on the channel
	if (std::find(current_chan.users.begin(), current_chan.users.end(), user_to_promote) == current_chan.users.end())
		return (send_response(current_user_socket, "442", user_to_promote + " " + current_chan.name + " :They aren't on that channel"));
	// check if user_to_promote exists
	if (!user_exists(user_to_promote))
		return (send_response(current_user_socket, "401", user_to_promote + " :No such nick"));
	current_chan.operators.push_back(user_to_promote);
}

void IrcServer::remove_chan_operator(const int current_user_socket, channel & current_chan, const std::string & user_to_remove)
{
	// check if user_to_remove is on the channel
	if (std::find(current_chan.users.begin(), current_chan.users.end(), user_to_remove) == current_chan.users.end())
		return (send_response(current_user_socket, "442", user_to_remove + " " + current_chan.name + " :They aren't on that channel"));
	// check if user_to_remove exists
	if (!user_exists(user_to_remove))
		return (send_response(current_user_socket, "401", user_to_remove + " :No such nick"));
	// check if the user isn't operator
	std::vector<std::string>::iterator it;
	if ((it = std::find(current_chan.operators.begin(), current_chan.operators.end(), user_to_remove)) == current_chan.operators.end())
		return ;
	current_chan.operators.erase(it);
}

void IrcServer::add_chan_mode(channel & current_chan, user & current_user, const std::vector<std::string> &tokens)
{
	std::string mode = tokens[2];
	if (mode[1] == 'i')
		current_chan.mode['i'] = true;
	else if (mode[1] == 't')
		current_chan.mode['t'] = true;
	else if (mode[1] == 'k')
	{
		if (tokens.size() != 4 || tokens[3].empty())
			return (send_response(current_user.socket, "461", "This option needs a parameter"));
		current_chan.mode['k'] = true;
		current_chan.key = tokens[3];
	}
	else if (mode[1] == 'o')
	{
		if (tokens.size() != 4 || tokens[3].empty())
			return (send_response(current_user.socket, "461", "This option needs a parameter"));
		add_chan_operator(current_user.socket, current_chan, tokens[3]);
		// current_user.channels[current_chan.name] = true;
	}
	else if (mode[1] == 'l')
	{
		if (tokens.size() != 4)
			return (send_response(current_user.socket, "461", "This option needs a parameter"));

		current_chan.mode['l'] = true;
		int i = atoi(tokens[3].c_str());
		if (i > 1)
			current_chan.user_limit = i;
		else
			return (send_response(current_user.socket, "461", "Last parameter must be > 1"));
	}
}

void IrcServer::remove_chan_mode(channel & current_chan, user & current_user, const std::vector<std::string> &tokens)
{
	std::string mode = tokens[2];
	if (mode[1] == 'i')
		current_chan.mode['i'] = false;
	else if (mode[1] == 't')
		current_chan.mode['t'] = false;
	else if (mode[1] == 'k')
	{
		if (tokens.size() != 4 || tokens[3].empty())
			return (send_response(current_user.socket, "461", "This option needs a parameter"));
		current_chan.mode['k'] = false;
		current_chan.key = "";
	}
	else if (mode[1] == 'o')
	{
		if (tokens.size() != 4 || tokens[3].empty())
			return (send_response(current_user.socket, "461", "This option needs a parameter"));
		remove_chan_operator(current_user.socket, current_chan, tokens[3]);
		// current_user.channels[current_chan.name] = false;
	}
	else if (mode[1] == 'l')
	{
		current_chan.mode['l'] = false;
		current_chan.user_limit = __INT_MAX__;
	}
}

void IrcServer::request_chan_modes(const std::vector<std::string> &tokens, user &current_user)
{
	if (tokens.size() < 2)
		return (send_response(current_user.socket, "461", "This command needs at least 1 parameter : channel"));
	//avoid the "no such channel" message at connection
	if (tokens[1][0] != '#' && tokens[1][0] != '&')
		return ;
	if (channels_list.find(tokens[1]) == channels_list.end())
		return (send_response(current_user.socket, "403", tokens[1] + " :No such channel"));
	channel &current_chan = channels_list.at(tokens[1]);
	std::string response = current_user.nickname + " " + current_chan.name + " +";
	for (std::map<char, bool>::iterator it = current_chan.mode.begin(); it != current_chan.mode.end(); ++it)
	{
		if (it->second == true)
			response += it->first;
	}
	send_response(current_user.socket, "324", response);
}

void IrcServer::mode_command(const std::vector<std::string> &tokens, user &current_user)
{
	if (tokens.size() < 3)
		return (request_chan_modes(tokens, current_user));
	//avoid the "no such channel" message at connection
	if (tokens[1][0] != '#' && tokens[1][0] != '&')
		return ;
	if (channels_list.find(tokens[1]) == channels_list.end())
		return (send_response(current_user.socket, "403", tokens[1] + " :No such channel"));
	channel &current_chan = channels_list.at(tokens[1]);
	if (std::find(current_chan.users.begin(), current_chan.users.end(), current_user.nickname) == current_chan.users.end())
		return (send_response(current_user.socket, "442", current_chan.name + " :You're not on that channel"));
	if (std::find(current_chan.operators.begin(), current_chan.operators.end(), current_user.nickname) == current_chan.operators.end())
		return (send_response(current_user.socket, "482", current_chan.name + " :You're not channel operator"));
	std::string mode = tokens[2];
	if (mode[0] == '+')
		add_chan_mode(current_chan, current_user, tokens);
	else if (mode[0] == '-')
		remove_chan_mode(current_chan, current_user, tokens);
	else
		send_response(current_user.socket, "472", "Unknown mode");
}

void IrcServer::whois_command(int client_socket, const std::string &nickname)
{
	nickname.empty();
	std::string message = "You are in invisible mode";
	send_response(client_socket, "WHOIS", message);
}

int IrcServer::check_channel_name(const std::string &channel_name)
{
	if (channel_name.empty() || channel_name.size() > 200)
		return (0);
	if (channel_name[0] != '#' && channel_name[0] != '&')
		return (0);
	for (size_t i = 1; i < channel_name.size(); i++)
	{
		if (channel_name[i] == ' ' || channel_name[i] == 7 || channel_name[i] == ',' || channel_name[i] == '#' || channel_name[i] == '&')
			return (0);
	}
	return (1);
}

int IrcServer::is_operator(const std::string & current_user, const channel & current_chan)
{
	return (std::find(current_chan.operators.begin(), current_chan.operators.end(), current_user) != current_chan.operators.end());
}

void IrcServer::join_command(user &current_user, const std::vector<std::string> &tokens)
{
	// Vérifier si le nombre de paramètres est correct
	if (tokens.size() < 2)
		return (send_response(current_user.socket, "461", "This command needs at least 1 parameter : channel name"));
	std::string channel_name = tokens[1];
	// Vérifier si le canal existe déjà
	if (channel_name[0] != '#' && channel_name[0] != '&')
		return (send_response(current_user.socket, "403", channel_name + " :No such channel"));
	std::map<std::string, channel>::iterator it = channels_list.find(channel_name);
	if (it == channels_list.end())
	{
		if (!check_channel_name(channel_name))
			return (send_response(current_user.socket, "403", channel_name + " :No such channel"));
		channel new_channel(current_user.nickname, channel_name);
		channels_list.insert(std::pair<std::string, channel>(channel_name, new_channel));
	}
	else
	{
		// Si le canal existe, ajouter l'utilisateur à la liste des utilisateurs du canal
		it->second.users.push_back(current_user.nickname);
	}

	// if the topic is set send it to the user
	if (it != channels_list.end() && !it->second.topic.empty())
		send_response(current_user.socket, "332", current_user.nickname + " " + channel_name + " :" + it->second.topic);

	// Envoyer la liste des utilisateurs du canal à l'utilisateur
	std::string formatted_message;
	channel current_chan = channels_list.at(channel_name);
	formatted_message = current_user.nickname + " = " + channel_name + " :";
	for (std::vector<std::string>::iterator user_it = current_chan.users.begin(); user_it != current_chan.users.end(); ++user_it)
	{
		if (is_operator(*user_it, current_chan))
			formatted_message += "@";
		formatted_message += *user_it + " ";
	}
	send_response(current_user.socket, "353", formatted_message);

	// Envoyer un message de fin de liste des utilisateurs du canal à l'utilisateur
	formatted_message = current_user.nickname + " " + channel_name + " :End of /NAMES list.";
	send_response(current_user.socket, "366", formatted_message);

	// Notify users in the channel that a new user joined
	formatted_message = ":" + current_user.username + "!" + current_user.nickname + "@" + SERVER_NAME + " JOIN :" + channel_name;
	send_message_to_channel(current_user.nickname, current_chan, formatted_message);
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

int IrcServer::user_command(user &current_user, const std::vector<std::string>& tokens)
{
	if (current_user.status < NICKNAME_SET)
		return -1;
	if (tokens.size() < 5)
	{
		send_response(current_user.socket, "461", "ERR_NEEDMOREPARAMS, you should provide 4 parameters. Your current username is your nickname.");
		return -1;
	}
	if (current_user.status == REGISTERED)
	{
		send_response(current_user.socket, "462", "ERR_ALREADYREGISTRED");
		return -1;
	}
	current_user.username = tokens[1];
	// remove ':' from realname and concatenate all tokens
	current_user.realname = tokens[4];
	for (size_t i = 5; i < tokens.size(); ++i)
		current_user.realname += " " + tokens[i];
	current_user.realname.erase(0, 1);
	current_user.status = REGISTERED;
	send_message_to_client(current_user.socket, ":" + std::string(SERVER_NAME) + " 001 " + current_user.nickname + " :Welcome to the server " + SERVER_NAME + ", " + current_user.nickname + " !");
	return (0);
}
#include <stdlib.h>
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