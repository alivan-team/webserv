
#include "./hpp/ServerManager.hpp"
#include "./hpp/HTTPResponseBuild.hpp"
#include "./hpp/HTTPRequestParser.hpp"
#include "./hpp/printDebug.hpp"
#include "./hpp/HTTPParseException.hpp"

// ServerManager::ServerManager() {};

const std::map<int, std::vector<ServerConfig>>& ServerManager::getServerManager() const {
	return _serversMap;
}

void ServerManager::acceptNewClient(int serverFd) {

	int newClientFd = accept(serverFd, NULL, NULL);

	if (newClientFd < 0)
		return ;

	setNonBlocking(newClientFd);

	addFd(newClientFd, FD_CLIENT_SOCKET, newClientFd);
	_clients[newClientFd] = Client(newClientFd, serverFd);
}

void ServerManager::removeClient(size_t index) {

	if (index >= _pollfds.size())
		return;

	int clientFd = _pollfds[index].fd;

	std::map<int, Client>::iterator it = _clients.find(clientFd);

    if (it == _clients.end()) {
        removeFd(clientFd);
        return;
    }

	Client& client = it->second;

	if (client.getCgiInputFd() != -1) {
		removeFd(client.getCgiInputFd());
		client.setCgiInputFd(-1);
	}

	if (client.getCgiOutputFd() != -1) {
		removeFd(client.getCgiOutputFd());
		client.setCgiOutputFd(-1);
	}

	if (client.getCgiPid() > 0) {

		unregisterFd(clientFd);

		kill(client.getCgiPid(), SIGKILL);
		client.setPendingRemoval(true);
		return ;
	}

	removeFd(clientFd);
	_clients.erase(it);
}

bool ServerManager::shouldKeepAlive(const HTTPRequest& request) const {
	std::string connection;

	if (request.hasHeader("Connection"))
		connection = toLower(trim(request.getHeader("Connection")));

	if (request.getVersion() == "1.1")
		return connection != "close";

	if (request.getVersion() == "1.0")
		return connection == "keep-alive";

	return false;
}

bool ServerManager::readClientData(size_t index) {

	int clientFd = _pollfds[index].fd;

	char buffer[4096];
	std::memset(buffer, 0, sizeof(buffer));

	int bytes = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

	if (bytes == 0) {
		removeClient(index);
		return true;

	} else if (bytes < 0) {
		removeClient(index);
		return true;
	}

	Client& client = _clients.at(clientFd);
	client.updateLastActivity();
	client.appendToRequestBuffer(buffer, static_cast<size_t>(bytes));

	return processRequestBuffer(index);
}

void ServerManager::queueResponse(size_t index, Client& client, HTTPResponse& response) {
	
	client.setResponseBuffer(response.toString(response));
	_pollfds[index].events &= ~POLLIN;
	_pollfds[index].events |= POLLOUT;
}


bool ServerManager::isServerSocket(int fd) const
{
	for (size_t i = 0; i < _serverSockets.size(); ++i)
	{
		if (_serverSockets[i] == fd)
			return true;
	}
	return false;
}

int ServerManager::findServerFd(const std::string& host, int port) const {

	for(std::map<int, std::vector<ServerConfig>>::const_iterator it = _serversMap.begin();
		it != _serversMap.end(); ++it)
	{
			if (!it->second.empty() && it->second[0].getHost() == host && it->second[0].getPort() == port) {
				return it->first;
			}
	}
	return -1;
}


void ServerManager::initialize(const std::vector<ServerConfig>& servers) {

	if(servers.empty())
		throw std::runtime_error("No servers configured");
	
	for (size_t i = 0; i < servers.size(); i++) {
		
		const std::string& host = servers[i].getHost();
		int port = servers[i].getPort();
		int serverFd = findServerFd(host, port);

		if (serverFd == -1)
			serverFd = createListeningSockets(servers[i]);

		_serversMap[serverFd].push_back(servers[i]);
	}
}

