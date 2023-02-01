#include "FileResponse.hpp"
#include "util.hpp"

FileResponse::FileResponse(const std::string &request_path){
	this->_file_dir = "./root/var/www"; // to be defined by config
	this->_filename = this->_file_dir + request_path;
	this->_file_accessible = this->can_open_file();
	if (request_path == "/") // TODO: dit verbeteren want dit is lelijk
		this->_is_home = true;
	else
		this->_is_home = false;
}

bool FileResponse::can_open_file() {
	std::ifstream file(this->_filename);
	if (file.is_open()) {
		file.close();
		return true;
	}
	else {
		return false;
	}
}

void FileResponse::load_content(void) {
	if (this->_file_accessible) {
		if (this->_is_home)
			this->_filename = this->_filename + "index.html";
		this->_page_content = util::file_to_str(this->_filename);
	}
	else {
		this->_page_content = util::file_to_str(this->_file_dir + "/notfound.html");
	}
}

void FileResponse::generate_response(void) {
	std::string file_extension;

	this->load_content();
	if (!this->_file_accessible) {
		// TODO: response headers beter opslaan, status code en content type splitsen
		this->_response_headers = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n";
	}
	else {
		file_extension = this->_filename.substr(this->_filename.find_last_of(".") + 1);
		this->_response_headers = "HTTP/1.1 200 OK\r\nContent-Type: text/" + file_extension + "\r\n\r\n";
	}
}

std::string FileResponse::get_response(void) const {
	return this->_response_headers + this->_page_content;
}
