#include "./hpp/HTTPRequestParser.hpp"
#include "./hpp/HTTPParseException.hpp"
#include <iostream>
#include <algorithm>

std::string HTTPRequestParser::trim(const std::string &text) const {

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
		throw HTTPParseException(400, "Missing space after method");
	
	size_t secondSpace = line.find(' ', firstSpace + 1);

	if (secondSpace == std::string::npos)
		throw HTTPParseException(400, "Missing space after URI");

	size_t thirdSpace = line.find(' ', secondSpace + 1);

	if (thirdSpace != std::string::npos)
		throw HTTPParseException(400, "Too many spaces in request line");

	std::string strMethod = line.substr(0, firstSpace);
	if (strMethod.empty())
        throw HTTPParseException(400, "Empty HTTP method");
	request.setMethod(parseMethod(strMethod));

	std::string strUri = line.substr(firstSpace + 1, secondSpace - firstSpace - 1);
	if (strUri.empty())
        throw HTTPParseException(400, "Empty request URI");
	request.setUri(strUri);

	std::string strVer = line.substr(secondSpace + 1);

	if (strVer == "HTTP/1.1")
		request.setVersion("1.1");
	else if (strVer == "HTTP/1.0")
		request.setVersion("1.0");
	else
		throw HTTPParseException(505, "HTTP version not supported");
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
			std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
        		return std::tolower(c);
    		});

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

HTTPRequest HTTPRequestParser::parse(const std::string &buffer, size_t requestSize) const{

	if (requestSize > buffer.size())
		throw HTTPParseException(400, "Invalid request size");

	HTTPRequest httpparseresult;
	
	size_t endFirstLine = buffer.find("\r\n");
	if (endFirstLine == std::string::npos)
		throw HTTPParseException(400, "Missing request-line terminator");
	std::string firstLine = buffer.substr(0, endFirstLine);
	parseRequestLine(firstLine, httpparseresult);

	size_t headersEnd = buffer.find("\r\n\r\n", endFirstLine + 2);
	if (headersEnd == std::string::npos)
		throw HTTPParseException(400, "Missing end of headers");

	std::string headers = buffer.substr(endFirstLine + 2, headersEnd - endFirstLine - 2);
	parseHeaders(headers, httpparseresult);

	const std::map<std::string, std::string> allHeaders = httpparseresult.getHeaders();
	std::map<std::string, std::string>::const_iterator it;

	it = allHeaders.find("content-type");

	if (it != allHeaders.end()) {
		size_t semicolon = it->second.find(";");
		std::string lowSecond = toLower(it->second);
		if (semicolon != std::string::npos) {
			if (trim(lowSecond.substr(0,semicolon)) =="multipart/form-data") {
				size_t boundPos = lowSecond.find("boundary=");
				if (boundPos != std::string::npos) {
					httpparseresult.setBodyType(BODY_MULTIPART);
					httpparseresult.setBoundary(it->second.substr(boundPos + 9));
				}
			}
			else
				httpparseresult.setBodyType(BODY_RAW);
		}
		else
			httpparseresult.setBodyType(BODY_RAW);
	}

	size_t bodyOffset = headersEnd + 4;
	if (bodyOffset > requestSize)
		throw HTTPParseException(400, "Invalid body offset");
	httpparseresult.setBodyLocation(buffer, bodyOffset, requestSize - bodyOffset);

	return httpparseresult;
};