void ServerManager::run() {

	while (true) {
		
		int ready = poll(_pollfds.data(), _pollfds.size(), 1000);
		if (ready < 0)
			throw std::runtime_error("poll() failed");

		size_t i = 0;
		while (i < _pollfds.size())
		{

			int fd = _pollfds[i].fd;
			short revents = _pollfds[i].revents;
			FdInfo info = _fdInfo.at(fd);
			bool removed = false;

			if (info.type == FD_SERVER_SOCKET) {

				if (revents & (POLLERR | POLLHUP | POLLNVAL))
					throw std::runtime_error("Listening socket error");

				if (revents & POLLIN)
					acceptNewClient(fd);

			} else if (info.type == FD_CLIENT_SOCKET) {

				if (revents & (POLLERR | POLLHUP | POLLNVAL)) {
					removeClient(i);
					removed = true;
				} else {

					if (revents & POLLIN)
						removed = readClientData(i);
					if (!removed && (revents & POLLOUT))
						removed = writeClientData(i);
				}

			} else if (info.type == FD_CGI_INPUT) {

				if (revents & (POLLERR | POLLHUP | POLLNVAL)) {

					Client& client = _clients.at(info.clientFd);
					failCgi(client);
					removed = true;
				} else if (revents & POLLOUT) {

					removed = writeToCgi(fd, info.clientFd);
				}

			} else if (info.type == FD_CGI_OUTPUT) {

				if (revents & (POLLERR | POLLNVAL)) {

					Client& client = _clients.at(info.clientFd);
					failCgi(client);
					removed = true;
				} else if (revents & (POLLIN | POLLHUP)){

					removed = readFromCgi(fd, info.clientFd);
				}
			}

			if (!removed)
				i++;
		}

		reapCgiChildern();
		checkCgiTimeouts();
		removeTimeOutClients();
	}
}

void ServerManager::setNonBlocking(int fd) {

	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error("fcntl(F_SETFL) failed");
}

int ServerManager::createListeningSockets(const ServerConfig& server) {

	const std::string& host = server.getHost();
	int port = server.getPort();
	std::string portString = std::to_string(port);
	struct addrinfo hints;
	struct addrinfo* result;

	std::memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	int status = getaddrinfo(host.c_str(), portString.c_str(), &hints, &result);

	if(status != 0) {
		throw std::runtime_error(std::string("getaddrinfo() failed: ") + gai_strerror(status));
	}

	int serverFd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (serverFd < 0) {
		freeaddrinfo(result);
		throw std::runtime_error("socket() failed");
	}

	int opt = 1;
	if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		freeaddrinfo(result);
		close(serverFd);
		throw std::runtime_error("setsockopt() failed");
	}

	if (bind(serverFd, result->ai_addr, result->ai_addrlen) < 0) {
		freeaddrinfo(result);
		close(serverFd);
		std::string error = "bind() failed for " + host + ":" + portString + ": " + std::strerror(errno);
		throw std::runtime_error(error);
	}

	freeaddrinfo(result);

	if (listen(serverFd, 128) < 0) {
		close(serverFd);
		throw std::runtime_error("listen() failed");
	}

	setNonBlocking(serverFd);
	_serverSockets.push_back(serverFd);
	addFd(serverFd, FD_SERVER_SOCKET, -1);

	std::cout << "Listening on  " << host << ":" << port << std::endl;

	return serverFd;
}

const ServerConfig& ServerManager::getClientServerManager(int serverIndex, const std::string& host) const {

	std::map<int, std::vector<ServerConfig>>::const_iterator it = _serversMap.find(serverIndex);

	if (it == _serversMap.end())
		throw std::runtime_error("Server configuration not found.");
	if (it->second.empty())
		throw std::runtime_error("Server configuration list is empty.");
	
	const std::vector<ServerConfig>& servers = it->second;

	for (size_t i = 0; i < servers.size(); i++) {

		const std::vector<std::string>& serverNames = servers[i].getServerName();

		for (size_t j = 0; j < serverNames.size(); j++) {
			if (serverNames[j] == host) {
				return servers[i];
			}
		}
	} 

	return servers[0];
}

