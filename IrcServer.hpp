#ifndef IRCSERVER_HPP
# define IRCSERVER_HPP

# include <iostream>
# include <cstdlib>
# include <cstring>
# include <cstdio>
# include <cerrno>
# include <unistd.h>
# include <netdb.h>
# include <arpa/inet.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <csignal>
# include <poll.h>
# include <vector>
# include <map>
# include <set>
# include <sstream>
# include <algorithm>
# include <string>
# include <stdlib.h>

# define MAX_CLIENTS 100
# define SERVER_NAME "ft_ircserv.fr"

// user status
# define UNAUTHENTIFIED 0
# define AUTHENTIFIED 1
# define NICKNAME_SET 2
# define REGISTERED 3
# define DISCONNECTED -1

# define OFFLINE -1

class IrcServer
{
public:
	//	IrcServer.cpp
	IrcServer(int port, const std::string &password);
	void poll_client_connections();

private:
	//	IrcServer.cpp
	class user
	{
	public:
		user(const int fd = -1) : nickname(), username(), realname(), status(UNAUTHENTIFIED), socket(fd), channels() {}
		user(const user &other) : nickname(other.nickname), username(other.username), realname(other.realname), status(other.status), socket(other.socket), channels(other.channels) {}
		user & operator=(const user &rhs) {
			if (this != &rhs)
			{
				nickname = rhs.nickname;
				username = rhs.username;
				realname = rhs.realname;
				status = rhs.status;
				socket = rhs.socket;
				channels = rhs.channels;
			}
			return *this;
		}
		std::string					nickname;
		std::string					username;
		std::string					realname;
		int							status;
		int							socket;
		// std::map<std::string, bool>	channels;// if not needed use a vector instead
		std::vector<std::string>	channels;
	};
	class channel
	{
	public:
		channel(const std::string & creator, const std::string &name = "") : name(name), topic(), users(), operators(), key() {
			mode['i'] = false;
			mode['t'] = false;
			mode['k'] = false;
			mode['l'] = false;
			user_limit = -1;
			users.push_back(creator);
			operators.push_back(creator);
		}
		channel(const channel &other) : name(other.name), topic(other.topic), users(other.users), operators(other.operators), mode(other.mode), key(other.key), user_limit(other.user_limit) {}
		std::string					name;
		std::string					topic;
		std::vector<std::string>	users;
		std::vector<std::string>	operators;
		std::vector<std::string>	invited_users;
		std::map<char, bool>		mode; // char = cle (i, t, k, o, l) & bool = false/true (0, 1)
		std::string					key;
		long unsigned int						user_limit;
	private:
		channel() {}
	};

	std::vector<std::string> tokenize(int client_socket, std::string &message);

	//	handle.cpp
	int handle_client_connection(int client_socket);
	int handle_command(int client_socket, const std::vector<std::string> &tokens);
	int nick_command(user &current_user, const std::vector<std::string> & tokens);
	int user_command(user &current_user, const std::vector<std::string>& tokens);
	void join_command(user &current_user, const std::vector<std::string> &tokens);
	void privmsg_command(user &current_user, const std::vector<std::string> &tokens);
	void part_command(user &current_user, const std::string &channel_name);
	void mode_command(const std::vector<std::string> &tokens, user &current_user);
	void quit_command(user &current_user, const std::string &message);
	void names_command(user &current_user, const std::string &channel_name);
	int handle_client_first_connection(user & current_user, std::vector<std::string> tokens);
	int fix_nickname_collision(user &current_user, std::string nickname);
	int add_chan_operator(const int current_user_socket, channel & current_chan, const std::string & user_to_prommote);
	int remove_chan_operator(const int current_user_socket, channel & current_chan, const std::string & user_to_remove);
	int	user_exists(const std::string & nick);
	void add_chan_mode(channel & current_chan, user & current_user, const std::vector<std::string> &tokens);
	void remove_chan_mode(channel & current_chan, user & current_user, const std::vector<std::string> &tokens);
	void request_chan_modes(const std::vector<std::string> &tokens, user &current_user);
	int	get_user_socket_by_nick(const std::string & nick);
	void update_nick_in_channels(user & current_user, const std::string & old_nickname);
	int user_can_join_channel(const user & current_user, const channel & current_chan, const std::string & password);
	void topic_command(const user & current_user, const std::vector<std::string> & tokens);
	void invite_command(const user & current_user, const std::vector<std::string> & tokens);
	void kick_command(const user & current_user, const std::vector<std::string> & tokens);
	
	//	send.cpp
	void send_response(int client_socket, const std::string &response_code, const std::string &message);
	void send_message_to_client(int client_socket, const std::string &message);
	void send_message_to_channel(const channel &current_chan, const std::string &message);
	void send_message_to_channel_except(const std::string & sender, const channel &current_chan, const std::string &message);
	void send_message_to_joined_channels(const user & current_user, const std::string &message);
	void send_message_to_all(const std::string& message);

	bool check_password(const std::string &password, user &current_user);
	int check_channel_name(const std::string &channel_name);
	int is_operator(const std::string & current_user, const channel & current_chan);

	int port_;
	std::string password_;
	int server_socket_;
	struct sockaddr_in server_address_;
	struct sockaddr_in client_address_;


	// users list where int parameter is the fd corresponding to the user
	std::map<int , user> unregistered_users_list;
	std::map<int , user> users_list;
	std::map<std::string, channel> channels_list;
	std::map<int, std::string> msg_buffer;
};
void split(const std::string &s, char delim, std::vector<std::string> & dest);
#endif
