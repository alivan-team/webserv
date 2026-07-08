#include "./hpp/HTTPRequestParser.hpp"
#include <iostream>

std::string HTTPRequestParser::trim(const std::string &text) const

{

	size_t begin = 0;

	while (begin < text.length() && std::isspace(text[begin]))
		begin++;

	size_t end = text.length();

	while (end > begin && std::isspace(text[end - 1]))
		end--;

	return (text.substr(begin, end - begin));

}

void HTTPRequestParser::parseRequestLine(
	const std::string &line,
	HTTPRequest &request) const {
	
	size_t firstSpace = line.find(' ');

	if (firstSpace == std::string::npos)
		throw std::runtime_error("Invalid HTTP request");
	
	size_t secondSpace = line.find(' ', firstSpace + 1);

	if (secondSpace == std::string::npos)
		throw std::runtime_error("Invalid HTTP request");

	size_t thirdSpace = line.find(' ', secondSpace + 1);

	if (thirdSpace != std::string::npos)
		throw std::runtime_error("Invalid HTTP request");

	std::string strMethod = line.substr(0, firstSpace);
	request.setMethod(parseMethod(strMethod));

	std::string strUri = line.substr(firstSpace + 1, secondSpace - firstSpace - 1);
	request.setUri(strUri);

	std::string strVer = line.substr(secondSpace + 1);
	if (strVer == "HTTP/1.1")
		request.setVersion("1.1");
	if (strVer == "HTTP/1.0")
		request.setVersion("1.0");

	parseUri(strUri, request);
};

void HTTPRequestParser::parseHeaders(
    const std::string &headers,
    HTTPRequest &request) const {

		size_t begin = 0;

		while (begin < headers.length())
		{
		    size_t end = headers.find("\r\n", begin);

		    if (end == std::string::npos)
		        end = headers.length();

		    std::string line = headers.substr(begin, end - begin);
			size_t colon = line.find(':');

			if (colon == std::string::npos)
				return ;

			std::string name = trim(line.substr(0, colon));
			std::string value = trim(line.substr(colon + 1));
			request.addHeader(name, value);

			begin = end + 2;
		}
	};

void HTTPRequestParser::parseUri(
    const std::string &uri,
    HTTPRequest &request) const {

	size_t questMark = uri.find('?');

	if (questMark == std::string::npos){
		request.setPath(uri);
		return ;

	}
	
	std::string strPath = uri.substr(0, questMark);
	request.setPath(strPath);

	std::string strQuery = uri.substr(questMark + 1);
	request.setQuery(strQuery);	
};

Method HTTPRequestParser::parseMethod(const std::string &method) const {

	if (method == "GET")
		return Method::GET;
	if (method == "POST")
		return Method::POST;
	if (method == "DELETE")
		return Method::DELETE;

	return Method::UNKNOWN;
};

HTTPRequest HTTPRequestParser::parse(const std::string &buffer) const{

	HTTPRequest httpparseresult;
	
	size_t endFirstLine = buffer.find("\r\n");
	std::string firstLine = buffer.substr(0, endFirstLine);
	parseRequestLine(firstLine, httpparseresult);

	// size_t endSecondLine = buffer.find("\r\n", endFirstLine + 2);
	// std::string secondLine = buffer.substr(endFirstLine + 2, endSecondLine - endFirstLine - 2);
	// parseUri(secondLine, httpparseresult);	

	std::string restLine = buffer.substr(endFirstLine + 2);
	parseHeaders(restLine, httpparseresult);

	return httpparseresult;
};
