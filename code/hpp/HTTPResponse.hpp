#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include "./client.hpp"
#include <map>

class HTTPResponse {

	private: 
		int _statusCode;
		std::string _statusText;
		std::map<std::string, std::string> _headers;
		std::string _body;

	public:

		// void setStatus(int code, const std::string& text);
		// void setHeader(const std::string& key, const std::string& value);
		// void setBody(const std::string& body);
	
		std::string toString() const;

};

#endif







    // std::string response =
    // "HTTP/1.1 200 OK\r\n"
    // "Content-Type: text/plain\r\n"
    // "Content-Length: " + std::to_string(body.size()) + "\r\n"
    // "\r\n" +
    // body;
    




// .html  -> text/html
// .htm   -> text/html
// .css   -> text/css
// .js    -> application/javascript
// .png   -> image/png
// .jpg   -> image/jpeg
// .jpeg  -> image/jpeg
// .gif   -> image/gif
// .ico   -> image/x-icon
// .txt   -> text/plain

// /favicon.ico
// /style.css
// /script.js
// /image.png

// 4. Is this enough to cover every request?

// Almost, but I would phrase the categories like this:

// 1. Match server
// 2. Match location
// 3. Check method allowed
// 4. Check body size
// 5. Handle redirect
// 6. Handle CGI
// 7. Handle GET
// 8. Handle POST/upload
// 9. Handle DELETE
// 10. Handle error pages

// HTTPResponse ResponseBuilder::build(const HTTPRequest& req,
//                                     const ServerConfig& server)
// {
//     LocationConfig loc = findBestLocation(req.getPath(), server);

//     if (!loc.allowsMethod(req.getMethod()))
//         return make405(loc);

//     if (bodyTooLarge(req, server))
//         return makeError(413, server);

//     if (loc.hasRedirect())
//         return makeRedirect(loc);

//     if (shouldRunCgi(req, loc))
//         return makeCgiResponse(req, loc, server);

//     if (req.getMethod() == GET)
//         return handleGet(req, loc, server);

//     if (req.getMethod() == POST)
//         return handlePost(req, loc, server);

//     if (req.getMethod() == DELETE)
//         return handleDelete(req, loc, server);

//     return makeError(501, server);
// }