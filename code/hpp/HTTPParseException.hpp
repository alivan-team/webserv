#ifndef HTTPPARSEEXCEPTION_HPP
#define HTTPPARSEEXCEPTION_HPP

#include <stdexcept>

class HTTPParseException : public std::runtime_error {

    private: 
        int _statucCode;

    public:
        HTTPParseException(int statusCode, const std::string& message);
        int getStatusCode() const;

};

#endif