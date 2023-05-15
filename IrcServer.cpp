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

std::set<std::string> IrcServer::get_client_channels(int client_socket)
{
	std::set<std::string> channels;

	std::map<int, std::set<std::string> >::iterator it;
	for (it = client_channels_.begin(); it != client_channels_.end(); ++it)
		if (it->first == client_socket)
			channels.insert(it->second.begin(), it->second.end());
	return channels;
}

void IrcServer::poll_client_connections()
{
	std::vector<pollfd> fds;
	pollfd server_fd;
	server_fd.fd = server_socket_;
	server_fd.events = POLLIN;
	fds.push_back(server_fd);

	while (true)
	{
		int poll_result = poll(fds.data(), fds.size(), -1);

		if (poll_result < 0)
		{
			perror("Erreur lors du poll");
			exit(EXIT_FAILURE);
		}

		for (size_t i = 0; i < fds.size(); ++i)
		{
			if (fds[i].revents & POLLIN)
			{
				if (fds[i].fd == server_socket_)
				{
					// Accepter une connexion entrante
					socklen_t client_address_length = sizeof(client_address_);
					int client_socket = accept(server_socket_, (struct sockaddr *)&client_address_, &client_address_length);
					if (client_socket == -1)
					{
						perror("Erreur lors de l'acceptation d'une connexion entrante");
						continue;
					}

					// Traiter la connexion entrante
					handle_client_connection(client_socket);
					// Ajouter le nouveau client_socket au pollfd
					pollfd new_client_fd;
					new_client_fd.fd = client_socket;
					new_client_fd.events = POLLIN;
					fds.push_back(new_client_fd);
				}
				else
				{
					// Supprimer le client_socket du pollfd s'il est fermé
					close(fds[i].fd);
					fds.erase(fds.begin() + i);
					i--;
				}
			}
		}
	}
}

IrcServer::IrcServer(int port, const std::string &password) : port_(port), password_(password)
{
	// Création du socket
	if ((server_socket_ = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Erreur lors de la création du socket");
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
