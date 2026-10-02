*This project has been created as part of the 42 curriculum by ipavlov, agerokhina.*

# Webserv

## Description

**Webserv** is a non-blocking HTTP server written in C++ for the 42/Codam curriculum. The goal of the project is to understand how an HTTP server works at a low level by implementing the server ourselves instead of relying on an existing web-server framework.

The server accepts TCP connections, parses HTTP requests, selects configuration rules, builds HTTP responses, serves static files, handles uploads and deletions, and can execute CGI programs. Network and CGI pipe I/O are handled through a single `poll()`-driven event loop so that one slow client or CGI process does not block the rest of the server.

This implementation is compiled as **C++17**, following the C++ version allowance used for our Codam Amsterdam project.

### Main features

- Non-blocking TCP server using `poll()`.
- Multiple listening sockets and ports.
- HTTP/1.0 and HTTP/1.1 request handling.
- Persistent connections / keep-alive handling.
- `GET`, `POST`, and `DELETE` methods.
- Per-location method restrictions with `allow_methods`.
- Static file serving.
- Configurable server and location roots.
- Configurable index files.
- Automatic directory listings with `autoindex`.
- Custom error pages and default fallback error responses.
- Request-body size limits with `client_max_body_size`.
- Request bodies using `Content-Length`.
- Chunked request decoding.
- File uploads, including `multipart/form-data` handling.
- File deletion.
- HTTP redirection.
- CGI execution based on configured file extensions.
- CGI request environment variables and request body forwarding through pipes.
- CGI output returned as an HTTP response.
- CGI timeout/process cleanup handling.
- Virtual-host selection through the `Host` header.
- Path validation to prevent requests from escaping the configured web root.

## Project structure

```text
.
├── main.cpp
├── Makefile
├── code/
│   ├── ConfigParser.cpp
│   ├── ServerManager.cpp
│   ├── ServerManagerCGI.cpp
│   ├── HTTPRequestParser.cpp
│   ├── HTTPResponseBuild.cpp
│   └── ...
├── code/hpp/
│   └── ...
├── config/
│   ├── default.conf
│   └── ...
├── site/
│   └── www/
└── tests/
    └── TestMain.cpp
```

## Instructions

### Requirements

You need:

- a C++ compiler available as `c++`;
- `make`;
- a POSIX-compatible environment providing sockets, `poll()`, `fork()`, pipes, and related system calls;
- a CGI interpreter if CGI is enabled in the configuration, for example Python 3.

### Compilation

Build the server with:

```sh
make
```

The project is compiled with:

```text
-Wall -Wextra -Werror -std=c++17
```

Available Makefile targets:

```sh
make          # Build webserv
make test     # Build and run the test executable
make clean    # Remove object and dependency files
make fclean   # Remove objects, dependencies, and binaries
make re       # Clean and rebuild webserv
```

### Running the server

Run with the default configuration:

```sh
./webserv
```

When no argument is supplied, Webserv loads:

```text
./config/default.conf
```

To use another configuration file:

```sh
./webserv path/to/config.conf
```

Only zero or one configuration-file argument is accepted.

### Basic test

With a configuration listening on port `8080`:

```sh
curl -i http://127.0.0.1:8080/
```

You can also open the same address in a web browser.

### Running the tests

```sh
make test
```

The included tests cover parts of configuration parsing, HTTP request parsing, response generation, request buffering, chunked-body decoding, host selection, body-size limits, uploads, and related server behaviour.

## Configuration

The configuration format is inspired by the `server` and `location` blocks used by NGINX.

Example:

```nginx
server {
    listen 8080;
    server_name localhost;

    root ./site/www;
    index index.html;
    client_max_body_size 1000000;

    error_page 404 /error_pages/404.html;
    error_page 500 /error_pages/500.html;

    location / {
        allow_methods GET;
        autoindex off;
    }

    location /upload {
        root ./site/www/upload;
        allow_methods GET POST DELETE;
        upload_store ./site/www/uploads;
        index index.html;
        autoindex on;
    }

    location /cgi-bin {
        root ./site/www/cgi-bin;
        allow_methods GET POST;
        cgi_extension .py;
        cgi_path /usr/bin/python3;
    }

    location /old-page {
        return 301 /new-page;
    }
}
```

