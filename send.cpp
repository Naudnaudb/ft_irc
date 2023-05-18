#include "IrcServer.hpp"

void IrcServer::send_response(int client_socket, const std::string &response_code, const std::string &message)
{
	std::string response = ":" + std::string(SERVER_NAME) + " " + response_code + " : " + message;
	send_message_to_client(client_socket, response);
}

void IrcServer::send_message_to_client(int client_socket, const std::string &message)
{
	// Ajoute un saut de ligne à la fin du message pour respecter le protocole IRC
	std::string formatted_message = message + "\r\n";

	// Envoyer le message au client
	ssize_t bytes_sent = send(client_socket, formatted_message.c_str(), formatted_message.length(), 0);

	// Vérifier si l'envoi a réussi
	if (bytes_sent == -1)
		std::cerr << "Error: sending message to client (socket: " << client_socket << "): " << strerror(errno) << std::endl;
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
