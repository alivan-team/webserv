#ifndef MULTIPART_PARSER_HPP
#define MULTIPART_PARSER_HPP

#include <string>
#include <map>
#include "externalStructures.hpp"
#include "MultipartPart.hpp"

class MultipartParser
{
private:
	const std::string& _buffer;
	size_t			_bodyOffset;
	size_t			_bodySize;
	std::string		_boundary;

public:
	MultipartParser(const std::string& buffer,
					size_t bodyOffset,
					size_t bodySize,
					const std::string& boundary);

	std::vector<MultipartPart> parse() const;
};

#endif