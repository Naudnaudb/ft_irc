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

# define MAX_CLIENTS 100
# define SERVER_NAME "ft_ircserv.fr"

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
		user(const int fd = -1) : nickname(), username(), mode(0), authentified(false), socket(fd), channels() {}
		user(const user &other) : nickname(other.nickname), username(other.username), mode(other.mode), authentified(other.authentified), socket(other.socket), channels(other.channels) {}
		user & operator=(const user &rhs) {
			if (this != &rhs)
			{
				nickname = rhs.nickname;
				username = rhs.username;
				mode = rhs.mode;
				authentified = rhs.authentified;
				socket = rhs.socket;
				channels = rhs.channels;
			}
			return *this;
		}
		std::string					nickname;
		std::string					username;
		int							mode; //masque logique
		bool						authentified;
		int							socket;
		std::vector<std::string>	channels;
	};
	std::vector<std::string> tokenize(const std::string &message);

	//	handle.cpp
	int handle_client_connection(int client_socket);
	int handle_command(int client_socket, const std::vector<std::string> &tokens);
	void nick_command(user &current_user, const std::string &nickname);
	void user_command(user &current_user, const std::string &username);
	void join_command(user &current_user, const std::string &channel);
	void privmsg_command(const std::string &recipient, const std::string &message);
	void part_command(user &current_user);
	void mode_command(int client_socket, const std::string &nickname);
	void whois_command(int client_socket, const std::string &nickname);
	int handle_client_first_connection(int client_socket, std::vector<std::string> tokens);
	
	//	send.cpp
	void send_response(int client_socket, const std::string &response_code, const std::string &message);
	void send_message_to_client(int client_socket, const std::string &message);
	void send_message_to_channel(const std::string &channel, const std::string &message);

	bool check_password(const std::string &password, user &current_user);
	int port_;
	std::string password_;
	int server_socket_;
	struct sockaddr_in server_address_;
	struct sockaddr_in client_address_;


	// users list where int parameter is the fd corresponding to the user
	std::map<int , user> users_list;

	class channel
	{
	public:
		channel(const std::string &name = "") : name(name) {}
		std::string					name;
		std::vector<std::string>	users;
	};
};

#endif
