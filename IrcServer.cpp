#include "IrcServer.hpp"

#define MAX_CONNECTIONS 1000

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



std::vector<std::string> IrcServer::tokenize(const std::string &message)
{
	std::vector<std::string> tokens;

	size_t start = 0;
	size_t end = message.find_first_of(" \n");

	while (end != std::string::npos)
	{
		if (end > start)
		{
			tokens.push_back(message.substr(start, end - start));
		}

		start = end + 1;
		end = message.find_first_of(" \n\r", start);
	}

	if (start < message.size())
	{
		tokens.push_back(message.substr(start));
	}

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

void IrcServer::send_message_to_client(int client_socket, const std::string &message)
{
	// Ajoute un saut de ligne à la fin du message pour respecter le protocole IRC
	std::string formatted_message = message + "\r\n";

	// Envoyer le message au client
	ssize_t bytes_sent = send(client_socket, formatted_message.c_str(), formatted_message.length(), 0);

	// Vérifier si l'envoi a réussi
	if (bytes_sent == -1)
		std::cerr << "Erreur lors de l'envoi du message au client (socket: " << client_socket << "): " << strerror(errno) << std::endl;
}

void IrcServer::send_message_to_channel(const std::string &channel, const std::string &message)
{
	std::set<int> clients;
	std::map<int, std::set<std::string> >::iterator it;
	for (it = client_channels_.begin(); it != client_channels_.end(); ++it)
		if (it->second.count(channel) > 0)
			clients.insert(it->first);

	std::set<int>::iterator client_it;
	for (client_it = clients.begin(); client_it != clients.end(); ++client_it)
		send_message_to_client(*client_it, message);
}