
#include "./hpp/HTTPResponseBuild.hpp"
#include "./hpp/HTTPResponse.hpp"

HTTPResponse HTTPResponseBuild::build(const HTTPRequest& request, const ServerConfig& servConf) {
	

    Method method = request.getMethod();
    std::string path = request.getPath();
    std::string version = request.getVersion();

    // std::cout << "REQUEST: " << version << std::endl;

    if (version != "1.0" && version != "1.1")
        return makeErrorResponse(505, request, servConf);

    switch (method) {
        // CGI function 
        case Method::GET:
            return handleGet(request, servConf);

        case Method::POST:
            return handlePost(request, servConf);

        case Method::DELETE:
            return handleDelete(request, servConf);

        default:
            return makeErrorResponse(501, request, servConf);
    }

    return {};
};

// GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET

HTTPResponse HTTPResponseBuild::handleGet(const HTTPRequest& request, const ServerConfig& servConf) {

    HTTPResponse res;
    std::string path;
    // std::cout << "    :    Request    : \n"  << "code:  "<<  request.getUri() << std::endl;
    
    try { 
        path = urlDecoder(request.getPath());
    } catch (const std::exception& e) {
        return makeErrorResponse(400, request, servConf);
    }
    // std::cout << "\n    ~~~~~~~~~~~~~    GET    ~~~~~~~~~~~~~\n" << "-> path:  "<<  path << std::endl;

    if (path.find('\0') != std::string::npos)
        return makeErrorResponse(400, request, servConf);

    if (containsParentTraversal(path)) {
        // std::cout << "path in containsParentTraversal -> " << path << std::endl;
        return makeErrorResponse(403, request, servConf);
    }

    const LocationConfig* location = findBestLocation(path ,servConf);
    // std::cout << "location:  " <<  location->getUriPath() << std::endl;

    if (location == NULL) {
        // std::cout << "fileExists1" << std::endl;
        return makeErrorResponse(400, request, servConf);
    }

    if (!location->isGetAllowed()) {
        HTTPResponse erRes = makeErrorResponse(405, request, servConf);
        erRes.setHeader("Allow: ", buildAllowHeader(*location));
        return erRes;
    }

    // std::cout << "\t location->getUriPath().size():  " <<  location->getUriPath().size() << std::endl;
    // std::cout << "\t location->getUriPath():  " <<  location->getUriPath() << std::endl;
    // std::cout << "\t location->getRoot() :  " <<  location->getRoot() << std::endl;
    // std::cout << "\t path :  " <<  path << std::endl;

    std::string baseDir;
    std::string fullPath;
    std::string relativePath = path;

    if (path.compare(0, location->getUriPath().size(), location->getUriPath()) == 0)
        relativePath = path.substr(location->getUriPath().size());
    // std::cout << "\t relativePath :  " <<  relativePath << std::endl;
    if (!location->getRoot().empty()) {
        baseDir = location->getRoot();
        fullPath = joinPath(location->getRoot(), relativePath);
    } else {
        if (servConf.getRoot().empty() || servConf.getRoot()[0].empty()) 
            return makeErrorResponse(500, request, servConf);

        baseDir = servConf.getRoot()[0];
        fullPath = joinPath(servConf.getRoot()[0], path);
    }

    // std::cout << "GET ---> fullPath :  " <<  fullPath << std::endl;

    if (!fileExists(fullPath)) {
        // std::cout << "fileExists2645" << std::endl;
        return makeErrorResponse(404, request, servConf);
    }

    if (!pathInsideBase(baseDir, fullPath))
        return makeErrorResponse(403, request, servConf);

    if (isDirectory(fullPath)) {

        std::string indexPath = findIndexFile(fullPath, *location, servConf);

        // std::cout << "\t -> indexPath: " << indexPath << "\n\t -> fullPath: " << fullPath << "\n\t -> request.getPath(): " << request.getPath() << std::endl;

        if (!indexPath.empty()) {
            fullPath = indexPath;
            if (!pathInsideBase(baseDir, fullPath)) {
                return makeErrorResponse(403, request, servConf);
            }
        } else if (location->getAutoIndex()) {
            return buildAutoIndexPage(request, servConf, fullPath, request.getPath());
        } else {
            return makeErrorResponse(403, request, servConf);
        }

    }
    // std::cout << "canReadFile(fullPath) : " << canReadFile(fullPath) << std::endl;

    if (!canReadFile(fullPath))
        return makeErrorResponse(403, request, servConf);

    // CGI FUNCTION and QUESTIONS -> this one was suggested from ChatGPT :D 
    // if the path end on .py -> we search in out Locations for cgi-bin ->
    // if (isCgiFile(fullPath, location))
    //     return handleCgi(request, servConf, location, fullPath);

    try {

        // throw std::runtime_error("Testing catch");

        std::string body = readReadFile(fullPath);
        
        res.setStatusCode(200);
        res.setStatus(getStatusText(200));
        res.setHeader("Content-Type", getContentType(fullPath));
        res.setHeader("Content-Length", std::to_string(body.size()));
        res.setHeader("Connection", decideConnection(request));
        res.setVersion(request.getVersion());
        res.setBody(body);
    
        return res;    
    } catch (const std::exception& e) {

        std::cerr << "Failed to serve file: " << fullPath << ": " << e.what() << std::endl;
        return makeErrorResponse(500, request, servConf);
    }

};

