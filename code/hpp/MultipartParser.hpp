#ifndef MULTIPART_PARSER_HPP
#define MULTIPART_PARSER_HPP

#include <string>
#include <vector>
#include <map>
#include "externalStructures.hpp"
#include "MultipartPart.hpp"

class MultipartParser
{
private:
	const std::string* _buffer;
	size_t			_bodyOffset;
	size_t			  _bodySize;
	std::string		  _boundary;

	size_t findBoundary(size_t start) const;
	bool isClosingBoundary(size_t position) const;
	size_t findHeadersEnd(size_t start) const;

	void parseContentDisposition(
		const std::string& headers,
		MultipartPart& part
	) const;

public:
	MultipartParser();
	MultipartParser(
		const std::string& buffer,
		size_t bodyOffset,
		size_t bodySize,
		const std::string& boundary
	);

	MultipartParser(const MultipartParser& other);
	MultipartParser& operator=(const MultipartParser& other);
	~MultipartParser();

	std::vector<MultipartPart> parse() const;
};

#endif