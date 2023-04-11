#include "IrcServer.hpp"

void IrcServer::handle_client_connection(int client_socket)
{
	struct sockaddr_in client_address_;

	socklen_t client_address_length = sizeof(client_address_);
	getpeername(client_socket, (struct sockaddr *)&client_address_, &client_address_length);
	std::cout << "Nouvelle connexion entrante : " << inet_ntoa(client_address_.sin_addr) << std::endl;

	// Envoyer le message de bienvenue au client
	std::string welcome_message = "Bienvenue sur le serveur IRC";
	send_message_to_client(client_socket, welcome_message);

	char buffer[4096];
	int bytes_received = recv(client_socket, buffer, 4096, 0);

	// Vérifier le mot de passe du client
	std::string password;
	std::string message(buffer, bytes_received);
	std::vector<std::string> tokens = tokenize(message);
	int is_connected = 0;
	if (!tokens.empty())
	{
		for (size_t i = 0; i < tokens.size(); i++)
		{
			std::string command = tokens[i];
			// std::cout << "Commande reçue : " << command << std::endl;
			if (command == "CAP" || command == "LS")
				continue;
			else if (command == "PASS" && tokens.size() > 1)
			{
				password = tokens[i+1];
				if (password != password_)
				{
					// Mot de passe incorrect, fermer la connexion
					std::string error_message = "Mot de passe incorrect";
					send_message_to_client(client_socket, error_message);
					close(client_socket);
					return;
				}
				is_connected = 1;
			}
			else if (command == "NICK" && tokens.size() > 1)
			{
				std::string nickname = tokens[i+1];
				handle_nick_command(client_socket, nickname);
			}
			else if (command == "USER" && tokens.size() > 1)
			{
				std::string username = tokens[i+1];
				handle_user_command(client_socket, username);
			}
		}
		if (!is_connected)
		{
			// Mot de passe incorrect, fermer la connexion
			std::string error_message = "Mot de passe absent";
			send_message_to_client(client_socket, error_message);
			close(client_socket);
			return;
		}
	}
}

void IrcServer::handle_command(int client_socket)
{
	char buffer[4096];
	int bytes_received = recv(client_socket, buffer, 4096, 0);
	std::string message(buffer, bytes_received);
	std::vector<std::string> tokens = tokenize(message);

	while (bytes_received > 0)
	{
		std::string command = tokens[0];
		std::cout << "Commande reçue : " << command << std::endl;
		if (command == "JOIN")
		{
			std::string channel = "#test";
			std::string nickname = client_nicknames_[client_socket];
			handle_join_command(client_socket, channel, nickname);
		}
		else if (command == "NICK")
		{
			std::string nickname = "test";
			handle_nick_command(client_socket, nickname);
		}
		else if (command == "PRIVMSG")
		{
			std::string recipient = "#test";
			std::string message = "test";
			handle_privmsg_command(recipient, message);
		}
		else if (command == "PING")
		{
			std::string message = "PONG";
			send_message_to_client(client_socket, message);
		}
		else
		{
			std::cout << "Commande non reconnue" << std::endl;
		}
		bytes_received = recv(client_socket, buffer, 4096, 0);
	}
	std::cout << "Connexion fermée" << std::endl;
	close(client_socket);
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

void IrcServer::handle_user_command(int client_socket, const std::string &username)
{
	client_usernames_[client_socket] = username;
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
		for (std::set<std::string>::iterator channel_it = it->second.begin(); channel_it != it->second.end(); ++channel_it)
			if (*channel_it == recipient)
				clients.insert(it->first);

	for (std::set<int>::iterator client_it = clients.begin(); client_it != clients.end(); ++client_it)
		send_message_to_client(*client_it, message);
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