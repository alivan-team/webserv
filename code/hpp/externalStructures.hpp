#ifndef EXTERNAL_STRUCRURES_HPP
#define EXTERNAL_STRUCRURES_HPP


#include <string>


struct AllowMethods{
	bool get;
	bool post;
	bool del;
};

struct Redirection{
	int	_number; // status 
	std::string _redirPath; // - redirect for this error
};

enum class Method
{
    UNKNOWN,
    GET,
    POST,
    DELETE,
    HEAD
};

enum BodyType
{
    BODY_UNKNOWN,
    BODY_MULTIPART,
    BODY_RAW
};
/;
#endif