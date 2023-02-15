#pragma once
#ifndef ENUMS_HPP
# define ENUMS_HPP

enum Method {
	GET,
	POST,
	PUT,
	DELETE,
	HEAD,
	CONNECT,
	OPTIONS,
	TRACE,
	PATCH,
	UNKNOWN
};

enum HTTP_STATUS_CODES {
	HTTP_CONTINUE = 100,
	HTTP_OK = 200,
	HTTP_CREATED = 201,
	HTTP_NO_CONTENT = 204,
	HTTP_BAD_REQUEST = 400, // use this if we can't parse the request?
	// HTTP_UNAUTHORIZED = 401,
	// HTTP_FORBIDDEN = 403,
	HTTP_NOT_FOUND = 404,
	HTTP_METHOD_NOT_ALLOWED = 405
	// HTTP_INTERNAL_SERVER_ERROR = 500, // how could we ever use this one?
	// HTTP_SERVICE_UNAVAILABLE = 503
};

#endif