bool ServerManager::writeClientData(size_t index) {
	
	int clientFd = _pollfds[index].fd;

	Client& client = _clients.at(clientFd);

	const std::string& response = client.getResponseBuffer();
	size_t sentAlreay = client.getResponseSent();

	ssize_t sent = send(clientFd, response.data() + sentAlreay, response.size() - sentAlreay, MSG_NOSIGNAL);

	if (sent < 0) {
		removeClient(index);
		return true;
	}

	if (sent > 0) {
		client.updateLastActivity();
		client.setResponseSent(sentAlreay + static_cast<size_t>(sent));
	}

	if (client.getResponseSent() == response.size()) {
		_pollfds[index].events &= ~POLLOUT;

		if (client.getCloseAfterReponse() == false) {
			if(!shouldKeepAlive(client.getRequest())) {
				removeClient(index);
				return true;
			}
		} else {
			removeClient(index);
			return true;
		}

		client.consumeRequest();
		client.clearResponse();

		if (client.getRequestBuffer().empty()) {
			_pollfds[index].events |= POLLIN;
		} else {
			return processRequestBuffer(index);
		}
	}

	return false;
};

void ServerManager::checkCgiTimeouts() {

	const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

	std::map<int, Client>::iterator it = _clients.begin();

	while (it != _clients.end()) {

		Client& client = it->second;

		if (client.getPendingRemoval()){
			it++;
			continue;
		}

		if (client.getCgiState() == CGI_NONE) {
			it++;
			continue;
		}

		std::chrono::seconds elapse = std::chrono::duration_cast<std::chrono::seconds>(now - client.getCgiTime());

		if (elapse.count() > 30) {
			failCgi(client);
		}
		it++;
	}
}

void ServerManager::removeTimeOutClients() {

	const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	std::map<int, Client>::iterator it = _clients.begin();
	
	while (it != _clients.end()) {
		
		int clientFd = it->first;

		 if (it->second.getPendingRemoval()) {
			it++;
			continue;
		}

		if (it->second.getCgiState() != CGI_NONE) {
			it++;
			continue ;
		}
		std::chrono::seconds timeLeft = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.getLastActivity());
		
		++it;

		if (timeLeft.count() > 30) {

			for (size_t j = 0; j < _pollfds.size(); j++) {

				if (_pollfds[j].fd ==clientFd) {
					removeClient(j);
					break ;
				}
			}
		}
	}
};

bool ServerManager::startCgi(Client& client, const HTTPRequest& request, const CgiRoute& route, const ServerConfig& servConf) {

	client.resetCgiForNewRequest();

	int inputPipe[2];
	int outputPipe[2];

	if (pipe(inputPipe) < 0)
		return false;

	if (pipe(outputPipe) < 0)
	{
		close(inputPipe[0]);
		close(inputPipe[1]);
		return false;
	}

	pid_t pid = fork();
	if (pid < 0) {
		close(inputPipe[0]);
		close(inputPipe[1]);
		close(outputPipe[0]);
		close(outputPipe[1]);
		return false;
	}

	if (pid == 0) {

		signal(SIGPIPE, SIG_DFL);

		close(inputPipe[1]);
		close(outputPipe[0]);

		if (dup2(inputPipe[0], STDIN_FILENO) < 0)
			_exit(1);
		if (dup2(outputPipe[1], STDOUT_FILENO) < 0)
			_exit(1);

		close(inputPipe[0]);
		close(outputPipe[1]);

		if (chdir(route.workingDirectory.c_str()) < 0)
			_exit(1);
		
		std::vector<std::string> cgiEnvironment = buildCgiEnvironment(request, servConf);
	
		std::vector<char*> evnp;
		std::vector<char*> argv;

		for (size_t i = 0; i < cgiEnvironment.size(); i++) 
			evnp.push_back(cgiEnvironment[i].data());

		evnp.push_back(NULL);

		std::string scriptName = std::filesystem::path(route.scriptPath).filename().string();
		argv.push_back(const_cast<char*>(route.cgiPath.c_str()));
		argv.push_back(scriptName.data());
		argv.push_back(NULL);

		execve(route.cgiPath.c_str(), argv.data() , evnp.data());
		_exit(1);
	}

	close(inputPipe[0]);
	close(outputPipe[1]);

	setNonBlocking(inputPipe[1]);
	setNonBlocking(outputPipe[0]);

	client.setCgiInputFd(inputPipe[1]);
	client.setCgiOutputFd(outputPipe[0]);
	client.setCgiState(CGI_WRITING);
	client.setCgiPid(pid);
	client.setCgiStartTime();
	client.setCgiProcessFailed(false);

	addFd(inputPipe[1], FD_CGI_INPUT, client.getClientFd());
	addFd(outputPipe[0], FD_CGI_OUTPUT, client.getClientFd());

	setFdEvents(inputPipe[1], POLLOUT);

	return true;
}

