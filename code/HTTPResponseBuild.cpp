
#include "./hpp/HTTPResponseBuild.hpp"
#include "./hpp/HTTPResponse.hpp"

	// HTTP/version status reason
	// Content-Type
	// Content-Length
	// Connection

    // Client requests /images/
    //         ↓
    // Is it a directory?
    //         ↓
    // Yes
    //         ↓
    // Find index file
    //         ↓
    // Found?
    //     /    \
    //     yes    no
    //     |       |
    // serve      autoindex?
    // index        /   \
    //             on   off
    //             |     |
    // read directory   403
    // generate HTML
    // return HTML

HTTPResponse HTTPResponseBuild::build(const HTTPRequest& request, const ServerConfig& servConf) {
	

    Method method = request.getMethod();
    std::string path = request.getPath();
    std::string version = request.getVersion();

    std::cout << "REQUEST: " << version << std::endl;
    printDebug("\t _uri: ", request.getUri());
    printDebug("\t _path: ", request.getPath());
    printDebug("\t _query: ", request.getQuery());
    printDebug("\t _version: ", request.getVersion());

    if (version != "1.0" && version != "1.1")
        return makeErrorResponse(505, request, servConf);

    switch (method)
    {
        // CGI function 
        case Method::GET:
            return handleGet(request, servConf);

        // case Method::POST:
        //     return handlePost(request, servConf);
        // -> IMPORTANT Check for POST:  Transfer-Encoding: chunked + Content-length

        // case Method::DELETE:
        //     return handleDelete(request, servConf);

        // investigate: decide later 
        // case Method::HEAD:
        //     return handleHead(request, servConf);

        default:
            return makeErrorResponse(400, request, servConf);
    }

    return {};
};

// GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET
HTTPResponse HTTPResponseBuild::handleGet(const HTTPRequest& request, const ServerConfig& servConf) {

    HTTPResponse res;
    // std::cout << "    :    Request    : \n" << "code:  "<<  request.getUri() << std::endl;
    
    std::string path = request.getPath();
    // std::cout << "\n    ~~~~~~~~~~~~~    GET    ~~~~~~~~~~~~~\n" << "path:  "<<  path << std::endl;

    const LocationConfig* location = findBestLocation(path ,servConf);
    // std::cout << "location:  " <<  location.getUriPath() << std::endl;

    if (location == NULL)
        return makeErrorResponse(404, request, servConf);

    if (!location->isGetAllowed())
        return makeErrorResponse(405, request, servConf);




    std::string fullPath;
    if (location->getRoot().empty())
        fullPath = joinPath(servConf.getRoot()[0], path);
    else 
        fullPath = joinPath(location->getRoot(), path);

    // std::cout << "fullPath :  " <<  fullPath << std::endl;

    if (!fileExists(fullPath))
        return makeErrorResponse(404, request, servConf);

    if (isDirectory(fullPath)) {

        std::string indexPath = findIndexFile(fullPath, *location, servConf);

        std::cout << "\t -> indexPath: " << indexPath << "\n\t -> fullPath: " << fullPath << "\n\t -> request.getPath(): " << request.getPath() << std::endl;

        if (!indexPath.empty()) {
            fullPath = indexPath;
        } else if (location->getAutoIndex()) {
            // return the buildAutoIndexPage -> because there is no 
            // return makeErrorResponse(404, request, servConf);
            return buildAutoIndexPage(request, servConf, fullPath, request.getPath());
        } else {
            return makeErrorResponse(403, request, servConf);
        }

//  AUTO INDEX
        // This is for autoIndex in Location
        // std::string indexPath = findIndexFile(fullPath, *location, servConf);

        // if (!indexPath.empty()) {
        //     fullPath = indexPath;
        // } else if (location->getAutoIndex()) {
        //     return buildAutoIndexPage( const std::string& directoryPath, const std::string& requestPath, const HTTPRequest& request, const ServerConfig& servConf);
        // } else {
        //     return makeErrorResponse(403, request, servConf);
        // }

    }
    // std::cout << "canReadFile(fullPath) : " << canReadFile(fullPath) << std::endl;

    if (!canReadFile(fullPath))
        return makeErrorResponse(403, request, servConf);

    // CGI Function -> this one was suggested from ChatGPT :D 
    // if (isCgiFile(fullPath, location))
    //     return handleCgi(request, servConf, location, fullPath);

        
    std::string body = readReadFile(fullPath);

    res.setStatusCode(200);
    res.setStatus(getStatusText(200));
    res.setHeader("Content-Type", "text/html");
    res.setHeader("Content-Length", std::to_string(body.size()));
    res.setHeader("Connection", decideConnection(request));
    res.setVersion(request.getVersion());
    res.setBody(body);


    return res;
    
};

