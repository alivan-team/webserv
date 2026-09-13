
#include "./hpp/HTTPResponseBuild.hpp"
#include "./hpp/HTTPResponse.hpp"

int HTTPResponseBuild::prepareRequestPath(const HTTPRequest &request, const ServerConfig &servConf, 
	std::string &path, const LocationConfig *&location) {

	try {
		path = urlDecoder(request.getPath());
	} catch (const std::exception &e) {
		return 400;
	}

	if (path.find('\0') != std::string::npos)
		return 400;

	if (containsParentTraversal(path)) 
		return 403;
	
	location = findBestLocation(path, servConf);

	if (location == NULL) 
		return 404;

	return 0;
}


HTTPResponse HTTPResponseBuild::build(const HTTPRequest &request, const ServerConfig &servConf) {

	Method method = request.getMethod();
	std::string version = request.getVersion();
	std::string path;
	const LocationConfig *location = NULL;
	
	if (version != "1.0" && version != "1.1")
		return makeErrorResponse(505, request, servConf);

	int errorCode = prepareRequestPath(request, servConf, path, location);

	if(errorCode != 0) 
		return makeErrorResponse(errorCode, request, servConf);

	if (location->hasRedirect()) {

		Redirection redirect = location->getRedirect();

		HTTPResponse res;
		res.setStatusCode(redirect._number);
		res.setStatus(getStatusText(redirect._number));
		res.setHeader("Location", redirect._redirPath);
		res.setHeader("Content-Length", "0");
		res.setHeader("Connection", decideConnection(request));
		res.setVersion(request.getVersion());

		return res;
	}

	switch (method) {

		case Method::GET:
			return handleGet(request, servConf, path, location);

		case Method::POST:
			return handlePost(request, servConf, path, location);

		case Method::DELETE:
			return handleDelete(request, servConf, path, location);

		default:
			return makeErrorResponse(501, request, servConf);
	}

};

// GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET GET

HTTPResponse HTTPResponseBuild::handleGet(
	const HTTPRequest &request, 
	const ServerConfig &servConf, 
	std::string &path,  
	const LocationConfig *&location)
{

	HTTPResponse res;

	if (!location->isGetAllowed()) {
		HTTPResponse erRes = makeErrorResponse(405, request, servConf);
		erRes.setHeader("Allow", buildAllowHeader(*location));
		return erRes;
	}

	std::string baseDir;
	std::string fullPath;
	std::string relativePath = path;

	if (path.compare(0, location->getUriPath().size(), location->getUriPath()) == 0)
		relativePath = path.substr(location->getUriPath().size());
	if (!location->getRoot().empty()) {
		baseDir = location->getRoot();
		fullPath = joinPath(location->getRoot(), relativePath);
	} else {
		if (servConf.getRoot().empty() || servConf.getRoot()[0].empty())
			return makeErrorResponse(500, request, servConf);

		baseDir = servConf.getRoot()[0];
		fullPath = joinPath(servConf.getRoot()[0], path);
	}

	if (!fileExists(fullPath))
		return makeErrorResponse(404, request, servConf);

	if (!pathInsideBase(baseDir, fullPath))
		return makeErrorResponse(403, request, servConf);

	if (isDirectory(fullPath)) {

		std::string indexPath = findIndexFile(fullPath, *location, servConf);

		if (!indexPath.empty()) {
			fullPath = indexPath;
			if (!pathInsideBase(baseDir, fullPath)) {
				return makeErrorResponse(403, request, servConf);
			}
		}
		else if (location->getAutoIndex()) {
			return buildAutoIndexPage(request, servConf, fullPath, request.getPath());
		} else {
			return makeErrorResponse(403, request, servConf);
		}
	}

	if (!canReadFile(fullPath))
		return makeErrorResponse(403, request, servConf);

	try {

		std::string body = readReadFile(fullPath);

		res.setStatusCode(200);
		res.setStatus(getStatusText(200));
		res.setHeader("Content-Type", getContentType(fullPath));
		res.setHeader("Content-Length", std::to_string(body.size()));
		res.setHeader("Connection", decideConnection(request));
		res.setVersion(request.getVersion());
		res.setBody(body);

		return res;
	} catch (const std::exception &e) {

		std::cerr << "Failed to serve file: " << fullPath << ": " << e.what() << std::endl;
		return makeErrorResponse(500, request, servConf);
	}
};