### Server directives

The server configuration supports directives including:

- `listen` — interface/port on which the server listens;
- `server_name` — names associated with a server block;
- `root` — document root;
- `index` — default files for directories;
- `client_max_body_size` — maximum accepted request-body size;
- `error_page` — custom error pages for one or more status codes.

### Location directives

Location blocks support directives including:

- `allow_methods` — accepted methods (`GET`, `POST`, `DELETE`);
- `root` — location-specific filesystem root;
- `index` — location-specific index files;
- `autoindex` — enable or disable directory listing;
- `upload_store` — destination directory for uploaded files;
- `return` — HTTP redirection;
- `cgi_extension` — file extension handled as CGI;
- `cgi_path` — interpreter/executable used for the corresponding CGI extension.

For CGI, each configured CGI extension must have a corresponding CGI executable path.

## HTTP and CGI behaviour

### Static resources

For a normal `GET` request, Webserv resolves the request against the selected server/location root. Depending on the resource and configuration it can return a file, serve an index page, generate an autoindex page, redirect the client, or return an appropriate error response.

### Uploads

Locations configured with `POST` and `upload_store` can receive request bodies and save uploaded files. Multipart form uploads are parsed so that uploaded file content can be stored separately from the multipart metadata.

The configured upload directory must exist before starting the server.

### DELETE

When `DELETE` is allowed for a location, Webserv validates the requested filesystem path before removing the target resource.

### CGI

CGI programs are selected using the requested file extension and the CGI rules in the matching location. Webserv:

1. creates pipes for CGI input and output;
2. forks a child process;
3. connects the CGI process's standard input/output to the pipes;
4. builds CGI environment variables from the HTTP request;
5. changes to the CGI working directory;
6. executes the configured interpreter/program with `execve()`;
7. communicates with the CGI pipes through the main event loop;
8. converts the CGI output into the response sent to the client.

Chunked request bodies are decoded before they are provided to CGI. EOF on the CGI input/output pipes is used as part of the CGI request/response lifecycle where applicable.

## Resources

The following references were useful while studying and implementing the project:

- **42 Webserv subject** — mandatory project requirements and allowed project scope.
- **RFC 9110 — HTTP Semantics** — HTTP methods, status codes, headers, and general protocol semantics: https://www.rfc-editor.org/rfc/rfc9110
- **RFC 9112 — HTTP/1.1** — HTTP/1.1 message framing and connection behaviour: https://www.rfc-editor.org/rfc/rfc9112
- **RFC 3875 — The Common Gateway Interface (CGI) Version 1.1** — CGI environment and server/CGI communication: https://www.rfc-editor.org/rfc/rfc3875
- **NGINX documentation** — reference behaviour and configuration concepts: https://nginx.org/en/docs/
- **Linux/POSIX manual pages** for `socket`, `bind`, `listen`, `accept`, `poll`, `recv`, `send`, `fcntl`, `pipe`, `fork`, `dup2`, `execve`, `waitpid`, and related system calls: https://man7.org/linux/man-pages/
- **cppreference** — C++ language and standard-library reference: https://en.cppreference.com/

### Use of AI

AI tools were used as a supporting learning and review resource during the project. In particular, they were used to:

- explain networking, HTTP, CGI, process, pipe, and file-descriptor concepts;
- discuss possible edge cases and failure scenarios;
- review parts of the code for subject compliance and robustness;
- suggest tests for HTTP, CGI, uploads, timeouts, and concurrent behaviour;
- help review configuration and error-handling logic;
- assist with documentation wording and the evaluation/demo website.

AI output was treated as suggestions rather than authoritative answers. Proposed changes were reviewed against the project subject, the existing codebase, documentation, testing, and team discussion before being accepted. The authors remain responsible for understanding and being able to explain the submitted implementation.

## Team

- **Ivan Pavlov** — Codam login: `ipavlov` — GitHub: `12Ivan03`
- **Anastasia Erokhina** — Codam login: `agerokhina` — GitHub: `agerokhina`
