// =================================
// Copyright (c) 2017 Seppo Laakko
// Distributed under the MIT license
// =================================

#ifndef CMAJOR_BINDER_OPERATION_REPOSITORY_INCLUDED
#define CMAJOR_BINDER_OPERATION_REPOSITORY_INCLUDED
#include <cmajor/binder/BoundExpression.hpp>
#include <cmajor/symbols/Exception.hpp>

namespace cmajor { namespace binder {

using namespace cmajor::symbols;

class BoundCompileUnit;

class Operation
{
public:
    Operation(const std::u32string& groupName_, int arity_, BoundCompileUnit& boundCompileUnit_);
    virtual ~Operation();
    virtual void CollectViableFunctions(ContainerScope* containerScope_, const std::vector<std::unique_ptr<BoundExpression>>& arguments, std::unordered_set<FunctionSymbol*>& viableFunctions, 
        std::unique_ptr<Exception>& exception, const Span& span) = 0;
    const std::u32string& GroupName() const { return groupName; }
    int Arity() const { return arity; }
    SymbolTable* GetSymbolTable();
    BoundCompileUnit& GetBoundCompileUnit();
private:
    std::u32string groupName;
    int arity;
    BoundCompileUnit& boundCompileUnit;
};

class ArityOperation
{
public:
    void Add(Operation* operation);
    void CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, std::unordered_set<FunctionSymbol*>& viableFunctions, 
        std::unique_ptr<Exception>& exception, const Span& span);
private:
    std::vector<Operation*> operations;
};

class OperationGroup
{
public:
    void Add(Operation* operation);
    void CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, std::unordered_set<FunctionSymbol*>& viableFunctions, 
        std::unique_ptr<Exception>& exception, const Span& span);
private:
    std::vector<std::unique_ptr<ArityOperation>> arityOperations;
};

class OperationRepository
{
public:
    OperationRepository(BoundCompileUnit& boundCompileUnit_);
    void Add(Operation* operation);
    void CollectViableFunctions(const std::u32string& groupName, ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, 
        std::unordered_set<FunctionSymbol*>& viableFunctions, std::unique_ptr<Exception>& exception, const Span& span);
private:
    std::unordered_map<std::u32string, OperationGroup*> operationGroupMap;
    std::vector<std::unique_ptr<OperationGroup>> operationGroups;
    std::vector<std::unique_ptr<Operation>> operations;
};

} } // namespace cmajor::binder

#endif // CMAJOR_BINDER_OPERATION_REPOSITORY_INCLUDED