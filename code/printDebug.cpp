#include "./hpp/printDebug.hpp"
#include "./hpp/LocationConfig.hpp"
#include "./hpp/HTTPRequest.hpp"


void printDebug(std::string title, const bool& value) {

	std::cout << title;
    std::cout << (value? "true" : "false") << " ";
    std::cout << "\n";
}

void printDebug(std::string title, const int& value) {

	std::cout << title;
    std::cout << value << " ";
    std::cout << "\n";
}

void printDebug(std::string title, const size_t& value) {

	std::cout << title;
    std::cout << value;
}

void printDebug(std::string title, const std::string& str) {
	std::cout << title;
    // for (int value : str)
        std::cout << str << " ";

    std::cout << "\n";
}

void printDebug(std::string title, const std::vector<int>& vec) {
	std::cout << title;
    for (int value : vec)
        std::cout << value << " ";

    std::cout << "\n";
}

void printDebug(std::string title, const std::vector<unsigned int>& vec) {
	std::cout << title;
    for (int value : vec)
        std::cout << value << " ";

    std::cout << "\n";
}

void printDebug(std::string title, const std::vector<std::string>& vec) {
	std::cout << title;
    for (const auto& value : vec)
        std::cout << value << " ";

    std::cout << "\n";
}

void printDebug(std::string title, const std::map<int, std::string>& vec) {
	std::cout << title;
    for (const auto& value : vec) {
        std::cout << "\t code: " << value.first << " | path: " << value.second << " ";
        std::cout << "\n";
    }

}

void printDebug(std::string title, const std::map<std::string, std::string>& vec) {
	std::cout << title;
    for (const auto& value : vec) {
        std::cout << "\t code: " << value.first << " | path: " << value.second << " ";
        std::cout << "\n";
    }

}

void printDebug(std::string title, const LocationConfig& location) {
	std::cout << title << "\n";
	printDebug("\t getUriPath: ", location.getUriPath());
	if (location.isGetAllowed()) {
		printDebug( "\t\tGET: ",location.isGetAllowed());
	}
	if (location.isPostAllowed()) {
		printDebug( "\t\tPOST: ", location.isPostAllowed());
	}
	if (location.isDeleteAllowed()) {
		printDebug( "\t\tDELETE: ", location.isDeleteAllowed());
	}
	printDebug("\t getRedirect: \n\t\t Number: ", location.getRedirect()._number);
	printDebug("\t getRedirect: \n\t\t redirPath: ", location.getRedirect()._redirPath);
	printDebug("\t getCgiExtension: ", location.getCgiExtension());
	printDebug("\t getIndex: ",location.getIndex());
	printDebug("\t getRoot: ", location.getRoot());
	printDebug("\t getUploadStore: ",location.getUploadStore());
	if (location.hasRedirect()) {
		printDebug("\t hasRedirect: ", location.hasRedirect());
	}
	printDebug("\t getAutoIndex: ", location.getAutoIndex());
}

void printDebug(std::string title, const HTTPRequest &request) {
	std::cout << title << "\n";
	std::cout << "Method: ";
	if (request.getMethod() == Method::GET) {
		std::cout << "\t\tGET: \n";
	}
	if (request.getMethod() == Method::POST) {
		std::cout << "\t\tPOST: \n";
	}
	if (request.getMethod() == Method::DELETE) {
		std::cout << "\t\tDELETE: \n";
	}
	if (request.getMethod() == Method::UNKNOWN) {
		std::cout << "\t\tUNKNOWN: \n";
	}

	std::cout << "Path:\t\t" << request.getUri() << "\n";
	std::cout << "Query:\t\t" << request.getQuery() << "\n";
	std::cout << "Version:\t\t" << request.getVersion() << "\n";

	std::map<std::string, std::string> headers = request.getHeaders();
	printDebug("Headers:\n", headers);
}