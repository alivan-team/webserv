#ifndef MULTIPART_PART_HPP
#define MULTIPART_PART_HPP

#include <string>
#include "externalStructures.hpp"

class MultipartPart
{
private:
	std::string _name;
	std::string _filename;

	size_t _dataOffset;
	size_t _dataSize;

public:
	MultipartPart();
	MultipartPart(const MultipartPart& other);
	MultipartPart& operator=(const MultipartPart& other);
	~MultipartPart();

	void setName(const std::string& name);
	void setFilename(const std::string& filename);

	void setDataOffset(size_t offset);
	void setDataSize(size_t size);

	const std::string& getName() const;
	const std::string& getFilename() const;

	size_t getDataOffset() const;
	size_t getDataSize() const;

	bool hasFilename() const;
};

#endif