// POST POST POST POST POST POST POST POST POST POST POST POST POST POST POST POST  POST POST POST POST POST POST POST POST  POST POST POST POST POST
HTTPResponse HTTPResponseBuild::handlePost(
	const HTTPRequest &request,
	const ServerConfig &servConf, 
	std::string path,  
	const LocationConfig *&location)
{

	(void)path;
	if (!location->isPostAllowed()) {
		HTTPResponse erRes = makeErrorResponse(405, request, servConf);
		erRes.setHeader("Allow", buildAllowHeader(*location));
		return erRes;
	}

	const std::string &uploadStore = location->getUploadStore();

	if (uploadStore.empty() || !isDirectory(uploadStore) || access(uploadStore.c_str(), W_OK | X_OK) != 0)
	{
		return makeErrorResponse(500, request, servConf);
	}

	const std::string &requestBuffer =
		request.getRequestBuffer();

	const size_t bodyOffset =
		request.getBodyOffset();

	const size_t bodySize =
		request.getBodySize();

	if (bodyOffset > requestBuffer.size() || bodySize > requestBuffer.size() - bodyOffset)
	{
		return makeErrorResponse(400, request, servConf);
	}

	/*
	 * Determine which part of the current request body
	 * must be saved.
	 */
	size_t dataOffset = bodyOffset;
	size_t dataSize = bodySize;
	std::string filename;

	if (request.getBodyType() == BODY_MULTIPART)
	{
		MultipartParser parser(
			requestBuffer,
			bodyOffset,
			bodySize,
			request.getBoundary());

		const std::vector<MultipartPart> parts =
			parser.parse();

		bool foundFile = false;

		for (std::vector<MultipartPart>::const_iterator it =
				 parts.begin();
			 it != parts.end();
			 ++it)
		{
			if (!it->hasFilename())
				continue;

			dataOffset = it->getDataOffset();
			dataSize = it->getDataSize();

			filename = it->getFilename();

			foundFile = true;
			break;
		}

		if (!foundFile)
			return makeErrorResponse(400, request, servConf);

		if (dataOffset > requestBuffer.size() || dataSize > requestBuffer.size() - dataOffset)
		{
			return makeErrorResponse(400, request, servConf);
		}

		const size_t slash = filename.find_last_of("/\\");

		if (slash != std::string::npos)
			filename = filename.substr(slash + 1);

		if (filename.empty())
			return makeErrorResponse(400, request, servConf);
	}

	if (filename.empty())
	{
		std::ostringstream name;

		name << "upload-"
			 << std::time(NULL)
			 << "-"
			 << getpid();

		filename = name.str();
	}

	const std::string outputPath =
		joinPath(uploadStore, filename);

	const int outputFd = open(
		outputPath.c_str(),
		O_WRONLY | O_CREAT | O_TRUNC,
		0644);

	if (outputFd < 0)
		return makeErrorResponse(500, request, servConf);

	size_t written = 0;

	while (written < dataSize)
	{
		ssize_t result = write(
			outputFd,
			requestBuffer.data() + dataOffset + written,
			dataSize - written);

		if (result < 0 && errno == EINTR)
			continue;

		if (result <= 0)
		{
			close(outputFd); 
			std::error_code ec;
			std::filesystem::remove(outputPath, ec);

			return makeErrorResponse(500, request, servConf);
		}

		written += static_cast<size_t>(result);
	}

	close(outputFd);

	HTTPResponse res;

	std::string body =
		readReadFile("./site/www/post-result.html");

	res.setStatusCode(201);
	res.setStatus(getStatusText(201));
	res.setVersion(request.getVersion());
	res.setHeader(
		"Content-Type",
		getContentType("./site/www/post-result.html"));
	res.setHeader(
		"Content-Length",
		std::to_string(body.size()));
	res.setHeader(
		"Connection",
		decideConnection(request));
	res.setHeader(
		"Location",
		joinPath(
			location->getUriPath(),
			filename));
	res.setBody(body);

	return res;
}

