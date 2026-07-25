#include "ConfigParser.hpp"
#include "ClientData.hpp"
#include "HTTPRequestParser.hpp"
#include "HTTPResponse.hpp"
#include "LocationConfig.hpp"
#include "ServerConfig.hpp"
#include "client.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

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
    check(location.getIndex().size() == 2, "location indexes are stored");
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
    server.setPort(std::vector<std::string>{"9090"});
    server.setServerName(std::vector<std::string>{"example.test", "api-example"});
    server.setRoot(std::vector<std::string>{"./public"});
    server.setIndex(std::vector<std::string>{"home.html"});
    server.setClientMaxBodySize(std::vector<std::string>{"2048"});
    server.setErrorPage(std::vector<std::string>{"404", "500", "/errors/error.html"});

    check(server.getPort() == 9090, "configured port overrides the default");
    check(server.getServerName().size() == 3, "server names are appended to defaults");
    check(server.getRoot().at(0) == "./public", "server root is replaced");
    check(server.getIndex().back() == "home.html", "server index is appended");
    check(server.getClientMaxBodySize().back() == 2048U, "body-size limit is parsed");
    check(server.hasErrorPage(404) && server.getOneErrorPage(500) == "/errors/error.html",
          "error pages are mapped to every specified status");

    checkThrows([&server] { server.setPort(std::vector<std::string>{"70000"}); },
                "ports above 65535 are rejected");
    checkThrows([&server] { server.setServerName(std::vector<std::string>{"bad name"}); },
                "server names with whitespace are rejected");
    checkThrows([&server] { server.setClientMaxBodySize(std::vector<std::string>{"12KB"}); },
                "non-numeric body-size limits are rejected");
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

void testHttpRequestParser()
{
    HTTPRequestParser parser;
    HTTPRequest request = parser.parse(
        "GET /search?q=webserv HTTP/1.1\r\nHost: example.test\r\nConnection: close\r\n\r\n");

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

void testClientRequestBuffer()
{
    Client client(42, 7);
    const std::string firstPart = "POST /upload HTTP/1.1\r\nContent-Length: 5\r\n\r\nhe";
    const std::string secondPart = "llo";

    client.appendToRequestBuffer(firstPart.c_str(), firstPart.size());
    check(!client.hasCompleteRequest(), "a request waits until Content-Length bytes arrive");
    client.appendToRequestBuffer(secondPart.c_str(), secondPart.size());
    check(client.hasCompleteRequest(), "a complete request is detected across recv chunks");
    check(client.getFullBodyRequest() == "hello", "the complete request body is retained");
    check(client.getPartBodyRequest(1, 99) == "ello", "body slices are bounded to available data");

    client.clearRequestBuffer();
    const std::string headerOnly = "GET / HTTP/1.1\r\nHost: unit.test\r\n\r\n";
    client.appendToRequestBuffer(headerOnly.c_str(), headerOnly.size());
    check(client.hasCompleteRequest(), "header-only requests are complete after CRLF CRLF");
}

void run(const std::string& name, void (*test)())
{
    const int failuresBefore = g_failures;
    test();
    std::cout << (failuresBefore == g_failures ? "PASS" : "FAIL") << " " << name << '\n';
}

} // namespace

int main()
{
    run("validation helpers", testValidationHelpers);
    run("LocationConfig", testLocationConfig);
    run("ServerConfig", testServerConfig);
    run("ConfigParser", testConfigParser);
    run("HTTPRequestParser", testHttpRequestParser);
    run("Client request buffer", testClientRequestBuffer);
    run("HTTPResponse", testHttpResponse);

    if (g_failures != 0) {
        std::cerr << g_failures << " assertion(s) failed\n";
        return 1;
    }
    std::cout << "All unit tests passed\n";
    return 0;
}
