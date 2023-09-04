#include "IrcServer.hpp"

void IrcServer::join_command(user &current_user, const std::vector<std::string> &tokens)
{
	// Vérifier si le nombre de paramètres est correct
	if (tokens.size() < 2)
		return (send_response(current_user.socket, "461", "This command needs at least 1 parameter : channel name"));
	std::string channel_name = tokens[1];
	// Vérifier si le canal existe déjà
	if (channel_name[0] != '#' && channel_name[0] != '&')
		return (send_response(current_user.socket, "403", channel_name + " :No such channel"));
	// check if password is given
	std::string password;
	if (tokens.size() >= 3)
		password = tokens[2];
	std::map<std::string, channel>::iterator it = channels_list.find(channel_name);
	if (it == channels_list.end())
	{
		if (!check_channel_name(channel_name))
			return (send_response(current_user.socket, "403", channel_name + " :No such channel"));
		channel new_channel(current_user.nickname, channel_name);
		channels_list.insert(std::pair<std::string, channel>(channel_name, new_channel));
	}
	else if (user_can_join_channel(current_user, it->second, password))
	{
		it->second.users.push_back(current_user.nickname);
		// remove user from invited list
		if (it->second.mode.at('i'))
			it->second.invited_users.erase(std::find(it->second.invited_users.begin(), it->second.invited_users.end(), current_user.nickname));
	}
	else
		return ;

	// add channel to user's channel list
	current_user.channels.push_back(channel_name);
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
	formatted_message = ":" + current_user.nickname + "!" + current_user.username + "@" + SERVER_NAME + " JOIN :" + channel_name;
	send_message_to_channel(current_chan, formatted_message);
}

void IrcServer::privmsg_command(user &current_user, const std::vector<std::string> &tokens)
{
	// Vérifier si le nombre de paramètres est correct
	if (tokens.size() < 3)
		return (send_response(current_user.socket, "411", ":No recipient given PRIVMSG command"));
	std::vector<std::string> targets;
	// Put the targets in a vector
	split(tokens[1], ',', targets);
	// Get the message
	std::string message = tokens[2];

	// Send the message to each target
	for (std::vector<std::string>::iterator it = targets.begin(); it != targets.end(); ++it)
	{
		std::string target = *it;
		std::string formatted_message = ":" + current_user.nickname + "!" + current_user.username + "@" + SERVER_NAME + " PRIVMSG " + target + " " + message;
		// Vérifier si c'est un channel
		if (target[0] == '#' || target[0] == '&')
		{
			// Vérifier si le channel existe et envoyer le message
			std::map<std::string, channel>::iterator chan_it = channels_list.find(target);
			if (chan_it != channels_list.end())
				send_message_to_channel_except(current_user.nickname, chan_it->second, formatted_message);
		}
		else
		{
			// Vérifier si l'utilisateur existe
			int target_socket = get_user_socket_by_nick(target);
			if (target_socket != -1)
				send_message_to_client(target_socket, formatted_message);
		}
	}
}

