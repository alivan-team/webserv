#ifndef CLIENTDATA_HPP
#define CLIENTDATA_HPP

#include <string>
#include <cstdlib>


class Client {

    private:
        std::string _requestBuffer;
        int _client_fd;
        int _server_fd;

    public:
        Client();
        Client(int clinet_fd, int server_fd);

        void appendToRequestBuffer(const char* buffer, size_t bytes);
        bool hasCompleteHeaders() const;
        bool hasCompleteRequest() const;

        void clearRequestBuffer();

        int getClientFd() const;
        int getServerFd() const;
        const std::string& getRequestBuffer() const;
};

#endif