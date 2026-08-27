# webserv

A work-in-progress HTTP/1.1 web server written in C++17 for the 42/CODAM
**webserv** project. The server is event-driven: it uses non-blocking sockets
and `poll()` to serve multiple clients without one connection blocking the
others.

## Current status

The following functionality is implemented and covered at least in part by the
included unit tests:

- HTTP/1.0 and HTTP/1.1 request handling, including keep-alive behaviour.
- Non-blocking TCP server with `poll()`-based I/O.
- Multiple `server` blocks on one port and virtual-host selection through the
  `Host` header.
- Parsing of server and location configuration blocks.
- Static-file delivery with MIME-type detection, index-file lookup, optional
  directory listing, custom error pages, and path-traversal checks.
- `GET`, `POST`, and `DELETE`, controlled per location with `allow_methods`.
- Request bodies sent with `Content-Length` or `Transfer-Encoding: chunked`.
- Binary uploads to `upload_store`; `multipart/form-data` uploads save the
  first part that has a `filename` parameter.
- Configurable request-body limits and standard error responses such as 400,
  403, 404, 405, 413, 500, 501, and 505.

`cgi_extension`, `cgi_path`, and `return` are accepted by the configuration
parser, but CGI execution and redirect handling are not connected to the
response path yet. They should therefore be considered unfinished.

## Requirements

- A C++17-compatible compiler (`c++`)
- `make`
- A POSIX-compatible system (the current implementation uses sockets,
  `poll()`, and POSIX filesystem APIs)

## Build and run

```sh
make
./webserv
```

Without arguments, the server reads `./config/default.conf`. To use another
configuration file:

```sh
./webserv path/to/webserv.conf
```

The default configuration listens on port `8080`. With the supplied site
files available, try:

```sh
curl -i http://127.0.0.1:8080/
```

## Tests

```sh
make test
```

The current test suite verifies configuration parsing, request parsing,
response serialization, request buffering, chunked-body decoding, virtual
hosts, body-size limits, and binary upload handling. The test suite passes in
the supplied project snapshot.

## Configuration overview

The syntax follows the familiar `server` / `location` block structure:

```nginx
server {
    listen 8080;
    server_name localhost;
    root ./site/www;
    index index.html;
    client_max_body_size 1000000;
    error_page 404 /error_pages/404.html;

    location / {
        allow_methods GET;
        autoindex off;
    }

    location /upload {
        allow_methods GET POST DELETE;
        upload_store ./site/www/uploads;
        autoindex on;
    }
}
```

Supported server directives are `listen`, `server_name`, `root`, `index`,
`client_max_body_size`, and `error_page`. Supported location directives are
`allow_methods`, `upload_store`, `autoindex`, `root`, `index`,
`cgi_extension`, `cgi_path`, and `return`.

For file uploads, create the configured upload directory before starting the
server; it is not created automatically:

```sh
mkdir -p site/www/uploads
curl -i --data-binary @./photo.webp http://127.0.0.1:8080/upload
```

## Project layout

```text
main.cpp          Application entry point
code/             Server, parsing, request, response, and upload code
code/hpp/         Headers
config/           Example server configurations
site/             Static website and error-page assets
tests/            Unit tests and configuration fixtures
```

## Team

- Ivan Pavlov — [12Ivan03](https://github.com/12Ivan03)
- Anastasia Erokhina — [agerokhina](https://github.com/agerokhina)

## Make targets

```sh
make          # build webserv
make test     # build and run unit tests
make clean    # remove object and dependency files
make fclean   # also remove binaries
make re       # rebuild the server
```
