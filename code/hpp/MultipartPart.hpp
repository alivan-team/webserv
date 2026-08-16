#ifndef MULTIPART_PART_HPP
#define MULTIPART_PART_HPP

#include <string>
#include "externalStructures.hpp"

class MultipartPart
{
private:
    size_t      _dataOffset;
    size_t      _dataSize;
    std::string _name;
    std::string _filename;

public:
    MultipartPart();
    ~MultipartPart() = default;


    void setDataOffset(size_t offset);
    void setDataSize(size_t size);
    void setName(const std::string& name);
    void setFilename(const std::string& filename);

    size_t getDataOffset() const;
    size_t getDataSize() const;

    const std::string& getName() const;
    const std::string& getFilename() const;

    bool hasFilename() const;
};

#endif