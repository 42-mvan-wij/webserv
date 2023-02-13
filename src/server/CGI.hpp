#pragma once
#ifndef CGI_HPP
# define CGI_HPP

#include "ParsedRequest.hpp"
// #include "Client.hpp"
#include "Optional.hpp"
#include <map>
#include <vector>

class Client;
class CGI {
public:
	CGI(ParsedRequest const& request, Client& client);
	void write(const void *buf, size_t count);
	void wait();

protected:
	CGI() {};

private:
	// Client *_client;
	// Client&	_client;
	int	_pipe_fd[2];
	std::map<std::string, std::string> _env;
	pid_t _pid;

	void _start(Client &client);
	std::vector<char *> _get_argv() const;
	std::vector<char *> _get_envp() const;
	void _init_env(ParsedRequest const& request, Client& client);
};

#endif
