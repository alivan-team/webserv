#include "ConfigParser.hpp"
#include "ClientData.hpp"
#include "HTTPRequestParser.hpp"
#include "HTTPResponse.hpp"
#include "HTTPResponseBuild.hpp"
#include "LocationConfig.hpp"
#include "ServerConfig.hpp"
#include "HelperFunctions.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <dirent.h>
#include <fstream>
#include <iterator>
#include <sys/stat.h>
#include <unistd.h>

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

// void testClientConsumeRequestPreservesNextRequest()
// {
//     Client client(42, 7);

//     const std::string firstRequest =
//         "GET /one HTTP/1.1\r\n"
//         "Host: unit.test\r\n"
//         "\r\n";

//     const std::string secondRequest =
//         "GET /two HTTP/1.1\r\n"
//         "Host: unit.test\r\n"
//         "\r\n";

//     const std::string combined =
//         firstRequest + secondRequest;

//     client.appendToRequestBuffer(
//         combined.c_str(),
//         combined.size()
//     );

//     check(
//         client.checkRequestState() == RequestState::Complete,
//         "first request is detected when two requests are buffered"
//     );

//     check(
//         client.getRequestEnd() == firstRequest.size(),
//         "request end points to the end of only the first request"
//     );

//     client.consumeRequest();

//     check(
//         client.getRequestBuffer() == secondRequest,
//         "consumeRequest preserves an already-buffered next request"
//     );

//     check(
//         client.checkRequestState() == RequestState::Complete,
//         "preserved second request can immediately be processed"
//     );
// }

// void testClientConsumeRequestPreservesPartialNextRequest()
// {
//     Client client(42, 7);

//     const std::string firstRequest =
//         "GET /one HTTP/1.1\r\n"
//         "Host: unit.test\r\n"
//         "\r\n";

//     const std::string partialSecondRequest =
//         "GET /two HTTP/1.1\r\n"
//         "Host:";

//     const std::string combined =
//         firstRequest + partialSecondRequest;

//     client.appendToRequestBuffer(
//         combined.c_str(),
//         combined.size()
//     );

//     check(
//         client.checkRequestState() == RequestState::Complete,
//         "first request completes even when part of the next request is buffered"
//     );

//     client.consumeRequest();

//     check(
//         client.getRequestBuffer() == partialSecondRequest,
//         "consumeRequest preserves partial bytes belonging to the next request"
//     );

//     check(
//         client.checkRequestState() == RequestState::Incomplete,
//         "partial preserved request correctly waits for more POLLIN data"
//     );
// }

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

// void testClientRequestBuffer()
// {
//     Client client(42, 7);
//     const std::string firstPart = "POST /upload HTTP/1.1\r\nContent-Length: 5\r\n\r\nhe";
//     const std::string secondPart = "llo";

//     client.appendToRequestBuffer(firstPart.c_str(), firstPart.size());
//     check(!client.hasCompleteRequest(), "a request waits until Content-Length bytes arrive");
//     client.appendToRequestBuffer(secondPart.c_str(), secondPart.size());
//     check(client.hasCompleteRequest(), "a complete request is detected across recv chunks");
//     check(client.getFullBodyRequest() == "hello", "the complete request body is retained");
//     check(client.getPartBodyRequest(1, 99) == "ello", "body slices are bounded to available data");

//     client.clearRequestBuffer();
//     const std::string headerOnly = "GET / HTTP/1.1\r\nHost: unit.test\r\n\r\n";
//     client.appendToRequestBuffer(headerOnly.c_str(), headerOnly.size());
//     check(client.hasCompleteRequest(), "header-only requests are complete after CRLF CRLF");
// }

void run(const std::string& name, void (*test)())
{
    const int failuresBefore = g_failures;
    test();
    std::cout << (failuresBefore == g_failures ? "PASS" : "FAIL") << " " << name << '\n';
}

// void testClientChunkedRequestBuffer()
// {
//     {
//         Client client(42, 7);

