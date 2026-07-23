#ifndef CLIENTDATA_HPP
#define CLIENTDATA_HPP

#include <string>
#include <cstdlib>
#include <cstddef>
#include <cctype>
#include <sstream>
#include <limits>
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"

enum class RequestState
{
    Incomplete,
    Complete,
    BadRequest
};

class Client {

        private:
                std::string _requestBuffer;
                int _client_fd;
                int _server_fd;

                size_t _bodyPos;
                size_t _bodySize;
                size_t _requestEnd;
                int _requestErrorCode;

                HTTPRequest _request;
                // HTTPResponse _response;

                

                bool parseContentLength(const std::string& value, size_t& result) const;
                bool hasCompleteChunkedBody(size_t bodyPos, size_t& requestEnd) const;

                std::string trim(const std::string& value) const;
                std::string toLower(const std::string& value) const;

        public:
                        
                Client();
                Client(int clinet_fd, int server_fd);

                void appendToRequestBuffer(const char* buffer, size_t bytes);
                bool hasCompleteHeaders() const;
                bool parseHexSize(const std::string& value, size_t& result) const;
                RequestState checkChunkedBody(size_t value, size_t& result) const;
                RequestState checkRequestState();

                void clearRequestBuffer();

                void setClientRequest(const HTTPRequest& req);

                std::string getFullBodyRequest() const;
                std::string getPartBodyRequest(size_t start, size_t length) const;
                size_t getBodyPos() const;
                size_t getBodySize() const;

                int getClientFd() const;
                int getServerFd() const;

                const std::string& getRequestBuffer() const;
                const HTTPRequest& getRequest() const;
                int     getRequestErrorCode() const;

};

#endif