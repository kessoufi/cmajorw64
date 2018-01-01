// =================================
// Copyright (c) 2018 Seppo Laakko
// Distributed under the MIT license
// =================================

#ifndef CMAJOR_PARSER_FILE_REGISTRY_INCLUDED
#define CMAJOR_PARSER_FILE_REGISTRY_INCLUDED
#include <cmajor/util/Error.hpp>
#include <memory>
#include <string>
#include <vector>
#include <stack>

namespace cmajor { namespace parser {

class FileRegistry
{
public:
    static void Init();
    static FileRegistry& Instance() { Assert(instance, "file registry not initialized"); return *instance; }
    uint32_t RegisterFile(const std::string& filePath);
    std::string GetFilePath(uint32_t filePathIndex);
    uint32_t GetNumberOfFilePaths();
    void PushObtainSystemFileIndeces();
    void PopObtainSystemFileIndeces();
private:
    static std::unique_ptr<FileRegistry> instance;
    std::vector<std::string> filePaths;
    FileRegistry();
    bool obtainSystemFileIndeces;
    std::stack<bool> obtainsSystemFileIndecesStack;
};

} } // namespace cmajor::parser

#endif // CMAJOR_PARSER_FILE_REGISTRY_INCLUDED