//         const std::string request =
//             "POST /upload HTTP/1.1\r\n"
//             "Host: unit.test\r\n"
//             "Transfer-Encoding: chunked\r\n"
//             "\r\n"
//             "5\r\n"
//             "Hello\r\n"
//             "0\r\n"
//             "\r\n";

//         client.appendToRequestBuffer(request.c_str(), request.size());

//         check(
//             client.checkRequestState() == RequestState::Complete,
//             "a valid single-chunk request is complete"
//         );
//     }

//     {
//         Client client(42, 7);

//         const std::string request =
//             "POST /upload HTTP/1.1\r\n"
//             "Host: unit.test\r\n"
//             "Transfer-Encoding: chunked\r\n"
//             "\r\n"
//             "5\r\n"
//             "Hello\r\n"
//             "6\r\n"
//             " World\r\n"
//             "0\r\n"
//             "\r\n";

//         client.appendToRequestBuffer(request.c_str(), request.size());

//         check(
//             client.checkRequestState() == RequestState::Complete,
//             "a valid multi-chunk request is complete"
//         );
//     }

//     {
//         Client client(42, 7);

//         const std::string request =
//             "POST /upload HTTP/1.1\r\n"
//             "Host: unit.test\r\n"
//             "Transfer-Encoding: chunked\r\n"
//             "\r\n"
//             "A\r\n"
//             "0123456789\r\n"
//             "0\r\n"
//             "\r\n";

//         client.appendToRequestBuffer(request.c_str(), request.size());

//         check(
//             client.checkRequestState() == RequestState::Complete,
//             "hexadecimal chunk sizes are accepted"
//         );
//     }

//     {
//         Client client(42, 7);

//         const std::string request =
//             "POST /upload HTTP/1.1\r\n"
//             "Host: unit.test\r\n"
//             "Transfer-Encoding: chunked\r\n"
//             "\r\n"
//             "0\r\n"
//             "\r\n";

//         client.appendToRequestBuffer(request.c_str(), request.size());

//         check(
//             client.checkRequestState() == RequestState::Complete,
//             "an empty chunked body is complete"
//         );
//     }

//     {
//         Client client(42, 7);

//         const std::string request =
//             "POST /upload HTTP/1.1\r\n"
//             "Host: unit.test\r\n"
//             "Transfer-Encoding: chunked\r\n"
//             "\r\n"
//             "5;name=value\r\n"
//             "Hello\r\n"
//             "0\r\n"
//             "\r\n";

//         client.appendToRequestBuffer(request.c_str(), request.size());

//         check(
//             client.checkRequestState() == RequestState::Complete,
//             "chunk extensions are accepted"
//         );
//     }

//     {
//         Client client(42, 7);

//         const std::string request =
//             "POST /upload HTTP/1.1\r\n"
//             "Host: unit.test\r\n"
//             "Transfer-Encoding: chunked\r\n"
//             "\r\n"
//             "G\r\n"
//             "Hello\r\n"
//             "0\r\n"
//             "\r\n";

//         client.appendToRequestBuffer(request.c_str(), request.size());

//         check(
//             client.checkRequestState() == RequestState::BadRequest,
//             "invalid hexadecimal chunk sizes are rejected"
//         );
//     }

//     {
//         Client client(42, 7);

//         const std::string request =
//             "POST /upload HTTP/1.1\r\n"
//             "Host: unit.test\r\n"
//             "Transfer-Encoding: chunked\r\n"
//             "\r\n"
//             "5\r\n"
//             "HelloXX";

//         client.appendToRequestBuffer(request.c_str(), request.size());

//         check(
//             client.checkRequestState() == RequestState::BadRequest,
//             "invalid CRLF after chunk data is rejected"
//         );
//     }

//     {
//         Client client(42, 7);

//         const std::string request =
//             "POST /upload HTTP/1.1\r\n"
//             "Host: unit.test\r\n"
//             "Transfer-Encoding: chunked\r\n"
//             "\r\n"
//             "4\r\n"
//             "Hello\r\n"
//             "0\r\n"
//             "\r\n";

//         client.appendToRequestBuffer(request.c_str(), request.size());