// POST POST POST POST POST POST POST POST POST POST POST POST POST POST POST POST  POST POST POST POST POST POST POST POST  POST POST POST POST POST 
// DELETE DELETE DELETE DELETE DELETE DELETE DLETE DELETE DELETE DELETE DELETE DELETE DELETE DLETE DELETE DELETE DELETE DELETE DELETE DELETE DLETE

// ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR 

HTTPResponse HTTPResponseBuild::makeErrorResponse(int code, const HTTPRequest& request, const ServerConfig& servConf) {

    HTTPResponse res;

    std::string text = getStatusText(code);
    std::string body = buildErrorBody(code, servConf);

    // std::cout << "    :    BODY    : \n" << "code:  "<<  code << std::endl;
    // std::cout << "Code: " << code << std::endl;

    res.setStatusCode(code);
    res.setVersion(request.getVersion());
    res.setStatus(text);
    res.setHeader("Content-Type", "text.html");
    res.setHeader("Content-Length", std::to_string(body.size()));
    res.setHeader("Connection", decideConnection(request));
    res.setBody(body);

    return res;
};

std::string  HTTPResponseBuild::buildErrorBody(int code, const ServerConfig& servConf) {


    // std::cout << "\nCode: " << code << std::endl;
    // std::cout << "servConf.hasErrorPage(code): " << servConf.hasErrorPage(code) << std::endl;
    // std::cout << "error_path from servConf: " << servConf.getOneErrorPage(code) << std::endl;
    // std::cout << "ROOT from servConf: " << servConf.getRoot().back() << std::endl;
    if (servConf.hasErrorPage(code)) {

        std::string error_path = servConf.getOneErrorPage(code);
        std::string root = servConf.getRoot().back();
        std::string fullPath = joinPath(root, error_path);

        // std::cout << "\tfileExists(fullPath) : " << fileExists(fullPath) 
        // << "\n\t canReadFile(fullPath) : " <<  canReadFile(fullPath) << std::endl;
        
        if (fileExists(fullPath) && canReadFile(fullPath)) {
            
            // std::cout << "\tFULL PATH : " << fullPath << std::endl;
            
            // cgi -> child -> execute that file with query -> return result -> we put that result in body -> and send it  
            
            return readReadFile(fullPath);
        }

    
    }


    std::string text = getStatusText(code);

    return readReadFile("./site/www/error_pages/index.html");
        //     "<html><body><h1>" +
        //    std::to_string(code) + " " + text +
        //    "</h1></body></html>";
};

std::string HTTPResponseBuild::getStatusText(int code)
{
    switch (code)
    {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 505: return "HTTP Version Not Supported";
        default:  return "Error";
    }
}

std::string HTTPResponseBuild::decideConnection(const HTTPRequest& request) {
    std::string version = request.getVersion();

    if (version == "1.0")
    {
        if (request.hasHeader("Connection") &&
            request.getHeader("Connection") == "keep-alive")
            return "keep-alive";

        return "close";
    }

    if (version == "1.1")
    {
        if (request.hasHeader("Connection") &&
            request.getHeader("Connection") == "close")
            return "close";

        return "keep-alive";
    }

    return "close";
};



// HELPERHELPERHELPERHELPERHELPERHELPERHELPERHELPERHELPERHELPERHELPERHELPERHELPERHELPER
// HELPER ERROR Functions that maybe I will use later for other requests ??

//  AUTO INDEX
// This is for autoIndex in Location

HTTPResponse HTTPResponseBuild::buildAutoIndexPage(const HTTPRequest& request, const ServerConfig& servConf, const std::string& fullPath, const std::string& requestPath) {
    HTTPResponse res;
    
    // std::cout << " Hello from HTTPResponseBuild " << std::endl;
    
    DIR* dir = opendir(fullPath.c_str());

    if (dir == NULL)
        return makeErrorResponse(403, request, servConf);

    std::string body;

    body += "<!DOCTYPE html>\n";
    body += "<html>\n";
    body += "<head>\n";
    body += "    <meta charset=\"UTF-8\">\n";
    body += "    <title>Index of " + requestPath + "</title>\n";
    body += "</head>\n";
    body += "<body>\n";
    body += "    <h1>Index of " + requestPath + "</h1>\n";
    body += "    <hr>\n";
    body += "    <ul>\n";

    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;
        // std::cout << "\t\t name --> " << name << std::endl;

        if (name == "." || name == ".." || name.front() == '.')
            continue;

        std::string extension;
        size_t dot = name.rfind('.');

        if (dot != std::string::npos)
            extension = name.substr(dot);
    
        std::string href = requestPath;
        // std::cout << "\t\thref --> " << href << std::endl;

        if (href.empty() || href[href.size() - 1] != '/')
            href += "/";

        href += name;

        if (checkExtensionOfFile(extension)) {
            body += "        <li><img src=\"" + href + "\" width=\"200\"><br></li>\n";
        } else {
            body += "        <li><a href=\"" + href + "\">" + name + "</a><br>";
        }
        // std::cout << "\t\t body --> " << body << std::endl;
    }

    body += "    </ul>\n";
    body += "    <hr>\n";
    body += "</body>\n";
    body += "</html>\n";

    closedir(dir);

    res.setStatusCode(200);
    res.setStatus(getStatusText(200));
    res.setVersion(request.getVersion());
    res.setHeader("Content-Type", "text/html");
    res.setHeader("Content-Length", std::to_string(body.size()));
    res.setHeader("Connection", decideConnection(request));
    res.setBody(body);

    return res;
}

