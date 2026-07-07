
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

    if (version != "HTTP/1.0" && version != "HTTP/1.1")
        return makeErrorResponse(505);

    switch (method)
    {
        case Method::GET:
            return handleGet(request, servConf);

        case Method::POST:
            return handlePost(request, servConf);

        case Method::DELETE:
            return handleDelete(request, servConf);

        case Method::HEAD:
            return handleHead(request, servConf);

        default:
            return makeErrorResponse(400);
    }



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

HTTPResponse HTTPResponseBuild::makeErrorResponse(int code) {

    HTTPResponse res;

    

    res.setStatusCode(code);
    res.setHeader();

};

HTTPResponse HTTPResponseBuild::handleGet(const HTTPRequest& request, const ServerConfig& servConf) {

};

// std::string HTTPResponseBuild::decideConnection(const HTTPRequest& request)
// {
//     std::string version = request.getVersion();

//     if (version == "HTTP/1.0")
//     {
//         if (request.hasHeader("Connection") &&
//             request.getHeader("Connection") == "keep-alive")
//             return "keep-alive";

//         return "close";
//     }

//     if (version == "HTTP/1.1")
//     {
//         if (request.hasHeader("Connection") &&
//             request.getHeader("Connection") == "close")
//             return "close";

//         return "keep-alive";
//     }

//     return "close";
// }