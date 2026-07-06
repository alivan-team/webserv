
#include "./hpp/HTTPResponseBuild.hpp"

HTTPResponse HTTPResponseBuild::build(const HTTPRequest& request, const ServerConfig& servConf) {
	
     
    HTTPRequest req = request;
    ServerConfig ser = servConf;
	
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