const LocationConfig* HTTPResponseBuild::findBestLocation (const std::string& path,  const ServerConfig& servConf) {

    const std::vector<LocationConfig>& locations = servConf.getLocations();
    // /////////////////////////////////////////
    // std::cout << "LOCATIONS " << "\n" << std::endl;

    const LocationConfig* bestLoc = NULL;

    for (const auto& loc : locations)
    {
        if (startsWithLocation(path, loc.getUriPath()))
        {
            if (!bestLoc || loc.getUriPath().size() > bestLoc->getUriPath().size())
                bestLoc = &loc;
        }
    }

    if (bestLoc == NULL)
       return NULL;

    return bestLoc;

};

bool HTTPResponseBuild::startsWithLocation(const std::string& path, const std::string& loc) {

    if (loc.empty())
        return false;

    if (path.compare(0, loc.size(), loc) != 0)
        return false;

    if (path.size() == loc.size())
        return true;

    if (loc.back() == '/')
        return true;

    return path[loc.size()] == '/';
}

std::string HTTPResponseBuild::joinPath(const std::string& root, const std::string& path) {
    
    if (root.empty())
        return path;

    if (path.empty())
        return root;

    bool rootEndsWithSlash = root[root.size() - 1] == '/';
    bool pathStartsWithSlash = path[0] == '/';

    if (rootEndsWithSlash && pathStartsWithSlash)
        return root + path.substr(1);

    if (!rootEndsWithSlash && !pathStartsWithSlash)
        return root + "/" + path;

    return root + path;
};

bool HTTPResponseBuild::fileExists(const std::string& file) {

    struct stat st;
    return stat(file.c_str(), &st) == 0;
};

bool HTTPResponseBuild::canReadFile(const std::string& file) {
    // std::cout << "file -> canReadFile : " << file << std::endl;

    return access(file.c_str(), R_OK) == 0;
};

bool HTTPResponseBuild::isDirectory(const std::string& path)
{
    struct stat st;

    if (stat(path.c_str(), &st) != 0)
        return false;

    return S_ISDIR(st.st_mode); // S_ISDIR(st.st_mode) asks -> "Do the type bits inside st_mode indicate a directory?"
}

std::string HTTPResponseBuild::findIndexFile(std::string fullPath, const LocationConfig& location, const ServerConfig& servConf) {

    std::cout << "location.getIndex().empty() : " << location.getIndex().empty() << std::endl;
    std::cout << "servConf.getIndex()[0] : " << servConf.getIndex()[0] << std::endl;

    const std::vector<std::string>* indexes;

    if (!location.getIndex().empty()) {
        indexes = &location.getIndex();
    } else {
        indexes = &servConf.getIndex();
    }

    for (size_t i = 0; i < indexes->size(); i++) {
        std::string indexCandidate = joinPath(fullPath, (*indexes)[i]);

        // std::cout << "Trying index: " << indexCandidate << std::endl;
        // std::cout << "exists: " << fileExists(indexCandidate) << " readable: " << canReadFile(indexCandidate) << std::endl;

        if (fileExists(indexCandidate) && canReadFile(indexCandidate))
            return indexCandidate;
    }

    return "";
};

std::string HTTPResponseBuild::readReadFile(const std::string& file) {

    std::ifstream inputFile(file);
    
    if (!inputFile.is_open())
        return "";
    
    std::string line;
    std::string body;

    while (std::getline(inputFile, line)) {
        // std::cout << "read file: " << line << std::endl;
        body += line;
        body += "\n";
    }
    inputFile.close();

    return body;

// Chat suggestion ????
//  Check the throw error... 

    // std::ifstream inputFile(file.c_str(), std::ios::binary);

    // if (!inputFile.is_open())
    //     throw std::runtime_error("Could not open file: " + file);

    // std::ostringstream buffer;
    // buffer << inputFile.rdbuf();

    // if (inputFile.bad())
    //     throw std::runtime_error("Could not read file: " + file);

    // return buffer.str();
};

bool HTTPResponseBuild::checkExtensionOfFile(const std::string& extension) {
    if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || 
        extension == ".gif" || extension == ".webp")
        return true;
    return false;
}