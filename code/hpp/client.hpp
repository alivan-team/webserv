#ifndef CLIEN_HPP
#define CLIEN_HPP

#include <string>

bool check_num(const std::string& value);
bool checkUriPath(const std::string& uripath);
bool checkFSPath(const std::string &fspath);
bool hasControlChar(const std::string& s);

#endif

// penchoivanov@Mac ~ % printf 

    // 'POST /upload HTTP/1.1\r\n
    // Host: localhost:8080\r\n
    // Transfer-Encoding: chunked\r\n
    // Connection: close\r\n
    // \r\n
    // 5\r\n
    // Hello\r\n
    // 6\r\n
    //  World\r\n
    // 0\r\n
    // \r\n'
    
//  | nc localhost 8080