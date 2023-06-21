#include "IrcServer.hpp"

#define MAX_CONNECTIONS 1000


std::vector<std::string> IrcServer::tokenize(const std::string &message)
{
	std::vector<std::string> tokens;

	size_t start = 0;
	size_t end = message.find_first_of(" \n");

	while (end != std::string::npos)
	{
		if (end > start && end <= message.size())
			tokens.push_back(message.substr(start, end - start));

		start = end + 1;
		end = message.find_first_of(" \n\r", start);
	}

	if (start < message.size())
		tokens.push_back(message.substr(start));

	return tokens;
}

void IrcServer::poll_client_connections()
{
	std::vector<pollfd> fds_to_poll;
	pollfd server_poll;
	server_poll.fd = server_socket_;
	server_poll.events = POLLIN;
	fds_to_poll.push_back(server_poll);

	while (true)
	{
		int poll_result = poll(fds_to_poll.data(), fds_to_poll.size(), -1);
		if (poll_result < 0)
		{
			perror("Error : Poll");
			exit(EXIT_FAILURE);
		}

		for (size_t i = 0; i < fds_to_poll.size(); ++i)
		{
			if (fds_to_poll[i].revents & POLLIN)
			{
				std::cout << "fds_to_poll size() = " << fds_to_poll.size() << std::endl;
				if (fds_to_poll[i].fd == server_socket_)
				{
					// Accepter une connexion entrante
					socklen_t client_address_length = sizeof(client_address_);
					int client_socket = accept(server_socket_, reinterpret_cast<sockaddr *>(&client_address_), &client_address_length);
					if (client_socket == -1)
					{
						perror("Error: accepting an incoming connection");
						continue;
					}
					// Set I/O fds to non-blocking
					if (fcntl(client_socket, F_SETFL, O_NONBLOCK) == -1)
					{
						perror("Error: setting client fd to non-blocking failed");
						exit(EXIT_FAILURE);
					}
					// Ajouter le nouveau client_socket au pollfd
					pollfd new_client_fd;
					new_client_fd.fd = client_socket;
					new_client_fd.events = POLLIN;
					fds_to_poll.push_back(new_client_fd);
				}
				else
				{
					int client_state = handle_client_connection(fds_to_poll[i].fd);
					// Supprimer le client_socket du pollfd s'il est fermé
					if (client_state == OFFLINE)
					{
						std::cout << "OFFLINE" << std::endl;
						close(fds_to_poll[i].fd);
						fds_to_poll.erase(fds_to_poll.begin() + i);
						--i;
					}
				}
			}
		}
	}
}

IrcServer::IrcServer(int port, const std::string &password) : port_(port), password_(password), users_list()
{
	// Création du socket
	if ((server_socket_ = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Erreur lors de la création du socket");
		exit(EXIT_FAILURE);
	}
	if (fcntl(server_socket_, F_SETFL, O_NONBLOCK) == -1)
	{
		perror("Error: setting server fd to non-blocking failed");
		exit(EXIT_FAILURE);
	}
	int opt = 1;
	if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	// Configuration de l'adresse du serveur
	server_address_.sin_family = AF_INET;
	server_address_.sin_addr.s_addr = INADDR_ANY;
	server_address_.sin_port = htons(port_);

	// Liaison du socket à l'adresse du serveur
	if (bind(server_socket_, (struct sockaddr *)&server_address_, sizeof(server_address_)) < 0)
	{
		perror("Erreur lors de la liaison du socket");
		exit(EXIT_FAILURE);
	}

	// Écoute des connexions entrantes
	if (listen(server_socket_, MAX_CONNECTIONS) == -1)
	{
		perror("Erreur lors de l'écoute des connexions entrantes");
		exit(EXIT_FAILURE);
	}
}
