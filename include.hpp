#ifndef INCLUDE_HPP
# define INCLUDE_HPP

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
#include <sstream>

void handle_client_connection(int client_socket, sockaddr_in client_address, std::map<int, std::string>& client_nicknames, std::map<std::string, std::set<int> >& channel_clients);

#endif