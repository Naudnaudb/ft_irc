#include "IrcServer.hpp"

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
		if (add_chan_operator(current_user.socket, current_chan, tokens[3]) == 0)
			return ;
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
	// send formatted message to the channel
	//if tokens[3] is empty, send only the mode
	std::string formatted_message;
	if (tokens.size() == 3)
		formatted_message = ":" + current_user.nickname + "!" + current_user.username + "@" + SERVER_NAME + " MODE " + current_chan.name + " " + mode;
	else
		formatted_message = ":" + current_user.nickname + "!" + current_user.username + "@" + SERVER_NAME + " MODE " + current_chan.name + " " + mode + " " + tokens[3];
	send_message_to_channel(current_chan, formatted_message);
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
		if (remove_chan_operator(current_user.socket, current_chan, tokens[3]) == 0)
			return ;
		// current_user.channels[current_chan.name] = false;
	}
	else if (mode[1] == 'l')
	{
		current_chan.mode['l'] = false;
		current_chan.user_limit = __INT_MAX__;
	}
	// send formatted message to the channel
	//if tokens[3] is empty, send only the mode
	std::string formatted_message;
	if (tokens.size() == 3)
		formatted_message = ":" + current_user.nickname + "!" + current_user.username + "@" + SERVER_NAME + " MODE " + current_chan.name + " " + mode;
	else
		formatted_message = ":" + current_user.nickname + "!" + current_user.username + "@" + SERVER_NAME + " MODE " + current_chan.name + " " + mode + " " + tokens[3];
	send_message_to_channel(current_chan, formatted_message);
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
		return send_response(current_user.socket, "472", "Unknown mode");
}
