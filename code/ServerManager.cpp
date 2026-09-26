
#include "./hpp/ServerManager.hpp"
#include "./hpp/HTTPResponseBuild.hpp"
#include "./hpp/HTTPRequestParser.hpp"
#include "./hpp/printDebug.hpp"
#include "./hpp/HTTPParseException.hpp"

// ServerManager::ServerManager() {};

const std::map<int, std::vector<ServerConfig>>& ServerManager::getServerManager() const {
	return _serversMap;
};

void ServerManager::acceptNewClient(int serverFd) {

	int newClientFd = accept(serverFd, NULL, NULL);

	if (newClientFd < 0)
		return ;

	setNonBlocking(newClientFd);

	addFd(newClientFd, FD_CLIENT_SOCKET, newClientFd);
	_clients[newClientFd] = Client(newClientFd, serverFd);
};

void ServerManager::removeClient(size_t index) {
	if (index >= _pollfds.size())
		return;

	int clientFd = _pollfds[index].fd;
	close(clientFd);
	_clients.erase(clientFd);
	removeFd(clientFd);
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
};

void ServerManager::queueResponse(size_t index, Client& client, HTTPResponse& response) {
	
	client.setResponseBuffer(response.toString(response));
	_pollfds[index].events &= ~POLLIN;
	_pollfds[index].events |= POLLOUT;
};


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
};


void ServerManager::initialize(const std::vector<ServerConfig>& servers) {

	if(servers.empty())
		throw std::runtime_error("No servers configured");
	
	for (size_t i = 0; i < servers.size(); i++) {
		
		const std::string& host = servers[i].getHost();
		int port = servers[i].getPort();
		int serverFd = findServerFd(host, port);

		if (serverFd == -1)
			serverFd = createListeningSockets(servers[i]);

		// std::cout << "\tserversFd: " << serverFd << "\t servers[i].getPort():  " << servers[i].getPort() <<"\t servers[i].getServerName():  " << servers[i].getServerName()[0] << std::endl;
		
		_serversMap[serverFd].push_back(servers[i]);
	}
};

void ServerManager::run() {

	while (true) {
		
		// std::cout << "HELLO from the run manager" << std::endl;
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
				if (revents & POLLOUT)
					removed = writeToCgi(fd, info.clientFd);
				std::cout << "Hello you should write in a pipe" << std::endl;
			} else if (info.type == FD_CGI_OUTPUT) {
				if (revents & (POLLIN | POLLHUP))
					removed = readFromCgi(fd, info.clientFd);
				std::cout << "Hello you should read from a pipe" << std::endl;
			}

			if (!removed)
				i++;
		}

		// checkCgiTimeouts();
		removeTimeOutClients();
	}

};

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

	// Old manual pollfd registration.
	// Replaced by addFd() to keep _pollfds and _fdInfo synchronized.
	// commented and replaced when adding CGI
	// pollfd server_poll;
	// server_poll.fd = serverFd;
	// server_poll.events = POLLIN;
	// server_poll.revents = 0;

	// _pollfds.push_back(server_poll);
	addFd(serverFd, FD_SERVER_SOCKET, -1);

	std::cout << "Listening on  " << host << ":" << port << std::endl;

	return serverFd;
};

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
};

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

void ServerManager::removeTimeOutClients() {

	const std::chrono::steady_clock::time_point now = 
			std::chrono::steady_clock::now();
	std::map<int, Client>::iterator it = _clients.begin();
	std::vector<int> timeOutFds;
	
	// std::cout << "TIMEOUT client fd: " << std::endl;

	while (it != _clients.end()) {
		
		std::chrono::seconds timeLeft = 
			std::chrono::duration_cast<std::chrono::seconds>(now - it->second.getLastActivity());
		
			if (timeLeft.count() > 30) 
			timeOutFds.push_back(it->first);

		++it;
	}

	for (size_t i = 0; i < timeOutFds.size(); i++) {
		for (size_t j = 0; j < _pollfds.size(); j++) {
			if (_pollfds[j].fd == timeOutFds[i]) {
				// std::cout << "TIMEOUT client fd: " << timeOutFds[i] << std::endl;
				removeClient(j);
				break ;
			}
		}
	}
};