// POST POST POST POST POST POST POST POST POST POST POST POST POST POST POST POST  POST POST POST POST POST POST POST POST  POST POST POST POST POST 
HTTPResponse HTTPResponseBuild::handlePost(const HTTPRequest& request, const ServerConfig& servConf) {

	// std::cout << "\n ------------------------>\n"  << "Received reqiest buffer: " << request.getRequestBuffer() << "\n <------------------------" << std::endl;
	// std::cout << " >>" << request.getRequestBuffer().data() + request.getBodyOffset()  << "<<\n;" ;
	const LocationConfig* location = findBestLocation(request.getPath(), servConf);
	if (location == NULL)
		return makeErrorResponse(404, request, servConf);
	if (!location->isPostAllowed())
		return makeErrorResponse(405, request, servConf);

	const std::string& uploadStore = location->getUploadStore();
	if (uploadStore.empty() || !isDirectory(uploadStore) || access(uploadStore.c_str(), W_OK | X_OK) != 0)
		return makeErrorResponse(500, request, servConf);

	const std::string& requestBuffer = request.getRequestBuffer();
	const size_t bodyOffset = request.getBodyOffset();
	const size_t bodySize = request.getBodySize();
	if (bodyOffset > requestBuffer.size() || bodySize > requestBuffer.size() - bodyOffset)
		return makeErrorResponse(400, request, servConf);

	std::string outputPath;
	int outputFd = -1;
	for (size_t attempt = 0; attempt < 100; ++attempt) {
		std::ostringstream name;
		name << "upload-" << std::time(NULL) << "-" << getpid() << "-" << attempt;
		outputPath = joinPath(uploadStore, name.str());
		outputFd = open(outputPath.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
		if (outputFd >= 0)
			break;
		if (errno != EEXIST)
			return makeErrorResponse(500, request, servConf);
	}
	if (outputFd < 0)
		return makeErrorResponse(500, request, servConf);

	size_t written = 0;
	while (written < bodySize) {
		ssize_t result = write(outputFd, requestBuffer.data() + bodyOffset + written, bodySize - written);
		if (result < 0 && errno == EINTR)
			continue;
		if (result <= 0) {
			close(outputFd);
			unlink(outputPath.c_str());
			return makeErrorResponse(500, request, servConf);
		}
		written += static_cast<size_t>(result);
	}
	close(outputFd);

	HTTPResponse res;
    std::string body = readReadFile("./site/www/post-result.html");
	
    res.setStatusCode(201);
	res.setStatus(getStatusText(201));
	res.setVersion(request.getVersion());
    res.setHeader("Content-Type", getContentType("./site/www/post-result.html"));
	res.setHeader("Content-Length", std::to_string(body.size()));
	res.setHeader("Connection", decideConnection(request));
	res.setHeader("Location", joinPath(location->getUriPath(), outputPath.substr(uploadStore.size())));
    res.setBody(body);

	return res;
}

// DELETE DELETE DELETE DELETE DELETE DELETE DLETE DELETE DELETE DELETE DELETE DELETE DELETE DLETE DELETE DELETE DELETE DELETE DELETE DELETE DLETE

HTTPResponse HTTPResponseBuild::handleDelete(const HTTPRequest& request, const ServerConfig& servConf) {

    std::string path;
    std::string baseDir;
    std::string fullPath;
    struct stat targetStat;

    // std::cout << "1 - DETELE -> request.getPath(): " << request.getPath() << std::endl;
    try {
        path = urlDecoder(request.getPath());
    } catch (const std::exception &e) {
        // std::cout << "??? catch ???" << std::endl;

        return makeErrorResponse(400, request, servConf);
    }

    // std::cout << "\t path :  " <<  path << std::endl;

    // std::cout << "2 - DETELE -> path: " << path << std::endl;

    // check for something after the \0 terminator? 
    if (path.find('\0') != std::string::npos) 
        return makeErrorResponse(400, request, servConf);

    // check for parent traversal /../.. -> not allowed
    // curl --path-as-is -v -X DELETE http://localhost:8080/upload/../test.txt
    //  with --paht-as-is -> so that is not normalized...
    if (containsParentTraversal(path))
        return makeErrorResponse(403 ,request ,servConf);

    // std::cout << "3 - DETELE -> request.getUri(): " << request.getUri() << std::endl;
    
    const LocationConfig* location = findBestLocation(path, servConf);
    // std::cout << "4 - DETELE -> location: " << location->getUriPath() << std::endl;
    // std::cout << "location->getUploadStore();" <<location->getUploadStore() <<std::endl;

    if (location == NULL)
        return makeErrorResponse(400, request, servConf);
    
    // DELETE allowed 
    // location root ?
    // then -> server root 

    if (!location->isDeleteAllowed()) {
        HTTPResponse res = makeErrorResponse(405, request, servConf);
        res.setHeader("Allow", buildAllowHeader(*location));
        return res;
    }

    // if (path.compare(0, location->getUriPath().size(), location->getUriPath()) == 0)
    //     relativePath = path.substr(location->getUriPath().size());
    // // std::cout << "\t relativePath :  " <<  relativePath << std::endl;
    // if (!location->getRoot().empty())
    //     fullPath = joinPath(location->getRoot(), relativePath);
    // else
    //     fullPath = joinPath(servConf.getRoot()[0], path);

    if (!location->getRoot().empty()) {
        // std::cout << "\t location->getRoot() :  " <<  location->getRoot() << std::endl;
        // std::cout << "\t location->getUriPath() :  " <<  location->getUriPath() << std::endl;
        // std::cout << "\t location->getPath() :  " <<  location->getRoot << std::endl;

        baseDir = location->getRoot();
        std::string fileName = path.substr(location->getUriPath().size());
        fullPath = joinPath(baseDir, fileName);
        // std::cout << "4-A -> - path.substr(location->getUriPath().size()): " << location->getUriPath() << std::endl;
        // std::cout << "4-BBB ---> fileName: " << fileName << std::endl;

    } else {
        if (servConf.getRoot().empty() || servConf.getRoot()[0].empty())
            return makeErrorResponse(500, request, servConf);
        baseDir = servConf.getRoot()[0];
        fullPath = joinPath(baseDir, path);
    }

    // std::cout << "5 - location->getRoot();: " << location->getRoot() << std::endl;
    // std::cout << "6 - servConf.getRoot()[0];: " << servConf.getRoot()[0] << std::endl;
    // std::cout << "7 - result: " << baseDir << std::endl;
    // std::cout << "8 - fullPath: " << fullPath << std::endl;
    // std::cout << "8 - location->getUriPath : " << location->getUriPath() << std::endl;
    // location->getUriPath

    if (lstat(fullPath.c_str(), &targetStat) == -1) {

        // std::cout << "9: " << std::endl;
        // std::cout << "lstat FAILED" << std::endl;
        // std::cout << "fullPath: " << fullPath << std::endl;
        // std::cout << "errno: " << errno << std::endl;
        // std::cout << "error: " << strerror(errno) << std::endl;

        if (errno == ENOENT || errno == ENOTDIR) 
            return makeErrorResponse(404, request, servConf);
        if (errno == EACCES)
            return makeErrorResponse(403, request, servConf);
        return makeErrorResponse(500, request, servConf);
    }

    if (S_ISDIR(targetStat.st_mode))
        return makeErrorResponse(403,request, servConf);
    // ?? is we want to reject totally symbolic links and check is we have a regular file
        // Check the subject and ask peers from Codam . :)
    // if (!S_ISREG(targetStat.st_mode) && !S_ISLNK(targetStat.st_mode)) 
    //     return makeErrorResponse(403, request, servConf);

    // std::cout << "before sending --- baseDir -> base :  " << baseDir << std::endl;     
    // std::cout << "before sending --- fullPath -> target :  " << fullPath << std::endl;     
    
    if (!deleteParentInsideBase(baseDir, fullPath))
        return makeErrorResponse(403, request, servConf);


    if (unlink(fullPath.c_str()) == -1) {

        // std::cerr << "unlink() failed for: " << fullPath << std::endl;
        // std::cerr << "errno: " << errno << std::endl;
        // std::cerr << "error: " << strerror(errno) << std::endl;

        if (errno == ENOENT || errno == ENOTDIR)
            return makeErrorResponse(404, request, servConf);

        if (errno == EACCES || errno == EPERM)
            return makeErrorResponse(403, request, servConf);

        if (errno == EISDIR)
            return makeErrorResponse(403, request, servConf);

        return makeErrorResponse(500, request, servConf);
    }

    HTTPResponse res;
    std::string body = readReadFile("./site/www/delete_page/index.html");

    res.setStatusCode(204);
    res.setStatus(getStatusText(204));
    res.setHeader("Content-Type", getContentType(fullPath));
    res.setHeader("Content-Length", std::to_string(body.size())); // send body for successful deleting file... 
    res.setHeader("Connection", decideConnection(request));

    res.setVersion(request.getVersion());
    res.setBody(body);

    return res;

};

std::string HTTPResponseBuild::buildAllowHeader(const LocationConfig& location) {

    std::string allow;

    if (location.isGetAllowed())
        allow += "GET";
    if (location.isPostAllowed()) {
        if (!allow.empty())
            allow += ", ";
        allow += "POST";
    }
    if (location.isDeleteAllowed()) {
        if (!allow.empty())
            allow += ", ";
        allow += "DELETE";
    }

    return allow;
};

bool HTTPResponseBuild::deleteParentInsideBase(const std::string& base, const std::string& target) {
    
    char resolvedBase[PATH_MAX];
    char resolvedParent[PATH_MAX];

    if (realpath(base.c_str(), resolvedBase) == NULL)
        return false;

    // realpath(base.c_str(), resolvedBase);
    // std::cout << "base AFTER ->resolvedBase<- realpath:  " << resolvedBase << std::endl;     
    // return false;

    size_t slash = target.find_last_of('/');

    std::string parentPath;

    if (slash == std::string::npos)
        parentPath = ".";
    else if (slash == 0)
        parentPath = "/";
    else
        parentPath = target.substr(0, slash);

    // std::cout << "base ->parentPath<-  :  " << parentPath << std::endl;     
    

    if (realpath(parentPath.c_str(), resolvedParent) == NULL)
        return false;
    // std::cout << "2 - base AFTER ->resolvedParent<- realpath():  " << resolvedParent << std::endl;     
    

    std::string canonicalBase(resolvedBase);
    std::string canonicalParent(resolvedParent);

    if (canonicalParent == canonicalBase)
        return true;

    if (!canonicalBase.empty() && canonicalBase[canonicalBase.size() - 1] != '/') {
        canonicalBase += '/';
    }

    return canonicalParent.compare(0, canonicalBase.size(), canonicalBase) == 0;
}

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
    res.setHeader("Content-Type", "text/html");
    res.setHeader("Content-Length", std::to_string(body.size()));
    res.setHeader("Connection", decideConnection(request));
    res.setBody(body);

    return res;
};

