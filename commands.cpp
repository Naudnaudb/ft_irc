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

void IrcServer::join_command(user &current_user, const std::string &channel)
{
	// ajouter join
}

void IrcServer::privmsg_command(const std::string &recipient, const std::string &message)
{
	// ajouter privmsg
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