bool ServerManager::startCgi(Client& client, const HTTPRequest& request, const CgiRoute& route, const ServerConfig& servConf)
{

	(void)request;
	(void)route;

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
		// std::cerr <<"\t\t scriptName " << scriptName << std::endl;
		argv.push_back(const_cast<char*>(route.cgiPath.c_str()));
		argv.push_back(scriptName.data());
		argv.push_back(NULL);

		// SEEEEEEE //
			// for (size_t i = 0; i < cgiEnvironment.size(); i++) {
			// 	std::cerr << i << " " << cgiEnvironment[i] << std::endl;
			// }

			// std::cerr <<"\t route.cgiPath.c_str() " << route.cgiPath.c_str() << std::endl;
			// std::cerr <<"\t route.scriptPath.c_str() " << route.scriptPath.c_str() << std::endl;
			// std::cerr <<"\t route.workingDirectory.c_str() " << route.workingDirectory.c_str() << std::endl;

			// for (size_t i = 0; i < cgiEnvironment.size(); ++i)
			// std::cerr << i << " " << cgiEnvironment[i] << std::endl;

			// std::cerr << "argv[0] = " << argv[0] << std::endl;
			// std::cerr << "argv[1] = " << argv[1] << std::endl;

			// _exit(0);

		// SEEEEEEE //

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

	addFd(inputPipe[1], FD_CGI_INPUT, client.getClientFd());
	addFd(outputPipe[0], FD_CGI_OUTPUT, client.getClientFd());

	setFdEvents(inputPipe[1], POLLOUT);

	// CGI process creation will be implemented here.

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
};


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
		// std::cout << "Hello from try " << std::endl;
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
};

void ServerManager::addFd(int fd, FdType type, int clientFd)
{
	struct pollfd pollFd;

	pollFd.fd = fd;
	pollFd.events = POLLIN;
	pollFd.revents = 0;

	_pollfds.push_back(pollFd);
	_fdInfo[fd] = FdInfo{type, clientFd};
}

void ServerManager::removeFd(int fd)
{
	for (size_t i = 0; i < _pollfds.size(); ++i)
	{
		if (_pollfds[i].fd == fd)
		{
			_pollfds.erase(_pollfds.begin() + i);
			break;
		}
	}

	_fdInfo.erase(fd);
	close(fd); // added by Ivan :) -> dunno if for future removeFd we should not need the close the fd... lets see. 
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

std::vector<std::string> ServerManager::buildCgiEnvironment(const HTTPRequest& request,const ServerConfig& servConf) {

	std::vector<std::string> cgiEnv;
	std::string method;

	cgiEnv.push_back("GATEWAY_INTERFACE=CGI/1.1");
	if (request.getMethod() == Method::GET)
		method = "GET";
	else if (request.getMethod() == Method::POST)
		method = "POST";
	else 
		method = "DELETE";
	cgiEnv.push_back("REQUEST_METHOD=" + method);
	cgiEnv.push_back("SERVER_PROTOCOL=HTTP/" + request.getVersion());
	cgiEnv.push_back("SCRIPT_NAME=" + request.getPath());
	cgiEnv.push_back("QUERY_STRING=" + request.getQuery());
	cgiEnv.push_back("SERVER_NAME=" + servConf.getServerName()[0]);
	cgiEnv.push_back("SERVER_PORT=" + std::to_string(servConf.getPort()));

	const std::map<std::string, std::string>& headers = request.getHeaders();

	for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); it++) {
		std::string firstName = it->first;
		std::string secondValue = it->second;

		if (firstName == "content-type") {
			cgiEnv.push_back("CONTENT_TYPE=" + secondValue);
			continue;
		}
		if (firstName == "content-length") {
			cgiEnv.push_back("CONTENT_LENGTH=" + secondValue);
			continue;
		}
		for (size_t i = 0; i < firstName.size(); i++) {
			
			if (firstName[i] == '-') 
				firstName[i] = '_';
			else
				firstName[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(firstName[i])));
		}
		cgiEnv.push_back("HTTP_" + firstName + "=" + secondValue);
	}

	return cgiEnv;
};

bool ServerManager::writeToCgi(int fd, int clientFdInfo) {

	Client& client = _clients.at(clientFdInfo);
	const HTTPRequest& request = client.getRequest();
	size_t bodySize = request.getBodySize();
	// std::cerr << " HERE ___> write to cgi written-> " << std::endl;

	if (bodySize == 0) {
		
		removeFd(fd);
		client.setCgiInputFd(-1);
		client.setCgiState(CGI_READING);
		return true;
	}

	const std::string& buffer = request.getRequestBuffer();
	size_t bodyOffset = request.getBodyOffset();
	size_t sent = client.getCgiInputOffset();

	
	ssize_t written = write(fd, buffer.data() + bodyOffset + sent, bodySize - sent);
	// std::cerr << " HERE ___> write to cgi written-> " << written << std::endl;
	if (written > 0) 
		client.setCgiInputOffset(sent + static_cast<size_t>(written));

	if (client.getCgiInputOffset() >= bodySize) {

		removeFd(fd);
		client.setCgiInputFd(-1);
		client.setCgiState(CGI_READING);
		return true;
	}
	return false;
};


/*
	1. Find the Client using clientFd
	2. Find the already parsed HTTP request/body
	3. Check how many body bytes have already been written
	4. write() only after POLLOUT
	5. Update the CGI input offset
	6. If the full body is sent:
	- close the CGI input pipe
	- remove that pipe fd from poll()
	- child sees EOF on stdin
	- switch CGI state toward reading
*/