HTTPResponse HTTPResponseBuild::makeEarlyErrorResponse(int code, const ServerConfig& servConf) {

    HTTPRequest errRequest;

    errRequest.setVersion("1.1");
    errRequest.addHeader("Connection", "close");

    return makeErrorResponse(code, errRequest, servConf);
};


std::string  HTTPResponseBuild::buildErrorBody(int code, const ServerConfig& servConf) {


    // std::cout << "\nCode: " << code << std::endl;
    // std::cout << "servConf.hasErrorPage(code): " << servConf.hasErrorPage(code) << std::endl;
    // std::cout << "error_path from servConf: " << servConf.getOneErrorPage(code) << std::endl;
    // std::cout << "ROOT from servConf: " << servConf.getRoot().back() << std::endl;
    std::string error_message = getStatusText(code);

    if (servConf.hasErrorPage(code)) {

        std::string error_path = servConf.getOneErrorPage(code);
        std::string root = servConf.getRoot().back();
        std::string fullPath = joinPath(root, error_path);

        // std::cout << "\tfileExists(fullPath) : " << fileExists(fullPath) 
        // << "\n\t canReadFile(fullPath) : " <<  canReadFile(fullPath) << std::endl;
        try {

            // throw std::runtime_error("Forced error");
            if (fileExists(fullPath) && canReadFile(fullPath)) {
                // std::cout << "\tFULL PATH : " << fullPath << std::endl;
        
                // cgi -> child -> execute that file with query -> return result -> we put that result in body -> and send it          
                return readReadFile(fullPath);
            }
        } catch (const std::exception& e) {
            std::cerr << "Could not read custom error page " << error_path << ": " << e.what() << std::endl;
        }

        return "<!DOCTYPE html>\n"
                "<html>\n"
                "<head><title>" + std::to_string(code) + " " + error_message +
                "</title></head>\n"
                "<body>\n"
                "<h1>" + std::to_string(code) + " " + error_message + "</h1>\n"
                "</body>\n"
                "</html>\n";

    }


    std::string text = getStatusText(code);

    return readReadFile("./site/www/error_pages/index.html");
        //     "<html><body><h1>" +
        //    std::to_string(code) + " " + text +
        //    "</h1></body></html>";
};

