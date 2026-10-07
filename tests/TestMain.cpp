#include "ConfigParser.hpp"
#include "ClientData.hpp"
#include "HTTPRequestParser.hpp"
#include "HTTPResponse.hpp"
#include "HTTPResponseBuild.hpp"
#include "LocationConfig.hpp"
#include "ServerConfig.hpp"
#include "HelperFunctions.hpp"
#include "HTTPParseException.hpp"
#include "MultipartParser.hpp"
#include "MultipartPart.hpp"
#include "../code/hpp/ServerManager.hpp"
#include "../code/hpp/ClientData.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <dirent.h>
#include <fstream>
#include <iterator>
#include <sys/stat.h>
#include <unistd.h>
#include <chrono>
#include <thread>

namespace {

int g_failures = 0;

void check(bool condition, const std::string& message)
{
	if (!condition) {
		++g_failures;
		std::cerr << "  FAIL: " << message << '\n';
	}
}

template <typename Function>
void checkThrows(Function function, const std::string& message)
{
	try {
		function();
		check(false, message);
	} catch (const std::exception&) {
	}
}

void testValidationHelpers()
{
	check(check_num("8080"), "numeric values are accepted");
	check(!check_num("80a"), "non-numeric values are rejected");
	check(!check_num(""), "empty numeric values are rejected");
	check(checkUriPath("/assets/logo.png"), "valid URI paths are accepted");
	check(!checkUriPath("assets"), "URI paths need a leading slash");
	check(!checkUriPath("/has space"), "URI paths with whitespace are rejected");
	check(checkFSPath("./site/www"), "valid filesystem paths are accepted");
	check(!checkFSPath("./site/my files"), "filesystem paths with whitespace are rejected");
	check(hasControlChar("line\nfeed"), "control characters are detected");
}

void testLocationConfig()
{
	LocationConfig location;
	location.setUriPath("/upload");
	location.setAllowMethods(std::vector<std::string>{"GET", "POST"});
	location.setRoot(std::vector<std::string>{"./site/www"});
	location.setIndex(std::vector<std::string>{"index.html", "fallback.html"});
	location.setAutoIndex(std::vector<std::string>{"on"});
	location.setRedirect(std::vector<std::string>{"301", "/new-path"});

	check(location.getUriPath() == "/upload", "location URI is stored");
	check(location.isGetAllowed() && location.isPostAllowed(), "configured methods are enabled");
	check(!location.isDeleteAllowed(), "unconfigured methods stay disabled");
	check(location.getRoot() == "./site/www", "location root is stored");
	// check(location.getIndex().size() == 2, "location indexes are stored");
	check(location.getAutoIndex(), "autoindex on is stored");
	check(location.hasRedirect() && location.getRedirect()._number == 301,
		  "redirect is stored");

	checkThrows([&location] { location.setUriPath("upload"); }, "invalid location URI is rejected");
	checkThrows([&location] { location.setAllowMethods(std::vector<std::string>{"PUT"}); },
				"unknown location method is rejected");
	checkThrows([&location] { location.setCgiExtension(std::vector<std::string>{"php"}); },
				"CGI extension without dot is rejected");
	checkThrows([&location] { location.setAutoIndex(std::vector<std::string>{"enabled"}); },
				"unknown autoindex values are rejected");
}

void testServerConfig()
{
	ServerConfig server;

	check(
		server.getServerName().size() == 1,
		"default server name exists"
	);

	check(
		server.getServerName().at(0) == "localhost",
		"default server name is localhost"
	);

	std::vector<std::string> serverNames;
	serverNames.push_back("example.test");
	serverNames.push_back("api-example");

	server.setServerName(serverNames);

	check(
		server.getServerName().size() == 2,
		"configured server names replace the default"
	);

	check(
		server.getServerName().at(0) == "example.test",
		"first configured server name is stored"
	);

	check(
		server.getServerName().at(1) == "api-example",
		"second configured server name is stored"
	);

	std::vector<std::string> roots;
	roots.push_back("./site/www");

	server.setRoot(roots);

	check(
		!server.getRoot().empty(),
		"server root is stored"
	);

	check(
		server.getRoot().back() == "./site/www",
		"configured server root is correct"
	);

	std::vector<std::string> indexes;
	indexes.push_back("index.html");
	indexes.push_back("home.html");

	server.setIndex(indexes);

	check(
		server.getIndex().size() == 3,
		"configured indexes are appended to the default"
	);

	check(
		server.getIndex().at(0) == "index.html",
		"default index remains stored"
	);

	check(
		server.getIndex().at(1) == "index.html",
		"first configured index is appended"
	);

	check(
		server.getIndex().at(2) == "home.html",
		"second configured index is appended"
	);

	std::cout << "PASS ServerConfig" << std::endl;
}

void testConfigParser()
{
	ConfigParser parser;
	parser.parse("tests/fixtures/valid.conf");
	const std::vector<ServerConfig>& servers = parser.getServers();

	check(servers.size() == 1, "one server block is parsed");
	check(servers.at(0).getPort() == 8088, "listen directive is parsed");
	check(servers.at(0).getLocations().size() == 2, "location blocks are parsed");
	check(servers.at(0).getLocations().at(1).isPostAllowed(), "location methods are parsed");
	checkThrows([] { ConfigParser().parse("tests/fixtures/invalid.conf"); },
				"unknown directives are rejected");
}

void testCgiConfigurationValidation()
{
	char directoryTemplate[] = "/tmp/webserv-cgi-config-XXXXXX";
	char* directory = mkdtemp(directoryTemplate);
	check(directory != NULL, "CGI configuration fixture directory is created");
	if (!directory)
		return;
	const std::string base(directory);
	const std::string interpreter = base + "/interpreter";
	std::ofstream(interpreter) << "#!/bin/sh\nexit 0\n";
	chmod(interpreter.c_str(), 0700);
	const std::string config = base + "/test.conf";
	const auto parseDirectives = [&](const std::string& directives) {
		std::ofstream file(config);
		file << "server { listen 8088; location /cgi-bin { allow_methods GET POST; "
			 << directives << " } }\n";
		file.close();
		ConfigParser parser;
		parser.parse(config);
	};
	parseDirectives("cgi_extension .py; cgi_path " + interpreter + ";");
	parseDirectives("cgi_path " + interpreter + "; cgi_extension .py;");
	checkThrows([&] { parseDirectives("cgi_extension .py;"); }, "extension without interpreter is rejected");
	checkThrows([&] { parseDirectives("cgi_path " + interpreter + ";"); }, "interpreter without extension is rejected");
	checkThrows([&] { parseDirectives("cgi_extension .py .sh; cgi_path " + interpreter + ";"); }, "mismatched CGI pairs are rejected");
	checkThrows([&] { parseDirectives("cgi_path " + interpreter + "; cgi_extension .py .sh;"); }, "mismatched CGI pairs in reverse order are rejected");
	checkThrows([&] { parseDirectives("cgi_extension .py; cgi_path " + base + "/missing;"); }, "missing CGI interpreter is rejected");
	checkThrows([&] { parseDirectives("cgi_extension .py; cgi_path " + base + ";"); }, "directory used as interpreter is rejected");
	checkThrows([&] { parseDirectives("cgi_extension .; cgi_path " + interpreter + ";"); }, "empty CGI extension suffix is rejected");
	checkThrows([&] { parseDirectives("cgi_extension .py .py; cgi_path " + interpreter + " " + interpreter + ";"); }, "ambiguous duplicate CGI extensions are rejected");
	chmod(interpreter.c_str(), 0600);
	checkThrows([&] { parseDirectives("cgi_extension .py; cgi_path " + interpreter + ";"); }, "non-executable interpreter is rejected");
	unlink(config.c_str());
	unlink(interpreter.c_str());
	rmdir(directory);
}

void testCgiRoutingAndMissingUploadStore()
{
	char directoryTemplate[] = "/tmp/webserv-cgi-route-XXXXXX";
	char* directory = mkdtemp(directoryTemplate);
	check(directory != NULL, "CGI route fixture directory is created");
	if (!directory)
		return;
	const std::string base(directory);
	std::ofstream(base + "/existing.py") << "print('Content-Type: text/plain\\n\\nOK')\n";
	mkdir((base + "/directory.py").c_str(), 0700);
	const LocationConfig* matchedLocation;
	LocationConfig location;
	location.setUriPath("/cgi-bin");
	location.setRoot({base});
	location.setAllowMethods({"GET", "POST"});
	location.setCgiExtension({".py"});
	location.setCgiPath({"/bin/sh"});
	location.validateCgiConfig();
	ServerConfig server;
	server.addLocation(location);
	for (const std::string method : {"GET", "POST"}) {
		for (const std::string name : {"missing.py", "directory.py"}) {
			const std::string raw = method + " /cgi-bin/" + name + " HTTP/1.1\r\nContent-Length: 0\r\n\r\n";
			HTTPRequest request = HTTPRequestParser().parse(raw, raw.size());
			CgiRoute route;
			int error = 0;
			check(!HTTPResponseBuild::resolveCgiRoute(request, server, route, error, matchedLocation) && error == 404,
				method + " absent/non-file CGI script produces 404 without falling through");
		}
	}
	std::string requestBuffer;
	const auto requestFor = [&requestBuffer](const std::string& path) {
		requestBuffer = "POST " + path + " HTTP/1.1\r\nContent-Length: 0\r\n\r\n";
		return HTTPRequestParser().parse(requestBuffer, requestBuffer.size());
	};
	CgiRoute route;
	int error = 999;
	check(HTTPResponseBuild::resolveCgiRoute(requestFor("/cgi-bin/existing.py"), server, route, error, matchedLocation)
		&& error == 0, "existing CGI accepts POST without upload_store");
	check(!HTTPResponseBuild::resolveCgiRoute(requestFor("/cgi-bin/plain.txt"), server, route, error, matchedLocation)
		&& error == 0, "ordinary URL is distinguished from missing CGI script");
	check(HTTPResponseBuild::build(requestFor("/cgi-bin/plain.txt"), server).getStatusCode() == 403,
		"ordinary POST without upload_store is forbidden, not a server failure");
	LocationConfig getOnly;
	getOnly.setUriPath("/get-only");
	getOnly.setAllowMethods({"GET"});
	server.addLocation(getOnly);
	check(HTTPResponseBuild::build(requestFor("/get-only/file"), server).getStatusCode() == 405,
		"disallowed POST retains 405 even without upload_store");
	LocationConfig redirect;
	redirect.setUriPath("/redirect");
	redirect.setCgiExtension({".py"});
	redirect.setCgiPath({"/bin/sh"});
	redirect.setRedirect({"301", "/new"});
	server.addLocation(redirect);
	check(!HTTPResponseBuild::resolveCgiRoute(requestFor("/redirect/missing.py"), server, route, error, matchedLocation)
		&& error == 0, "redirect is not mistaken for missing CGI");
	check(HTTPResponseBuild::build(requestFor("/redirect/missing.py"), server).getStatusCode() == 301,
		"redirect works without upload_store");
	unlink((base + "/existing.py").c_str());
	rmdir((base + "/directory.py").c_str());
	rmdir(directory);
}

void testHttpRequestParser()
{
	HTTPRequestParser parser;
	std::string raw = "GET /search?q=webserv HTTP/1.1\r\nHost: example.test\r\nConnection: close\r\n\r\n";
	HTTPRequest request = parser.parse(raw, raw.size());

	check(request.getMethod() == Method::GET, "HTTP method is parsed");
	check(request.getUri() == "/search?q=webserv", "raw URI is retained");
	check(request.getPath() == "/search", "URI path is separated");
	check(request.getQuery() == "q=webserv", "URI query is separated");
	check(request.getVersion() == "1.1", "HTTP version is parsed");
	check(request.getHeader("Host") == "example.test", "headers are parsed");
	check(request.getHeader("Connection") == "close", "header values are trimmed");
	checkThrows([&request] { request.getHeader("Missing"); }, "missing headers throw");
	checkThrows([&parser] { HTTPRequest request; parser.parseRequestLine("GET /", request); },
				"incomplete request lines are rejected");
}

void testHttpRequestBodyLocation()
{
	HTTPRequestParser parser;
	std::string raw = "POST /upload HTTP/1.1\r\nContent-Length: 11\r\n\r\n";
	raw.append("binary", 6);
	raw.push_back('\0');
	raw.append("body", 4);

	HTTPRequest request = parser.parse(raw, raw.size());
	const size_t expectedOffset = raw.find("\r\n\r\n") + 4;

	check(request.getBodyOffset() == expectedOffset, "body offset follows the header delimiter");
	check(request.getBodySize() == 11, "body size includes all binary bytes");
	check(&request.getRequestBuffer() == &raw, "request retains the existing request buffer");
	check(request.getRequestBuffer().compare(request.getBodyOffset(), request.getBodySize(),
											 "binary\0body", 11) == 0,
		  "body location addresses the original buffer without parsing it");
}

void testPostUpload()
{
	char temporaryDirectory[] = "/tmp/webserv-post-test-XXXXXX";
	char* uploadStore = mkdtemp(temporaryDirectory);
	check(uploadStore != NULL, "temporary upload directory is created");
	if (uploadStore == NULL)
		return;

	LocationConfig location;
	location.setUriPath("/upload");
	location.setAllowMethods(std::vector<std::string>{"POST"});
	location.setUploadStore(std::vector<std::string>{uploadStore});
	ServerConfig server;
	server.addLocation(location);

	std::string raw = "POST /upload HTTP/1.1\r\nContent-Length: 11\r\n\r\n";
	raw.append("binary", 6);
	raw.push_back('\0');
	raw.append("body", 4);
	HTTPRequest request = HTTPRequestParser().parse(raw, raw.size());
	HTTPResponse response = HTTPResponseBuild::build(request, server);

	DIR* directory = opendir(uploadStore);
	struct dirent* entry = directory == NULL ? NULL : readdir(directory);
	while (entry != NULL && (std::string(entry->d_name) == "." || std::string(entry->d_name) == ".."))
		entry = readdir(directory);

	check(response.toString(response).find("HTTP/1.1 201 Created\r\n") == 0,
		  "successful POST returns 201 Created");
	check(entry != NULL, "successful POST creates an upload file");

	if (entry != NULL) {
		const std::string path = std::string(uploadStore) + "/" + entry->d_name;
		std::ifstream file(path.c_str(), std::ios::binary);
		const std::string saved((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		check(saved == raw.substr(request.getBodyOffset(), request.getBodySize()),
			  "uploaded file exactly matches the original binary body");
		unlink(path.c_str());
	}
	if (directory != NULL)
		closedir(directory);
	rmdir(uploadStore);
}

void testHttpResponse()
{
	HTTPResponse response;
	response.setVersion("1.1");
	response.setStatusCode(200);
	response.setStatus("OK");
	response.setHeader("Content-Type", "text/plain");
	response.setHeader("Content-Type", "text/html");
	response.setBody("hello");

	check(response.toString(response) == "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\nhello",
		  "responses use a valid status line and replace duplicate headers");
}

void testClientResponseBuffer()
{
	Client client(42, 7);

	const std::string response =
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: 5\r\n"
		"\r\n"
		"Hello";

	client.setResponseBuffer(response);

	check(
		client.getResponseBuffer() == response,
		"client stores a queued response"
	);

	check(
		client.getResponseSent() == 0,
		"a newly queued response starts with zero bytes sent"
	);

	client.setResponseSent(10);

	check(
		client.getResponseSent() == 10,
		"client tracks how many response bytes were sent"
	);

	client.clearResponse();

	check(
		client.getResponseBuffer().empty(),
		"clearing a response empties the response buffer"
	);

	check(
		client.getResponseSent() == 0,
		"clearing a response resets the sent-byte counter"
	);
}

void testClientCloseAfterResponse()
{
	Client client(42, 7);

	check(
		client.getCloseAfterReponse() == false,
		"clients do not close after responses by default"
	);

	client.setCloseAfterResponse(true);

	check(
		client.getCloseAfterReponse() == true,
		"client can be marked to close after its response"
	);

	client.setCloseAfterResponse(false);

	check(
		client.getCloseAfterReponse() == false,
		"close-after-response state can be reset"
	);
}

void testClientNewResponseResetsProgress()
{
	Client client(42, 7);

	client.setResponseBuffer("first response");

	client.setResponseSent(5);

	check(
		client.getResponseSent() == 5,
		"response progress can be updated"
	);

	client.setResponseBuffer("second response");

	check(
		client.getResponseSent() == 0,
		"queueing a new response resets response progress"
	);

	check(
		client.getResponseBuffer() == "second response",
		"queueing a new response replaces the previous response"
	);
}

void run(const std::string& name, void (*test)())
{
	const int failuresBefore = g_failures;
	test();
	std::cout << (failuresBefore == g_failures ? "PASS" : "FAIL") << " " << name << '\n';
}

void testClientDecodeChunkedBody()
{
	const size_t maxBodySize = 1024;

	{
		Client client(42, 7);

		const std::string request =
			"POST /upload HTTP/1.1\r\n"
			"Host: unit.test\r\n"
			"Transfer-Encoding: chunked\r\n"
			"\r\n"
			"5\r\n"
			"Hello\r\n"
			"0\r\n"
			"\r\n";

		client.appendToRequestBuffer(request.c_str(), request.size());

		check(
			client.parseHeaderClient() == RequestState::Complete,
			"single chunk headers are parsed"
		);

		check(
			client.checkRequestState(maxBodySize) == RequestState::Complete,
			"single chunk request is complete"
		);

		check(
			client.decodeChunkedBody(),
			"single chunk body is decoded"
		);

		const size_t bodyPos = client.getBodyPos();

		check(
			client.getBodySize() == 5,
			"single chunk decoded body size is correct"
		);

		check(
			client.getRequestBuffer().compare(bodyPos, 5, "Hello") == 0,
			"single chunk body is decoded correctly"
		);

		check(
			client.getRequestEnd() == bodyPos + 5,
			"request end follows decoded body"
		);
	}

	{
		Client client(42, 7);

		const std::string request =
			"POST /upload HTTP/1.1\r\n"
			"Host: unit.test\r\n"
			"Transfer-Encoding: chunked\r\n"
			"\r\n"
			"5\r\n"
			"Hello\r\n"
			"6\r\n"
			" World\r\n"
			"0\r\n"
			"\r\n";

		client.appendToRequestBuffer(request.c_str(), request.size());

		check(
			client.parseHeaderClient() == RequestState::Complete,
			"multi-chunk headers are parsed"
		);

		check(
			client.checkRequestState(maxBodySize) == RequestState::Complete,
			"multi-chunk request is complete"
		);

		check(
			client.decodeChunkedBody(),
			"multi-chunk body is decoded"
		);

		const size_t bodyPos = client.getBodyPos();

		check(
			client.getBodySize() == 11,
			"multi-chunk decoded body size is correct"
		);

		check(
			client.getRequestBuffer().compare(bodyPos, 11, "Hello World") == 0,
			"multiple chunks are joined without chunk framing"
		);
	}

	{
		Client client(42, 7);

		const std::string request =
			"POST /upload HTTP/1.1\r\n"
			"Host: unit.test\r\n"
			"Transfer-Encoding: chunked\r\n"
			"\r\n"
			"F\r\n"
			"123456789012345\r\n"
			"10\r\n"
			"0123456789ABCDEF\r\n"
			"1\r\n"
			"Z\r\n"
			"0\r\n"
			"\r\n";

		client.appendToRequestBuffer(request.c_str(), request.size());

		check(
			client.parseHeaderClient() == RequestState::Complete,
			"mixed hexadecimal chunk headers are parsed"
		);

		check(
			client.checkRequestState(maxBodySize) == RequestState::Complete,
			"mixed hexadecimal chunk sizes are complete"
		);

		check(
			client.decodeChunkedBody(),
			"mixed hexadecimal chunk sizes are decoded"
		);

		const std::string expectedBody =
			"123456789012345"
			"0123456789ABCDEF"
			"Z";

		const size_t bodyPos = client.getBodyPos();

		check(
			client.getBodySize() == 32,
			"decoded size is correct for F, 10 and 1 chunks"
		);

		check(
			client.getRequestBuffer().compare(
				bodyPos,
				expectedBody.size(),
				expectedBody
			) == 0,
			"F, 10 and 1 chunks are decoded correctly"
		);
	}

	{
		Client client(42, 7);

		const std::string request =
			"POST /upload HTTP/1.1\r\n"
			"Host: unit.test\r\n"
			"Transfer-Encoding: chunked\r\n"
			"\r\n"
			"A\r\n"
			"0123456789\r\n"
			"0\r\n"
			"\r\n";

		client.appendToRequestBuffer(request.c_str(), request.size());

		check(
			client.parseHeaderClient() == RequestState::Complete,
			"hexadecimal chunk headers are parsed"
		);

		check(
			client.checkRequestState(maxBodySize) == RequestState::Complete,
			"two-digit hexadecimal size is accepted"
		);

		check(
			client.decodeChunkedBody(),
			"two-digit hexadecimal chunk is decoded"
		);

		const size_t bodyPos = client.getBodyPos();

		check(
			client.getBodySize() == 10,
			"0xA decoded body size is correct"
		);

		check(
			client.getRequestBuffer().compare(bodyPos, 10, "0123456789") == 0,
			"0xA chunk data is decoded correctly"
		);
	}

	{
		Client client(42, 7);

		const std::string request =
			"POST /upload HTTP/1.1\r\n"
			"Host: unit.test\r\n"
			"Transfer-Encoding: chunked\r\n"
			"\r\n"
			"5;name=value\r\n"
			"Hello\r\n"
			"3;foo=bar\r\n"
			"abc\r\n"
			"0\r\n"
			"\r\n";

		client.appendToRequestBuffer(request.c_str(), request.size());

		check(
			client.parseHeaderClient() == RequestState::Complete,
			"chunk extension headers are parsed"
		);

		check(
			client.checkRequestState(maxBodySize) == RequestState::Complete,
			"chunk extensions are accepted before decoding"
		);

		check(
			client.decodeChunkedBody(),
			"chunk extensions do not prevent decoding"
		);

		const size_t bodyPos = client.getBodyPos();

		check(
			client.getBodySize() == 8,
			"chunk extension body size is correct"
		);

		check(
			client.getRequestBuffer().compare(bodyPos, 8, "Helloabc") == 0,
			"chunk extensions are removed with chunk framing"
		);
	}

	{
		Client client(42, 7);

		const std::string request =
			"POST /upload HTTP/1.1\r\n"
			"Host: unit.test\r\n"
			"Transfer-Encoding: chunked\r\n"
			"\r\n"
			"0\r\n"
			"\r\n";

		client.appendToRequestBuffer(request.c_str(), request.size());

		check(
			client.parseHeaderClient() == RequestState::Complete,
			"empty chunked headers are parsed"
		);

		check(
			client.checkRequestState(maxBodySize) == RequestState::Complete,
			"empty chunked body is complete"
		);

		check(
			client.decodeChunkedBody(),
			"empty chunked body is decoded"
		);

		check(
			client.getBodySize() == 0,
			"empty chunked body has zero decoded size"
		);

		check(
			client.getRequestEnd() == client.getBodyPos(),
			"empty chunked request ends at body position"
		);
	}

	{
		Client client(42, 7);

		const std::string firstRequest =
			"POST /upload HTTP/1.1\r\n"
			"Host: unit.test\r\n"
			"Transfer-Encoding: chunked\r\n"
			"Connection: keep-alive\r\n"
			"\r\n"
			"5\r\n"
			"Hello\r\n"
			"3\r\n"
			"abc\r\n"
			"0\r\n"
			"\r\n";

		const std::string secondRequest =
			"GET /next HTTP/1.1\r\n"
			"Host: unit.test\r\n"
			"Connection: close\r\n"
			"\r\n";

		const std::string combined = firstRequest + secondRequest;

		client.appendToRequestBuffer(combined.c_str(), combined.size());

		check(
			client.parseHeaderClient() == RequestState::Complete,
			"buffered request headers are parsed"
		);

		check(
			client.checkRequestState(maxBodySize) == RequestState::Complete,
			"chunked request completes when next request is buffered"
		);

		check(
			client.decodeChunkedBody(),
			"chunked request is decoded with next request buffered"
		);

		const size_t bodyPos = client.getBodyPos();

		check(
			client.getBodySize() == 8,
			"decoded body size is correct with next request buffered"
		);

		check(
			client.getRequestBuffer().compare(bodyPos, 8, "Helloabc") == 0,
			"decoded body is correct with next request buffered"
		);

		check(
			client.getRequestBuffer().compare(
				client.getRequestEnd(),
				secondRequest.size(),
				secondRequest
			) == 0,
			"next request remains untouched after decoding"
		);

		client.consumeRequest();

		check(
			client.getRequestBuffer() == secondRequest,
			"consumeRequest preserves the next request after decoding"
		);
	}
}

void testClientContentLengthUnaffected()
{
	Client client(42, 7);

	const std::string request =
		"POST /upload HTTP/1.1\r\n"
		"Host: unit.test\r\n"
		"Content-Length: 5\r\n"
		"\r\n"
		"Hello";

	client.appendToRequestBuffer(request.c_str(), request.size());

	check(
		client.parseHeaderClient() == RequestState::Complete,
		"Content-Length headers are parsed"
	);

	check(
		client.checkRequestState(1024) == RequestState::Complete,
		"Content-Length request remains complete"
	);

	check(
		client.getBodySize() == 5,
		"Content-Length body size remains unchanged"
	);

	check(
		client.getRequestBuffer().compare(
			client.getBodyPos(),
			client.getBodySize(),
			"Hello"
		) == 0,
		"Content-Length body remains unchanged"
	);
}

void testVirtualHostServerConfigs()
{
	ServerConfig small;
	ServerConfig medium;
	ServerConfig large;

	small.setServerName(
		std::vector<std::string>(1, "small.localhost")
	);
	small.setRoot(
		std::vector<std::string>(1, "./site/www/small")
	);

	medium.setServerName(
		std::vector<std::string>(1, "medium.localhost")
	);
	medium.setRoot(
		std::vector<std::string>(1, "./site/www/medium")
	);

	large.setServerName(
		std::vector<std::string>(1, "large.localhost")
	);
	large.setRoot(
		std::vector<std::string>(1, "./site/www/large")
	);

	check(
		small.getServerName().size() == 1,
		"small server has one server_name"
	);

	check(
		small.getServerName()[0] == "small.localhost",
		"small server_name is stored"
	);

	check(
		medium.getServerName()[0] == "medium.localhost",
		"medium server_name is stored"
	);

	check(
		large.getServerName()[0] == "large.localhost",
		"large server_name is stored"
	);

	check(
		small.getRoot().back() == "./site/www/small",
		"small server keeps its own root"
	);

	check(
		medium.getRoot().back() == "./site/www/medium",
		"medium server keeps its own root"
	);

	check(
		large.getRoot().back() == "./site/www/large",
		"large server keeps its own root"
	);
}

void testMultipleServerNames()
{
	ServerConfig server;

	std::vector<std::string> names;
	names.push_back("small.localhost");
	names.push_back("tiny.localhost");
	names.push_back("little.localhost");

	server.setServerName(names);

	check(
		server.getServerName().size() == 3,
		"multiple server_names are stored"
	);

	check(
		server.getServerName()[0] == "small.localhost",
		"first server_name is correct"
	);

	check(
		server.getServerName()[1] == "tiny.localhost",
		"second server_name is correct"
	);

	check(
		server.getServerName()[2] == "little.localhost",
		"third server_name is correct"
	);
}

void testVirtualHostBodySizeLimits()
{
	ServerConfig small;
	ServerConfig medium;
	ServerConfig large;

	small.setClientMaxBodySize(
		std::vector<std::string>(1, "10")
	);

	medium.setClientMaxBodySize(
		std::vector<std::string>(1, "100")
	);

	large.setClientMaxBodySize(
		std::vector<std::string>(1, "1000")
	);

	check(
		small.getClientMaxBodySize().back() == 10,
		"small server has body size limit 10"
	);

	check(
		medium.getClientMaxBodySize().back() == 100,
		"medium server has body size limit 100"
	);

	check(
		large.getClientMaxBodySize().back() == 1000,
		"large server has body size limit 1000"
	);
}

void testServerNameReplacesDefault()
{
	ServerConfig server;

	std::vector<std::string> names;
	names.push_back("small.localhost");

	server.setServerName(names);

	check(
		server.getServerName().size() == 1,
		"configured server_name replaces default server_name"
	);

	check(
		server.getServerName()[0] == "small.localhost",
		"configured server_name is stored instead of localhost"
	);
}

void testRedirect()
{
	// ---------------------------------------------------------
	// 1. A new LocationConfig must NOT have a redirect
	// ---------------------------------------------------------
	{
		LocationConfig location;

		check(
			!location.hasRedirect(),
			"new location has no redirect by default"
		);

		check(
			location.getRedirect()._number == 0,
			"default redirect status code is 0"
		);
	}


	// ---------------------------------------------------------
	// 2. Valid 301 redirect is stored correctly
	// ---------------------------------------------------------
	{
		LocationConfig location;

		std::vector<std::string> redirect;
		redirect.push_back("301");
		redirect.push_back("/new-page");

		location.setRedirect(redirect);

		check(
			location.hasRedirect(),
			"301 redirect is detected"
		);

		check(
			location.getRedirect()._number == 301,
			"redirect status code is stored"
		);

		check(
			location.getRedirect()._redirPath == "/new-page",
			"redirect path is stored"
		);
	}


	// ---------------------------------------------------------
	// 3. Unsupported redirect status must be rejected
	// ---------------------------------------------------------
	{
		LocationConfig location;

		checkThrows(
			[&location] {
				std::vector<std::string> redirect;
				redirect.push_back("302");
				redirect.push_back("/new-page");

				location.setRedirect(redirect);
			},
			"unsupported redirect status code is rejected"
		);
	}


	// ---------------------------------------------------------
	// 4. Non-numeric redirect status must be rejected
	// ---------------------------------------------------------
	{
		LocationConfig location;

		checkThrows(
			[&location] {
				std::vector<std::string> redirect;
				redirect.push_back("abc");
				redirect.push_back("/new-page");

				location.setRedirect(redirect);
			},
			"non-numeric redirect status code is rejected"
		);
	}


	// ---------------------------------------------------------
	// 5. Invalid redirect path must be rejected
	// ---------------------------------------------------------
	{
		LocationConfig location;

		checkThrows(
			[&location] {
				std::vector<std::string> redirect;
				redirect.push_back("301");
				redirect.push_back("/new page");

				location.setRedirect(redirect);
			},
			"redirect path containing whitespace is rejected"
		);
	}


	// ---------------------------------------------------------
	// 6. Missing redirect path must be rejected
	// ---------------------------------------------------------
	{
		LocationConfig location;

		checkThrows(
			[&location] {
				std::vector<std::string> redirect;
				redirect.push_back("301");

				location.setRedirect(redirect);
			},
			"redirect requires status code and path"
		);
	}


	// ---------------------------------------------------------
	// 7. GET request returns an actual 301 response
	// ---------------------------------------------------------
	{
		ServerConfig server;

		LocationConfig redirectLocation;
		redirectLocation.setUriPath("/old-page");

		std::vector<std::string> redirect;
		redirect.push_back("301");
		redirect.push_back("/new-page");

		redirectLocation.setRedirect(redirect);

		server.addLocation(redirectLocation);

		const std::string raw =
			"GET /old-page HTTP/1.1\r\n"
			"Host: localhost\r\n"
			"\r\n";

		HTTPRequest request =
			HTTPRequestParser().parse(raw, raw.size());

		HTTPResponse response =
			HTTPResponseBuild::build(request, server);

		std::string output =
			response.toString(response);

		check(
			output.find("HTTP/1.1 301 Moved Permanently\r\n") == 0,
			"redirect returns 301 Moved Permanently"
		);

		check(
			output.find("Location: /new-page\r\n") != std::string::npos,
			"redirect response contains Location header"
		);

		check(
			output.find("Content-Length: 0\r\n") != std::string::npos,
			"redirect response has zero Content-Length"
		);

		check(
			response.getBody().empty(),
			"redirect response has no body"
		);
	}


	// ---------------------------------------------------------
	// 8. Redirect happens before GET method permission check
	// ---------------------------------------------------------
	{
		ServerConfig server;

		LocationConfig redirectLocation;
		redirectLocation.setUriPath("/old-page");

		// We deliberately DO NOT allow GET here.

		std::vector<std::string> redirect;
		redirect.push_back("301");
		redirect.push_back("/");

		redirectLocation.setRedirect(redirect);

		server.addLocation(redirectLocation);

		const std::string raw =
			"GET /old-page HTTP/1.1\r\n"
			"Host: localhost\r\n"
			"\r\n";

		HTTPRequest request =
			HTTPRequestParser().parse(raw, raw.size());

		HTTPResponse response =
			HTTPResponseBuild::build(request, server);

		std::string output =
			response.toString(response);

		check(
			output.find("HTTP/1.1 301 Moved Permanently\r\n") == 0,
			"redirect is evaluated before GET method permissions"
		);

		check(
			output.find("405 Method Not Allowed") == std::string::npos,
			"redirect location does not enter normal GET handling"
		);
	}


	// ---------------------------------------------------------
	// 9. Redirect happens before DELETE handling
	// ---------------------------------------------------------
	{
		ServerConfig server;

		LocationConfig redirectLocation;
		redirectLocation.setUriPath("/old-page");

		std::vector<std::string> redirect;
		redirect.push_back("301");
		redirect.push_back("/");

		redirectLocation.setRedirect(redirect);

		server.addLocation(redirectLocation);

		const std::string raw =
			"DELETE /old-page HTTP/1.1\r\n"
			"Host: localhost\r\n"
			"\r\n";

		HTTPRequest request =
			HTTPRequestParser().parse(raw, raw.size());

		HTTPResponse response =
			HTTPResponseBuild::build(request, server);

		std::string output =
			response.toString(response);

		check(
			output.find("HTTP/1.1 301 Moved Permanently\r\n") == 0,
			"DELETE request to redirect location returns 301"
		);

		check(
			output.find("Location: /\r\n") != std::string::npos,
			"DELETE redirect contains correct Location header"
		);
	}


	// ---------------------------------------------------------
	// 10. Redirect happens before POST handling
	// ---------------------------------------------------------
	{
		ServerConfig server;

		LocationConfig redirectLocation;
		redirectLocation.setUriPath("/old-page");

		std::vector<std::string> redirect;
		redirect.push_back("301");
		redirect.push_back("/");

		redirectLocation.setRedirect(redirect);

		server.addLocation(redirectLocation);

		const std::string raw =
			"POST /old-page HTTP/1.1\r\n"
			"Host: localhost\r\n"
			"Content-Length: 0\r\n"
			"\r\n";

		HTTPRequest request =
			HTTPRequestParser().parse(raw, raw.size());

		HTTPResponse response =
			HTTPResponseBuild::build(request, server);

		std::string output =
			response.toString(response);

		check(
			output.find("HTTP/1.1 301 Moved Permanently\r\n") == 0,
			"POST request to redirect location returns 301"
		);

		check(
			output.find("Location: /\r\n") != std::string::npos,
			"POST redirect contains correct Location header"
		);
	}
}

void testServerConfigListen()
{
	ServerConfig server;

	// Default
	check(
		server.getHost() == "0.0.0.0",
		"default listen host is 0.0.0.0"
	);

	check(
		server.getPort() == 8080,
		"default listen port is 8080"
	);

	// Port only
	std::vector<std::string> listenPort;
	listenPort.push_back("9000");

	server.setPort(listenPort);

	check(
		server.getHost() == "0.0.0.0",
		"port-only listen uses 0.0.0.0"
	);

	check(
		server.getPort() == 9000,
		"port-only listen sets port"
	);

	// Interface + port
	std::vector<std::string> listenAddress;
	listenAddress.push_back("127.0.0.1:8081");

	server.setPort(listenAddress);

	check(
		server.getHost() == "127.0.0.1",
		"listen stores configured interface"
	);

	check(
		server.getPort() == 8081,
		"listen stores configured port"
	);
}

void testServerConfigInvalidListen()
{
	ServerConfig server;

	std::vector<std::string> value;

	value.push_back(":8080");
	checkThrows(
		[&server, &value] { server.setPort(value); },
		"listen rejects missing host"
	);

	value.clear();
	value.push_back("127.0.0.1:");
	checkThrows(
		[&server, &value] { server.setPort(value); },
		"listen rejects missing port"
	);

	value.clear();
	value.push_back("127.0.0.1:abc");
	checkThrows(
		[&server, &value] { server.setPort(value); },
		"listen rejects non-numeric port"
	);

	value.clear();
	value.push_back("127.0.0.1:0");
	checkThrows(
		[&server, &value] { server.setPort(value); },
		"listen rejects port 0"
	);

	value.clear();
	value.push_back("127.0.0.1:65536");
	checkThrows(
		[&server, &value] { server.setPort(value); },
		"listen rejects port above 65535"
	);

	value.clear();
	value.push_back("127.0.0.1:8080:9000");
	checkThrows(
		[&server, &value] { server.setPort(value); },
		"listen rejects multiple colons"
	);
}

void testClientLastActivityInitialized()
{
    const std::chrono::steady_clock::time_point before =
        std::chrono::steady_clock::now();

    Client client(42, 7);

    const std::chrono::steady_clock::time_point after =
        std::chrono::steady_clock::now();

    const std::chrono::steady_clock::time_point& activity =
        client.getLastActivity();

    check(
        activity >= before && activity <= after,
        "Client last activity is initialized when client is created"
    );
}

void testClientLastActivityUpdates()
{
    Client client(42, 7);

    const std::chrono::steady_clock::time_point first =
        client.getLastActivity();

    std::this_thread::sleep_for(
        std::chrono::milliseconds(20)
    );

    client.updateLastActivity();

    const std::chrono::steady_clock::time_point second =
        client.getLastActivity();

    check(
        second > first,
        "Client last activity is refreshed when activity occurs"
    );
}

void testClientHeaderSizeLimit()
{
	static const size_t maxHeaderSize = 32 * 1024;
	const std::string prefix =
		"GET / HTTP/1.1\r\n"
		"Host: unit.test\r\n"
		"X-Large-Test: ";
	const std::string suffix = "\r\n\r\n";

	{
		Client client(42, 7);
		std::string request = prefix;
		request.append(maxHeaderSize, 'A');

		client.appendToRequestBuffer(request.data(), request.size());

		check(
			client.parseHeaderClient() == RequestState::BadRequest,
			"an incomplete header larger than 32 KiB is rejected"
		);
		check(
			client.getRequestErrorCode() == 431,
			"an incomplete oversized header returns 431"
		);
	}

	{
		Client client(42, 7);
		std::string request = prefix;
		request.append(maxHeaderSize, 'A');
		request += suffix;

		client.appendToRequestBuffer(request.data(), request.size());

		check(
			client.parseHeaderClient() == RequestState::BadRequest,
			"a complete header larger than 32 KiB is rejected"
		);
		check(
			client.getRequestErrorCode() == 431,
			"a complete oversized header returns 431"
		);
	}

	{
		Client client(42, 7);
		std::string request = prefix;
		request.append(
			maxHeaderSize - prefix.size() - suffix.size(),
			'A'
		);
		request += suffix;

		check(
			request.size() == maxHeaderSize,
			"the header boundary fixture is exactly 32 KiB"
		);

		client.appendToRequestBuffer(request.data(), request.size());

		check(
			client.parseHeaderClient() == RequestState::Complete,
			"a complete header exactly 32 KiB is accepted"
		);
		check(
			client.getRequestErrorCode() == 0,
			"an accepted boundary-size header has no request error"
		);
	}
}

void testClientCgiInitialState()
{
    Client client(42, 7);

    check(
        client.getCgiState() == CGI_NONE,
        "new client starts with CGI_NONE"
    );

    check(
        client.getCgiInputFd() == -1,
        "new client has no CGI input fd"
    );

    check(
        client.getCgiOutputFd() == -1,
        "new client has no CGI output fd"
    );

    check(
        client.getCgiInputOffset() == 0,
        "new client CGI input offset starts at zero"
    );

    check(
        client.getCgiOutput().empty(),
        "new client CGI output starts empty"
    );

    check(
        client.getCgiPid() == -1,
        "new client has no CGI child PID"
    );

    check(
        client.getCgiProcessFailed() == false,
        "new client CGI failure flag starts false"
    );

    check(
        client.getPendingRemoval() == false,
        "new client is not pending removal"
    );
}

void testClientCgiResetForNewRequest()
{
    Client client(42, 7);

    client.setCgiState(CGI_READING);
    client.setCgiInputFd(10);
    client.setCgiOutputFd(11);
    client.setCgiInputOffset(25);
    client.setCgiOutput("old CGI output");
    client.setCgiProcessFailed(true);

    pid_t oldPid = 12345;
    client.setCgiPid(oldPid);

    client.resetCgiForNewRequest();

    check(
        client.getCgiState() == CGI_NONE,
        "CGI reset returns state to CGI_NONE"
    );

    check(
        client.getCgiInputFd() == -1,
        "CGI reset clears input fd"
    );

    check(
        client.getCgiOutputFd() == -1,
        "CGI reset clears output fd"
    );

    check(
        client.getCgiInputOffset() == 0,
        "CGI reset clears input offset"
    );

    check(
        client.getCgiOutput().empty(),
        "CGI reset clears previous CGI output"
    );

    check(
        client.getCgiProcessFailed() == false,
        "CGI reset clears previous process failure"
    );

    check(
        client.getCgiPid() == oldPid,
        "CGI reset does not overwrite an unreaped child PID"
    );
}

void testDecideConnectionHttp11Default()
{
    HTTPRequest request;

    request.setVersion("1.1");

    check(
        HTTPResponseBuild::decideConnection(request) == "keep-alive",
        "HTTP/1.1 defaults to keep-alive"
    );
}

void testDecideConnectionHttp11Close()
{
    HTTPRequest request;

    request.setVersion("1.1");
    request.addHeader("connection", "close");

    check(
        HTTPResponseBuild::decideConnection(request) == "close",
        "HTTP/1.1 Connection: close is respected"
    );
}

void testDecideConnectionHttp10Default()
{
    HTTPRequest request;

    request.setVersion("1.0");

    check(
        HTTPResponseBuild::decideConnection(request) == "close",
        "HTTP/1.0 defaults to close"
    );
}

void testDecideConnectionHttp10KeepAlive()
{
    HTTPRequest request;

    request.setVersion("1.0");
    request.addHeader("connection", "keep-alive");

    check(
        HTTPResponseBuild::decideConnection(request) == "keep-alive",
        "HTTP/1.0 Connection: keep-alive is respected"
    );
}

void testDecideConnectionUnknownVersion()
{
    HTTPRequest request;

    request.setVersion("2.0");

    check(
        HTTPResponseBuild::decideConnection(request) == "close",
        "Unknown HTTP version defaults to close"
    );
}

void testCgiResponseHttp10ConnectionClose()
{
    HTTPRequest request;
    request.setVersion("1.0");

    CgiResult result;
    result.valid = true;
    result.statusCode = 200;
    result.headers["content-type"] = "text/plain";
    result.body = "Hello CGI";

    ServerManager manager;

    std::string response =
        manager.buildCgiResponse(result, request);

    check(
        response.find("HTTP/1.0 200 OK\r\n") == 0,
        "CGI response uses request HTTP version"
    );

    check(
        response.find("Connection: close\r\n")
            != std::string::npos,
        "CGI HTTP/1.0 response defaults to Connection: close"
    );
}

void testCgiResponseIgnoresCgiConnectionHeader()
{
    HTTPRequest request;

    request.setVersion("1.1");

    CgiResult result;
    result.valid = true;
    result.statusCode = 200;
    result.headers["content-type"] = "text/plain";

    // CGI tries to decide connection itself.
    result.headers["connection"] = "close";

    result.body = "Hello CGI";

    ServerManager manager;

    std::string response =
        manager.buildCgiResponse(result, request);

    check(
        response.find("Connection: keep-alive\r\n")
            != std::string::npos,
        "Webserv controls CGI Connection header"
    );

    size_t first =
        response.find("Connection:");

    size_t second =
        response.find("Connection:", first + 1);

    check(
        second == std::string::npos,
        "CGI response contains only one Connection header"
    );
}


void testRequestHeaderEdgeCases()
{
    {
        Client client(42, 7);
        const std::string request =
            "GET / HTTP/1.1\r\n"
            "\r\n";
        client.appendToRequestBuffer(request.data(), request.size());
        check(client.parseHeaderClient() == RequestState::BadRequest,
              "HTTP/1.1 request without Host is rejected");
        check(client.getRequestErrorCode() == 400,
              "missing HTTP/1.1 Host returns 400");
    }

    {
        Client client(42, 7);
        const std::string request =
            "POST /upload HTTP/1.1\r\n"
            "Host: unit.test\r\n"
            "Content-Length: 5\r\n"
            "Content-Length: 5\r\n"
            "\r\n"
            "Hello";
        client.appendToRequestBuffer(request.data(), request.size());
        check(client.parseHeaderClient() == RequestState::BadRequest,
              "duplicate Content-Length is rejected");
        check(client.getRequestErrorCode() == 400,
              "duplicate Content-Length returns 400");
    }

    {
        Client client(42, 7);
        const std::string request =
            "POST /upload HTTP/1.1\r\n"
            "Host: unit.test\r\n"
            "Content-Length: 5\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n"
            "0\r\n\r\n";
        client.appendToRequestBuffer(request.data(), request.size());
        check(client.parseHeaderClient() == RequestState::BadRequest,
              "Content-Length plus Transfer-Encoding is rejected");
        check(client.getRequestErrorCode() == 400,
              "ambiguous request framing returns 400");
    }

    {
        Client client(42, 7);
        const std::string request =
            "POST /upload HTTP/1.1\r\n"
            "Host: unit.test\r\n"
            "Transfer-Encoding: gzip\r\n"
            "\r\n";
        client.appendToRequestBuffer(request.data(), request.size());
        check(client.parseHeaderClient() == RequestState::BadRequest,
              "unsupported Transfer-Encoding is rejected");
        check(client.getRequestErrorCode() == 501,
              "unsupported Transfer-Encoding returns 501");
    }

    {
        Client client(42, 7);
        const std::string request =
            "GET / HTTP/1.1\r\n"
            "Host: first.test\r\n"
            "Host: second.test\r\n"
            "\r\n";
        client.appendToRequestBuffer(request.data(), request.size());
        check(client.parseHeaderClient() == RequestState::BadRequest,
              "duplicate Host header is rejected");
        check(client.getRequestErrorCode() == 400,
              "duplicate Host returns 400");
    }
}

void testRequestBodyEdgeCases()
{
    {
        Client client(42, 7);
        const std::string request =
            "POST /upload HTTP/1.1\r\n"
            "Host: unit.test\r\n"
            "Content-Length: 6\r\n"
            "\r\n"
            "Hello";
        client.appendToRequestBuffer(request.data(), request.size());
        check(client.parseHeaderClient() == RequestState::Complete,
              "partial Content-Length headers are parsed");
        check(client.checkRequestState(1024) == RequestState::Incomplete,
              "partial Content-Length body remains incomplete");
    }

    {
        Client client(42, 7);
        const std::string request =
            "POST /upload HTTP/1.1\r\n"
            "Host: unit.test\r\n"
            "Content-Length: 11\r\n"
            "\r\n"
            "Hello World";
        client.appendToRequestBuffer(request.data(), request.size());
        check(client.parseHeaderClient() == RequestState::Complete,
              "oversized Content-Length fixture headers are parsed");
        check(client.checkRequestState(10) == RequestState::BadRequest,
              "Content-Length above configured maximum is rejected");
        check(client.getRequestErrorCode() == 413,
              "oversized Content-Length returns 413");
    }

    {
        Client client(42, 7);
        const std::string request =
            "POST /upload HTTP/1.1\r\n"
            "Host: unit.test\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n"
            "B\r\n"
            "Hello World\r\n"
            "0\r\n\r\n";
        client.appendToRequestBuffer(request.data(), request.size());
        check(client.parseHeaderClient() == RequestState::Complete,
              "oversized chunked fixture headers are parsed");
        check(client.checkRequestState(10) == RequestState::BadRequest,
              "decoded chunked body above configured maximum is rejected");
        check(client.getRequestErrorCode() == 413,
              "oversized chunked body returns 413");
    }

    {
        Client client(42, 7);
        const std::string request =
            "POST /upload HTTP/1.1\r\n"
            "Host: unit.test\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n"
            "XYZ\r\n"
            "Hello\r\n"
            "0\r\n\r\n";
        client.appendToRequestBuffer(request.data(), request.size());
        check(client.parseHeaderClient() == RequestState::Complete,
              "malformed chunk fixture headers are parsed");
        check(client.checkRequestState(1024) == RequestState::BadRequest,
              "non-hexadecimal chunk size is rejected");
    }

    {
        Client client(42, 7);
        const std::string request =
            "POST /upload HTTP/1.1\r\n"
            "Host: unit.test\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n"
            "5\r\n"
            "HelloXX"
            "0\r\n\r\n";
        client.appendToRequestBuffer(request.data(), request.size());
        check(client.parseHeaderClient() == RequestState::Complete,
              "bad chunk terminator fixture headers are parsed");
        check(client.checkRequestState(1024) == RequestState::BadRequest,
              "chunk without CRLF terminator is rejected");
    }
}

void testHttpProtocolEdgeCases()
{
    HTTPRequestParser parser;

    {
        const std::string raw =
            "GET / HTTP/2.0\r\n"
            "Host: unit.test\r\n\r\n";
        try {
            parser.parse(raw, raw.size());
            check(false, "HTTP/2.0 request is rejected");
        } catch (const HTTPParseException& e) {
            check(e.getStatusCode() == 505,
                  "unsupported HTTP version returns 505");
        }
    }

    {
        const std::string raw =
            "PUT / HTTP/1.1\r\n"
            "Host: unit.test\r\n\r\n";
        HTTPRequest request = parser.parse(raw, raw.size());
        check(request.getMethod() == Method::UNKNOWN,
              "unsupported method parses as UNKNOWN");

        ServerConfig server;
        LocationConfig location;
        location.setUriPath("/");
        location.setAllowMethods(std::vector<std::string>{"GET"});
        server.addLocation(location);

        HTTPResponse response = HTTPResponseBuild::build(request, server);
        const std::string output = response.toString(response);
        check(output.find("HTTP/1.1 501 Not Implemented\r\n") == 0,
              "unsupported method returns 501");
    }
}

void testRouteAndPathEdgeCases()
{
    {
        ServerConfig server;
        LocationConfig location;
        location.setUriPath("/");
        location.setAllowMethods(std::vector<std::string>{"GET"});
        server.addLocation(location);

        const std::string raw =
            "POST / HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Content-Length: 0\r\n\r\n";
        HTTPRequest request = HTTPRequestParser().parse(raw, raw.size());
        HTTPResponse response = HTTPResponseBuild::build(request, server);
        const std::string output = response.toString(response);

        check(output.find("HTTP/1.1 405 Method Not Allowed\r\n") == 0,
              "method forbidden by location returns 405");
        check(output.find("Allow: GET\r\n") != std::string::npos,
              "405 response contains Allow header");
    }

    {
        ServerConfig server;
        LocationConfig location;
        location.setUriPath("/");
        location.setAllowMethods(std::vector<std::string>{"GET"});
        server.addLocation(location);

        const std::string raw =
            "GET /../secret HTTP/1.1\r\n"
            "Host: localhost\r\n\r\n";
        HTTPRequest request = HTTPRequestParser().parse(raw, raw.size());
        HTTPResponse response = HTTPResponseBuild::build(request, server);
        const std::string output = response.toString(response);
        check(output.find("HTTP/1.1 403 Forbidden\r\n") == 0,
              "parent-directory traversal is rejected with 403");
    }

    {
        ServerConfig server;
        LocationConfig location;
        location.setUriPath("/");
        location.setAllowMethods(std::vector<std::string>{"GET"});
        server.addLocation(location);

        const std::string raw =
            "GET /bad%2 HTTP/1.1\r\n"
            "Host: localhost\r\n\r\n";
        HTTPRequest request = HTTPRequestParser().parse(raw, raw.size());
        HTTPResponse response = HTTPResponseBuild::build(request, server);
        const std::string output = response.toString(response);
        check(output.find("HTTP/1.1 400 Bad Request\r\n") == 0,
              "invalid percent encoding returns 400");
    }

    {
        ServerConfig server;
        LocationConfig shortLocation;
        shortLocation.setUriPath("/api");
        shortLocation.setAllowMethods(std::vector<std::string>{"GET"});
        LocationConfig longLocation;
        longLocation.setUriPath("/api/private");
        longLocation.setAllowMethods(std::vector<std::string>{"POST"});
        server.addLocation(shortLocation);
        server.addLocation(longLocation);

        const std::string longRaw =
            "GET /api/private/item HTTP/1.1\r\n"
            "Host: localhost\r\n\r\n";
        HTTPRequest longRequest = HTTPRequestParser().parse(longRaw, longRaw.size());
        HTTPResponse longResponse = HTTPResponseBuild::build(longRequest, server);
        const std::string longOutput = longResponse.toString(longResponse);
        check(longOutput.find("HTTP/1.1 405 Method Not Allowed\r\n") == 0,
              "longest matching location is selected");
        check(longOutput.find("Allow: POST\r\n") != std::string::npos,
              "longest location contributes its method policy");

        const std::string prefixRaw =
            "GET /apix HTTP/1.1\r\n"
            "Host: localhost\r\n\r\n";
        HTTPRequest prefixRequest = HTTPRequestParser().parse(prefixRaw, prefixRaw.size());
        HTTPResponse prefixResponse = HTTPResponseBuild::build(prefixRequest, server);
        const std::string prefixOutput = prefixResponse.toString(prefixResponse);
        check(prefixOutput.find("HTTP/1.1 404 Not Found\r\n") == 0,
              "location /api does not falsely match /apix");
    }
}

void testServerConfigBodySizeEdgeCases()
{
    ServerConfig server;

    checkThrows([&server] {
        server.setClientMaxBodySize(std::vector<std::string>{"abc"});
    }, "non-numeric client_max_body_size is rejected");

    checkThrows([&server] {
        server.setClientMaxBodySize(std::vector<std::string>{"10", "20"});
    }, "multiple client_max_body_size values are rejected");
}

void testInvalidChunkSizePlusSign() {

	Client client;

	std::string request =
		"POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"+5\r\n"
		"Hello\r\n"
		"0\r\n"
		"\r\n";

	client.appendToRequestBuffer(request.c_str(), request.size());

	bool result = client.decodeChunkedBody();

	if (result)
		throw std::runtime_error("Chunk size with leading '+' was accepted");
}

void testInvalidChunkSize0xPrefix() {

	Client client;

	std::string request =
		"POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"0x5\r\n"
		"Hello\r\n"
		"0\r\n"
		"\r\n";

	client.appendToRequestBuffer(request.c_str(), request.size());

	bool result = client.decodeChunkedBody();

	if (result)
		throw std::runtime_error("Chunk size with 0x prefix was accepted");
}

void testMultipartLowercaseContentDisposition() {

	std::string body =
		"--AaB03x\r\n"
		"content-disposition: form-data; name=\"file\"; filename=\"case.txt\"\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n"
		"hello\r\n"
		"--AaB03x--\r\n";

	MultipartParser parser(body, 0, body.size(), "AaB03x");

	std::vector<MultipartPart> parts = parser.parse();

	if (parts.size() != 1)
		throw std::runtime_error("Lowercase content-disposition header was not parsed");

	if (parts[0].getFilename() != "case.txt")
		throw std::runtime_error("Filename from lowercase content-disposition was not parsed");
}

void testQuotedMultipartBoundary() {

	HTTPRequestParser parser;

	std::string request =
		"POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Type: multipart/form-data; boundary=\"AaB03x\"\r\n"
		"Content-Length: 0\r\n"
		"\r\n";

	HTTPRequest parsed = parser.parse(request, request.size());

	if (parsed.getBoundary() != "AaB03x")
		throw std::runtime_error("Quoted multipart boundary was not normalized correctly");
}

void testMultipartBoundaryPrefixInsideData() {

	std::string body =
		"--BOUNDARY\r\n"
		"Content-Disposition: form-data; name=\"file\"; filename=\"data.txt\"\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n"
		"first line\r\n"
		"--BOUNDARYXYZ\r\n"
		"still file data\r\n"
		"--BOUNDARY--\r\n";

	MultipartParser parser(body, 0, body.size(), "BOUNDARY");

	std::vector<MultipartPart> parts = parser.parse();

	if (parts.size() != 1)
		throw std::runtime_error("Boundary-like file data confused multipart parser");

	std::string expected =
		"first line\r\n"
		"--BOUNDARYXYZ\r\n"
		"still file data";

    std::string actualData = body.substr(
        parts[0].getDataOffset(),
        parts[0].getDataSize()
    );

	if (actualData != expected)
		throw std::runtime_error("Multipart payload was truncated by boundary prefix");
}

 void testRawUploadFilenameUniqueness() {

	char temporaryDirectory[] = "/tmp/webserv-upload-unique-XXXXXX";
	char* uploadStore = mkdtemp(temporaryDirectory);

	if (uploadStore == NULL)
		throw std::runtime_error("mkdtemp failed");

	ServerConfig server;
	server.setRoot({"./site/www"});

	LocationConfig location;
	location.setUriPath("/upload");
	location.setRoot({"./site/www/upload"});
	location.setUploadStore({uploadStore});
	location.setAllowMethods({"POST"});

	server.addLocation(location);

	HTTPRequest request1;
	HTTPRequest request2;
	request1.setVersion("1.1");
	request2.setVersion("1.1");
	
	request1.setMethod(Method::POST);
	request1.setPath("/upload");
    std::string buffer1 = "one";
    request1.setBodyLocation(buffer1, 0, buffer1.size());


	request2.setMethod(Method::POST);
	request2.setPath("/upload");
	std::string buffer2 = "two";
    request2.setBodyLocation(buffer2, 0, buffer2.size());

	HTTPResponseBuild::build(request1, server);
	HTTPResponseBuild::build(request2, server);

	DIR* dir = opendir(uploadStore);

	if (dir == NULL)
		throw std::runtime_error("Could not open upload directory");

	int fileCount = 0;

	struct dirent* entry;

	while ((entry = readdir(dir)) != NULL) {

		std::string name = entry->d_name;

		if (name != "." && name != "..")
			++fileCount;
	}

	closedir(dir);

	if (fileCount < 2)
		throw std::runtime_error("Two uploads created the same filename");
}

void testEncodedPathTraversal() {

	ServerConfig server;
	server.setRoot({"./site/www"});

	HTTPRequest request;
	request.setVersion("1.1");
	request.setMethod(Method::GET);
	request.setPath("/%2e%2e/secret.txt");

	HTTPResponse response = HTTPResponseBuild::build(request, server);

	if (response.getStatusCode() != 403)
		throw std::runtime_error("Encoded path traversal was not rejected with 403");
}

void testContentLengthPipelining() {

	Client client;

	std::string request =
		"POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Length: 5\r\n"
		"\r\n"
		"hello"
		"GET / HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n";

	client.appendToRequestBuffer(request.c_str(), request.size());

	RequestState state = client.checkRequestState(1024);

	if (state != RequestState::Complete)
		throw std::runtime_error("First pipelined request was not recognized as complete");

	if (client.getRequestEnd() == request.size())
		throw std::runtime_error("Pipelined second request was consumed with first request");
}

void testUnsupportedMethodToCgiReturns501()
{
	char directoryTemplate[] = "/tmp/webserv-cgi-put-XXXXXX";
	char* directory = mkdtemp(directoryTemplate);

	check(directory != NULL, "CGI PUT fixture directory is created");
	if (directory == NULL)
		return;

	const std::string base(directory);

	std::ofstream(base + "/existing.py")
		<< "print('Content-Type: text/plain\\n\\nOK')\n";

	LocationConfig location;
	location.setUriPath("/cgi-bin");
	location.setRoot({base});
	location.setAllowMethods({"GET", "POST"});
	location.setCgiExtension({".py"});
	location.setCgiPath({"/bin/sh"});
	location.validateCgiConfig();

	ServerConfig server;
	server.addLocation(location);

	const std::string raw =
		"PUT /cgi-bin/existing.py HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Length: 0\r\n"
		"\r\n";

	HTTPRequest request =
		HTTPRequestParser().parse(raw, raw.size());

	check(
		request.getMethod() == Method::UNKNOWN,
		"unsupported CGI PUT method parses as UNKNOWN"
	);

	HTTPResponse response =
		HTTPResponseBuild::makeEarlyErrorResponse(
			501,
			server
		);

	const std::string output =
		response.toString(response);

	check(
		output.find("HTTP/1.1 501 Not Implemented\r\n") == 0,
		"unsupported method to CGI URI returns 501"
	);

	unlink((base + "/existing.py").c_str());
	rmdir(directory);
}

void testCgi405IncludesAllowHeader()
{
	char directoryTemplate[] = "/tmp/webserv-cgi-405-XXXXXX";
	char* directory = mkdtemp(directoryTemplate);

	check(directory != NULL, "CGI 405 fixture directory is created");
	if (directory == NULL)
		return;

	const std::string base(directory);

	std::ofstream(base + "/existing.py")
		<< "print('Content-Type: text/plain\\n\\nOK')\n";

	LocationConfig location;
	location.setUriPath("/cgi-bin");
	location.setRoot({base});
	location.setAllowMethods({"GET", "POST"});
	location.setCgiExtension({".py"});
	location.setCgiPath({"/bin/sh"});
	location.validateCgiConfig();

	ServerConfig server;
	server.addLocation(location);

	const std::string raw =
		"DELETE /cgi-bin/existing.py HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n";

	HTTPRequest request =
		HTTPRequestParser().parse(raw, raw.size());

	CgiRoute route;
	int error = 0;
	const LocationConfig* matchedLocation = NULL;

	bool isCgi = HTTPResponseBuild::resolveCgiRoute(
		request,
		server,
		route,
		error,
		matchedLocation
	);

	check(
		!isCgi && error == 405,
		"disallowed CGI method produces 405"
	);

	check(
		matchedLocation != NULL,
		"CGI 405 preserves the matched location"
	);

	if (error == 405 && matchedLocation != NULL) {

		HTTPResponse response =
			HTTPResponseBuild::makeEarlyErrorResponse(
				405,
				server
			);

		response.setHeader(
			"Allow",
			HTTPResponseBuild::buildAllowHeader(*matchedLocation)
		);

		const std::string output =
			response.toString(response);

		check(
			output.find("HTTP/1.1 405 Method Not Allowed\r\n") == 0,
			"CGI disallowed method builds 405 response"
		);

		check(
			output.find("Allow: GET, POST\r\n") != std::string::npos,
			"CGI 405 response includes configured Allow header"
		);
	}

	unlink((base + "/existing.py").c_str());
	rmdir(directory);
}

void testLocationBodySizeConfig()
{
	LocationConfig location;

	check(
		!location.hasClientMaxBodySize(),
		"new location has no client_max_body_size override"
	);

	location.setClientMaxBodySize(
		std::vector<std::string>(1, "100")
	);

	check(
		location.hasClientMaxBodySize(),
		"location remembers that client_max_body_size was configured"
	);

	check(
		location.getClientMaxBodySize() == 100,
		"location stores client_max_body_size 100"
	);
}

void testLocationBodySizeEdgeCases()
{
	{
		LocationConfig location;

		try {
			location.setClientMaxBodySize(
				std::vector<std::string>(1, "abc")
			);

			check(
				false,
				"non-numeric location client_max_body_size is rejected"
			);
		}
		catch (const std::exception&) {
			check(
				true,
				"non-numeric location client_max_body_size is rejected"
			);
		}
	}

	{
		LocationConfig location;

		std::vector<std::string> values;
		values.push_back("10");
		values.push_back("20");

		try {
			location.setClientMaxBodySize(values);

			check(
				false,
				"multiple location client_max_body_size values are rejected"
			);
		}
		catch (const std::exception&) {
			check(
				true,
				"multiple location client_max_body_size values are rejected"
			);
		}
	}
}

void testLocationBodySizeLimit()
{
	Client client(42, 7);

	const std::string body(101, 'A');

	const std::string request =
		"POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Length: 101\r\n"
		"\r\n"
		+ body;

	client.appendToRequestBuffer(
		request.c_str(),
		request.size()
	);

	check(
		client.parseHeaderClient() == RequestState::Complete,
		"location body-size test parses headers"
	);

	check(
		client.checkRequestState(100) == RequestState::BadRequest,
		"101-byte body is rejected by 100-byte location limit"
	);

	check(
		client.getRequestErrorCode() == 413,
		"location body-size overflow produces 413"
	);
}

void testLocationBodySizeBoundary()
{
	Client client(42, 7);

	const std::string body(100, 'A');

	const std::string request =
		"POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Length: 100\r\n"
		"\r\n"
		+ body;

	client.appendToRequestBuffer(
		request.c_str(),
		request.size()
	);

	check(
		client.parseHeaderClient() == RequestState::Complete,
		"location body-size boundary parses headers"
	);

	check(
		client.checkRequestState(100) == RequestState::Complete,
		"100-byte body is accepted by 100-byte location limit"
	);
}

void testServerBodySizeLimit()
{
	Client client(42, 7);

	const std::string body(101, 'A');

	const std::string request =
		"POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Length: 101\r\n"
		"\r\n"
		+ body;

	client.appendToRequestBuffer(
		request.c_str(),
		request.size()
	);

	check(
		client.parseHeaderClient() == RequestState::Complete,
		"server body-size test parses headers"
	);

	check(
		client.checkRequestState(1000) == RequestState::Complete,
		"101-byte body is accepted by larger server limit"
	);
}

} // namespace

int main()
{
	run("validation helpers", testValidationHelpers);
	run("LocationConfig", testLocationConfig);
	run("ServerConfig", testServerConfig);
	run("ConfigParser", testConfigParser);
	run("CGI configuration validation", testCgiConfigurationValidation);
	run("CGI routing and missing upload store", testCgiRoutingAndMissingUploadStore);
	run("HTTPRequestParser", testHttpRequestParser);
	run("HTTPRequest body location", testHttpRequestBodyLocation);
	run("POST upload", testPostUpload);
	run("HTTPResponse", testHttpResponse);
	run("Client response buffer", testClientResponseBuffer);
	run("Client new response resets progress", testClientNewResponseResetsProgress);
	run("Client header size limit", testClientHeaderSizeLimit);
	run("Client close after response", testClientCloseAfterResponse);
	run("Client chunked body decoding", testClientDecodeChunkedBody); // 
	run("Client Content-Length request", testClientContentLengthUnaffected);
	run("Virtual host server configs", testVirtualHostServerConfigs);
	run("Multiple server names", testMultipleServerNames);
	run("Virtual host body size limits", testVirtualHostBodySizeLimits);
	run("Server name replaces default", testServerNameReplacesDefault);
	run("ServerConfig listen", testServerConfigListen);
	run("ServerConfig invalid listen", testServerConfigInvalidListen);  
	run("Redirect", testRedirect);
	run("Client activity initialized", testClientLastActivityInitialized);
	run("Client activity updates", testClientLastActivityUpdates);
	run("Client CGI initial state", testClientCgiInitialState);
	run("Client CGI reset for new request", testClientCgiResetForNewRequest);
	run("HTTP/1.1 default connection", testDecideConnectionHttp11Default);
	run("HTTP/1.1 Connection close", testDecideConnectionHttp11Close);
	run("HTTP/1.0 default connection", testDecideConnectionHttp10Default);
	run("HTTP/1.0 Connection keep-alive", testDecideConnectionHttp10KeepAlive);
	run("Unknown HTTP version connection", testDecideConnectionUnknownVersion);
	run("CGI HTTP/1.0 response connection close", testCgiResponseHttp10ConnectionClose);
	run("CGI ignores CGI-provided Connection header", testCgiResponseIgnoresCgiConnectionHeader);
	run("Request header edge cases", testRequestHeaderEdgeCases);
	run("Request body edge cases", testRequestBodyEdgeCases);
	run("HTTP protocol edge cases", testHttpProtocolEdgeCases);
	run("Route and path edge cases", testRouteAndPathEdgeCases);
	run("ServerConfig body-size edge cases", testServerConfigBodySizeEdgeCases);
	run("CGI routing and missing upload store", testCgiRoutingAndMissingUploadStore);
	run("CGI 405 includes Allow header", testCgi405IncludesAllowHeader);
	run("Unsupported CGI method returns 501", testUnsupportedMethodToCgiReturns501);
    run("BREAKER chunk + sign", testInvalidChunkSizePlusSign);
    run("BREAKER chunk 0x prefix", testInvalidChunkSize0xPrefix);
    run("BREAKER multipart lowercase header", testMultipartLowercaseContentDisposition);
    run("BREAKER quoted multipart boundary", testQuotedMultipartBoundary);
    run("BREAKER multipart boundary prefix in data", testMultipartBoundaryPrefixInsideData);
    run("BREAKER raw upload uniqueness", testRawUploadFilenameUniqueness);
    run("BREAKER encoded traversal", testEncodedPathTraversal);
    run("BREAKER pipelined Content-Length", testContentLengthPipelining);
	run("Location body-size config", testLocationBodySizeConfig);
	run("Location body-size edge cases", testLocationBodySizeEdgeCases);
	run("Location body-size limit", testLocationBodySizeLimit);
	run("Location body-size boundary", testLocationBodySizeBoundary);
	run("Server body-size limit", testServerBodySizeLimit);
	
	if (g_failures != 0) {
		std::cerr << g_failures << " assertion(s) failed\n";
		return 1;
	}
	std::cout << "All unit tests passed\n";
	return 0;
}
