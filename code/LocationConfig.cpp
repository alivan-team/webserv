#include "./hpp/LocationConfig.hpp"

class LocationConfig {

    private:
        std::string _uriPath;                  // "/upload" -> CGI
        Method methods;  // GET POST DELETE
        std::string upload_store;

	public:
        LocationConfig::LocationConfig(){

		};

        LocationConfig::~LocationConfig(){

		};

		// evaluations for Location cofig
		bool LocationConfig::checkUriPath(std::string uripath){

		};

        bool LocationConfig::setUriPath(std::string uripath){

		};

        bool LocationConfig::getUriPath(std::string uripath){
			
		};

        
};
