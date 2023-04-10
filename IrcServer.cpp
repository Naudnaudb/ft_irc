#include "IrcServer.hpp"

#define MAX_CONNECTIONS 10

IrcServer::IrcServer(int port, const std::string &password) : password_(password), port_(port)
{
	// Création du socket
	if ((server_socket_ = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Erreur lors de la création du socket");
		exit(EXIT_FAILURE);
	}

	int opt = 1;
	if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	// Configuration de l'adresse du serveur
	server_address_.sin_family = AF_INET;
	server_address_.sin_addr.s_addr = INADDR_ANY;
	server_address_.sin_port = htons(port_);

	// Liaison du socket à l'adresse du serveur
	if (bind(server_socket_, (struct sockaddr *)&server_address_, sizeof(server_address_)) == -1)
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

void IrcServer::run()
{
	// Boucle principale du serveur
	while (true)
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
		// Gérer le protocole IRC
		// Gérer les erreurs et les exceptions
	}

	// Fermer le socket du serveur
	close(server_socket_);
}

void IrcServer::handle_client_connection(int client_socket)
{
	struct sockaddr_in client_address_;

	socklen_t client_address_length = sizeof(client_address_);
	getpeername(client_socket, (struct sockaddr *)&client_address_, &client_address_length);
	std::cout << "Nouvelle connexion entrante : " << inet_ntoa(client_address_.sin_addr) << std::endl;

	char buffer[4096];
	int bytes_received = recv(client_socket, buffer, 4096, 0);
	while (bytes_received > 0)
	{
		std::string message(buffer, bytes_received);
		std::cout << "Message reçu : " << message << std::endl;

		std::vector<std::string> tokens = tokenize(message);
		if (!tokens.empty())
		{
			std::string command = tokens[0];
			if (command == "JOIN" && tokens.size() > 1)
			{
				std::string channel = tokens[1];
				std::string nickname = client_nicknames_[client_socket];
				handle_join_command(client_socket, channel, nickname);
			}
			else if (command == "NICK" && tokens.size() > 1)
			{
				std::string nickname = tokens[1];
				handle_nick_command(client_socket, nickname);
			}
			else if (command == "PRIVMSG" && tokens.size() > 2)
			{
				std::string recipient = tokens[1];
				std::string message = tokens[2];
				handle_privmsg_command(recipient, message);
			}
			else
			{
				std::cout << "Commande non reconnue" << std::endl;
			}
		}

		std::cout << "Message reçu : " << buffer << std::endl;
		send(client_socket, buffer, bytes_received, 0);
		bytes_received = recv(client_socket, buffer, 4096, 0);
	}

	std::cout << "Connexion fermée" << std::endl;
	close(client_socket);
}

std::vector<std::string> IrcServer::tokenize(const std::string &message)
{
	std::vector<std::string> tokens;
	std::stringstream ss(message);
	std::string token;
	while (std::getline(ss, token, ' '))
	{
		tokens.push_back(token);
	}
	return tokens;
}

std::set<std::string> IrcServer::get_client_channels(int client_socket)
{
	std::set<std::string> channels;

	std::map<int, std::set<std::string> >::iterator it;
	for (it = client_channels_.begin(); it != client_channels_.end(); ++it)
	{
		if (it->first == client_socket)
		{
			channels.insert(it->second.begin(), it->second.end());
		}
	}
	return channels;
}

void IrcServer::handle_nick_command(int client_socket, const std::string &nickname)
{
	// Vérifier si le pseudonyme est déjà utilisé
	std::map<int, std::string>::iterator it;
	for (it = client_nicknames_.begin(); it != client_nicknames_.end(); ++it)
	{
		if (it->second == nickname && it->first != client_socket)
		{
			send_message_to_client(client_socket, "ERR_NICKNAMEINUSE :Ce pseudonyme est déjà utilisé");
			return;
		}
	}

	// Changer le pseudonyme du client
	client_nicknames_[client_socket] = nickname;

	// Envoyer un message de bienvenue au client avec son nouveau pseudonyme
	std::set<std::string> channels = get_client_channels(client_socket);
	std::set<std::string>::iterator it_set;
	for (it_set = channels.begin(); it_set != channels.end(); ++it_set)
	{
		std::string welcome_message = "Bienvenue sur le canal " + (*it_set) + ", " + nickname + "!";
		send_message_to_channel((*it_set), welcome_message);
	}
}

void IrcServer::handle_join_command(int client_socket, const std::string &channel, const std::string &nickname)
{
	std::set<int> clients;
	std::map<int, std::set<std::string> >::iterator it;
	for (it = client_channels_.begin(); it != client_channels_.end(); ++it)
	{
		if (it->second.count(channel) > 0)
		{
			clients.insert(it->first);
		}
	}

	if (client_channels_[client_socket].count(channel) == 0)
	{
		client_channels_[client_socket].insert(channel);
	}

	std::string welcome_message = "Bienvenue sur le canal " + channel + ", " + nickname + "!";
	std::set<int>::iterator client_it;
	for (client_it = clients.begin(); client_it != clients.end(); ++client_it)
	{
		send_message_to_client(*client_it, welcome_message);
	}
}

void IrcServer::handle_privmsg_command(const std::string &recipient, const std::string &message)
{
	std::cout << "Message privé de " << recipient << " : " << message << std::endl;

	std::set<int> clients;
	for (std::map<int, std::set<std::string> >::iterator it = client_channels_.begin(); it != client_channels_.end(); ++it)
	{
		for (std::set<std::string>::iterator channel_it = it->second.begin(); channel_it != it->second.end(); ++channel_it)
		{
			if (*channel_it == recipient)
			{
				clients.insert(it->first);
			}
		}
	}

	for (std::set<int>::iterator client_it = clients.begin(); client_it != clients.end(); ++client_it)
	{
		send_message_to_client(*client_it, message);
	}
}

void IrcServer::send_message_to_client(int client_socket, const std::string &message)
{
	// Ajoute un saut de ligne à la fin du message pour respecter le protocole IRC
	std::string formatted_message = message + "\r\n";

	// Envoyer le message au client
	ssize_t bytes_sent = send(client_socket, formatted_message.c_str(), formatted_message.length(), 0);

	// Vérifier si l'envoi a réussi
	if (bytes_sent == -1)
	{
		std::cerr << "Erreur lors de l'envoi du message au client (socket: " << client_socket << "): " << strerror(errno) << std::endl;
	}
}

void IrcServer::send_message_to_channel(const std::string &channel, const std::string &message)
{
	std::set<int> clients;
	std::map<int, std::set<std::string> >::iterator it;
	for (it = client_channels_.begin(); it != client_channels_.end(); ++it)
	{
		if (it->second.count(channel) > 0)
		{
			clients.insert(it->first);
		}
	}

	std::set<int>::iterator client_it;
	for (client_it = clients.begin(); client_it != clients.end(); ++client_it)
	{
		send_message_to_client(*client_it, message);
	}
}

void IrcServer::handle_part_command(int client_socket, const std::string &channel, const std::string &nickname)
{
	// Supprimer le client de la liste des clients du canal
	std::set<std::string> &channels = client_channels_[client_socket];
	channels.erase(channel);

	// Envoyer un message de départ aux autres clients du canal
	std::string goodbye_message = nickname + " a quitté le canal " + channel;
	send_message_to_channel(channel, goodbye_message);

	// Supprimer le canal de la liste des canaux du client si nécessaire
	if (channels.empty())
	{
		client_channels_.erase(client_socket);
	}
}