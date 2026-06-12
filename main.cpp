#include <iostream>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>

static void setNonBlocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return;

    fcntl(fd, F_SETFL, O_NONBLOCK);
}

int main()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1)
	{
		std::cerr << "socket() failed\n";
		return 1;
	}

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_KEEPALIVE, &opt, sizeof(opt));

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);

    bind(server_fd, (sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 128);

    setNonBlocking(server_fd);

    std::vector<pollfd> fds;

    pollfd server_poll;
    server_poll.fd = server_fd;
    server_poll.events = POLLIN;
    server_poll.revents = 0;

    fds.push_back(server_poll);

    std::cout << "Listening on http://localhost:8080\n";

    while (true)
    {
        poll(&fds[0], fds.size(), -1);

        for (size_t i = 0; i < fds.size(); ++i)
        {
            if (fds[i].revents & POLLIN)
            {
                if (fds[i].fd == server_fd)
                {
                    int client_fd = accept(server_fd, NULL, NULL);
                    setNonBlocking(client_fd);

                    pollfd client_poll;
                    client_poll.fd = client_fd;
                    client_poll.events = POLLIN;
                    client_poll.revents = 0;

                    fds.push_back(client_poll);

                    std::cout << "New client: fd " << client_fd << "\n";
                }
                else
                {
                    char buffer[4096];
                    std::memset(buffer, 0, sizeof(buffer));

                    int bytes = recv(fds[i].fd, buffer, sizeof(buffer) - 1, 0);

                    if (bytes <= 0)
                    {
                        std::cout << "Client disconnected\n";
                        close(fds[i].fd);
                        fds.erase(fds.begin() + i);
                        --i;
                    }
                    else
                    {
                        std::cout << "Request from fd " << fds[i].fd << ":\n";
                        std::cout << buffer << "\n";

                        std::string body = "Hello from poll server\n";

                        std::string response =
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/plain\r\n"
                            "Content-Length: " + std::to_string(body.size()) + "\r\n"
                            "\r\n" +
                            body;

                        send(fds[i].fd, response.c_str(), response.size(), 0);
                    }
                }
            }
        }
    }
}