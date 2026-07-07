#include "./hpp/HTTPRequestParser.hpp"
#include "./hpp/HTTPResponse.hpp"

void HTTPResponse::setStatusCode(int code) {
    _statusCode = code;
};

void HTTPResponse::setHeader(const std::string& key, const std::string& value) {

    _headers.insert({key, value});
};

void HTTPResponse::setBody(const std::string& body) {
    _body = body;
};

void HTTPResponse::setStatus(const std::string& text) {
    _statusText = text;
};

std::string HTTPResponse::toString() const {
     std::string response;

    response += "HTTP/1.1 ";
    response += std::to_string(_statusCode);
    response += ": ";
    response += _statusText;
    response += "\r\n";

    for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
        response += it->first;
        response += ": ";
        response += it->second;
        response += "\r\n";
    }

    response += "\r\n";
    response += _body;

    return response;
}