bool ServerManager::readFromCgi(int fd, int clientFdInfo) {

	Client& client = _clients.at(clientFdInfo);

	char buffer[4096];
	ssize_t bytesRead = read(fd, buffer, sizeof(buffer));

	if(bytesRead > 0) {
		client.setCgiOutput(std::string(buffer, static_cast<size_t>(bytesRead)));
		return false;
	}
	
	if (bytesRead == 0) {

		removeFd(fd);
		client.setCgiOutputFd(-1);

		CgiResult result = parseCgiOutput(client.getCgiOutput());

		// std::cerr << "CGI RESULT:" << std::endl;
		// std::cerr << "\tvalid: " << result.valid << std::endl;
		// std::cerr << "\tstatus: " << result.statusCode << std::endl;

		// std::cerr << "\theaders:" << std::endl;
		// for (auto i = result.headers.begin(); i != result.headers.end(); ++i)
		// {
		// 	std::cerr << "\t\t>" << i->first
		// 			<< "< = >" << i->second
		// 			<< "<" << std::endl;
		// }

		// std::cerr << "\tbody: >\n\n"
		// 		<< result.body
		// 		<< "<" << std::endl;


		if (result.valid) {
			std::string response;
			response = buildCgiResponse(result);
			client.setResponseBuffer(response);
			// setFdEvents(clientFdInfo, POLLOUT);

			// return true;
		} else {
			const ServerConfig& servConf = getClientServerManager(client.getServerFd(), client.getHost());

			HTTPResponse errorResponse = HTTPResponseBuild::makeErrorResponse(500, client.getRequest(), servConf);
			client.setResponseBuffer(errorResponse.toString(errorResponse));

			// setFdEvents(clientFdInfo, POLLOUT);
			// return true;
		}
		setFdEvents(clientFdInfo, POLLOUT);

		return true;
	}
	removeFd(fd);
	client.setCgiOutputFd(-1);

	
	// CGI read failed.
    // Build/queue 500 response here.
	std::cerr << "CGI end of function\n";

	return true;
};

CgiResult ServerManager::parseCgiOutput(const std::string& cgiOutput) {
	
	CgiResult parseCgi;
	parseCgi.statusCode = 500;
	parseCgi.valid = false;
	bool hasStatus = false;

	if (cgiOutput.empty()) {
		// parseCgi.statusCode = 500;
		return parseCgi;
	}

	// std::cerr << " CGI OUTPUT ----> " << cgiOutput << std::endl;
	size_t headerEnd = cgiOutput.find("\r\n\r\n");
	size_t separator = 4;
	if (headerEnd == std::string::npos) {
		headerEnd = cgiOutput.find("\n\n");
		separator = 2;
	}

	if (headerEnd == std::string::npos) {
		// parseCgi.statusCode = 500;
		return parseCgi;
	}

	std::string headerPart = cgiOutput.substr(0, headerEnd);
	// std::string bodyPart = cgiOutput.substr(headerEnd + separator);
	parseCgi.body = cgiOutput.substr(headerEnd + separator);

	std::istringstream stream(headerPart);
	std::string line;

	while (std::getline(stream, line)) {
		std::cerr << "line -> " << line << std::endl;

		size_t colon = line.find(':');
		std::string name = toLower(line.substr(0, colon));
		std::string value = line.substr(colon + 1);

		// std::cerr << "\t name ->" << name << "<\t value ->" << value << "<" << std::endl;

		while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
			value.erase(0, 1);

		if (name == "status") {
			std::istringstream statusStream(value);
			int statusCode;

			if (!(statusStream >> statusCode) || statusCode < 100 || statusCode > 599) {
				
				return parseCgi;
			}
			parseCgi.statusCode = statusCode;
			hasStatus = true;
		} else {
			parseCgi.headers[name] = value;
		}


	}
	
	parseCgi.valid = true;
	if (!hasStatus)
		parseCgi.statusCode = 200;

	return parseCgi;

}

/*
for full CGI web page.
curl -H "Host: test.localhost" \
"http://127.0.0.1:8080/cgi-bin/dynamic.py?name=Ivan"
*/

std::string ServerManager::buildCgiResponse(const CgiResult& result) {

	std::string response;

	response += "HTTP/1.1 ";
	response += std::to_string(result.statusCode);
	response += " ";
	response += HTTPResponseBuild::getStatusText(result.statusCode);
	response += "\r\n";

	for (auto it = result.headers.begin(); it != result.headers.end(); it++) {
		
		if (toLower(it->first) == "content-length")
			continue;
		response += it->first;
		response += ": ";
		response += it->second;
		response += "\r\n";

	}
	response += "Content-Length: ";
	response += std::to_string(result.body.size());
	response += "\r\n";
	response += "Connection: keep-alive\r\n";
	response += "\r\n";
	response += result.body;

	return response;
}
