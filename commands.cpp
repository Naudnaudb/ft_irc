#include "IrcServer.hpp"

int to_int(char const *s)
{
	if ( s == NULL || *s == '\0' )
		throw std::invalid_argument("null or empty string argument");
	if ( *s == '-' )
		throw std::invalid_argument("positive integer only.");

	if ( *s == '+' ) 
		++s;

	int result = 0;
	while(*s)
	{
		if ( *s < '0' || *s > '9' )
			throw std::invalid_argument("invalid input string");
		result = result * 10  - (*s - '0');
		++s;
	}
	if (result < 0)
		throw std::invalid_argument("positive integer only.");
	return result;
}

void IrcServer::mode_command(int client_socket, const std::vector<std::string> &tokens, user current_user)
{
	std::string message = "You are in invisible mode";
	send_response(client_socket, "MODE", message);
	return;
	if (tokens.size() < 3)
		return (send_response(client_socket, "461", "ERR_NEEDMOREPARAMS"));
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
				return (send_response(client_socket, "461", "ERR_NEEDMOREPARAMS"));
			channel_t_w_change.mode['k'] = true;
			channel_t_w_change.key = tokens[3];
		}
		else if (mode[1] == 'o')
		{
			if (tokens.size() != 4)
				return (send_response(client_socket, "461", "ERR_NEEDMOREPARAMS"));
			channel_t_w_change.mode['o'] = true;
			current_user.channels[channel_t_w_change.name] = true;
		}
		else if (mode[1] == 'l')
		{
			if (tokens.size() != 4)
				return (send_response(client_socket, "461", "ERR_NEEDMOREPARAMS"));
			channel_t_w_change.mode['l'] = true;
			int i = to_int(tokens[3].c_str());
			channel_t_w_change.user_limit = i;
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
				return (send_response(client_socket, "461", "ERR_NEEDMOREPARAMS"));
			channel_t_w_change.mode['k'] = false;
			channel_t_w_change.key = "";
		}
		else if (mode[1] == 'o')
		{
			if (tokens.size() != 4)
				return (send_response(client_socket, "461", "ERR_NEEDMOREPARAMS"));
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
		send_response(client_socket, "472", "ERR_UNKNOWNMODE");
		return;
	}
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

void IrcServer::part_command(user &current_user, const std::vector<std::string> &tokens)
{
	// check if there is enough arguments
	if (tokens.size() < 2)
	{
		send_response(current_user.socket, "461", "ERR_NEEDMOREPARAMS");
		return;
	}
	// parse the channels to leave
	std::vector<std::string> channels_to_leave = split(tokens[1], ",");
	// for each channel to leave
	for (std::string channel_to_leave; !channels_to_leave.empty(); channels_to_leave.pop_back())
	{
		channel_to_leave = channels_to_leave.back();
		// check if the channel exists
		if (channels_list.find(channel_to_leave) == channels_list.end())
		{
			send_response(current_user.socket, "403", "ERR_NOSUCHCHANNEL");
			return;
		}
		// check if the user is in the channel
		if (current_user.channels.find(channel_to_leave) == current_user.channels.end())
		{
			send_response(current_user.socket, "442", "ERR_NOTONCHANNEL");
			return;
		}
		// remove the user from the channel
		current_user.channels.erase(channel_to_leave);
		// send a message to the channel
		std::string goodbye_message = current_user.nickname + " left the channel " + channel_to_leave;
		send_message_to_channel(channels_list[channel_to_leave], goodbye_message);
	}
}

// void IrcServer::part_command(user &current_user)
// {
// 	// Vérifier si l'utilisateur est dans un canal
// 	if (!current_user.channels.empty())
// 	{
// 		// Prendre le dernier canal de la liste des canaux du client
// 		std::string channel = current_user.channels.back();

// 		// Supprimer le canal de la liste des canaux du client
// 		current_user.channels.pop_back();

// 		// Envoyer un message de départ aux autres clients du canal
// 		std::string goodbye_message = current_user.nickname + " left the channel " + channel;
// 		send_message_to_channel(channel, goodbye_message);
// 	}
// 	else
// 	{
// 		// Envoyer un message indiquant que le client n'est pas dans un canal
// 		std::string error_message = "You're not in any channel";
// 		send_message_to_client(current_user.socket, error_message);
// 	}
// }

int IrcServer::user_command(user &current_user, const std::vector<std::string> &tokens)
{
	if (tokens.size() < 5)
	{
		send_response(current_user.socket, "461", "ERR_NEEDMOREPARAMS");
		return -1;
	}
	if (current_user.authentified == true)
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
	return (0);
}

int IrcServer::nick_command(user &current_user, const std::string &nickname)
{
	// Vérifier si le pseudonyme est vide
	if (nickname.empty())
	{
		send_response(current_user.socket, "431", "No nickname given");
		return -1;
	}
	// Vérifier si le pseudonyme est déjà utilisé
	std::map<int, user>::iterator it;
	for (it = users_list.begin(); it != users_list.end(); ++it)
	{
		user user_checked = it->second;
		if (user_checked.nickname == nickname && it->first != current_user.socket)
		{
			send_response(current_user.socket, "433", "This nickname is already in use");
			return -1;
		}
	}

	// Changer le pseudonyme du client
	std::string old_nickname = current_user.nickname;
	current_user.nickname = nickname;

	// Envoyer un message de bienvenue au client avec son nouveau pseudonyme
	if (current_user.authentified == true)
		send_message_to_client(current_user.socket, ":" + std::string(SERVER_NAME) + " 001 " + nickname + " :Your nickname has change from " + old_nickname + " to " + nickname);
	// 	send_message_to_client(current_user.socket, ":" + std::string(SERVER_NAME) + " 001 " + nickname + " :Welcome to the server " + SERVER_NAME + ", " + nickname + " !");
	// else
	return 0;
}