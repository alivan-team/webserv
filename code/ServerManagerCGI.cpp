#include "./hpp/ServerManager.hpp"
#include "./hpp/HTTPResponseBuild.hpp"
#include "./hpp/HTTPRequestParser.hpp"
#include "./hpp/printDebug.hpp"
#include "./hpp/HTTPParseException.hpp"

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
}

bool ServerManager::writeToCgi(int fd, int clientFdInfo) {

	Client& client = _clients.at(clientFdInfo);
	const HTTPRequest& request = client.getRequest();
	size_t bodySize = request.getBodySize();

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

	if (written < 0) {
		failCgi(client);
		return true;
	}

	if (written > 0) 
		client.setCgiInputOffset(sent + static_cast<size_t>(written));

	if (client.getCgiInputOffset() >= bodySize) {

		removeFd(fd);
		client.setCgiInputFd(-1);
		client.setCgiState(CGI_READING);
		return true;
	}
	return false;
}

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

		if (client.getCgiPid() <= 0) {
			finishCgiResponse(client);
		} else {
			client.setCgiState(CGI_WAITING_EXIT);
		}

		return true;
	}

    failCgi(client);
	return true;
}

void ServerManager::finishCgiResponse(Client& client) {

		if(client.getCgiProcessFailed()) {
			const ServerConfig& servConf = getClientServerManager(client.getServerFd(), client.getHost());

			HTTPResponse errorResponse = HTTPResponseBuild::makeErrorResponse(500, client.getRequest(), servConf);
			client.setResponseBuffer(errorResponse.toString(errorResponse));
		} else {

			CgiResult result = parseCgiOutput(client.getCgiOutput());
	
			if (result.valid) {
				std::string response;
				response = buildCgiResponse(result, client.getRequest());
				client.setResponseBuffer(response);
	
			} else {
				const ServerConfig& servConf = getClientServerManager(client.getServerFd(), client.getHost());
	
				HTTPResponse errorResponse = HTTPResponseBuild::makeErrorResponse(500, client.getRequest(), servConf);
				client.setResponseBuffer(errorResponse.toString(errorResponse));
			}
		}
		client.setCgiState(CGI_NONE);
		client.updateLastActivity();
		setFdEvents(client.getClientFd(), POLLOUT);
}

CgiResult ServerManager::parseCgiOutput(const std::string& cgiOutput) {
	
	CgiResult parseCgi;
	parseCgi.statusCode = 500;
	parseCgi.valid = false;
	bool hasStatus = false;

	if (cgiOutput.empty())
		return parseCgi;

	size_t headerEnd = cgiOutput.find("\r\n\r\n");
	size_t separator = 4;
	if (headerEnd == std::string::npos) {
		headerEnd = cgiOutput.find("\n\n");
		separator = 2;
	}

	if (headerEnd == std::string::npos)
		return parseCgi;

	std::string headerPart = cgiOutput.substr(0, headerEnd);
	parseCgi.body = cgiOutput.substr(headerEnd + separator);

	std::istringstream stream(headerPart);
	std::string line;

	while (std::getline(stream, line)) {

        if (!line.empty() && line.back() == '\r')
            line.pop_back();

		size_t colon = line.find(':');
        if (colon == std::string::npos)
            return parseCgi;

		std::string name = toLower(line.substr(0, colon));
		std::string value = line.substr(colon + 1);

		while (!value.empty() && (value.front() == ' ' || value.front() == '\t'))
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

std::string ServerManager::buildCgiResponse(const CgiResult& result, const HTTPRequest &request) {

	std::string response;

	response += "HTTP/" + request.getVersion() + " ";
	response += std::to_string(result.statusCode);
	response += " ";
	response += HTTPResponseBuild::getStatusText(result.statusCode);
	response += "\r\n";

	for (auto it = result.headers.begin(); it != result.headers.end(); it++) {
		
		if (toLower(it->first) == "content-length" || toLower(it->first) == "connection")
			continue;

		response += it->first;
		response += ": ";
		response += it->second;
		response += "\r\n";
	}
	response += "Content-Length: ";
	response += std::to_string(result.body.size());
	response += "\r\n";

	response += "Connection: ";
    response +=  HTTPResponseBuild::decideConnection(request);
    response +=  "\r\n\r\n";
	response += result.body;

	return response;
}

void ServerManager::failCgi(Client& client) {

    client.setCgiProcessFailed(true);
    
    if (client.getCgiInputFd() != -1) {
        removeFd(client.getCgiInputFd());
        client.setCgiInputFd(-1);
    }

    if (client.getCgiOutputFd() != -1) {
        removeFd(client.getCgiOutputFd());
        client.setCgiOutputFd(-1);
    }

    if (client.getCgiPid() > 0) {
        kill(client.getCgiPid(), SIGKILL);
        client.setCgiState(CGI_WAITING_EXIT);
        return ;
    }

    finishCgiResponse(client);
}


void ServerManager::reapCgiChildern() {
	
	std::vector<int> clientsToErase;

	for (auto it = _clients.begin(); it != _clients.end(); it++){

		Client& client = it->second;
		pid_t pid = client.getCgiPid();

		if (pid <= 0)
			continue;
		
		int status; 
		pid_t result = waitpid(client.getCgiPid(), &status, WNOHANG);

		if (result == 0)
			continue;
		
		if (result < 0) {
			client.setCgiProcessFailed(true);
			client.setCgiPid(-1);

			if (client.getPendingRemoval()) {
				clientsToErase.push_back(it->first);
			} else if (client.getCgiState() == CGI_WAITING_EXIT){
        		finishCgiResponse(client);
			}
			continue;
		}
	
		if (WIFEXITED(status)) {
			if (WEXITSTATUS(status) != 0) 
				client.setCgiProcessFailed(true);

		} else if (WIFSIGNALED(status)) {
			client.setCgiProcessFailed(true);
		}

		client.setCgiPid(-1);

		if (client.getPendingRemoval()) {
			clientsToErase.push_back(it->first);
			continue;
		}

		if (client.getCgiState() == CGI_WAITING_EXIT)
			finishCgiResponse(client);
	}

	for (size_t i = 0; i < clientsToErase.size(); i++) {
		// close(clientsToErase[i]);
		_clients.erase(clientsToErase[i]);
	}
}
