// =================================
// Copyright (c) 2018 Seppo Laakko
// Distributed under the MIT license
// =================================

#ifndef CMAJOR_UTIL_BINARY_READER_INCLUDED
#define CMAJOR_UTIL_BINARY_READER_INCLUDED
#include <cmajor/util/MappedInputFile.hpp>

namespace cmajor { namespace util {

class BinaryReader
{
public:
    BinaryReader(const std::string& fileName_);
    const std::string& FileName() const { return fileName; }
    virtual ~BinaryReader();
    bool ReadBool();
    uint8_t ReadByte();
    int8_t ReadSByte();
    uint16_t ReadUShort();
    int16_t ReadShort();
    uint32_t ReadUInt();
    int32_t ReadInt();
    uint64_t ReadULong();
    int64_t ReadLong();
    float ReadFloat();
    double ReadDouble();
    char ReadChar();
    char16_t ReadWChar();
    char32_t ReadUChar();
    std::string ReadUtf8String();
    std::u16string ReadUtf16String();
    std::u32string ReadUtf32String();
    uint32_t ReadEncodedUInt();
    uint32_t Pos() const { return pos; }
    void Skip(uint32_t size);
private:
    std::string fileName;
    MappedInputFile file;
    const uint8_t* begin;
    const uint8_t* end;
    uint32_t pos;
    void CheckEof();
};

} } // namespace cmajor::util

#endif // CMAJOR_UTIL_BINARY_READER_INCLUDED
