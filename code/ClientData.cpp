#include "./hpp/ClientData.hpp"

Client::Client() :  _responseSent(0), _client_fd(-1), _server_fd(-1), _headersParsed(false),
		_bodyType(BodyType::None), _contentLength(0), _bodyPos(0),
		_bodySize(0), _requestEnd(0), _requestErrorCode(0), _closeAfterResoinse(false), 
        _lastActivity(std::chrono::steady_clock::now()) {
	// std::cout << "Client: " << _client_fd << ", Server: " << _server_fd << std::endl;
};

Client::Client(int client_fd, int server_fd) :  _responseSent(0), _client_fd(client_fd), _server_fd(server_fd), 
		_headersParsed(false), _bodyType(BodyType::None), _contentLength(0), _bodyPos(0),
		_bodySize(0), _requestEnd(0), _requestErrorCode(0), _closeAfterResoinse(false), 
        _lastActivity(std::chrono::steady_clock::now()) {
	// std::cout << "Client: " << _client_fd << ", Server: " << _server_fd << std::endl;
};

void Client::updateLastActivity() {
    _lastActivity = std::chrono::steady_clock::now();
};

void Client::appendToRequestBuffer(const char* buffer, size_t bytes) {

	_requestBuffer.append(buffer, bytes);	
};

void Client::clearRequestBuffer() {
	_requestBuffer.clear();

	_headersParsed = false;
	_host.clear();
	_bodyType = BodyType::None;
	_contentLength = 0;
	_bodyPos = 0;
	_bodySize = 0;
	_requestEnd = 0;
	_requestErrorCode = 0;

	_request = HTTPRequest();
};

void Client::consumeRequest() {

	if (_requestEnd > _requestBuffer.size())
		_requestEnd = _requestBuffer.size();

	_requestBuffer.erase(0, _requestEnd);
	_headersParsed = false;
	_host.clear();
	_bodyType = BodyType::None;
	_contentLength = 0;
	_bodyPos = 0;
	_bodySize = 0;
	_requestEnd = 0;
	_requestErrorCode = 0;
	_closeAfterResoinse = false;

	_request = HTTPRequest();
};


void Client::setClientRequest(const HTTPRequest& req) {
	_request = req;
};

void Client::setResponseBuffer(const std::string& response) {
	_responseBuffer = response;
	_responseSent = 0;
};

void Client::setResponseSent(size_t sent) {
	_responseSent = sent;
};

void Client::setCloseAfterResponse(bool value) {
	_closeAfterResoinse = value;
};

void Client::setHost(const std::string& host) {
	_host = host;
};

const std::string& Client::getHost() const { return _host; };
bool Client::getCloseAfterReponse() const { return _closeAfterResoinse; };
size_t Client::getResponseSent() const { return _responseSent; };
const std::string& Client::getResponseBuffer() const { return _responseBuffer; };
size_t Client::getBodyPos() const { return _bodyPos; };
size_t Client::getBodySize() const { return _bodySize; };
BodyType Client::getBodyType() const { return _bodyType; }
int Client::getClientFd() const { return _client_fd; };
int Client::getServerFd() const { return _server_fd; };
const std::string& Client::getRequestBuffer() const { return _requestBuffer; };
const HTTPRequest& Client::getRequest() const { return _request; };
size_t Client::getRequestEnd() const { return _requestEnd; };
int Client::getRequestErrorCode() const { return _requestErrorCode; };
bool Client::getHeaderIsParsed() const {return _headersParsed; };
const std::chrono::steady_clock::time_point& Client::getLastActiviry() { return _lastActivity; };



void Client::clearResponse() {
	_responseBuffer.clear();
	_responseSent = 0;
};

bool Client::decodeChunkedBody()
{
	const size_t oldRequestEnd = _requestEnd;
	size_t readPos = _bodyPos;
	size_t writePos = _bodyPos;

	while (true)
	{
		size_t sizeLineEnd = _requestBuffer.find("\r\n", readPos);

		if (sizeLineEnd == std::string::npos || sizeLineEnd >= oldRequestEnd)
			return false;

		std::string sizeLine = _requestBuffer.substr(
			readPos,
			sizeLineEnd - readPos
		);

		size_t extensionPosition = sizeLine.find(";");

		if (extensionPosition != std::string::npos)
			sizeLine = sizeLine.substr(0, extensionPosition);

		sizeLine = trim(sizeLine);

		size_t chunkSize = 0;

		if (!parseHexSize(sizeLine, chunkSize))
			return false;

		readPos = sizeLineEnd + 2;

		if (chunkSize == 0)
			break;

		if (readPos > oldRequestEnd || chunkSize > oldRequestEnd - readPos)
			return false;

		std::memmove(&_requestBuffer[writePos], &_requestBuffer[readPos], chunkSize);

		writePos += chunkSize;
		readPos += chunkSize + 2;
	}

	const size_t newRequestEnd = writePos;

	_requestBuffer.erase(newRequestEnd, oldRequestEnd - newRequestEnd);

	_bodySize = newRequestEnd - _bodyPos;
	_requestEnd = newRequestEnd;

	return true;
}