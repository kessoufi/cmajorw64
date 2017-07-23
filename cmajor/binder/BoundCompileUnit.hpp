// =================================
// Copyright (c) 2017 Seppo Laakko
// Distributed under the MIT license
// =================================

#ifndef CMAJOR_BINDER_BOUND_COMPILE_UNIT_INCLUDED
#define CMAJOR_BINDER_BOUND_COMPILE_UNIT_INCLUDED
#include <cmajor/binder/BoundNode.hpp>
#include <cmajor/symbols/Module.hpp>
#include <cmajor/ast/CompileUnit.hpp>

namespace cmajor { namespace binder {

using namespace cmajor::symbols;

class BoundCompileUnit : public BoundNode
{
public:
    BoundCompileUnit(Module& module_, CompileUnitNode* compileUnitNode_);
    void Load(Emitter& emitter) override;
    void Store(Emitter& emitter) override;
    void Accept(BoundNodeVisitor& visitor) override;
    Module& GetModule() { return module; }
    SymbolTable& GetSymbolTable() { return symbolTable; }
    CompileUnitNode* GetCompileUnitNode() const { return compileUnitNode; }
    void AddFileScope(FileScope* fileScope);
    FileScope* FirstFileScope() const { Assert(!fileScopes.empty(), "file scopes empty");  return fileScopes.front().get(); }
    const std::vector<std::unique_ptr<FileScope>>& FileScopes() const { return fileScopes; }
    void AddBoundNode(std::unique_ptr<BoundNode>&& boundNode);
    const std::vector<std::unique_ptr<BoundNode>>& BoundNodes() const { return boundNodes; }
    FunctionSymbol* GetConversion(TypeSymbol* sourceType, TypeSymbol* targetType);
    const std::string& LLFilePath() const { return llFilePath; }
    const std::string& OptLLFilePath() const { return optLLFilePath; }
    const std::string& ObjectFilePath() const { return objectFilePath; }
    void SetHasGotos() { hasGotos = true; }
    bool HasGotos() const { return hasGotos; }
private:
    Module& module;
    SymbolTable& symbolTable;
    CompileUnitNode* compileUnitNode;
    std::string llFilePath;
    std::string optLLFilePath;
    std::string objectFilePath;
    std::vector<std::unique_ptr<FileScope>> fileScopes;
    std::vector<std::unique_ptr<BoundNode>> boundNodes;
    bool hasGotos;
};

} } // namespace cmajor::binder

#endif // CMAJOR_BINDER_BOUND_COMPILE_UNIT_INCLUDED
