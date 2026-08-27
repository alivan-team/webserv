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

// void testServerConfig()
// {
//     ServerConfig server;

//     check(
//         server.getServerName().size() == 1,
//         "default server name exists"
//     );

//     check(
//         server.getServerName().at(0) == "localhost",
//         "default server name is localhost"
//     );

//     std::vector<std::string> serverNames;
//     serverNames.push_back("example.test");
//     serverNames.push_back("api-example");

//     server.setServerName(serverNames);

//     check(
//         server.getServerName().size() == 2,
//         "configured server names replace the default"
//     );

//     check(
//         server.getServerName().at(0) == "example.test",
//         "first configured server name is stored"
//     );

//     check(
//         server.getServerName().at(1) == "api-example",
//         "second configured server name is stored"
//     );

//     std::vector<std::string> roots;
//     roots.push_back("./site/www");

//     server.setRoot(roots);

//     check(
//         !server.getRoot().empty(),
//         "server root is stored"
//     );

//     check(
//         server.getRoot().back() == "./site/www",
//         "configured server root is correct"
//     );

//     std::vector<std::string> indexes;
//     indexes.push_back("index.html");
//     indexes.push_back("home.html");

//     server.setIndex(indexes);

//     check(
//         server.getIndex().size() == 2,
//         "configured indexes are stored"
//     );

//     check(
//         server.getIndex().at(0) == "index.html",
//         "first configured index is correct"
//     );

//     check(
//         server.getIndex().at(1) == "home.html",
//         "second configured index is correct"
//     );

//     std::cout << "PASS ServerConfig" << std::endl;
// }

// void testServerConfig()
// {
//     ServerConfig server;
//     server.setPort(std::vector<std::string>{"9090"});
//     server.setServerName(std::vector<std::string>{"example.test", "api-example"});
//     server.setRoot(std::vector<std::string>{"./public"});
//     server.setIndex(std::vector<std::string>{"home.html"});
//     server.setClientMaxBodySize(std::vector<std::string>{"2048"});
//     server.setErrorPage(std::vector<std::string>{"404", "500", "/errors/error.html"});

//     check(server.getPort() == 9090, "configured port overrides the default");
//     check(server.getServerName().size() == 3, "server names are appended to defaults");
//     check(server.getRoot().at(0) == "./public", "server root is replaced");
//     check(server.getIndex().back() == "home.html", "server index is appended");
//     check(server.getClientMaxBodySize().back() == 2048U, "body-size limit is parsed");
//     check(server.hasErrorPage(404) && server.getOneErrorPage(500) == "/errors/error.html",
//           "error pages are mapped to every specified status");

//     checkThrows([&server] { server.setPort(std::vector<std::string>{"70000"}); },
//                 "ports above 65535 are rejected");
//     checkThrows([&server] { server.setServerName(std::vector<std::string>{"bad name"}); },
//                 "server names with whitespace are rejected");
//     checkThrows([&server] { server.setClientMaxBodySize(std::vector<std::string>{"12KB"}); },
//                 "non-numeric body-size limits are rejected");
// }

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
	run("Client chunked body decoding", testClientDecodeChunkedBody); // 
	run("Client Content-Length request", testClientContentLengthUnaffected);
	// run("Client consume preserves next request", testClientConsumeRequestPreservesNextRequest);
    // run("Client consume preserves partial next request", testClientConsumeRequestPreservesPartialNextRequest);

    run("Virtual host server configs", testVirtualHostServerConfigs);
    run("Multiple server names", testMultipleServerNames);
    run("Virtual host body size limits", testVirtualHostBodySizeLimits);
    run("Server name replaces default", testServerNameReplacesDefault);
    run("ServerConfig listen", testServerConfigListen);
    run("ServerConfig invalid listen", testServerConfigInvalidListen);  
    run("Redirect", testRedirect);

    if (g_failures != 0) {
        std::cerr << g_failures << " assertion(s) failed\n";
        return 1;
    }
    std::cout << "All unit tests passed\n";
    return 0;
}
