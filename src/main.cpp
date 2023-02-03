#include "Server.hpp"
#include "EventQueue.hpp"
#include "Logger.hpp"
#include "ParsedConfigFile.hpp"

#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sstream>

const int BACKLOG = 5;

Logger logger;

int main(int argc, char *argv[]) {
	char *config_filename;
	if (argc < 2)
		config_filename = "root/conf/server.conf";
	else
		config_filename = argv[1];

	ParsedConfigFile config(config_filename);
	logger << Logger::info << "Parsed config file: " << config_filename << std::endl;

	// Is zoiets de bedoeling? Hoe zorgen we ervoor dat we meerdere ports tegelijk kunnen openen?
	for (std::vector<Config>::iterator it = config.server_blocks.begin(); it < config.server_blocks.end(); it++) {
		try {
			Server serverSocket(PF_INET, SOCK_STREAM, 0);
			serverSocket.set_config((*it));
			logger << Logger::info << "Starting server on port " << (*it).get_port() << std::endl;

			serverSocket.set_address();
			serverSocket.bind();
			serverSocket.listen(BACKLOG);

			logger << Logger::info << "Listening on port " << (*it).get_port() << ": http://localhost:" << (*it).get_port() << std::endl;

			EventQueue keventQueue(serverSocket);
			keventQueue.add_event_listener(serverSocket.get_sockfd());
			keventQueue.event_loop();
			serverSocket.close();
		}
		catch(const std::exception& e) {
			logger << Logger::error << e.what() << std::endl;
		}
	}

	return 0;
}