RequestState ServerManager::getRequestState(Client& client, const ServerConfig*& serverConfig) {

	serverConfig = &getClientServerManager(client.getServerFd(), "");

	if (!client.getHeaderIsParsed()) {
		RequestState headerState = client.parseHeaderClient();
		if (headerState != RequestState::Complete) {
			return headerState;
		}
	}

	serverConfig = &getClientServerManager(client.getServerFd(), client.getHost());
	size_t maxBodySize = serverConfig->getClientMaxBodySize().back();
	RequestState state = client.checkRequestState(maxBodySize);

	return state;
}


bool ServerManager::processRequestBuffer(size_t index) {

	int clientFd = _pollfds[index].fd;
	Client& client = _clients.at(clientFd);
	const ServerConfig* serverConfig = NULL;

	RequestState state = getRequestState(client, serverConfig);

	if (state == RequestState::Incomplete) {
		_pollfds[index].events |= POLLIN;
		return false;
	}
	if (state == RequestState::BadRequest) {

		int errorCode = client.getRequestErrorCode();
		if (errorCode == 0)
			errorCode = 400;

		HTTPResponse errorResponse = HTTPResponseBuild::makeEarlyErrorResponse(errorCode, *serverConfig);
		client.setCloseAfterResponse(true);
		queueResponse(index, client, errorResponse);
		return false;
	}

	try {

		if (client.getBodyType() == BodyType::Chunked) {
			if (!client.decodeChunkedBody())
				throw HTTPParseException(500, "Internal Server Error");
		}
		client.setClientRequest(HTTPRequestParser().parse(client.getRequestBuffer(), client.getRequestEnd()));
		CgiRoute cgiRoute;
		int cgiError = 0;

		if (HTTPResponseBuild::resolveCgiRoute(client.getRequest(), *serverConfig, cgiRoute, cgiError))
		{
			if (!startCgi(client, client.getRequest(), cgiRoute, *serverConfig))
			{
				HTTPResponse errorResponse = HTTPResponseBuild::makeEarlyErrorResponse(500, *serverConfig);
				client.setCloseAfterResponse(true);
				queueResponse(index, client, errorResponse);
			}
			return false;
		}
		if (cgiError != 0)
		{
			HTTPResponse errorResponse =
				HTTPResponseBuild::makeEarlyErrorResponse(cgiError, *serverConfig);
			client.setCloseAfterResponse(true);
			queueResponse(index, client, errorResponse);
			return false;
		}

		HTTPResponse ClassResponse = HTTPResponseBuild::build(client.getRequest(), *serverConfig);
		queueResponse(index, client, ClassResponse);
		return false;

	} catch (const HTTPParseException& e) {
		HTTPResponse errorResponse = HTTPResponseBuild::makeEarlyErrorResponse(e.getStatusCode(), *serverConfig);
		client.setCloseAfterResponse(true);
		queueResponse(index, client, errorResponse);
		return false;

	} catch (const std::exception& e) {
		(void)e;
		HTTPResponse errorResponse = HTTPResponseBuild::makeEarlyErrorResponse(500, *serverConfig);
		client.setCloseAfterResponse(true);
		queueResponse(index, client, errorResponse);
		return false;
	}
}

void ServerManager::addFd(int fd, FdType type, int clientFd)
{
	struct pollfd pollFd;

	pollFd.fd = fd;
	pollFd.events = POLLIN;
	pollFd.revents = 0;

	_pollfds.push_back(pollFd);
	_fdInfo[fd] = FdInfo{type, clientFd};
}

void ServerManager::unregisterFd(int fd) {

	for (size_t i = 0; i < _pollfds.size(); ++i)
	{
		if (_pollfds[i].fd == fd)
		{
			_pollfds.erase(_pollfds.begin() + i);
			break;
		}
	}

	_fdInfo.erase(fd);
}

void ServerManager::removeFd(int fd) {

	unregisterFd(fd);
	close(fd);
}

void ServerManager::setFdEvents(int fd, short events)
{
	for (size_t i = 0; i < _pollfds.size(); ++i)
	{
		if (_pollfds[i].fd == fd)
		{
			_pollfds[i].events = events;
			return;
		}
	}
}