// DELETE DELETE DELETE DELETE DELETE DELETE DLETE DELETE DELETE DELETE DELETE DELETE DELETE DLETE DELETE DELETE DELETE DELETE DELETE DELETE DLETE

HTTPResponse HTTPResponseBuild::handleDelete(
	const HTTPRequest &request, 
	const ServerConfig &servConf, 
	std::string &path,  
	const LocationConfig *&location)
{

	std::string baseDir;
	std::string fullPath;

	if (!location->isDeleteAllowed()) {
		HTTPResponse res = makeErrorResponse(405, request, servConf);
		res.setHeader("Allow", buildAllowHeader(*location));
		return res;
	}

	if (!location->getRoot().empty()) {
		baseDir = location->getRoot();
		std::string fileName = path.substr(location->getUriPath().size());
		fullPath = joinPath(baseDir, fileName);
	} else {
		if (servConf.getRoot().empty() || servConf.getRoot()[0].empty())
			return makeErrorResponse(500, request, servConf);
		baseDir = servConf.getRoot()[0];
		fullPath = joinPath(baseDir, path);
	}

	std::error_code ec;
	std::filesystem::file_status targetStatus = std::filesystem::symlink_status(fullPath, ec);

	if (ec) {
		if (ec == std::errc::no_such_file_or_directory || ec == std::errc::not_a_directory)
			return makeErrorResponse(404, request, servConf);
		if (ec == std::errc::permission_denied)
			return makeErrorResponse(403, request, servConf);
		return makeErrorResponse(500, request, servConf);
	}

	if (std::filesystem::is_directory(targetStatus))
		return makeErrorResponse(403, request, servConf);

	if (!deleteParentInsideBase(baseDir, fullPath))
		return makeErrorResponse(403, request, servConf);

	ec.clear();

	bool removed = std::filesystem::remove(fullPath, ec);

	if (ec) {
		if (ec == std::errc::no_such_file_or_directory)
			return makeErrorResponse(404, request, servConf);
		if (ec == std::errc::permission_denied || ec == std::errc::operation_not_permitted)
			return makeErrorResponse(403, request, servConf);

		return makeErrorResponse(500, request, servConf);
	}

	if (!removed)
		return makeErrorResponse(404, request, servConf);

	HTTPResponse res;
	// std::string body = readReadFile("./site/www/delete_page/index.html");

	res.setStatusCode(204);
	res.setStatus(getStatusText(204));
	res.setHeader("Content-Type", getContentType(fullPath));
	// res.setHeader("Content-Length", std::to_string(body.size())); // send body for successful deleting file...
	res.setHeader("Content-Length", "0"); // send body for successful deleting file...
	res.setHeader("Connection", decideConnection(request));
	res.setVersion(request.getVersion());
	res.setBody("");

	return res;
};

