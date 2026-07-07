#ifndef CLIENTDATA_HPP
#define CLIENTDATA_HPP

#include <string>
#include <cstdlib>
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"


class Client {

    private:
        std::string _requestBuffer;
        int _client_fd;
        int _server_fd; // server FD
        size_t _bodyPos;
        size_t _bodySize;
        // HTTPResponse _response;
		
		
	public:
        HTTPRequest _request;
		
        Client();
        Client(int clinet_fd, int server_fd);

        void appendToRequestBuffer(const char* buffer, size_t bytes);
        bool hasCompleteHeaders() const;
        bool hasCompleteRequest() ;

        void clearRequestBuffer();

        std::string getFullBodyRequest() const;
        std::string getPartBodyRequest(size_t start, size_t length) const;
        size_t getBodyPos() const;
        size_t getBodySize() const;

        int getClientFd() const;
        int getServerFd() const;
        const std::string& getRequestBuffer() const;
        // const HTTPRequest& getRequest() const;


};

#endif