//         check(
//             client.checkRequestState() == RequestState::BadRequest,
//             "extra bytes beyond the declared chunk size are rejected"
//         );
//     }

//     {
//         Client client(42, 7);

//         const std::string request =
//             "POST /upload HTTP/1.1\r\n"
//             "Host: unit.test\r\n"
//             "Transfer-Encoding: chunked\r\n"
//             "\r\n"
//             "5\r\n"
//             "Hello\r\n"
//             "0\r\n"
//             "XX";

//         client.appendToRequestBuffer(request.c_str(), request.size());

//         check(
//             client.checkRequestState() == RequestState::BadRequest,
//             "an invalid final chunk terminator is rejected"
//         );
//     }

//     {
//         Client client(42, 7);

//         const std::string firstPart =
//             "POST /upload HTTP/1.1\r\n"
//             "Host: unit.test\r\n"
//             "Transfer-Encoding: chunked\r\n"
//             "\r\n"
//             "A\r\n"
//             "12345";

//         const std::string secondPart =
//             "67890\r\n"
//             "0\r\n"
//             "\r\n";

//         client.appendToRequestBuffer(firstPart.c_str(), firstPart.size());

//         check(
//             client.checkRequestState() == RequestState::Incomplete,
//             "partial chunk data is incomplete"
//         );

//         client.appendToRequestBuffer(secondPart.c_str(), secondPart.size());

//         check(
//             client.checkRequestState() == RequestState::Complete,
//             "chunked request completes after remaining data arrives"
//         );
//     }
//     //Test a size line split between two reads:
//     {
//         Client client(42, 7);

//         const std::string firstPart =
//             "POST /upload HTTP/1.1\r\n"
//             "Host: unit.test\r\n"
//             "Transfer-Encoding: chunked\r\n"
//             "\r\n"
//             "A";

//         const std::string secondPart =
//             "\r\n"
//             "0123456789\r\n"
//             "0\r\n"
//             "\r\n";

//         client.appendToRequestBuffer(firstPart.c_str(), firstPart.size());

//         check(
//             client.checkRequestState() == RequestState::Incomplete,
//             "a partial chunk-size line is incomplete"
//         );

//         client.appendToRequestBuffer(secondPart.c_str(), secondPart.size());

//         check(
//             client.checkRequestState() == RequestState::Complete,
//             "the request completes after the chunk-size line arrives"
//         );
//     }

//     {
//         Client client(42, 7);

//         const std::string request =
//             "POST /upload HTTP/1.1\r\n"
//             "Host: unit.test\r\n"
//             "Content-Length: 5\r\n"
//             "Transfer-Encoding: chunked\r\n"
//             "\r\n"
//             "5\r\n"
//             "Hello\r\n"
//             "0\r\n"
//             "\r\n";

//         client.appendToRequestBuffer(request.c_str(), request.size());

//         check(
//             client.checkRequestState() == RequestState::BadRequest,
//             "Content-Length and Transfer-Encoding together are rejected"
//         );
//     }
// }

} // namespace

int main()
{
    run("validation helpers", testValidationHelpers);
    run("LocationConfig", testLocationConfig);
    run("ServerConfig", testServerConfig);
    run("ConfigParser", testConfigParser);
    run("HTTPRequestParser", testHttpRequestParser);
    run("HTTPRequest body location", testHttpRequestBodyLocation);
    run("POST upload", testPostUpload);
    // run("Client request buffer", testClientRequestBuffer);
    run("HTTPResponse", testHttpResponse);
    // added on 20 Aug - Ivan - for log:
    run("Client response buffer", testClientResponseBuffer);
    run("Client new response resets progress", testClientNewResponseResetsProgress);
    run("Client close after response", testClientCloseAfterResponse);
    // run("Client consume preserves next request", testClientConsumeRequestPreservesNextRequest);
    // run("Client consume preserves partial next request", testClientConsumeRequestPreservesPartialNextRequest);

    if (g_failures != 0) {
        std::cerr << g_failures << " assertion(s) failed\n";
        return 1;
    }
    std::cout << "All unit tests passed\n";
    return 0;
}
