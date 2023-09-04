#include "IrcServer.hpp"

int		IrcServer::get_user_socket_by_nick(const std::string & nick)
{
	for (std::map<int, user>::iterator it = users_list.begin(); it != users_list.end(); ++it)
	{
		if (it->second.nickname == nick)
			return it->first;
	}
	return -1;
}

int		IrcServer::user_exists(const std::string & nick)
{
	for (std::map<int, user>::iterator it = users_list.begin(); it != users_list.end(); ++it)
	{
		if (it->second.nickname == nick)
			return 1;
	}
	return 0;
}

int IrcServer::add_chan_operator(const int current_user_socket, channel & current_chan, const std::string & user_to_promote)
{
	// check if user_to_promote is on the channel
	if (std::find(current_chan.users.begin(), current_chan.users.end(), user_to_promote) == current_chan.users.end())
		send_response(current_user_socket, "442", user_to_promote + " " + current_chan.name + " :They aren't on that channel");
	// check if user_to_promote exists
	else if (!user_exists(user_to_promote))
		send_response(current_user_socket, "401", user_to_promote + " :No such nick");
	else
	{
		current_chan.operators.push_back(user_to_promote);
		return 1;
	}
	return 0;
}

int IrcServer::remove_chan_operator(const int current_user_socket, channel & current_chan, const std::string & user_to_remove)
{
	std::vector<std::string>::iterator it;
	// check if user_to_remove is on the channel
	if (std::find(current_chan.users.begin(), current_chan.users.end(), user_to_remove) == current_chan.users.end())
		send_response(current_user_socket, "442", user_to_remove + " " + current_chan.name + " :They aren't on that channel");
	// check if user_to_remove exists
	else if (!user_exists(user_to_remove))
		send_response(current_user_socket, "401", user_to_remove + " :No such nick");
	// check if the user isn't operator
	else if ((it = std::find(current_chan.operators.begin(), current_chan.operators.end(), user_to_remove)) != current_chan.operators.end())
	{
		current_chan.operators.erase(it);
		return 1;
	}
	return 0;
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

int IrcServer::user_can_join_channel(const user & current_user, const channel & current_chan, const std::string & password)
{
	// check if the user is already in the channel
	if (std::find(current_chan.users.begin(), current_chan.users.end(), current_user.nickname) != current_chan.users.end())
		return (0);
	// check if the channel limit is reached
	if (current_chan.users.size() >= current_chan.user_limit)
		send_response(current_user.socket, "471", current_user.nickname + " " + current_chan.name + " :Cannot join channel (+l)");
	// check if the channel is invite only
	else if (current_chan.mode.at('i') && std::find(current_chan.invited_users.begin(), current_chan.invited_users.end(), current_user.nickname) == current_chan.invited_users.end())
		send_response(current_user.socket, "473", current_user.nickname + " " + current_chan.name + " :Cannot join channel (+i)");
	// check if the channel is password protected
	else if (current_chan.mode.at('k') && current_chan.key != password)
		send_response(current_user.socket, "475", current_user.nickname + " " + current_chan.name + " :Cannot join channel (+k)");
	else
		return (1);
	return (0);
}

void split(const std::string &s, char delim, std::vector<std::string> & dest)
{
	std::istringstream iss(s);
	std::string item;
	while (std::getline(iss, item, delim))
		dest.push_back(item);
}