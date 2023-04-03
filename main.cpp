#include "include.hpp"
# define MAX_CONNECTIONS 10

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		std::cout << "Usage: ./ircserv [port] [password]" << std::endl;
		return (1);
	}
	const int PORT = atoi(argv[1]);
	// const char *PASSWORD = argv[2];


	int server_socket, client_socket;
	struct sockaddr_in server_address, client_address;
	socklen_t client_address_length;

	// création des maps pour stocker les pseudos des clients et les canaux
	std::map<int, std::string> client_nicknames;
	std::map<std::string, std::set<int> > channel_clients;

	// création du socket
	if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Erreur lors de la création du socket");
		exit(EXIT_FAILURE);
	}

	int opt = 1;
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	// configuration de l'adresse du serveur
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(PORT);

	// liaison du socket à l'adresse du serveur
	if (bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) == -1) {
		perror("Erreur lors de la liaison du socket");
		exit(EXIT_FAILURE);
	}

	// écoute des connexions entrantes
	if (listen(server_socket, MAX_CONNECTIONS) == -1) {
		perror("Erreur lors de l'écoute des connexions entrantes");
		exit(EXIT_FAILURE);
	}

	// boucle principale du serveur
	while (true)
	{
		// accepter une connexion entrante
		client_address_length = sizeof(client_address);
		client_socket = accept(server_socket, (struct sockaddr *) &client_address, &client_address_length);
		if (client_socket == -1) {
			perror("Erreur lors de l'acceptation d'une connexion entrante");
			continue;
		}

		// traiter la connexion entrante
		handle_client_connection(client_socket, client_address, client_nicknames, channel_clients);
		// gérer le protocole IRC
		// gérer les erreurs et les exceptions
	}

	// fermer le socket du serveur
	close(server_socket);

	return 0;
}
