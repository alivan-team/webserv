#include "MultipartPart.hpp"

#include <iostream>

MultipartPart::MultipartPart()
	: _name(),
	  _filename(),
	  _dataOffset(0),
	  _dataSize(0)
{
}

MultipartPart::MultipartPart(const MultipartPart& other)
	: _name(other._name),
	  _filename(other._filename),
	  _dataOffset(other._dataOffset),
	  _dataSize(other._dataSize)
{
}

MultipartPart& MultipartPart::operator=(const MultipartPart& other)
{
	if (this != &other)
	{
		_name = other._name;
		_filename = other._filename;
		_dataOffset = other._dataOffset;
		_dataSize = other._dataSize;
	}

	return *this;
}

MultipartPart::~MultipartPart()
{
}

void MultipartPart::setName(const std::string& name)
{
	_name = name;
}

void MultipartPart::setFilename(const std::string& filename)
{
	_filename = filename;
	// std::cout << "Filename: " << filename << std::endl;
}

void MultipartPart::setDataOffset(size_t offset)
{
	_dataOffset = offset;
}

void MultipartPart::setDataSize(size_t size)
{
	_dataSize = size;
}

const std::string& MultipartPart::getName() const
{
	return _name;
}

const std::string& MultipartPart::getFilename() const
{
	return _filename;
}

size_t MultipartPart::getDataOffset() const
{
	return _dataOffset;
}

size_t MultipartPart::getDataSize() const
{
	return _dataSize;
}

bool MultipartPart::hasFilename() const
{
	return !_filename.empty();
}