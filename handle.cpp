#include "include.hpp"

void handle_nick_command(int client_socket, const std::string& new_nickname, std::map<int, std::string>& client_nicknames, std::map<std::string, std::set<int> >& channel_clients)
{
    // Vérifier si le nouveau pseudonyme est déjà utilisé
    std::map<int, std::string>::iterator it;
    for (it = client_nicknames.begin(); it != client_nicknames.end(); ++it) {
        if (it->second == new_nickname) {
            std::string error_message = "Le pseudonyme " + new_nickname + " est déjà utilisé.";
            send(client_socket, error_message.c_str(), error_message.size() + 1, 0);
            return;
        }
    }

    // Mettre à jour le pseudonyme de l'utilisateur
    int client_fd = client_socket;
    std::string old_nickname = client_nicknames[client_fd];
    client_nicknames[client_fd] = new_nickname;

    // Envoyer un message de confirmation à l'utilisateur
    std::string confirmation_message = "Votre pseudonyme a été changé en " + new_nickname;
    send(client_socket, confirmation_message.c_str(), confirmation_message.size() + 1, 0);

    // Envoyer un message de notification aux autres utilisateurs du canal
    std::string notification_message = old_nickname + " a changé son pseudonyme en " + new_nickname;

    // Parcourir tous les sockets clients qui sont dans le même canal
    std::string channel = client_nicknames[client_fd];
    if (channel_clients.count(channel) > 0) {
        std::set<int> clients = channel_clients[channel];
        std::set<int>::iterator it;
        for (it = clients.begin(); it != clients.end(); ++it) {
            // Envoyer le message de notification à chaque client dans le canal
            if (*it != client_socket) {
                send(*it, notification_message.c_str(), notification_message.size() + 1, 0);
            }
        }
    }
}

void handle_join_command(int client_socket, const std::string& channel, const std::string& nickname, std::map<int, std::string>& client_nicknames, std::map<std::string, std::set<int> >& channel_clients)
{
	// Mettre à jour le canal de l'utilisateur
	int client_fd = client_socket;
	client_nicknames[client_fd] = channel;

	// Ajouter le client au canal
	channel_clients[channel].insert(client_socket);

	// Envoyer un message de confirmation à l'utilisateur
	std::string welcome_message = "Bienvenue sur le canal " + channel + ", " + nickname + " !";
	send(client_socket, welcome_message.c_str(), welcome_message.size() + 1, 0);

	// Envoyer un message de notification aux autres utilisateurs du canal
	std::string notification_message = nickname + " a rejoint le canal " + channel;

	// Parcourir tous les sockets clients qui sont dans le même canal
	std::set<int> clients = channel_clients[channel];
	std::set<int>::iterator it;
	for (it = clients.begin(); it != clients.end(); ++it) {
		// Envoyer le message de notification à chaque client dans le canal
		if (*it != client_socket) {
			send(*it, notification_message.c_str(), notification_message.size() + 1, 0);
		}
	}
}

// Divise le message en token en utilisant l'espace comme délimiteur.
std::vector<std::string> tokenize(const std::string& message)
{
    std::vector<std::string> tokens;
    std::stringstream ss(message);
    std::string token;
    while (std::getline(ss, token, ' ')) {
        tokens.push_back(token);
    }
    return tokens;
}

void handle_client_connection(int client_socket, sockaddr_in client_address, std::map<int, std::string>& client_nicknames, std::map<std::string, std::set<int> >& channel_clients)
{
	// traiter la connexion entrante
	std::cout << "Nouvelle connexion entrante : " << inet_ntoa(client_address.sin_addr) << std::endl;

	// lire et écrire des données avec le client
	char buffer[4096];
	int bytes_received = recv(client_socket, buffer, 4096, 0);
	while (bytes_received > 0) {
		std::string message(buffer, bytes_received);
		std::cout << "Message reçu : " << message << std::endl;

		std::vector<std::string> tokens = tokenize(message);
		// Traitement des commandes IRC
		if (!tokens.empty()) {
			std::string command = tokens[0];
			if (command == "JOIN" && tokens.size() > 1) {
				std::string channel = tokens[1];
				std::string nickname = "default"; // TODO: replace with actual nickname
				handle_join_command(client_socket, channel, nickname, client_nicknames, channel_clients);
			} else if (command == "NICK" && tokens.size() > 1) {
				std::string nickname = tokens[1];
				handle_nick_command(client_socket, nickname, client_nicknames, channel_clients);
			} else if (command == "PRIVMSG" && tokens.size() > 2) {
				std::string recipient = tokens[1];
				std::string message = tokens[2];
				std::cout << "Message privé de " << recipient << " : " << message << std::endl;
			} else {
				std::cout << "Commande non reconnue" << std::endl;
			}
		}

		// afficher le message reçu sur la console
		std::cout << "Message reçu : " << buffer << std::endl;
		// envoyer la réponse au client
		send(client_socket, buffer, bytes_received, 0);
		// lire le prochain message du client
		bytes_received = recv(client_socket, buffer, 4096, 0);
	}

	std::cout << "Connexion fermée" << std::endl;
	close(client_socket);
}
