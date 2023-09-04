#include "IrcServer.hpp"

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

void IrcServer::update_nick_in_channels(user & current_user, const std::string & old_nickname)
{
	if (current_user.channels.empty())
		return;
	// update user's nickname in all channels
	for (std::vector<std::string>::iterator it = current_user.channels.begin(); it != current_user.channels.end(); ++it)
	{
		// update user's nickname in channel
		std::map<std::string, channel>::iterator chan_it = channels_list.find(*it);
		if (chan_it != channels_list.end())
		{
			// look for user nickname in channel's user list
			for (std::vector<std::string>::iterator user_it = chan_it->second.users.begin(); user_it != chan_it->second.users.end(); ++user_it)
			{
				if (*user_it == old_nickname)
				{
					*user_it = current_user.nickname;
					break;
				}
			}
			// look for user nickname in channel's operator list
			for (std::vector<std::string>::iterator oper_it = chan_it->second.operators.begin(); oper_it != chan_it->second.operators.end(); ++oper_it)
			{
				if (*oper_it == old_nickname)
				{
					*oper_it = current_user.nickname;
					break;
				}
			}
		}
	}
	// send NICK message to all channels
	std::string formatted_message = ":" + old_nickname + "!" + current_user.username + "@" + SERVER_NAME + " NICK :" + current_user.nickname;
	send_message_to_joined_channels(current_user, formatted_message);
}

int IrcServer::nick_command(user &current_user, const std::vector<std::string> & tokens)
{
	// Vérifier si le pseudonyme est vide
	if (tokens.size() < 2 || tokens[1].empty())
	{
		send_response(current_user.socket, "431", "No nickname given");
		return -1;
	}
	if (tokens[1].find_first_of("!@#$%^&*(){}[]|\\\"':;?/>.<,`~") != std::string::npos)
	{
		send_response(current_user.socket, "432", "Error: nickname contains forbidden characters");
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
		update_nick_in_channels(current_user, old_nickname);
	}
	else
		current_user.status = NICKNAME_SET;
	return 0;
}