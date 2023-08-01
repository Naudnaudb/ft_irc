#include "IrcServer.hpp"

void IrcServer::send_response(int client_socket, const std::string &response_code, const std::string &message)
{
	std::string response = ":" + std::string(SERVER_NAME) + " " + response_code + " " + message;
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

void IrcServer::send_message_to_channel(const channel &current_chan, const std::string &message)
{
    // Envoyer le message à tous les utilisateurs connectés au canal
    for (std::vector<std::string>::const_iterator user_it = current_chan.users.begin(); user_it != current_chan.users.end(); ++user_it)
    {
		std::cout << "sending to user : " << *user_it << std::endl;
        for (std::map<int, user>::iterator user_map_it = users_list.begin(); user_map_it != users_list.end(); ++user_map_it)
        {
            if (user_map_it->second.nickname == *user_it)
                send_message_to_client(user_map_it->second.socket, message);
        }
    }
}

void IrcServer::send_message_to_channel_except(const std::string & sender, const channel &current_chan, const std::string &message)
{
    // Envoyer le message à tous les utilisateurs connectés au canal sauf l'expéditeur
    for (std::vector<std::string>::const_iterator user_it = current_chan.users.begin(); user_it != current_chan.users.end(); ++user_it)
    {
        for (std::map<int, user>::iterator user_map_it = users_list.begin(); user_map_it != users_list.end(); ++user_map_it)
        {
            if (user_map_it->second.nickname == *user_it && user_map_it->second.nickname != sender)
                send_message_to_client(user_map_it->second.socket, message);
        }
    }
}

void IrcServer::send_message_to_joined_channels(const user & current_user, const std::string &message)
{
    // Envoyer le message à tous les utilisateurs connectés au canal
    for (std::vector<std::string>::const_iterator chan_it = current_user.channels.begin(); chan_it != current_user.channels.end(); ++chan_it)
	{
		std::map<std::string, channel>::iterator it = channels_list.find(*chan_it);
		if (it != channels_list.end())
			send_message_to_channel_except(current_user.nickname, it->second, message);
	}
}

void IrcServer::send_message_to_all(const std::string& message)
{
	// Parcourir tous les utilisateurs
	for (std::map<int, user>::iterator it = users_list.begin(); it != users_list.end(); ++it)
	{
		// Envoyer le message à l'utilisateur
		send_message_to_client(it->second.socket, message);
	}
}