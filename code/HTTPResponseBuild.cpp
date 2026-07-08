
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

HTTPResponse HTTPResponseBuild::makeErrorResponse(int code, const HTTPRequest& request, const ServerConfig& servConf) {

    HTTPResponse res;

    std::string text = getStatusText(code);
    std::string body = buildErrorBody(code, servConf);

    // std::cout << "    :    BODY    : \n" << "code:  "<<  code << std::endl;
    // std::cout << "Code: " << code << std::endl;

    res.setStatusCode(code);
    res.setStatus(text);
    res.setHeader("Content-Type", "text.html");
    res.setHeader("Content-Length", std::to_string(body.size()));
    res.setHeader("Connection", decideConnection(request));
    res.setBody(body);

    return res;
};


// GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET
HTTPResponse HTTPResponseBuild::handleGet(const HTTPRequest& request, const ServerConfig& servConf) {

    
};

// POST POST POST POST POST POST POST POST POST POST POST POST POST POST POST POST  POST POST POST POST POST POST POST POST  POST POST POST POST POST 
// DELETE DELETE DELETE DELETE DELETE DELETE DLETE DELETE DELETE DELETE DELETE DELETE DELETE DLETE DELETE DELETE DELETE DELETE DELETE DELETE DLETE

// ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR 

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

    return "<html><body><h1>" +
           std::to_string(code) + " " + text +
           "</h1></body></html>";
};

std::string HTTPResponseBuild::getStatusText(int code)
{
    switch (code)
    {
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

    return access(file.c_str(), R_OK) == 0;
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
