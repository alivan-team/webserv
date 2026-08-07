#include "./hpp/HTTPParseException.hpp"


HTTPParseException::HTTPParseException(int statusCode, const std::string& message) : std::runtime_error(message), _statucCode(statusCode) {
};

int HTTPParseException::getStatusCode() const { return _statucCode; };