std::string HTTPResponseBuild::getStatusText(int code)
{
    switch (code) {
        case 200: return "OK";
		case 201: return "Created";
        case 204: return "No Content";
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
    std::string connection;

    if (request.hasHeader("connection"))
        connection = toLower(trim(request.getHeader("connection")));

    if (version == "1.0") {
        if (connection == "keep-alive")
            return "keep-alive";

        return "close";
    }

    if (version == "1.1") {
        if (connection == "close")
            return "close";

        return "keep-alive";
    }

    return "close";
};



// HELPER HELPER HELPER HELPER HELPER HELPER HELPER HELPER HELPER HELPER HELPER HELPER HELPER HELPER
// HELPER ERROR Functions that maybe we can use later for other requests ??

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

        // if (checkExtensionOfFile(extension)) {
        //     body += "        <li><img src=\"" + href + "\" width=\"200\"><br></li>\n";
        // } else {
        body += "        <li><a href=\"" + href + "\">" + name + "</a><br>";
        // }
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

    for (const auto& loc : locations) {

        // std::cout << "LOCATIONS " << loc.getUriPath() << std::endl;
        // std::cout << "LOCATIONS path : " << path << std::endl;
        // std::cout << "LOCATIONS loc. : " << loc.getUriPath() << std::endl;
        
        if (startsWithLocation(path, loc.getUriPath())) {
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
    

    // std::cout << "root: " << root << std::endl;
    // std::cout << "path: " << path << std::endl;

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

    std::ifstream inputFile(file.c_str(), std::ios::binary);

    ////////////////////// HERE ////////////////////////////////

    if (!inputFile)
        throw std::runtime_error("Could not open file: " + file);

    std::ostringstream buffer;
    buffer << inputFile.rdbuf();

    if (inputFile.bad())
        throw std::runtime_error("Could not read file: " + file);

    return buffer.str();
};

bool HTTPResponseBuild::checkExtensionOfFile(const std::string& extension) {

    if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || 
        extension == ".gif" || extension == ".webp")
        return true;

    return false;
}

std::string HTTPResponseBuild::getContentType(const std::string& contenPath) {

    size_t dot = contenPath.rfind('.');

    if (dot == std::string::npos)
        return "application/octet-stream";

    std::string extension = contenPath.substr(dot + 1);
    // std::cout << " extension: --> " << extension << std::endl;

    if (extension == "html" || extension == "htm")
        return "text/html";
    if (extension == "css")
        return "text/css";
    if (extension == "js")
        return "application/javascript";
    if (extension == "json")
        return "application/json";
    if (extension == "txt")
        return "text/plain";
    if (extension == "png")
        return "image/png";
    if (extension == "jpg" || extension == "jpeg")
        return "image/jpeg";
    if (extension == "gif")
        return "image/gif";
    if (extension == "ico")
        return "image/x-icon";
    if (extension == "pdf")
        return "application/pdf";
    if (extension == "webp")
        return "image/webp";

    return "application/octet-stream";
}


bool HTTPResponseBuild::containsParentTraversal(const std::string& path)
{
    std::stringstream stream(path);
    std::string component;

    while (std::getline(stream, component, '/')) {
        if (component == "..")
            return true;
    }

    return false;
}

bool HTTPResponseBuild::pathInsideBase(const std::string& base, const std::string& target) {

    char resolveBase[PATH_MAX];
    char resolveTarget[PATH_MAX];

    if (realpath(base.c_str(), resolveBase) == NULL)
        return false;
    
    if (realpath(target.c_str(), resolveTarget) == NULL)
        return false;
    
    std::string canonicalBase(resolveBase);
    std::string canonicalTarget(resolveTarget);

    if (canonicalTarget == canonicalBase)
        return true;

    if (!canonicalBase.empty() && canonicalBase[canonicalBase.size() - 1] != '/')
        canonicalBase += '/';
    
    return canonicalTarget.compare(0, canonicalBase.size(), canonicalBase) == 0;
}


std::string HTTPResponseBuild::urlDecoder(std::string urlPath) {

    std::string decodedUrl;

    // std::cout << "urlDecoder HERE" << std::endl;

    for (size_t i = 0; i < urlPath.length(); i++) {

        if (urlPath[i] == '%') {
            if (i + 2 >= urlPath.size())
                throw std::runtime_error("Invalid percent encoding");

            char first = urlPath[i + 1];
            char second = urlPath[i + 2];

            if (!std::isxdigit(static_cast<unsigned char>(first)) ||
                !std::isxdigit(static_cast<unsigned char>(second))) {
                throw std::runtime_error("Invalid percent encoding");
            }

            std::string hex;
            hex += first;
            hex += second;

            char decodedChar = static_cast<char>(std::strtol(hex.c_str(), NULL, 16));

            decodedUrl += decodedChar;
            i += 2;
        } else {
            decodedUrl += urlPath[i];
        }
    }
    return decodedUrl;
}
