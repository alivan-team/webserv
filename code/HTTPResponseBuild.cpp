
#include "./hpp/HTTPResponseBuild.hpp"
#include "./hpp/HTTPResponse.hpp"

	// HTTP/version status reason
	// Content-Type
	// Content-Length
	// Connection

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

        // case Method::DELETE:
        //     return handleDelete(request, servConf);

        // investigate: decide later 
        // case Method::HEAD:
        //     return handleHead(request, servConf);

        default:
            return makeErrorResponse(400, request, servConf);
    }

    // detect SGI



    //// HTTPRequest req = request; ////
    //// ServerConfig ser = servConf; ////
	
	// // 1. Find matching location
    // LocationConfig location = findBestLocation(req.getPath(), servConf);

    // // 2. Check if method is allowed
    // if (!location.allowsMethod(req.getMethod()))
    //     return makeErrorResponse(405, servConf);

    // // 3. Build real filesystem path
    // std::string filePath = servConf.getRoot() + req.getPath();

    // // 4. If path is directory, use index
    // if (isDirectory(filePath))
    //     filePath += "/" + servConf.getIndex();

    // // 5. If file does not exist
    // if (!fileExists(filePath))
    //     return makeErrorResponse(404, servConf);

    // // 6. If no permission
    // if (!canRead(filePath))
    //     return makeErrorResponse(403, servConf);

    // // 7. Read file into body
    // std::string body = readFile(filePath);

    // // 8. Build 200 response
    // HTTPResponse res;
    // res.setStatus(200, "OK");
    // res.setHeader("Content-Type", getMimeType(filePath));
    // res.setHeader("Content-Length", toString(body.size()));
    // res.setHeader("Connection", "keep-alive");
    // res.setBody(body);

    return {};
};

// GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET
HTTPResponse HTTPResponseBuild::handleGet(const HTTPRequest& request, const ServerConfig& servConf) {

    HTTPResponse res;
    // std::cout << "    :    Request    : \n" << "code:  "<<  request.getUri() << std::endl;
    
    std::string path = request.getPath();
    // std::cout << "\n    ~~~~~~~~~~~~~    GET    ~~~~~~~~~~~~~\n" << "path:  "<<  path << std::endl;

    LocationConfig location = findBestLocation(path ,servConf);
    // std::cout << "location:  " <<  location.getUriPath() << std::endl;

    if (!location.isGetAllowed())
        return makeErrorResponse(405, request, servConf);

    std::string fullPath;
    if (location.getRoot().empty())
        fullPath = joinPath(servConf.getRoot()[0], path);
    else 
        fullPath = joinPath(location.getRoot(), path);

    // std::cout << "fullPath :  " <<  fullPath << std::endl;

    if (!fileExists(fullPath))
        return makeErrorResponse(404, request, servConf);

    if (isDirectory(fullPath)) {

        std::string indexPath = findIndexFile(fullPath, location, servConf);

        if (!indexPath.empty()) {
            fullPath = indexPath;
        } else {
            return makeErrorResponse(403, request, servConf);
        }

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

// HELPER ERROR Functions that maybe I will use later for other requests ??

LocationConfig HTTPResponseBuild::findBestLocation (const std::string& path,  const ServerConfig& servConf) {

    const std::vector<LocationConfig>& locations = servConf.getLocations();
    // /////////////////////////////////////////
    // std::cout << "LOCATIONS " << "\n" << std::endl;

    const LocationConfig* bestLoc = NULL;

    for (const auto& loc : locations) {
        // std::cout << "loc : " << loc.getUriPath() << std::endl;
        
        std::string locPath = loc.getUriPath();

        bool matches = path.compare(0, locPath.size(), locPath) == 0;
        // std::cout << "matches : " << matches << "\n" << std::endl;
        
        if (matches) {
            // std::cout << "locPath.size() : " << locPath.size() << "\n" << std::endl;
            // std::cout << "loc.getUriPath().size() : " << loc.getUriPath().size() << "\n" << std::endl;
            if (bestLoc == NULL || locPath.size() > bestLoc->getUriPath().size()) {
                bestLoc = &loc;
            }
        }
        
    }
    // dunno yet ??
    if (bestLoc == NULL)
       throw std::runtime_error("No matching location");
    // // /////////////////////////////////////////

    return *bestLoc;

};

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

    // std::cout << "location.getIndex().empty() : " << location.getIndex().empty() << std::endl;
    // std::cout << "servConf.getIndex()[0] : " << servConf.getIndex()[0] << std::endl;

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
    // std::ifstream inputFile(file.c_str());

    // if (!inputFile.is_open())
    //     return "";

    // std::stringstream buffer;
    // buffer << inputFile.rdbuf();

    // return buffer.str();
};