void IrcServer::part_command(user &current_user, const std::string &channel_name)
{
	// Vérifier si le canal existe
	std::map<std::string, channel>::iterator it = channels_list.find(channel_name);
	if (it == channels_list.end())
		return send_response(current_user.socket, "403", channel_name + " :No such channel");
	channel & current_channel = it->second;

	std::vector<std::string>::iterator user_it = std::find(current_channel.users.begin(), current_channel.users.end(), current_user.nickname);
	if (user_it == current_channel.users.end())
		return send_response(current_user.socket, "442", channel_name + " :You're not on that channel");
	// erase user from channel (send message before erasing)
	std::string formatted_message = ":" + current_user.nickname + "!" + current_user.username + "@" + SERVER_NAME + " PART " + channel_name;
	send_message_to_channel(current_channel, formatted_message);
	current_channel.users.erase(user_it);
	// if user is operator, remove from operator list
	if (is_operator(current_user.nickname, current_channel))
	{
		user_it = std::find(current_channel.operators.begin(), current_channel.operators.end(), current_user.nickname);
		current_channel.operators.erase(user_it);
	}
	// erase channel from user's channel list
	user_it = std::find(current_user.channels.begin(), current_user.channels.end(), channel_name);
	current_user.channels.erase(user_it);
	// if channel is empty, erase it
	if (current_channel.users.empty())
		channels_list.erase(it);
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

void IrcServer::quit_command(user &current_user, const std::string &message)
{
	// Envoyer un message de départ à tous les utilisateurs connectés au canal
	std::string formatted_message = ":" + current_user.nickname + "!" + current_user.username + "@" + SERVER_NAME + " QUIT :" + message;
	send_message_to_client(current_user.socket, formatted_message);
	send_message_to_joined_channels(current_user, formatted_message);
	// Supprimer l'utilisateur de tous les canaux
	for (std::vector<std::string>::iterator it = current_user.channels.begin(); it != current_user.channels.end(); ++it)
	{
		std::map<std::string, channel>::iterator chan_it = channels_list.find(*it);
		channel & current_chan = chan_it->second;
		if (chan_it != channels_list.end())
		{
			current_chan.users.erase(std::find(current_chan.users.begin(), current_chan.users.end(), current_user.nickname));
			if (is_operator(current_user.nickname, current_chan))
				current_chan.operators.erase(std::find(current_chan.operators.begin(), current_chan.operators.end(), current_user.nickname));
			// if channel is empty, delete it
			if (current_chan.users.empty())
				channels_list.erase(chan_it);
		}
	}
	// Supprimer les channels de l'utilisateur
	current_user.channels.clear();

	// Fermer la socket de l'utilisateur
	// shutdown(current_user.socket, SHUT_RDWR);
	close(current_user.socket);
	// changer le status de l'utilisateur
	current_user.status = DISCONNECTED;
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

void IrcServer::topic_command(const user & current_user, const std::vector<std::string> & tokens)
{
	// Pas de channel fourni
	if (tokens.size() < 2)
		return send_response(current_user.socket, "461", "TOPIC :Not enough parameters");
	// Vérifier si le canal existe
	std::map<std::string, channel>::iterator it = channels_list.find(tokens[1]);
	if (it == channels_list.end())
		return send_response(current_user.socket, "403", tokens[1] + " :No such channel");
	channel & current_channel = it->second;
	// Vérifier si l'utilisateur est connecté au canal
	if (std::find(current_channel.users.begin(), current_channel.users.end(), current_user.nickname) == current_channel.users.end())
		return send_response(current_user.socket, "442", current_channel.name + " :You're not on that channel");
	// Si l'utilisateur n'est pas opérateur, envoyer une erreur
	if (current_channel.mode.at('t') && !is_operator(current_user.nickname, current_channel))
		return send_response(current_user.socket, "482", current_channel.name + " :You're not channel operator");
	// Si pas de topic fourni, envoyer le topic du canal
	if (tokens.size() == 2)
	{
		if (current_channel.topic.empty())
			return send_response(current_user.socket, "331", current_channel.name + " :No topic is set");
		return send_response(current_user.socket, "332", current_channel.name + " :" + current_channel.topic);
	}
	// Changer le topic du canal
	current_channel.topic = tokens[2];
	// Notifier les utilisateurs du canal du changement de topic
	std::string formatted_message = ":" + current_user.nickname + "!" + current_user.username + "@" + SERVER_NAME + " TOPIC " + current_channel.name + " " + current_channel.topic;
	send_message_to_channel(current_channel, formatted_message);
}

void IrcServer::invite_command(const user & current_user, const std::vector<std::string> & tokens)
{
	// Pas de channel fourni
	if (tokens.size() < 3)
		return send_response(current_user.socket, "461", "INVITE :Not enough parameters");
	// Vérifier si l'utilisateur existe
	if (!user_exists(tokens[1]))
		return send_response(current_user.socket, "401", tokens[1] + " :No such nick");
	user & invited_user = users_list.at(get_user_socket_by_nick(tokens[1]));
	// Vérifier si le canal existe
	std::map<std::string, channel>::iterator chan_it = channels_list.find(tokens[2]);
	if (chan_it == channels_list.end())
		return send_response(current_user.socket, "403", tokens[2] + " :No such channel");
	channel & current_channel = chan_it->second;
	// Vérifier si l'utilisateur est connecté au canal
	if (std::find(current_channel.users.begin(), current_channel.users.end(), current_user.nickname) == current_channel.users.end())
		return send_response(current_user.socket, "442", current_channel.name + " :You're not on that channel");
	// Si l'utilisateur n'est pas opérateur, envoyer une erreur
	if (current_channel.mode.at('i') && !is_operator(current_user.nickname, current_channel))
		return send_response(current_user.socket, "482", current_channel.name + " :You're not channel operator");
	// Si l'utilisateur est déjà connecté au canal, envoyer une erreur
	if (std::find(current_channel.users.begin(), current_channel.users.end(), invited_user.nickname) != current_channel.users.end())
		return send_response(current_user.socket, "443", invited_user.nickname + " " + current_channel.name + " :is already on channel");
	// Confirmer l'invitation à l'utilisateur
	send_response(current_user.socket, "341", current_user.nickname + " " + invited_user.nickname + " " + current_channel.name);
	// Envoyer l'invitation à l'utilisateur
	std::string formatted_message = ":" + current_user.nickname + "!" + current_user.username + "@" + SERVER_NAME + " INVITE " + invited_user.nickname + " :" + current_channel.name;
	send_message_to_client(invited_user.socket, formatted_message);
	// Ajouter l'utilisateur à la liste des utilisateurs invités
	if (std::find(current_channel.invited_users.begin(), current_channel.invited_users.end(), invited_user.nickname) == current_channel.invited_users.end())
		current_channel.invited_users.push_back(invited_user.nickname);
}

void IrcServer::kick_command(const user & current_user, const std::vector<std::string> & tokens)
{
	// Pas de channel fourni
	if (tokens.size() < 3)
		return send_response(current_user.socket, "461", "KICK :Not enough parameters");
	// Vérifier si le canal existe
	std::map<std::string, channel>::iterator chan_it = channels_list.find(tokens[1]);
	if (chan_it == channels_list.end())
		return send_response(current_user.socket, "403", tokens[1] + " :No such channel");
	channel & current_channel = chan_it->second;
	// Vérifier si l'utilisateur est connecté au canal
	if (std::find(current_channel.users.begin(), current_channel.users.end(), current_user.nickname) == current_channel.users.end())
		return send_response(current_user.socket, "442", current_channel.name + " :You're not on that channel");
	// Si l'utilisateur n'est pas opérateur, envoyer une erreur
	if (!is_operator(current_user.nickname, current_channel))
		return send_response(current_user.socket, "482", current_channel.name + " :You're not channel operator");
	// Vérifier si l'utilisateur à kicker existe
	std::map<int, user>::iterator user_it = users_list.find(get_user_socket_by_nick(tokens[2]));
	if (user_it == users_list.end())
		return send_response(current_user.socket, "401", tokens[2] + " :No such nick");
	user & kicked_user = user_it->second;
	// Vérifier si l'utilisateur à kicker est connecté au canal
	if (std::find(current_channel.users.begin(), current_channel.users.end(), kicked_user.nickname) == current_channel.users.end())
		return send_response(current_user.socket, "441", kicked_user.nickname + " " + current_channel.name + " :isn't on that channel");
	// Vérifier si l'utilisateur à kicker est opérateur
	if (is_operator(kicked_user.nickname, current_channel))
		return send_response(current_user.socket, "482", current_channel.name + " :" + kicked_user.nickname + " is an operator");
	// Envoyer un message de kick à l'utilisateur
	std::string reason = tokens.size() > 3 ? tokens[3] : "";
	std::string formatted_message = ":" + current_user.nickname + "!" + current_user.username + "@" + SERVER_NAME + " KICK " + current_channel.name + " " + kicked_user.nickname + " :" + current_user.nickname + " " + reason;
	send_message_to_channel(current_channel, formatted_message);
	// Supprimer l'utilisateur du canal
	current_channel.users.erase(std::find(current_channel.users.begin(), current_channel.users.end(), kicked_user.nickname));
	// Supprimer le canal de la liste des canaux de l'utilisateur
	kicked_user.channels.erase(std::find(kicked_user.channels.begin(), kicked_user.channels.end(), current_channel.name));
}