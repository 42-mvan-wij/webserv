#include "CGI.hpp"
#include "Client.hpp"
#include "Logger.hpp"
#include "EventQueue.hpp"
#include <unistd.h>
#include <vector>
#include <cstring>

CGI::CGI(ParsedRequest const& request, Client& client, EventQueue& event_queue)
	: _client(&client)
	, _event_queue(&event_queue)
	, _stream_event_handler()
{
	this->_init_env(request, client);
	this->_start();
	event_queue.add_cgi_event_listener(this->_get_input_write_fd(), this->_get_output_read_fd(), this->_client->get_sockfd());
}

CGI::CGI(CGI const& src)
	: _env(src._env)
	, _pid(src._pid)
	, _client(src._client)
	, _event_queue(src._event_queue)
	, _stream_event_handler(src._get_output_read_fd(), src._get_input_write_fd(), *this, *this)
{
	this->_pipe_fd_input[0] = src._pipe_fd_input[0];
	this->_pipe_fd_input[1] = src._pipe_fd_input[1];
	this->_pipe_fd_output[0] = src._pipe_fd_output[0];
	this->_pipe_fd_output[1] = src._pipe_fd_output[1];
}

CGI& CGI::operator=(CGI const& src) {
	this->_pipe_fd_input[0] = src._pipe_fd_input[0];
	this->_pipe_fd_input[1] = src._pipe_fd_input[1];
	this->_pipe_fd_output[0] = src._pipe_fd_output[0];
	this->_pipe_fd_output[1] = src._pipe_fd_output[1];
	this->_env = src._env;
	this->_pid = src._pid;
	this->_client = src._client;
	this->_event_queue = src._event_queue;
	this->_stream_event_handler = NonBlockingRWStream(this->_get_output_read_fd(), this->_get_input_write_fd(), *this, *this);
	return *this;
}

void CGI::_init_env(ParsedRequest const& request, Client &client) {
	try {
		this->_env["AUTH_TYPE"] = request.get_auth_scheme();
		if (request.has_header("content-length")) {
			this->_env["CONTENT_LENGTH"] = request.get_header("content-length"); // NOTE: if and only if request has a body
		}
		if (request.has_header("content-type")) {
			this->_env["CONTENT_TYPE"] = request.get_header("content-type"); // // NOTE: if and only if request has a body
		}
		this->_env["GATEWAY_INTERFACE"] = "CGI/1.1";
		this->_env["PATH_INFO"] = request.get_script_name();
		this->_env["PATH_TRANSLATED"] = request.get_script_name();
		this->_env["QUERY_STRING"] = request.get_query_string();
		this->_env["REMOTE_ADDR"] = client.get_ip(); //
		this->_env["REMOTE_HOST"] =  client.get_ip();
		this->_env["REMOTE_IDENT"] = ""; // NOTE: does not need to be implemented
		this->_env["REQUEST_METHOD"] = request.method_string;
		this->_env["SCRIPT_NAME"] = request.get_script_name();
		this->_env["SERVER_NAME"] = client.config.get_server_names()[0]; // TODO: zorgen dat server_name altijd set is, default instellen als het niet in de config file staat
		this->_env["SERVER_PORT"] = std::to_string(client.config.get_port());
		this->_env["SERVER_PROTOCOL"] = request.http_version;
		this->_env["SERVER_SOFTWARE"] = "Webserv/1.0";
		if (request.location.is_set()) {
			this->_env["UPLOAD_PATH"] = client.config.get_root() + request.location.get_upload();
		}
	}
	catch (const std::exception& e) {
		logger << Logger::error << "Error setting environment variables for CGI: " << e.what() << std::endl;
	}
}

std::vector<char *> CGI::_get_envp() const {
	std::vector<char *> envp;

	for (decltype(this->_env)::const_iterator it = this->_env.begin(); it != this->_env.end(); ++it) {
		std::string s = it->first + "=" + it->second;
		char *ss = new char[s.size() + 1];
		std::copy(s.begin(), s.end(), ss);
		ss[s.size()] = '\0';
		envp.push_back(ss);
	}
	envp.push_back(NULL);

	return envp;
}

std::vector<char *> CGI::_get_argv() const {
	std::vector<std::string> argv;
	argv.push_back(this->_env.at("SCRIPT_NAME").c_str());

	std::vector<char *> converted_argv;
	for (decltype(argv)::const_iterator it = argv.begin(); it != argv.end(); ++it) {
		char *ss = new char[it->size() + 1];
		std::copy(it->begin(), it->end(), ss);
		ss[it->size()] = '\0';
		converted_argv.push_back(ss);
	}
	converted_argv.push_back(NULL);

	return converted_argv;
}

void CGI::write(char const* buf, size_t count) {
	this->_stream_event_handler.write((char *)buf, count);
}

void CGI::_start() {
	if (::pipe(this->_pipe_fd_input) == -1)
		throw std::runtime_error("pipe() failed");
	if (::pipe(this->_pipe_fd_output) == -1)
		throw std::runtime_error("pipe() failed");

	this->_stream_event_handler = NonBlockingRWStream(this->_get_output_read_fd(), this->_get_input_write_fd(), *this, *this);

	this->_pid = ::fork();
	if (this->_pid == -1) {
		throw std::runtime_error("fork() failed");
	}

	if (this->_pid == 0) {
		::close(this->_get_input_write_fd());
		::close(this->_get_output_read_fd());
		if (::dup2(this->_get_input_read_fd(), STDIN_FILENO) == -1)
			throw std::runtime_error("dup2() failed");
		if (::dup2(this->_get_output_write_fd(), STDOUT_FILENO) == -1)
			throw std::runtime_error("dup2() failed");
		::execve(this->_env.at("SCRIPT_NAME").c_str(), this->_get_argv().data(), this->_get_envp().data());
		throw std::runtime_error("execve() failed");
	}
	::close(this->_get_input_read_fd());
	::close(this->_get_output_write_fd());
}

void CGI::end_of_input() {
	this->_stream_event_handler.close();
}

void CGI::wait() {
	int status;
	::waitpid(this->_pid, &status, 0); // NOTE: this hangs the entire process, thus no other clients can be served
}

void CGI::handle_event(struct kevent& ev_rec) {
	this->_stream_event_handler.handle_event(ev_rec);
}

int CGI::_get_input_read_fd() const {
	return this->_pipe_fd_input[0];
}

int CGI::_get_input_write_fd() const {
	return this->_pipe_fd_input[1];
}

int CGI::_get_output_read_fd() const {
	return this->_pipe_fd_output[0];
}

int CGI::_get_output_write_fd() const {
	return this->_pipe_fd_output[1];
}

void CGI::_read_data(std::string str) {
	this->_client->send(str);
}

void CGI::_read_end() {
	::close(this->_get_output_read_fd());
	this->_event_queue->remove_fd(this->_get_output_read_fd());
	this->_client->close();
}

void CGI::_write_end() {
	::close(this->_get_input_write_fd());
	this->wait();
	this->_event_queue->remove_fd(this->_get_input_write_fd());
}
