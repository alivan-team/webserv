#include "MultipartParser.hpp"

MultipartParser::MultipartParser()
	: _buffer(NULL),
	  _bodyOffset(0),
	  _bodySize(0),
	  _boundary()
{
}

MultipartParser::MultipartParser(
	const MultipartParser &other)
	: _buffer(other._buffer),
	  _bodyOffset(other._bodyOffset),
	  _bodySize(other._bodySize),
	  _boundary(other._boundary)
{
}

MultipartParser::MultipartParser(
	const std::string &buffer,
	size_t bodyOffset,
	size_t bodySize,
	const std::string &boundary)
	: _buffer(&buffer),
	  _bodyOffset(bodyOffset),
	  _bodySize(bodySize),
	  _boundary(boundary)
{
}

MultipartParser &MultipartParser::operator=(
	const MultipartParser &other)
{
	if (this != &other)
	{
		_buffer = other._buffer;
		_bodyOffset = other._bodyOffset;
		_bodySize = other._bodySize;
		_boundary = other._boundary;
	}

	return *this;
}

MultipartParser::~MultipartParser()
{
}

size_t MultipartParser::findBoundary(size_t start) const
{
	if (_buffer == NULL)
		return std::string::npos;

	const std::string delimiter = "--" + _boundary;
	const size_t bodyEnd = _bodyOffset + _bodySize;

	size_t position = _buffer->find(delimiter, start);

	while (position != std::string::npos)
	{
		if (position < _bodyOffset || position + delimiter.size() > bodyEnd)
			return std::string::npos;

		if (position == _bodyOffset)
			return position;

		if (position >= _bodyOffset + 2 && (*_buffer)[position - 2] == '\r' && (*_buffer)[position - 1] == '\n')
			return position;

		position = _buffer->find(delimiter, position + 1);
	}

	return std::string::npos;
}

bool MultipartParser::isClosingBoundary(size_t position) const
{
	if (_buffer == NULL)
		return false;

	const std::string delimiter = "--" + _boundary;
	const size_t closingStart = position + delimiter.size();
	const size_t bodyEnd = _bodyOffset + _bodySize;

	if (closingStart + 2 > bodyEnd)
		return false;

	return (*_buffer)[closingStart] == '-' && (*_buffer)[closingStart + 1] == '-';
}

size_t MultipartParser::findHeadersEnd(size_t start) const
{
	if (_buffer == NULL)
		return std::string::npos;

	const std::string headersEnd = "\r\n\r\n";
	const size_t bodyEnd = _bodyOffset + _bodySize;

	size_t position = _buffer->find(headersEnd, start);

	if (position == std::string::npos)
		return std::string::npos;

	if (position + headersEnd.size() > bodyEnd)
		return std::string::npos;

	return position;
}
void MultipartParser::parseContentDisposition(
	const std::string &headers,
	MultipartPart &part) const
{
	const std::string headerName = "Content-Disposition:";
	const size_t headerStart = headers.find(headerName);

	if (headerStart == std::string::npos)
		return;

	const size_t lineEnd = headers.find("\r\n", headerStart);

	std::string value;

	if (lineEnd == std::string::npos)
	{
		value = headers.substr(
			headerStart + headerName.size());
	}
	else
	{
		value = headers.substr(
			headerStart + headerName.size(),
			lineEnd - headerStart - headerName.size());
	}

	size_t namePosition = value.find("name=");

	if (namePosition != std::string::npos)
	{
		namePosition += 5;

		if (namePosition < value.size() && value[namePosition] == '"')
		{
			++namePosition;

			size_t nameEnd = value.find('"', namePosition);

			if (nameEnd != std::string::npos)
				part.setName(
					value.substr(
						namePosition,
						nameEnd - namePosition));
		}
	}

	size_t filenamePosition = value.find("filename=");

	if (filenamePosition != std::string::npos)
	{
		filenamePosition += 9;

		if (filenamePosition < value.size() && value[filenamePosition] == '"')
		{
			++filenamePosition;

			size_t filenameEnd =
				value.find('"', filenamePosition);

			if (filenameEnd != std::string::npos)
			{
				part.setFilename(
					value.substr(
						filenamePosition,
						filenameEnd - filenamePosition));
			}
		}
	}
}

std::vector<MultipartPart> MultipartParser::parse() const
{
	std::vector<MultipartPart> parts;

	if (_buffer == NULL)
		return parts;

	if (_boundary.empty())
		return parts;

	const std::string delimiter = "--" + _boundary;

	size_t boundaryPosition = findBoundary(_bodyOffset);

	if (boundaryPosition == std::string::npos)
		return parts;

	while (boundaryPosition != std::string::npos)
	{
		if (isClosingBoundary(boundaryPosition))
			break;

		const size_t afterBoundary =
			boundaryPosition + delimiter.size();

		const size_t bodyEnd =
			_bodyOffset + _bodySize;

		if (afterBoundary + 2 > bodyEnd)
			break;

		if ((*_buffer)[afterBoundary] != '\r' || (*_buffer)[afterBoundary + 1] != '\n')
			break;

		const size_t headersStart = afterBoundary + 2;

		const size_t headersEnd =
			findHeadersEnd(headersStart);

		if (headersEnd == std::string::npos)
			break;

		const std::string headers =
			_buffer->substr(
				headersStart,
				headersEnd - headersStart);

		MultipartPart part;

		parseContentDisposition(headers, part);

		const size_t dataOffset = headersEnd + 4;

		if (dataOffset > bodyEnd)
			break;

		const size_t nextBoundary =
			findBoundary(dataOffset);

		if (nextBoundary == std::string::npos)
			break;

		if (nextBoundary < dataOffset + 2)
			break;

		const size_t dataSize =
			nextBoundary - 2 - dataOffset;

		part.setDataOffset(dataOffset);
		part.setDataSize(dataSize);

		parts.push_back(part);

		if (isClosingBoundary(nextBoundary))
			break;

		boundaryPosition = nextBoundary;
	}

	return parts;
}