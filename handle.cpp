#include "IrcServer.hpp"

void IrcServer::handle_quit_command(int client_socket, const std::string &nickname)
{
	nickname.empty();
	close(client_socket);
}

void IrcServer::handle_mode_command(int client_socket, const std::string &nickname)
{
	nickname.empty();
	std::string message = "Vous êtes en mode invisible";
	send_response(client_socket, "MODE", message);
}

void IrcServer::handle_whois_command(int client_socket, const std::string &nickname)
{
	nickname.empty();
	std::string message = "Vous êtes en mode invisible";
	send_response(client_socket, "WHOIS", message);
}

void IrcServer::handle_nick_command(int client_socket, const std::string &nickname)
{
	// Vérifier si le pseudonyme est déjà utilisé
	std::map<int, std::string>::iterator it;
	for (it = client_nicknames_.begin(); it != client_nicknames_.end(); ++it)
	{
		if (it->second == nickname && it->first != client_socket)
		{
			send_response(client_socket, "433", "Ce pseudonyme est déjà utilisé");
			return;
		}
	}

	// Changer le pseudonyme du client
	std::string old_nickname = client_nicknames_[client_socket];
	client_nicknames_[client_socket] = nickname;

	// Envoyer un message de bienvenue au client avec son nouveau pseudonyme
	if (old_nickname.empty())
		send_message_to_client(client_socket, ":" + std::string(SERVER_NAME) + " 001 " + nickname + " :Bienvenue sur le serveur " + SERVER_NAME + ", " + nickname + " !");
	else
		send_message_to_client(client_socket, ":" + std::string(SERVER_NAME) + " 001 " + nickname + " :Votre pseudonyme a été changé de " + old_nickname + " à " + nickname + " !");
}

void IrcServer::handle_user_command(int client_socket, const std::string &username)
{
	// Vérifier si le nom d'utilisateur est déjà utilisé
	std::map<int, std::string>::iterator it;
	for (it = client_usernames_.begin(); it != client_usernames_.end(); ++it)
	{
		if (it->second == username && it->first != client_socket)
		{
			send_response(client_socket, "462", "Vous êtes déjà enregistré");
			return;
		}
	}

	// Changer le nom d'utilisateur du client
	client_usernames_[client_socket] = username;
}

void IrcServer::handle_join_command(int client_socket, const std::string &channel, const std::string &nickname)
{
	std::set<int> clients;
	std::map<int, std::set<std::string> >::iterator it;
	for (it = client_channels_.begin(); it != client_channels_.end(); ++it)
		if (it->second.count(channel) > 0)
			clients.insert(it->first);

	if (client_channels_[client_socket].count(channel) == 0)
		client_channels_[client_socket].insert(channel);

	std::string welcome_message = "Bienvenue sur le canal " + channel + ", " + nickname + "!";
	std::set<int>::iterator client_it;
	for (client_it = clients.begin(); client_it != clients.end(); ++client_it)
		send_message_to_client(*client_it, welcome_message);
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
		client_channels_.erase(client_socket);
}

void IrcServer::handle_command(int client_socket, const std::vector<std::string> &tokens)
{

	std::string command = tokens[0];
	// std::cout << "Commande reçue : " << command << std::endl;
	if (command == "JOIN")
	{
		std::string channel = "#test";
		std::string nickname = client_nicknames_[client_socket];
		handle_join_command(client_socket, channel, nickname);
	}
	else if (command == "NICK")
	{
		handle_nick_command(client_socket, tokens[1]);
	}
	else if (command == "PRIVMSG")
	{
		std::string recipient = "#test";
		std::string message = "test";
		handle_privmsg_command(recipient, message);
	}
	else if (command == "PING")
	{
		send_message_to_client(client_socket, "PONG :" + tokens[1]);
	}
	else if (command == "MODE")
	{
		std::string nickname = client_nicknames_[client_socket];
		handle_mode_command(client_socket, nickname);
	}
	else if (command == "WHOIS")
	{
		std::string nickname = client_nicknames_[client_socket];
		handle_whois_command(client_socket, nickname);
	}
	else if (command == "QUIT")
	{
		std::string nickname = client_nicknames_[client_socket];
		handle_quit_command(client_socket, nickname);
	}
	else
		std::cout << "Commande non reconnue :" << command << std::endl;
}

void IrcServer::handle_client_connection(int client_socket)
{
	struct sockaddr_in client_address_;

	socklen_t client_address_length = sizeof(client_address_);
	getpeername(client_socket, (struct sockaddr *)&client_address_, &client_address_length);
	std::cout << "Nouvelle connexion entrante : " << inet_ntoa(client_address_.sin_addr) << std::endl;

	char buffer[4096];
	int bytes_received;
	bool authenticated = false;
	bool nickname_set = false;
	bool username_set = false;
	std::string nickname = "";

	while ((bytes_received = recv(client_socket, buffer, 4096, 0)) > 0)
	{
		std::string message(buffer, bytes_received);
		// std::cout << "Message reçu : " << message << std::endl;
		std::vector<std::string> tokens = tokenize(message);

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
					std::string password = tokens[i + 1];
					if (password == password_)
						authenticated = true;
					else
					{
						send_response(client_socket, "464", "Mot de passe incorrect");
						close(client_socket);
						return;
					}
					i++;
				}
				else if (!authenticated)
				{
					send_response(client_socket, "464", "Mot de passe absent");
					close(client_socket);
					return;
				}
				else if (command == "NICK" && tokens.size() > 1 && nickname_set == false)
				{
					nickname = tokens[i + 1];
					handle_nick_command(client_socket, nickname);
					nickname_set = true;
					i++;
				}
				else if (command == "USER" && tokens.size() > 1 && username_set == false)
				{
					std::string username = tokens[i + 5] + tokens[i + 6];
					handle_user_command(client_socket, username);
					username_set = true;
					i += 6;
				}
				else if (nickname_set && username_set)
					handle_command(client_socket, tokens);
			}
		}
	}
}