std::string HTTPResponseBuild::buildAllowHeader(const LocationConfig &location)
{

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

bool HTTPResponseBuild::deleteParentInsideBase(const std::string &base, const std::string &target) {

	std::error_code ec;
	std::filesystem::path resolvedBase = std::filesystem::weakly_canonical(base, ec);
	if(ec)
		return false;

	std::filesystem::path targetPath(target);
	std::filesystem::path parentString = targetPath.parent_path();

	ec.clear();
	std::filesystem::path resolvedParent = std::filesystem::weakly_canonical(parentString, ec);
	if (ec)
		return false;

	std::string canonicalBase = resolvedBase.string();
	std::string canonicalParent = resolvedParent.string();

	if (canonicalParent == canonicalBase)
		return true;

	if (!canonicalBase.empty() && canonicalBase[canonicalBase.size() - 1] != '/') 
		canonicalBase += '/';

	return canonicalParent.compare(0, canonicalBase.size(), canonicalBase) == 0;
}

// ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR

HTTPResponse HTTPResponseBuild::makeErrorResponse(int code, const HTTPRequest &request, const ServerConfig &servConf) {

	HTTPResponse res;

	std::string text = getStatusText(code);
	std::string body = buildErrorBody(code, servConf);

	res.setStatusCode(code);
	res.setVersion(request.getVersion());
	res.setStatus(text);
	res.setHeader("Content-Type", "text/html");
	res.setHeader("Content-Length", std::to_string(body.size()));
	res.setHeader("Connection", decideConnection(request));
	res.setBody(body);

	return res;
};

HTTPResponse HTTPResponseBuild::makeEarlyErrorResponse(int code, const ServerConfig &servConf) {

	HTTPRequest errRequest;

	errRequest.setVersion("1.1");
	errRequest.addHeader("Connection", "close");

	return makeErrorResponse(code, errRequest, servConf);
};

std::string HTTPResponseBuild::buildErrorBody(int code, const ServerConfig &servConf) {

	std::string error_message = getStatusText(code);

	if (servConf.hasErrorPage(code)) {

		std::string error_path = servConf.getOneErrorPage(code);
		std::string root = servConf.getRoot().back();
		std::string fullPath = joinPath(root, error_path);

		try {

			if (fileExists(fullPath) && canReadFile(fullPath)) {
				return readReadFile(fullPath);
			}
		} catch (const std::exception &e) {
			std::cerr << "Could not read custom error page " << error_path << ": " << e.what() << std::endl;
		}

		return "<!DOCTYPE html>\n"
			   "<html>\n"
			   "<head><title>" +
			   std::to_string(code) + " " + error_message +
			   "</title></head>\n"
			   "<body>\n"
			   "<h1>" +
			   std::to_string(code) + " " + error_message + "</h1>\n"
															"</body>\n"
															"</html>\n";
	}

	std::string text = getStatusText(code);

	return readReadFile("./site/www/error_pages/index.html");
};

std::string HTTPResponseBuild::getStatusText(int code) 
{
	switch (code) {
		case 200: return "OK";
		case 201: return "Created";
		case 204: return "No Content";
		case 301: return "Moved Permanently";
		case 400: return "Bad Request";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 413: return "Payload Too Large";
		case 431: return "Request Header Fields Too Large";
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 505: return "HTTP Version Not Supported";
		default: return "Error";
	}
}

std::string HTTPResponseBuild::decideConnection(const HTTPRequest &request) {

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

//  AUTO INDEX

HTTPResponse HTTPResponseBuild::buildAutoIndexPage(const HTTPRequest &request, const ServerConfig &servConf, const std::string &fullPath, const std::string &requestPath) {

	HTTPResponse res;

	DIR *dir = opendir(fullPath.c_str());

	if (dir == NULL)
		return makeErrorResponse(403, request, servConf);

	std::string body;

	body += "<!DOCTYPE html>\n";
	body += "<html>\n";
	body += "<head>\n";
	body += "	<meta charset=\"UTF-8\">\n";
	body += "	<title>Index of " + requestPath + "</title>\n";
	body += "</head>\n";
	body += "<body>\n";
	body += "	<h1>Index of " + requestPath + "</h1>\n";
	body += "	<hr>\n";
	body += "	<ul>\n";

	struct dirent *entry;

	while ((entry = readdir(dir)) != NULL) {
		std::string name = entry->d_name;
		// std::cout << "\t\t name --> " << name << std::endl;

		if (name == "." || name == ".." || name.front() == '.')
			continue;

		std::string extension;
		size_t dot = name.rfind('.');

		if (dot != std::string::npos)
			extension = name.substr(dot);

		std::string href = requestPath;

		if (href.empty() || href[href.size() - 1] != '/')
			href += "/";

		href += name;
		body += "		<li><a href=\"" + href + "\">" + name + "</a><br>";
	}

	body += "	</ul>\n";
	body += "	<hr>\n";
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

const LocationConfig *HTTPResponseBuild::findBestLocation(const std::string &path, const ServerConfig &servConf) {

	const std::vector<LocationConfig> &locations = servConf.getLocations();

	const LocationConfig *bestLoc = NULL;

	for (const auto &loc : locations) {
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

bool HTTPResponseBuild::startsWithLocation(const std::string &path, const std::string &loc) {

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

std::string HTTPResponseBuild::joinPath(const std::string &root, const std::string &path) {

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

bool HTTPResponseBuild::fileExists(const std::string &file) {

	struct stat st;
	return stat(file.c_str(), &st) == 0;
};

bool HTTPResponseBuild::canReadFile(const std::string &file) {
	
	return access(file.c_str(), R_OK) == 0;
};

bool HTTPResponseBuild::isDirectory(const std::string &path)
{
	struct stat st;

	if (stat(path.c_str(), &st) != 0)
		return false;

	return S_ISDIR(st.st_mode); // S_ISDIR(st.st_mode) asks -> "Do the type bits inside st_mode indicate a directory?"
}

std::string HTTPResponseBuild::findIndexFile(std::string fullPath, const LocationConfig &location, const ServerConfig &servConf) {
	
	const std::vector<std::string> *indexes;

	if (!location.getIndex().empty()) {
		indexes = &location.getIndex();
	} else {
		indexes = &servConf.getIndex();
	}

	for (size_t i = 0; i < indexes->size(); i++) {
		std::string indexCandidate = joinPath(fullPath, (*indexes)[i]);

		if (fileExists(indexCandidate) && canReadFile(indexCandidate))
			return indexCandidate;
	}

	return "";
};

std::string HTTPResponseBuild::readReadFile(const std::string &file) {

	std::ifstream inputFile(file.c_str(), std::ios::binary);

	if (!inputFile)
		throw std::runtime_error("Could not open file: " + file);

	std::ostringstream buffer;
	buffer << inputFile.rdbuf();

	if (inputFile.bad())
		throw std::runtime_error("Could not read file: " + file);

	return buffer.str();
};

bool HTTPResponseBuild::checkExtensionOfFile(const std::string &extension)
{

	if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" ||
		extension == ".gif" || extension == ".webp")
		return true;

	return false;
}

std::string HTTPResponseBuild::getContentType(const std::string &contenPath)
{

	size_t dot = contenPath.rfind('.');

	if (dot == std::string::npos)
		return "application/octet-stream";

	std::string extension = contenPath.substr(dot + 1);

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

bool HTTPResponseBuild::containsParentTraversal(const std::string &path)
{
	std::stringstream stream(path);
	std::string component;

	while (std::getline(stream, component, '/')) {
		if (component == "..")
			return true;
	}

	return false;
}

bool HTTPResponseBuild::pathInsideBase(const std::string &base, const std::string &target)
{

	std::error_code ec;
	std::filesystem::path resolveBase = std::filesystem::weakly_canonical(base, ec);
	if (ec)
		return false;
	ec.clear();
	std::filesystem::path resolveTarget = std::filesystem::weakly_canonical(target, ec);
	if (ec)
		return false;

	std::string canonicalBase = resolveBase.string();
	std::string canonicalTarget = resolveTarget.string();

	if (canonicalTarget == canonicalBase)
		return true;

	if (!canonicalBase.empty() && canonicalBase[canonicalBase.size() - 1] != '/')
		canonicalBase += '/';

	return canonicalTarget.compare(0, canonicalBase.size(), canonicalBase) == 0;
}

std::string HTTPResponseBuild::urlDecoder(std::string urlPath) {

	std::string decodedUrl;


	for (size_t i = 0; i < urlPath.length(); i++) {

		if (urlPath[i] == '%') {
			if (i + 2 >= urlPath.size())
				throw std::runtime_error("Invalid percent encoding");

			char first = urlPath[i + 1];
			char second = urlPath[i + 2];

			if (!std::isxdigit(static_cast<unsigned char>(first)) ||
				!std::isxdigit(static_cast<unsigned char>(second)))
			{
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
