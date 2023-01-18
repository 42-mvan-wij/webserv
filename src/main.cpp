#include "Server.hpp"
#include "EventQueue.hpp"

#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sstream>

int PORT = 8080;
const int BACKLOG = 5;

int main(int argc, char *argv[]) {
	if (argc > 1) {
		std::stringstream ss;
		std::string s(argv[1]);
		ss << s;
		ss >> PORT;
	}
	try {
		Server serverSocket(PF_INET, SOCK_STREAM, 0);
		serverSocket.set_address(PORT);
		serverSocket.bind();
		serverSocket.listen(BACKLOG);

		std::cout << "Listening on port " << PORT << std::endl;

		EventQueue keventQueue(serverSocket);
		keventQueue.add_event_listener(serverSocket.get_sockfd());
		keventQueue.event_loop();
		serverSocket.close();
	}
	catch(const std::exception& e) {
		std::cerr << e.what() << '\n';
	}

	return 0;
}
