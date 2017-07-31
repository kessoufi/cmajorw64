// =================================
// Copyright (c) 2017 Seppo Laakko
// Distributed under the MIT license
// =================================

#include <cmajor/binder/OperationRepository.hpp>
#include <cmajor/binder/BoundCompileUnit.hpp>
#include <cmajor/binder/BoundClass.hpp>
#include <cmajor/binder/BoundFunction.hpp>
#include <cmajor/binder/BoundStatement.hpp>
#include <cmajor/binder/ExpressionBinder.hpp>
#include <cmajor/binder/OverloadResolution.hpp>
#include <cmajor/symbols/BasicTypeOperation.hpp>
#include <cmajor/symbols/Exception.hpp>
#include <cmajor/symbols/ClassTypeSymbol.hpp>
#include <cmajor/ast/Identifier.hpp>
#include <cmajor/ast/Expression.hpp>
#include <cmajor/util/Unicode.hpp>
#include <llvm/IR/Constant.h>

namespace cmajor { namespace binder {

using namespace cmajor::unicode;

class PointerDefaultCtor : public FunctionSymbol
{
public:
    PointerDefaultCtor(TypeSymbol* type_, const Span& span);
    SymbolAccess DeclaredAccess() const override { return SymbolAccess::public_; }
    void GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags) override;
    bool IsBasicTypeOperation() const override { return true; }
private:
    TypeSymbol* type;
    llvm::Value* nullValue;
};

PointerDefaultCtor::PointerDefaultCtor(TypeSymbol* type_, const Span& span) : FunctionSymbol(span, U"@constructor"), type(type_), nullValue(nullptr)
{
    SetGroupName(U"@constructor");
    SetAccess(SymbolAccess::public_);
    ParameterSymbol* thisParam = new ParameterSymbol(span, U"this");
    thisParam->SetType(type->AddPointer(span));
    AddMember(thisParam);
    ComputeName();
}

void PointerDefaultCtor::GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags)
{
    Assert(genObjects.size() == 1, "default constructor needs one object");
    if (!nullValue)
    {
        nullValue = llvm::Constant::getNullValue(type->IrType(emitter));
    }
    emitter.Stack().Push(nullValue);
    genObjects[0]->Store(emitter, OperationFlags::none);
}

class PointerDefaultConstructorOperation : public Operation
{
public:
    PointerDefaultConstructorOperation(BoundCompileUnit& boundCompileUnit_);
    void CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, std::unordered_set<FunctionSymbol*>& viableFunctions, 
        std::unique_ptr<Exception>& exception, const Span& span) override;
private:
    std::unordered_map<TypeSymbol*, FunctionSymbol*> functionMap;
    std::vector<std::unique_ptr<FunctionSymbol>> functions;
};

PointerDefaultConstructorOperation::PointerDefaultConstructorOperation(BoundCompileUnit& boundCompileUnit_) : Operation(U"@constructor", 1, boundCompileUnit_)
{
}

void PointerDefaultConstructorOperation::CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, 
    std::unordered_set<FunctionSymbol*>& viableFunctions, std::unique_ptr<Exception>& exception, const Span& span)
{
    TypeSymbol* type = arguments[0]->GetType();
    if (type->PointerCount() <= 1) return;
    TypeSymbol* pointerType = type->RemovePointer(span);
    FunctionSymbol* function = functionMap[pointerType];
    if (!function)
    {
        function = new PointerDefaultCtor(pointerType, span);
        function->SetSymbolTable(GetSymbolTable());
        function->SetParent(&GetSymbolTable()->GlobalNs());
        functionMap[type] = function;
        functions.push_back(std::unique_ptr<FunctionSymbol>(function));
    }
    viableFunctions.insert(function);
}

class PointerCopyCtor : public FunctionSymbol
{
public:
    PointerCopyCtor(TypeSymbol* type_, const Span& span);
    SymbolAccess DeclaredAccess() const override { return SymbolAccess::public_; }
    void GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags) override;
    bool IsBasicTypeOperation() const override { return true; }
private:
    TypeSymbol* type;
};

PointerCopyCtor::PointerCopyCtor(TypeSymbol* type_, const Span& span) : FunctionSymbol(span, U"@constructor"), type(type_)
{
    SetGroupName(U"@constructor");
    SetAccess(SymbolAccess::public_);
    ParameterSymbol* thisParam = new ParameterSymbol(span, U"this");
    thisParam->SetType(type->AddPointer(span));
    AddMember(thisParam);
    ParameterSymbol* thatParam = new ParameterSymbol(span, U"that");
    thatParam->SetType(type);
    AddMember(thatParam);
    ComputeName();
}

void PointerCopyCtor::GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags)
{
    Assert(genObjects.size() == 2, "copy constructor needs two objects");
    genObjects[1]->Load(emitter, OperationFlags::none);
    genObjects[0]->Store(emitter, OperationFlags::none);
}

class PointerCopyConstructorOperation : public Operation
{
public:
    PointerCopyConstructorOperation(BoundCompileUnit& boundCompileUnit_);
    void CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, std::unordered_set<FunctionSymbol*>& viableFunctions, 
        std::unique_ptr<Exception>& exception, const Span& span) override;
private:
    std::unordered_map<TypeSymbol*, FunctionSymbol*> functionMap;
    std::vector<std::unique_ptr<FunctionSymbol>> functions;
};

PointerCopyConstructorOperation::PointerCopyConstructorOperation(BoundCompileUnit& boundCompileUnit_) : Operation(U"@constructor", 2, boundCompileUnit_)
{
}

void PointerCopyConstructorOperation::CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, 
    std::unordered_set<FunctionSymbol*>& viableFunctions, std::unique_ptr<Exception>& exception, const Span& span)
{
    TypeSymbol* type = arguments[0]->GetType();
    if (type->PointerCount() <= 1) return;
    TypeSymbol* pointerType = type->RemovePointer(span);
    FunctionSymbol* function = functionMap[pointerType];
    if (!function)
    {
        function = new PointerCopyCtor(pointerType, span);
        function->SetSymbolTable(GetSymbolTable());
        function->SetParent(&GetSymbolTable()->GlobalNs());
        functionMap[type] = function;
        functions.push_back(std::unique_ptr<FunctionSymbol>(function));
    }
    viableFunctions.insert(function);
}

class PointerCopyAssignment : public FunctionSymbol
{
public:
    PointerCopyAssignment(TypeSymbol* type_, TypeSymbol* voidType_, const Span& span);
    SymbolAccess DeclaredAccess() const override { return SymbolAccess::public_; }
    void GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags) override;
    bool IsBasicTypeOperation() const override { return true; }
private:
    TypeSymbol* type;
};

PointerCopyAssignment::PointerCopyAssignment(TypeSymbol* type_, TypeSymbol* voidType_, const Span& span) : FunctionSymbol(span, U"operator="), type(type_)
{
    SetGroupName(U"operator=");
    SetAccess(SymbolAccess::public_);
    ParameterSymbol* thisParam = new ParameterSymbol(span, U"this");
    thisParam->SetType(type->AddPointer(span));
    AddMember(thisParam);
    ParameterSymbol* thatParam = new ParameterSymbol(span, U"that");
    thatParam->SetType(type);
    AddMember(thatParam);
    SetReturnType(voidType_);
    ComputeName();
}

void PointerCopyAssignment::GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags)
{
    Assert(genObjects.size() == 2, "copy assignment needs two objects");
    genObjects[1]->Load(emitter, OperationFlags::none);
    genObjects[0]->Store(emitter, OperationFlags::none);
}

class PointerCopyAssignmentOperation : public Operation
{
public:
    PointerCopyAssignmentOperation(BoundCompileUnit& boundCompileUnit_);
    void CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, std::unordered_set<FunctionSymbol*>& viableFunctions, 
        std::unique_ptr<Exception>& exception, const Span& span) override;
private:
    std::unordered_map<TypeSymbol*, FunctionSymbol*> functionMap;
    std::vector<std::unique_ptr<FunctionSymbol>> functions;
};

PointerCopyAssignmentOperation::PointerCopyAssignmentOperation(BoundCompileUnit& boundCompileUnit_) : Operation(U"operator=", 2, boundCompileUnit_)
{
}

void PointerCopyAssignmentOperation::CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, 
    std::unordered_set<FunctionSymbol*>& viableFunctions, std::unique_ptr<Exception>& exception, const Span& span)
{
    TypeSymbol* type = arguments[0]->GetType();
    if (type->PointerCount() <= 1) return;
    TypeSymbol* pointerType = type->RemovePointer(span);
    FunctionSymbol* function = functionMap[pointerType];
    if (!function)
    {
        function = new PointerCopyAssignment(pointerType, GetSymbolTable()->GetTypeByName(U"void"), span);
        function->SetSymbolTable(GetSymbolTable());
        function->SetParent(&GetSymbolTable()->GlobalNs());
        functionMap[type] = function;
        functions.push_back(std::unique_ptr<FunctionSymbol>(function));
    }
    viableFunctions.insert(function);
}

class PointerReturn : public FunctionSymbol
{
public:
    PointerReturn(TypeSymbol* type_, const Span& span);
    SymbolAccess DeclaredAccess() const override { return SymbolAccess::public_; }
    void GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags) override;
    bool IsBasicTypeOperation() const override { return true; }
private:
    TypeSymbol* type;
};

PointerReturn::PointerReturn(TypeSymbol* type_, const Span& span) : FunctionSymbol(span, U"@return"), type(type_)
{
    SetGroupName(U"@return");
    SetAccess(SymbolAccess::public_);
    ParameterSymbol* valueParam = new ParameterSymbol(span, U"value");
    valueParam->SetType(type);
    AddMember(valueParam);
    SetReturnType(type);
    ComputeName();
}

void PointerReturn::GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags)
{
    Assert(genObjects.size() == 1, "return needs one object");
    genObjects[0]->Load(emitter, OperationFlags::none);
    if ((flags & OperationFlags::leaveFirstArg) != OperationFlags::none)
    {
        emitter.Stack().Dup();
        llvm::Value* ptr = emitter.Stack().Pop();
        emitter.SaveObjectPointer(ptr);
    }
}

class PointerReturnOperation : public Operation
{
public:
    PointerReturnOperation(BoundCompileUnit& boundCompileUnit_);
    void CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, std::unordered_set<FunctionSymbol*>& viableFunctions, 
        std::unique_ptr<Exception>& exception, const Span& span) override;
private:
    std::unordered_map<TypeSymbol*, FunctionSymbol*> functionMap;
    std::vector<std::unique_ptr<FunctionSymbol>> functions;
};

PointerReturnOperation::PointerReturnOperation(BoundCompileUnit& boundCompileUnit_) : Operation(U"@return", 1, boundCompileUnit_)
{
}

void PointerReturnOperation::CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, 
    std::unordered_set<FunctionSymbol*>& viableFunctions, std::unique_ptr<Exception>& exception, const Span& span)
{
    TypeSymbol* type = arguments[0]->GetType();
    if (!type->IsPointerType()) return;
    FunctionSymbol* function = functionMap[type];
    if (!function)
    {
        function = new PointerReturn(type, span);
        function->SetSymbolTable(GetSymbolTable());
        function->SetParent(&GetSymbolTable()->GlobalNs());
        functionMap[type] = function;
        functions.push_back(std::unique_ptr<FunctionSymbol>(function));
    }
    viableFunctions.insert(function);
}

class PointerPlusOffset : public FunctionSymbol
{
public:
    PointerPlusOffset(TypeSymbol* pointerType_, TypeSymbol* longType_, const Span& span);
    SymbolAccess DeclaredAccess() const override { return SymbolAccess::public_; }
    void GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags) override;
    bool IsBasicTypeOperation() const override { return true; }
};

PointerPlusOffset::PointerPlusOffset(TypeSymbol* pointerType_, TypeSymbol* longType_, const Span& span) : FunctionSymbol(span, U"operator+")
{
    SetGroupName(U"operator+");
    SetAccess(SymbolAccess::public_);
    ParameterSymbol* leftParam = new ParameterSymbol(span, U"left");
    leftParam->SetType(pointerType_);
    AddMember(leftParam);
    ParameterSymbol* rightParam = new ParameterSymbol(span, U"right");
    rightParam->SetType(longType_);
    AddMember(rightParam);
    SetReturnType(pointerType_);
    ComputeName();
}

void PointerPlusOffset::GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags)
{
    Assert(genObjects.size() == 2, "operator+ needs two objects");
    genObjects[0]->Load(emitter, OperationFlags::none);
    llvm::Value* left = emitter.Stack().Pop();
    genObjects[1]->Load(emitter, OperationFlags::none);
    llvm::Value* right = emitter.Stack().Pop();
    emitter.Stack().Push(emitter.Builder().CreateGEP(left, right));
}

class PointerPlusOffsetOperation : public Operation
{
public:
    PointerPlusOffsetOperation(BoundCompileUnit& boundCompileUnit_);
    void CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, std::unordered_set<FunctionSymbol*>& viableFunctions, 
        std::unique_ptr<Exception>& exception, const Span& span) override;
private:
    std::unordered_map<TypeSymbol*, FunctionSymbol*> functionMap;
    std::vector<std::unique_ptr<FunctionSymbol>> functions;
};

PointerPlusOffsetOperation::PointerPlusOffsetOperation(BoundCompileUnit& boundCompileUnit_) : Operation(U"operator+", 2, boundCompileUnit_)
{
}

void PointerPlusOffsetOperation::CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, 
    std::unordered_set<FunctionSymbol*>& viableFunctions, std::unique_ptr<Exception>& exception, const Span& span)
{
    TypeSymbol* leftType = arguments[0]->GetType();
    if (!leftType->IsPointerType()) return;
    FunctionSymbol* function = functionMap[leftType];
    if (!function)
    {
        function = new PointerPlusOffset(leftType, GetSymbolTable()->GetTypeByName(U"long"), span);
        function->SetSymbolTable(GetSymbolTable());
        function->SetParent(&GetSymbolTable()->GlobalNs());
        functionMap[leftType] = function;
        functions.push_back(std::unique_ptr<FunctionSymbol>(function));
    }
    viableFunctions.insert(function);
}

class OffsetPlusPointer : public FunctionSymbol
{
public:
    OffsetPlusPointer(TypeSymbol* longType_, TypeSymbol* pointerType_, const Span& span);
    SymbolAccess DeclaredAccess() const override { return SymbolAccess::public_; }
    void GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags) override;
    bool IsBasicTypeOperation() const override { return true; }
};

OffsetPlusPointer::OffsetPlusPointer(TypeSymbol* longType_, TypeSymbol* pointerType_, const Span& span) : FunctionSymbol(span, U"operator+")
{
    SetGroupName(U"operator+");
    SetAccess(SymbolAccess::public_);
    ParameterSymbol* leftParam = new ParameterSymbol(span, U"left");
    leftParam->SetType(longType_);
    AddMember(leftParam);
    ParameterSymbol* rightParam = new ParameterSymbol(span, U"right");
    rightParam->SetType(pointerType_);
    AddMember(rightParam);
    SetReturnType(pointerType_);
    ComputeName();
}

void OffsetPlusPointer::GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags)
{
    Assert(genObjects.size() == 2, "operator+ needs two objects");
    genObjects[0]->Load(emitter, OperationFlags::none);
    llvm::Value* left = emitter.Stack().Pop();
    genObjects[1]->Load(emitter, OperationFlags::none);
    llvm::Value* right = emitter.Stack().Pop();
    emitter.Stack().Push(emitter.Builder().CreateGEP(right, left));
}

class OffsetPlusPointerOperation : public Operation
{
public:
    OffsetPlusPointerOperation(BoundCompileUnit& boundCompileUnit_);
    void CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, std::unordered_set<FunctionSymbol*>& viableFunctions, 
        std::unique_ptr<Exception>& exception, const Span& span) override;
private:
    std::unordered_map<TypeSymbol*, FunctionSymbol*> functionMap;
    std::vector<std::unique_ptr<FunctionSymbol>> functions;
};

OffsetPlusPointerOperation::OffsetPlusPointerOperation(BoundCompileUnit& boundCompileUnit_) : Operation(U"operator+", 2, boundCompileUnit_)
{
}

void OffsetPlusPointerOperation::CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, 
    std::unordered_set<FunctionSymbol*>& viableFunctions, std::unique_ptr<Exception>& exception, const Span& span)
{
    TypeSymbol* rightType = arguments[1]->GetType();
    if (!rightType->IsPointerType()) return;
    TypeSymbol* leftType = GetSymbolTable()->GetTypeByName(U"long");
    FunctionSymbol* function = functionMap[rightType];
    if (!function)
    {
        function = new OffsetPlusPointer(leftType, rightType, span);
        function->SetSymbolTable(GetSymbolTable());
        function->SetParent(&GetSymbolTable()->GlobalNs());
        functionMap[rightType] = function;
        functions.push_back(std::unique_ptr<FunctionSymbol>(function));
    }
    viableFunctions.insert(function);
}

class PointerMinusOffset : public FunctionSymbol
{
public:
    PointerMinusOffset(TypeSymbol* pointerType_, TypeSymbol* longType_, const Span& span);
    SymbolAccess DeclaredAccess() const override { return SymbolAccess::public_; }
    void GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags) override;
    bool IsBasicTypeOperation() const override { return true; }
};

PointerMinusOffset::PointerMinusOffset(TypeSymbol* pointerType_, TypeSymbol* longType_, const Span& span) : FunctionSymbol(span, U"operator-")
{
    SetGroupName(U"operator-");
    SetAccess(SymbolAccess::public_);
    ParameterSymbol* leftParam = new ParameterSymbol(span, U"left");
    leftParam->SetType(pointerType_);
    AddMember(leftParam);
    ParameterSymbol* rightParam = new ParameterSymbol(span, U"right");
    rightParam->SetType(longType_);
    AddMember(rightParam);
    SetReturnType(pointerType_);
    ComputeName();
}

void PointerMinusOffset::GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags)
{
    Assert(genObjects.size() == 2, "operator- needs two objects");
    genObjects[0]->Load(emitter, OperationFlags::none);
    llvm::Value* left = emitter.Stack().Pop();
    genObjects[1]->Load(emitter, OperationFlags::none);
    llvm::Value* right = emitter.Stack().Pop();
    llvm::Value* offset = emitter.Builder().CreateNeg(right);
    emitter.Stack().Push(emitter.Builder().CreateGEP(left, offset));
}

class PointerMinusOffsetOperation : public Operation
{
public:
    PointerMinusOffsetOperation(BoundCompileUnit& boundCompileUnit_);
    void CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, std::unordered_set<FunctionSymbol*>& viableFunctions, 
        std::unique_ptr<Exception>& exception, const Span& span) override;
private:
    std::unordered_map<TypeSymbol*, FunctionSymbol*> functionMap;
    std::vector<std::unique_ptr<FunctionSymbol>> functions;
};

PointerMinusOffsetOperation::PointerMinusOffsetOperation(BoundCompileUnit& boundCompileUnit_) : Operation(U"operator-", 2, boundCompileUnit_)
{
}

void PointerMinusOffsetOperation::CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, 
    std::unordered_set<FunctionSymbol*>& viableFunctions, std::unique_ptr<Exception>& exception, const Span& span)
{
    TypeSymbol* leftType = arguments[0]->GetType();
    if (!leftType->IsPointerType()) return;
    FunctionSymbol* function = functionMap[leftType];
    if (!function)
    {
        function = new PointerMinusOffset(leftType, GetSymbolTable()->GetTypeByName(U"long"), span);
        function->SetSymbolTable(GetSymbolTable());
        function->SetParent(&GetSymbolTable()->GlobalNs());
        functionMap[leftType] = function;
        functions.push_back(std::unique_ptr<FunctionSymbol>(function));
    }
    viableFunctions.insert(function);
}

class PointerMinusPointer : public FunctionSymbol
{
public:
    PointerMinusPointer(TypeSymbol* pointerType_, TypeSymbol* longType_, const Span& span);
    SymbolAccess DeclaredAccess() const override { return SymbolAccess::public_; }
    void GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags) override;
    bool IsBasicTypeOperation() const override { return true; }
};

PointerMinusPointer::PointerMinusPointer(TypeSymbol* pointerType_, TypeSymbol* longType_, const Span& span) : FunctionSymbol(span, U"operator-")
{
    SetGroupName(U"operator-");
    SetAccess(SymbolAccess::public_);
    ParameterSymbol* leftParam = new ParameterSymbol(span, U"left");
    leftParam->SetType(pointerType_);
    AddMember(leftParam);
    ParameterSymbol* rightParam = new ParameterSymbol(span, U"right");
    rightParam->SetType(pointerType_);
    AddMember(rightParam);
    SetReturnType(longType_);
    ComputeName();
}

void PointerMinusPointer::GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags)
{
    Assert(genObjects.size() == 2, "operator- needs two objects");
    genObjects[0]->Load(emitter, OperationFlags::none);
    llvm::Value* left = emitter.Stack().Pop();
    genObjects[1]->Load(emitter, OperationFlags::none);
    llvm::Value* right = emitter.Stack().Pop();
    emitter.Stack().Push(emitter.Builder().CreatePtrDiff(left, right));
}

class PointerMinusPointerOperation : public Operation
{
public:
    PointerMinusPointerOperation(BoundCompileUnit& boundCompileUnit_);
    void CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, std::unordered_set<FunctionSymbol*>& viableFunctions, 
        std::unique_ptr<Exception>& exception, const Span& span) override;
private:
    std::unordered_map<TypeSymbol*, FunctionSymbol*> functionMap;
    std::vector<std::unique_ptr<FunctionSymbol>> functions;
};

PointerMinusPointerOperation::PointerMinusPointerOperation(BoundCompileUnit& boundCompileUnit_) : Operation(U"operator-", 2, boundCompileUnit_)
{
}

void PointerMinusPointerOperation::CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, 
    std::unordered_set<FunctionSymbol*>& viableFunctions, std::unique_ptr<Exception>& exception, const Span& span)
{
    TypeSymbol* leftType = arguments[0]->GetType();
    if (!leftType->IsPointerType()) return;
    TypeSymbol* rightType = arguments[1]->GetType();
    if (!rightType->IsPointerType()) return;
    FunctionSymbol* function = functionMap[leftType];
    if (!function)
    {
        function = new PointerMinusPointer(leftType, GetSymbolTable()->GetTypeByName(U"long"), span);
        function->SetSymbolTable(GetSymbolTable());
        function->SetParent(&GetSymbolTable()->GlobalNs());
        functionMap[leftType] = function;
        functions.push_back(std::unique_ptr<FunctionSymbol>(function));
    }
    viableFunctions.insert(function);
}

class PointerEqual : public FunctionSymbol
{
public:
    PointerEqual(TypeSymbol* pointerType_, TypeSymbol* boolType_, const Span& span);
    SymbolAccess DeclaredAccess() const override { return SymbolAccess::public_; }
    void GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags) override;
    bool IsBasicTypeOperation() const override { return true; }
};

PointerEqual::PointerEqual(TypeSymbol* pointerType_, TypeSymbol* boolType_, const Span& span) : FunctionSymbol(span, U"operator==")
{
    SetGroupName(U"operator==");
    SetAccess(SymbolAccess::public_);
    ParameterSymbol* leftParam = new ParameterSymbol(span, U"left");
    leftParam->SetType(pointerType_);
    AddMember(leftParam);
    ParameterSymbol* rightParam = new ParameterSymbol(span, U"right");
    rightParam->SetType(pointerType_);
    AddMember(rightParam);
    SetReturnType(boolType_);
    ComputeName();
}

void PointerEqual::GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags)
{
    Assert(genObjects.size() == 2, "operator== needs two objects");
    genObjects[0]->Load(emitter, OperationFlags::none);
    llvm::Value* left = emitter.Stack().Pop();
    genObjects[1]->Load(emitter, OperationFlags::none);
    llvm::Value* right = emitter.Stack().Pop();
    emitter.Stack().Push(emitter.Builder().CreateICmpEQ(left, right));
}

class PointerEqualOperation : public Operation
{
public:
    PointerEqualOperation(BoundCompileUnit& boundCompileUnit_);
    void CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, std::unordered_set<FunctionSymbol*>& viableFunctions, 
        std::unique_ptr<Exception>& exception, const Span& span) override;
private:
    std::unordered_map<TypeSymbol*, FunctionSymbol*> functionMap;
    std::vector<std::unique_ptr<FunctionSymbol>> functions;
};

PointerEqualOperation::PointerEqualOperation(BoundCompileUnit& boundCompileUnit_) : Operation(U"operator==", 2, boundCompileUnit_)
{
}

void PointerEqualOperation::CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, 
    std::unordered_set<FunctionSymbol*>& viableFunctions, std::unique_ptr<Exception>& exception, const Span& span)
{
    TypeSymbol* leftType = arguments[0]->GetType();
    if (!leftType->IsPointerType()) return;
    TypeSymbol* rightType = arguments[1]->GetType();
    if (!rightType->IsPointerType()) return;
    FunctionSymbol* function = functionMap[leftType];
    if (!function)
    {
        function = new PointerEqual(leftType, GetSymbolTable()->GetTypeByName(U"bool"), span);
        function->SetSymbolTable(GetSymbolTable());
        function->SetParent(&GetSymbolTable()->GlobalNs());
        functionMap[leftType] = function;
        functions.push_back(std::unique_ptr<FunctionSymbol>(function));
    }
    viableFunctions.insert(function);
}

class PointerLess : public FunctionSymbol
{
public:
    PointerLess(TypeSymbol* pointerType_, TypeSymbol* boolType_, const Span& span);
    SymbolAccess DeclaredAccess() const override { return SymbolAccess::public_; }
    void GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags) override;
    bool IsBasicTypeOperation() const override { return true; }
};

PointerLess::PointerLess(TypeSymbol* pointerType_, TypeSymbol* boolType_, const Span& span) : FunctionSymbol(span, U"operator<")
{
    SetGroupName(U"operator<");
    SetAccess(SymbolAccess::public_);
    ParameterSymbol* leftParam = new ParameterSymbol(span, U"left");
    leftParam->SetType(pointerType_);
    AddMember(leftParam);
    ParameterSymbol* rightParam = new ParameterSymbol(span, U"right");
    rightParam->SetType(pointerType_);
    AddMember(rightParam);
    SetReturnType(boolType_);
    ComputeName();
}

void PointerLess::GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags)
{
    Assert(genObjects.size() == 2, "operator< needs two objects");
    genObjects[0]->Load(emitter, OperationFlags::none);
    llvm::Value* left = emitter.Stack().Pop();
    genObjects[1]->Load(emitter, OperationFlags::none);
    llvm::Value* right = emitter.Stack().Pop();
    emitter.Stack().Push(emitter.Builder().CreateICmpULT(left, right));
}

class PointerLessOperation : public Operation
{
public:
    PointerLessOperation(BoundCompileUnit& boundCompileUnit_);
    void CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, std::unordered_set<FunctionSymbol*>& viableFunctions, 
        std::unique_ptr<Exception>& exception, const Span& span) override;
private:
    std::unordered_map<TypeSymbol*, FunctionSymbol*> functionMap;
    std::vector<std::unique_ptr<FunctionSymbol>> functions;
};

PointerLessOperation::PointerLessOperation(BoundCompileUnit& boundCompileUnit_) : Operation(U"operator<", 2, boundCompileUnit_)
{
}

void PointerLessOperation::CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, 
    std::unordered_set<FunctionSymbol*>& viableFunctions, std::unique_ptr<Exception>& exception, const Span& span)
{
    TypeSymbol* leftType = arguments[0]->GetType();
    if (!leftType->IsPointerType()) return;
    TypeSymbol* rightType = arguments[1]->GetType();
    if (!rightType->IsPointerType()) return;
    FunctionSymbol* function = functionMap[leftType];
    if (!function)
    {
        function = new PointerLess(leftType, GetSymbolTable()->GetTypeByName(U"bool"), span);
        function->SetSymbolTable(GetSymbolTable());
        function->SetParent(&GetSymbolTable()->GlobalNs());
        functionMap[leftType] = function;
        functions.push_back(std::unique_ptr<FunctionSymbol>(function));
    }
    viableFunctions.insert(function);
}

class PointerArrow : public FunctionSymbol
{
public:
    PointerArrow(TypeSymbol* type_, const Span& span);
    SymbolAccess DeclaredAccess() const override { return SymbolAccess::public_; }
    void GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags) override;
    bool IsBasicTypeOperation() const override { return true; }
private:
    TypeSymbol* type;
};

PointerArrow::PointerArrow(TypeSymbol* type_, const Span& span) : FunctionSymbol(span, U"operator->"), type(type_)
{
    SetGroupName(U"operator->");
    SetAccess(SymbolAccess::public_);
    ParameterSymbol* operandParam = new ParameterSymbol(span, U"operand");
    operandParam->SetType(type->AddPointer(span));
    AddMember(operandParam);
    SetReturnType(type);
    ComputeName();
}

void PointerArrow::GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags)
{
    Assert(genObjects.size() == 1, "return needs one object");
    genObjects[0]->Load(emitter, OperationFlags::none);
}

class PointerArrowOperation : public Operation
{
public:
    PointerArrowOperation(BoundCompileUnit& boundCompileUnit_);
    void CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, std::unordered_set<FunctionSymbol*>& viableFunctions,
        std::unique_ptr<Exception>& exception, const Span& span) override;
private:
    std::unordered_map<TypeSymbol*, FunctionSymbol*> functionMap;
    std::vector<std::unique_ptr<FunctionSymbol>> functions;
};

PointerArrowOperation::PointerArrowOperation(BoundCompileUnit& boundCompileUnit_) : Operation(U"operator->", 1, boundCompileUnit_)
{
}

void PointerArrowOperation::CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments,
    std::unordered_set<FunctionSymbol*>& viableFunctions, std::unique_ptr<Exception>& exception, const Span& span)
{
    TypeSymbol* type = arguments[0]->GetType();
    if (type->PointerCount() <= 1) return;
    TypeSymbol* pointerType = type->RemovePointer(span);
    FunctionSymbol* function = functionMap[pointerType];
    if (!function)
    {
        function = new PointerArrow(pointerType, span);
        function->SetSymbolTable(GetSymbolTable());
        function->SetParent(&GetSymbolTable()->GlobalNs());
        functionMap[pointerType] = function;
        functions.push_back(std::unique_ptr<FunctionSymbol>(function));
    }
    viableFunctions.insert(function);
}

class LvalueRefefenceCopyCtor : public FunctionSymbol
{
public:
    LvalueRefefenceCopyCtor(TypeSymbol* type_, const Span& span);
    SymbolAccess DeclaredAccess() const override { return SymbolAccess::public_; }
    void GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags) override;
    bool IsBasicTypeOperation() const override { return true; }
private:
    TypeSymbol* type;
};

LvalueRefefenceCopyCtor::LvalueRefefenceCopyCtor(TypeSymbol* type_, const Span& span) : FunctionSymbol(span, U"@constructor"), type(type_)
{
    SetGroupName(U"@constructor");
    SetAccess(SymbolAccess::public_);
    ParameterSymbol* thisParam = new ParameterSymbol(span, U"this");
    thisParam->SetType(type->AddPointer(span));
    AddMember(thisParam);
    ParameterSymbol* thatParam = new ParameterSymbol(span, U"that");
    thatParam->SetType(type);
    AddMember(thatParam);
    ComputeName();
}

void LvalueRefefenceCopyCtor::GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags)
{
    Assert(genObjects.size() == 2, "reference constructor needs two objects");
    genObjects[1]->Load(emitter, OperationFlags::none);
    genObjects[0]->Store(emitter, OperationFlags::none);
}

class LvalueReferenceCopyConstructorOperation : public Operation
{
public:
    LvalueReferenceCopyConstructorOperation(BoundCompileUnit& boundCompileUnit_);
    void CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, std::unordered_set<FunctionSymbol*>& viableFunctions, 
        std::unique_ptr<Exception>& exception, const Span& span) override;
private:
    std::unordered_map<TypeSymbol*, FunctionSymbol*> functionMap;
    std::vector<std::unique_ptr<FunctionSymbol>> functions;
};

LvalueReferenceCopyConstructorOperation::LvalueReferenceCopyConstructorOperation(BoundCompileUnit& boundCompileUnit_) : Operation(U"@constructor", 2, boundCompileUnit_)
{
}

void LvalueReferenceCopyConstructorOperation::CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, 
    std::unordered_set<FunctionSymbol*>& viableFunctions, std::unique_ptr<Exception>& exception, const Span& span)
{
    TypeSymbol* type = arguments[0]->GetType();
    if (type->PointerCount() < 1 || !type->IsLvalueReferenceType()) return;
    TypeSymbol* lvalueRefType = type->RemovePointer(span);
    FunctionSymbol* function = functionMap[lvalueRefType];
    if (!function)
    {
        function = new LvalueRefefenceCopyCtor(lvalueRefType, span);
        function->SetSymbolTable(GetSymbolTable());
        function->SetParent(&GetSymbolTable()->GlobalNs());
        functionMap[lvalueRefType] = function;
        functions.push_back(std::unique_ptr<FunctionSymbol>(function));
    }
    viableFunctions.insert(function);
}

class LvalueReferenceCopyAssignment : public FunctionSymbol
{
public:
    LvalueReferenceCopyAssignment(TypeSymbol* type_, TypeSymbol* voidType_, const Span& span);
    SymbolAccess DeclaredAccess() const override { return SymbolAccess::public_; }
    void GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags) override;
    bool IsBasicTypeOperation() const override { return true; }
private:
    TypeSymbol* type;
};

LvalueReferenceCopyAssignment::LvalueReferenceCopyAssignment(TypeSymbol* type_, TypeSymbol* voidType_, const Span& span) : FunctionSymbol(span, U"operator="), type(type_)
{
    SetGroupName(U"operator=");
    SetAccess(SymbolAccess::public_);
    ParameterSymbol* thisParam = new ParameterSymbol(span, U"this");
    thisParam->SetType(type->AddPointer(span));
    AddMember(thisParam);
    ParameterSymbol* thatParam = new ParameterSymbol(span, U"that");
    thatParam->SetType(type);
    AddMember(thatParam);
    SetReturnType(voidType_);
    ComputeName();
}

void LvalueReferenceCopyAssignment::GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags)
{
    Assert(genObjects.size() == 2, "copy assignment needs two objects");
    genObjects[1]->Load(emitter, OperationFlags::none);
    genObjects[0]->Store(emitter, OperationFlags::none);
}

class LvalueReferenceCopyAssignmentOperation : public Operation
{
public:
    LvalueReferenceCopyAssignmentOperation(BoundCompileUnit& boundCompileUnit_);
    void CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, std::unordered_set<FunctionSymbol*>& viableFunctions, 
        std::unique_ptr<Exception>& exception, const Span& span) override;
private:
    std::unordered_map<TypeSymbol*, FunctionSymbol*> functionMap;
    std::vector<std::unique_ptr<FunctionSymbol>> functions;
};

LvalueReferenceCopyAssignmentOperation::LvalueReferenceCopyAssignmentOperation(BoundCompileUnit& boundCompileUnit_) : Operation(U"operator=", 2, boundCompileUnit_)
{
}

void LvalueReferenceCopyAssignmentOperation::CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, 
    std::unordered_set<FunctionSymbol*>& viableFunctions, std::unique_ptr<Exception>& exception, const Span& span)
{
    TypeSymbol* type = arguments[0]->GetType();
    if (type->PointerCount() < 1 || !type->IsLvalueReferenceType()) return;
    TypeSymbol* lvalueRefType = type->RemovePointer(span);
    FunctionSymbol* function = functionMap[lvalueRefType];
    if (!function)
    {
        function = new LvalueReferenceCopyAssignment(lvalueRefType, GetSymbolTable()->GetTypeByName(U"void"), span);
        function->SetSymbolTable(GetSymbolTable());
        function->SetParent(&GetSymbolTable()->GlobalNs());
        functionMap[lvalueRefType] = function;
        functions.push_back(std::unique_ptr<FunctionSymbol>(function));
    }
    viableFunctions.insert(function);
}

class LvalueReferenceReturn : public FunctionSymbol
{
public:
    LvalueReferenceReturn(TypeSymbol* type_, const Span& span);
    SymbolAccess DeclaredAccess() const override { return SymbolAccess::public_; }
    void GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags) override;
    bool IsBasicTypeOperation() const override { return true; }
private:
    TypeSymbol* type;
};

LvalueReferenceReturn::LvalueReferenceReturn(TypeSymbol* type_, const Span& span) : FunctionSymbol(span, U"@return"), type(type_)
{
    SetGroupName(U"@return");
    SetAccess(SymbolAccess::public_);
    ParameterSymbol* valueParam = new ParameterSymbol(span, U"value");
    valueParam->SetType(type);
    AddMember(valueParam);
    SetReturnType(type);
    ComputeName();
}

void LvalueReferenceReturn::GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags)
{
    Assert(genObjects.size() == 1, "return needs one object");
    genObjects[0]->Load(emitter, OperationFlags::none);
}

class LvalueReferenceReturnOperation : public Operation
{
public:
    LvalueReferenceReturnOperation(BoundCompileUnit& boundCompileUnit_);
    void CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, std::unordered_set<FunctionSymbol*>& viableFunctions, 
        std::unique_ptr<Exception>& exception, const Span& span) override;
private:
    std::unordered_map<TypeSymbol*, FunctionSymbol*> functionMap;
    std::vector<std::unique_ptr<FunctionSymbol>> functions;
};

LvalueReferenceReturnOperation::LvalueReferenceReturnOperation(BoundCompileUnit& boundCompileUnit_) : Operation(U"@return", 1, boundCompileUnit_)
{
}

void LvalueReferenceReturnOperation::CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, 
    std::unordered_set<FunctionSymbol*>& viableFunctions, std::unique_ptr<Exception>& exception, const Span& span)
{
    TypeSymbol* type = arguments[0]->GetType();
    if (!type->IsLvalueReferenceType()) return;
    FunctionSymbol* function = functionMap[type];
    if (!function)
    {
        function = new LvalueReferenceReturn(type, span);
        function->SetSymbolTable(GetSymbolTable());
        function->SetParent(&GetSymbolTable()->GlobalNs());
        functionMap[type] = function;
        functions.push_back(std::unique_ptr<FunctionSymbol>(function));
    }
    viableFunctions.insert(function);
}

class ClassDefaultConstructor : public ConstructorSymbol
{
public:
    ClassDefaultConstructor(ClassTypeSymbol* classType_, const Span& span_);
    SymbolAccess DeclaredAccess() const override { return SymbolAccess::public_; }
    ClassTypeSymbol* ClassType() { return classType; }
private:
    ClassTypeSymbol* classType;
};

ClassDefaultConstructor::ClassDefaultConstructor(ClassTypeSymbol* classType_, const Span& span_) :
    ConstructorSymbol(classType_->GetSpan(), U"@constructor"), classType(classType_)
{
    SetAccess(SymbolAccess::public_);
    SetParent(classType);
    ParameterSymbol* thisParam = new ParameterSymbol(span_, U"this");
    thisParam->SetType(classType->AddPointer(span_));
    AddMember(thisParam);
    ComputeName();
}

class ClassDefaultConstructorOperation : public Operation
{
public:
    ClassDefaultConstructorOperation(BoundCompileUnit& boundCompileUnit_);
    void CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, std::unordered_set<FunctionSymbol*>& viableFunctions, 
        std::unique_ptr<Exception>& exception, const Span& span) override;
    void GenerateImplementation(ClassDefaultConstructor* defaultConstructor, ContainerScope* containerScope, std::unique_ptr<Exception>& exception, const Span& span);
private:
    std::unordered_map<TypeSymbol*, FunctionSymbol*> functionMap;
    std::vector<std::unique_ptr<FunctionSymbol>> functions;
};

ClassDefaultConstructorOperation::ClassDefaultConstructorOperation(BoundCompileUnit& boundCompileUnit_) : Operation(U"@constructor", 1, boundCompileUnit_)
{
}

void ClassDefaultConstructorOperation::CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, 
    std::unordered_set<FunctionSymbol*>& viableFunctions, std::unique_ptr<Exception>& exception, const Span& span)
{
    TypeSymbol* type = arguments[0]->GetType();
    if (!type->RemovePointer(span)->PlainType(span)->IsClassTypeSymbol()) return;
    ClassTypeSymbol* classType = static_cast<ClassTypeSymbol*>(type->BaseType());
    if (classType->IsStatic())
    {
        exception.reset(new Exception("cannot create an instance of a static class", span, classType->GetSpan()));
        return;
    }
    if (classType->IsAbstract())
    {
        exception.reset(new Exception("cannot create an instance of an abstract class", span, classType->GetSpan()));
        return;
    }
    if (classType->DefaultConstructor()) 
    {
        viableFunctions.insert(classType->DefaultConstructor());
        return;
    }
    FunctionSymbol* function = functionMap[classType];
    if (!function)
    {
        ClassDefaultConstructor* defaultConstructor = new ClassDefaultConstructor(classType, span);
        GenerateImplementation(defaultConstructor, containerScope, exception, span);
        function = defaultConstructor;
        function->SetSymbolTable(GetSymbolTable());
        function->SetParent(classType);
        functionMap[classType] = function;
        classType->AddMember(defaultConstructor);
    }
    viableFunctions.insert(function);
}

void ClassDefaultConstructorOperation::GenerateImplementation(ClassDefaultConstructor* defaultConstructor, ContainerScope* containerScope, std::unique_ptr<Exception>& exception, const Span& span)
{
    ClassTypeSymbol* classType = defaultConstructor->ClassType();
    try
    {
        std::unique_ptr<BoundFunction> boundFunction(new BoundFunction(defaultConstructor));
        boundFunction->SetBody(std::unique_ptr<BoundCompoundStatement>(new BoundCompoundStatement(span)));
        if (classType->BaseClass())
        {
            std::vector<FunctionScopeLookup> baseConstructorCallLookups;
            baseConstructorCallLookups.push_back(FunctionScopeLookup(ScopeLookup::this_and_base_and_parent, containerScope));
            baseConstructorCallLookups.push_back(FunctionScopeLookup(ScopeLookup::this_, classType->GetContainerScope()));
            baseConstructorCallLookups.push_back(FunctionScopeLookup(ScopeLookup::fileScopes, nullptr));
            std::vector<std::unique_ptr<BoundExpression>> baseConstructorCallArguments;
            ParameterSymbol* thisParam = defaultConstructor->Parameters()[0];
            FunctionSymbol* thisToBaseConversion = GetBoundCompileUnit().GetConversion(thisParam->GetType(), classType->BaseClass()->AddPointer(span), span);
            if (!thisToBaseConversion)
            {
                throw Exception("base class conversion not found", span, classType->GetSpan());
            }
            BoundExpression* baseClassPointerConversion = new BoundConversion(std::unique_ptr<BoundExpression>(new BoundParameter(thisParam)), thisToBaseConversion);
            baseConstructorCallArguments.push_back(std::unique_ptr<BoundExpression>(baseClassPointerConversion));
            std::unique_ptr<BoundFunctionCall> baseConstructorCall = ResolveOverload(U"@constructor", containerScope, baseConstructorCallLookups, baseConstructorCallArguments, GetBoundCompileUnit(),
                boundFunction.get(), span);
            boundFunction->Body()->AddStatement(std::unique_ptr<BoundStatement>(new BoundExpressionStatement(std::move(baseConstructorCall))));
        }
        if (classType->IsPolymorphic())
        {
            ParameterSymbol* thisParam = defaultConstructor->Parameters()[0];
            BoundExpression* classPtr = nullptr;
            ClassTypeSymbol* vmtPtrHolderClass = classType->VmtPtrHolderClass();
            if (vmtPtrHolderClass == classType)
            {
                classPtr = new BoundParameter(thisParam);
            }
            else
            {
                FunctionSymbol* thisToHolderConversion = GetBoundCompileUnit().GetConversion(thisParam->GetType(), vmtPtrHolderClass->AddPointer(span), span);
                if (!thisToHolderConversion)
                {
                    throw Exception("base class conversion not found", span, classType->GetSpan());
                }
                classPtr = new BoundConversion(std::unique_ptr<BoundExpression>(new BoundParameter(thisParam)), thisToHolderConversion);
            }
            boundFunction->Body()->AddStatement(std::unique_ptr<BoundStatement>(new BoundSetVmtPtrStatement(std::unique_ptr<BoundExpression>(classPtr), classType)));
        }
        int n = classType->MemberVariables().size();
        for (int i = 0; i < n; ++i)
        {
            MemberVariableSymbol* memberVariableSymbol = classType->MemberVariables()[i];
            std::vector<FunctionScopeLookup> memberConstructorCallLookups;
            memberConstructorCallLookups.push_back(FunctionScopeLookup(ScopeLookup::this_and_base_and_parent, containerScope));
            memberConstructorCallLookups.push_back(FunctionScopeLookup(ScopeLookup::this_, memberVariableSymbol->GetType()->BaseType()->ClassInterfaceOrNsScope()));
            memberConstructorCallLookups.push_back(FunctionScopeLookup(ScopeLookup::fileScopes, nullptr));
            std::vector<std::unique_ptr<BoundExpression>> memberConstructorCallArguments;
            BoundMemberVariable* boundMemberVariable = new BoundMemberVariable(memberVariableSymbol);
            boundMemberVariable->SetClassPtr(std::unique_ptr<BoundExpression>(new BoundParameter(defaultConstructor->GetThisParam())));
            memberConstructorCallArguments.push_back(std::unique_ptr<BoundExpression>(
                new BoundAddressOfExpression(std::unique_ptr<BoundExpression>(boundMemberVariable), boundMemberVariable->GetType()->AddPointer(span))));
            std::unique_ptr<BoundFunctionCall> memberConstructorCall = ResolveOverload(U"@constructor", containerScope, memberConstructorCallLookups, memberConstructorCallArguments,
                GetBoundCompileUnit(), boundFunction.get(), span);
            boundFunction->Body()->AddStatement(std::unique_ptr<BoundStatement>(new BoundExpressionStatement(std::move(memberConstructorCall))));
        }
        GetBoundCompileUnit().AddBoundNode(std::move(boundFunction));
    }
    catch (const Exception& ex)
    {
        throw Exception("cannot create default constructor for class '" + ToUtf8(classType->FullName()) + "'. Reason: " + ex.Message(), span, ex.References());
    }
}

void GenerateDestructorImplementation(BoundClass* boundClass, DestructorSymbol* destructorSymbol, BoundCompileUnit& boundCompileUnit, ContainerScope* containerScope, const Span& span)
{
    ClassTypeSymbol* classType = boundClass->GetClassTypeSymbol();
    try
    {
        std::unique_ptr<BoundFunction> boundFunction(new BoundFunction(destructorSymbol));
        boundFunction->SetBody(std::unique_ptr<BoundCompoundStatement>(new BoundCompoundStatement(span)));
        if (classType->IsPolymorphic())
        {
            ParameterSymbol* thisParam = destructorSymbol->Parameters()[0];
            BoundExpression* classPtr = nullptr;
            ClassTypeSymbol* vmtPtrHolderClass = classType->VmtPtrHolderClass();
            if (vmtPtrHolderClass == classType)
            {
                classPtr = new BoundParameter(thisParam);
            }
            else
            {
                FunctionSymbol* thisToHolderConversion = boundCompileUnit.GetConversion(thisParam->GetType(), vmtPtrHolderClass->AddPointer(span), span);
                if (!thisToHolderConversion)
                {
                    throw Exception("base class conversion not found", span, classType->GetSpan());
                }
                classPtr = new BoundConversion(std::unique_ptr<BoundExpression>(new BoundParameter(thisParam)), thisToHolderConversion);
            }
            boundFunction->Body()->AddStatement(std::unique_ptr<BoundStatement>(new BoundSetVmtPtrStatement(std::unique_ptr<BoundExpression>(classPtr), classType)));
        }
        int n = classType->MemberVariables().size();
        for (int i = n - 1; i >= 0; --i)
        {
            MemberVariableSymbol* memberVariableSymbol = classType->MemberVariables()[i];
            if (memberVariableSymbol->GetType()->HasNontrivialDestructor())
            {
                std::vector<FunctionScopeLookup> memberDestructorCallLookups;
                memberDestructorCallLookups.push_back(FunctionScopeLookup(ScopeLookup::this_and_base_and_parent, containerScope));
                memberDestructorCallLookups.push_back(FunctionScopeLookup(ScopeLookup::this_, memberVariableSymbol->GetType()->BaseType()->ClassInterfaceOrNsScope()));
                memberDestructorCallLookups.push_back(FunctionScopeLookup(ScopeLookup::fileScopes, nullptr));
                std::vector<std::unique_ptr<BoundExpression>> memberDestructorCallArguments;
                BoundMemberVariable* boundMemberVariable = new BoundMemberVariable(memberVariableSymbol);
                boundMemberVariable->SetClassPtr(std::unique_ptr<BoundExpression>(new BoundParameter(destructorSymbol->GetThisParam())));
                memberDestructorCallArguments.push_back(std::unique_ptr<BoundExpression>(
                    new BoundAddressOfExpression(std::unique_ptr<BoundExpression>(boundMemberVariable), boundMemberVariable->GetType()->AddPointer(span))));
                std::unique_ptr<BoundFunctionCall> memberDestructorCall = ResolveOverload(U"@destructor", containerScope, memberDestructorCallLookups, memberDestructorCallArguments,
                    boundCompileUnit, boundFunction.get(), span);
                boundFunction->Body()->AddStatement(std::unique_ptr<BoundStatement>(new BoundExpressionStatement(std::move(memberDestructorCall))));
            }
        }
        if (classType->BaseClass() && classType->BaseClass()->HasNontrivialDestructor())
        {
            std::vector<FunctionScopeLookup> baseDestructorCallLookups;
            baseDestructorCallLookups.push_back(FunctionScopeLookup(ScopeLookup::this_and_base_and_parent, containerScope));
            baseDestructorCallLookups.push_back(FunctionScopeLookup(ScopeLookup::this_, classType->GetContainerScope()));
            baseDestructorCallLookups.push_back(FunctionScopeLookup(ScopeLookup::fileScopes, nullptr));
            std::vector<std::unique_ptr<BoundExpression>> baseDestructorCallArguments;
            ParameterSymbol* thisParam = destructorSymbol->Parameters()[0];
            FunctionSymbol* thisToBaseConversion = boundCompileUnit.GetConversion(thisParam->GetType(), classType->BaseClass()->AddPointer(span), span);
            if (!thisToBaseConversion)
            {
                throw Exception("base class conversion not found", span, classType->GetSpan());
            }
            BoundExpression* baseClassPointerConversion = new BoundConversion(std::unique_ptr<BoundExpression>(new BoundParameter(thisParam)), thisToBaseConversion);
            baseDestructorCallArguments.push_back(std::unique_ptr<BoundExpression>(baseClassPointerConversion));
            std::unique_ptr<BoundFunctionCall> baseDestructorCall = ResolveOverload(U"@destructor", containerScope, baseDestructorCallLookups, baseDestructorCallArguments, boundCompileUnit,
                boundFunction.get(), span);
            boundFunction->Body()->AddStatement(std::unique_ptr<BoundStatement>(new BoundExpressionStatement(std::move(baseDestructorCall))));
        }
        boundClass->AddMember(std::move(boundFunction));
    }
    catch (const Exception& ex)
    {
        throw Exception("cannot create destructor for class '" + ToUtf8(classType->FullName()) + "'. Reason: " + ex.Message(), span, ex.References());
    }
}

void GenerateClassInitialization(ConstructorSymbol* constructorSymbol, ConstructorNode* constructorNode, BoundCompoundStatement* boundCompoundStatement, BoundFunction* boundFunction, 
    BoundCompileUnit& boundCompileUnit, ContainerScope* containerScope, StatementBinder* statementBinder)
{
    Symbol* parent = constructorSymbol->Parent();
    Assert(parent->GetSymbolType() == SymbolType::classTypeSymbol || parent->GetSymbolType() == SymbolType::classTemplateSpecializationSymbol, "class type symbol expected");
    ClassTypeSymbol* classType = static_cast<ClassTypeSymbol*>(parent);
    try
    { 
        ParameterSymbol* thisParam = constructorSymbol->GetThisParam();
        Assert(thisParam, "this parameter expected");
        ThisInitializerNode* thisInitializer = nullptr;
        BaseInitializerNode* baseInitializer = nullptr;
        std::unordered_map<std::u32string, MemberInitializerNode*> memberInitializerMap;
        int ni = constructorNode->Initializers().Count();
        for (int i = 0; i < ni; ++i)
        {
            InitializerNode* initializer = constructorNode->Initializers()[i];
            if (initializer->GetNodeType() == NodeType::thisInitializerNode)
            {
                if (thisInitializer)
                {
                    throw Exception("already has 'this' initializer", initializer->GetSpan());
                }
                else if (baseInitializer)
                {
                    throw Exception("cannot have both 'this' and 'base' initializer", initializer->GetSpan());
                }
                thisInitializer = static_cast<ThisInitializerNode*>(initializer);
            }
            else if (initializer->GetNodeType() == NodeType::baseInitializerNode)
            {
                if (baseInitializer)
                {
                    throw Exception("already has 'base' initializer", initializer->GetSpan());
                }
                else if (thisInitializer)
                {
                    throw Exception("cannot have both 'this' and 'base' initializer", initializer->GetSpan());
                }
                baseInitializer = static_cast<BaseInitializerNode*>(initializer);
            }
            else if (initializer->GetNodeType() == NodeType::memberInitializerNode)
            {
                MemberInitializerNode* memberInitializer = static_cast<MemberInitializerNode*>(initializer);
                std::u32string memberName = memberInitializer->MemberId()->Str();
                auto it = memberInitializerMap.find(memberName);
                if (it != memberInitializerMap.cend())
                {
                    throw Exception("already has initializer for member variable '" + ToUtf8(memberName) + "'", initializer->GetSpan());
                }
                memberInitializerMap[memberName] = memberInitializer;
            }
        }
        if (thisInitializer)
        {
            std::vector<FunctionScopeLookup> lookups;
            lookups.push_back(FunctionScopeLookup(ScopeLookup::this_and_base_and_parent, containerScope));
            lookups.push_back(FunctionScopeLookup(ScopeLookup::this_, classType->GetContainerScope()));
            lookups.push_back(FunctionScopeLookup(ScopeLookup::fileScopes, nullptr));
            std::vector<std::unique_ptr<BoundExpression>> arguments;
            arguments.push_back(std::unique_ptr<BoundExpression>(new BoundParameter(thisParam)));
            int n = thisInitializer->Arguments().Count();
            for (int i = 0; i < n; ++i)
            {
                Node* argumentNode = thisInitializer->Arguments()[i];
                std::unique_ptr<BoundExpression> argument = BindExpression(argumentNode, boundCompileUnit, boundFunction, containerScope, statementBinder);
                arguments.push_back(std::move(argument));
            }
            std::unique_ptr<BoundFunctionCall> constructorCall = ResolveOverload(U"@constructor", containerScope, lookups, arguments, boundCompileUnit, boundFunction, constructorNode->GetSpan());
            boundCompoundStatement->AddStatement(std::unique_ptr<BoundStatement>(new BoundExpressionStatement(std::move(constructorCall))));
        }
        else if (baseInitializer)
        {
            std::vector<FunctionScopeLookup> lookups;
            if (!classType->BaseClass())
            {
                throw Exception("class '" + ToUtf8(classType->FullName()) + "' does not have a base class", constructorNode->GetSpan(), classType->GetSpan());
            }
            lookups.push_back(FunctionScopeLookup(ScopeLookup::this_and_base_and_parent, containerScope));
            lookups.push_back(FunctionScopeLookup(ScopeLookup::this_, classType->BaseClass()->GetContainerScope()));
            lookups.push_back(FunctionScopeLookup(ScopeLookup::fileScopes, nullptr));
            std::vector<std::unique_ptr<BoundExpression>> arguments;
            FunctionSymbol* thisToBaseConversion = boundCompileUnit.GetConversion(thisParam->GetType(), classType->BaseClass()->AddPointer(constructorNode->GetSpan()), constructorNode->GetSpan());
            if (!thisToBaseConversion)
            {
                throw Exception("base class conversion not found", constructorNode->GetSpan(), classType->GetSpan());
            }
            BoundExpression* baseClassPointerConversion = new BoundConversion(std::unique_ptr<BoundExpression>(new BoundParameter(thisParam)), thisToBaseConversion);
            arguments.push_back(std::unique_ptr<BoundExpression>(baseClassPointerConversion));
            int n = baseInitializer->Arguments().Count();
            for (int i = 0; i < n; ++i)
            {
                Node* argumentNode = baseInitializer->Arguments()[i];
                std::unique_ptr<BoundExpression> argument = BindExpression(argumentNode, boundCompileUnit, boundFunction, containerScope, statementBinder);
                arguments.push_back(std::move(argument));
            }
            std::unique_ptr<BoundFunctionCall> constructorCall = ResolveOverload(U"@constructor", containerScope, lookups, arguments, boundCompileUnit, boundFunction, constructorNode->GetSpan());
            boundCompoundStatement->AddStatement(std::unique_ptr<BoundStatement>(new BoundExpressionStatement(std::move(constructorCall))));
        }
        if (classType->IsPolymorphic() && !thisInitializer)
        {
            BoundExpression* classPtr = nullptr;
            ClassTypeSymbol* vmtPtrHolderClass = classType->VmtPtrHolderClass();
            if (vmtPtrHolderClass == classType)
            {
                classPtr = new BoundParameter(thisParam);
            }
            else
            {
                FunctionSymbol* thisToHolderConversion = boundCompileUnit.GetConversion(thisParam->GetType(), vmtPtrHolderClass->AddPointer(constructorNode->GetSpan()), constructorNode->GetSpan());
                if (!thisToHolderConversion)
                {
                    throw Exception("base class conversion not found", constructorNode->GetSpan(), classType->GetSpan());
                }
                classPtr = new BoundConversion(std::unique_ptr<BoundExpression>(new BoundParameter(thisParam)), thisToHolderConversion);
            }
            boundCompoundStatement->AddStatement(std::unique_ptr<BoundStatement>(new BoundSetVmtPtrStatement(std::unique_ptr<BoundExpression>(classPtr), classType)));
        }
        int nm = classType->MemberVariables().size();
        for (int i = 0; i < nm; ++i)
        {
            MemberVariableSymbol* memberVariableSymbol = classType->MemberVariables()[i];
            auto it = memberInitializerMap.find(memberVariableSymbol->Name());
            if (it != memberInitializerMap.cend())
            {
                MemberInitializerNode* memberInitializer = it->second;
                memberInitializerMap.erase(memberInitializer->MemberId()->Str());
                std::vector<FunctionScopeLookup> lookups;
                lookups.push_back(FunctionScopeLookup(ScopeLookup::this_and_base_and_parent, containerScope));
                lookups.push_back(FunctionScopeLookup(ScopeLookup::this_, memberVariableSymbol->GetType()->BaseType()->GetContainerScope()));
                lookups.push_back(FunctionScopeLookup(ScopeLookup::fileScopes, nullptr));
                std::vector<std::unique_ptr<BoundExpression>> arguments;
                BoundMemberVariable* boundMemberVariable = new BoundMemberVariable(memberVariableSymbol);
                boundMemberVariable->SetClassPtr(std::unique_ptr<BoundExpression>(new BoundParameter(thisParam)));
                arguments.push_back(std::unique_ptr<BoundExpression>(
                    new BoundAddressOfExpression(std::unique_ptr<BoundExpression>(boundMemberVariable), boundMemberVariable->GetType()->AddPointer(constructorNode->GetSpan()))));
                int n = memberInitializer->Arguments().Count();
                for (int i = 0; i < n; ++i)
                {
                    Node* argumentNode = memberInitializer->Arguments()[i];
                    std::unique_ptr<BoundExpression> argument = BindExpression(argumentNode, boundCompileUnit, boundFunction, containerScope, statementBinder);
                    arguments.push_back(std::move(argument));
                }
                std::unique_ptr<BoundFunctionCall> constructorCall = ResolveOverload(U"@constructor", containerScope, lookups, arguments, boundCompileUnit, boundFunction, constructorNode->GetSpan());
                boundCompoundStatement->AddStatement(std::unique_ptr<BoundStatement>(new BoundExpressionStatement(std::move(constructorCall))));
            }
            else if (!thisInitializer)
            {
                bool copyConstructor = constructorSymbol->Parameters().size() == 2 &&
                    TypesEqual(constructorSymbol->Parameters()[1]->GetType(), classType->AddConst(constructorNode->GetSpan())->AddLvalueReference(constructorNode->GetSpan()));
                if (copyConstructor)
                {
                    std::vector<FunctionScopeLookup> lookups;
                    lookups.push_back(FunctionScopeLookup(ScopeLookup::this_and_base_and_parent, containerScope));
                    lookups.push_back(FunctionScopeLookup(ScopeLookup::this_, memberVariableSymbol->GetType()->BaseType()->GetContainerScope()));
                    lookups.push_back(FunctionScopeLookup(ScopeLookup::fileScopes, nullptr));
                    std::vector<std::unique_ptr<BoundExpression>> arguments;
                    BoundMemberVariable* boundMemberVariable = new BoundMemberVariable(memberVariableSymbol);
                    boundMemberVariable->SetClassPtr(std::unique_ptr<BoundExpression>(new BoundParameter(thisParam)));
                    arguments.push_back(std::unique_ptr<BoundExpression>(
                        new BoundAddressOfExpression(std::unique_ptr<BoundExpression>(boundMemberVariable), boundMemberVariable->GetType()->AddPointer(constructorNode->GetSpan()))));
                    CloneContext cloneContext;
                    DotNode thatMemberVarNode(constructorNode->GetSpan(), constructorNode->Parameters()[0]->Clone(cloneContext),
                        new IdentifierNode(constructorNode->GetSpan(), memberVariableSymbol->Name()));
                    std::unique_ptr<BoundExpression> thatMemberVarArgument = BindExpression(&thatMemberVarNode, boundCompileUnit, boundFunction, containerScope, statementBinder);
                    arguments.push_back(std::move(thatMemberVarArgument));
                    std::unique_ptr<BoundFunctionCall> constructorCall = ResolveOverload(U"@constructor", containerScope, lookups, arguments, boundCompileUnit, boundFunction,
                        constructorNode->GetSpan());
                    boundCompoundStatement->AddStatement(std::unique_ptr<BoundStatement>(new BoundExpressionStatement(std::move(constructorCall))));
                }
                else
                {
                    std::vector<FunctionScopeLookup> lookups;
                    lookups.push_back(FunctionScopeLookup(ScopeLookup::this_and_base_and_parent, containerScope));
                    lookups.push_back(FunctionScopeLookup(ScopeLookup::this_, memberVariableSymbol->GetType()->BaseType()->GetContainerScope()));
                    lookups.push_back(FunctionScopeLookup(ScopeLookup::fileScopes, nullptr));
                    std::vector<std::unique_ptr<BoundExpression>> arguments;
                    BoundMemberVariable* boundMemberVariable = new BoundMemberVariable(memberVariableSymbol);
                    boundMemberVariable->SetClassPtr(std::unique_ptr<BoundExpression>(new BoundParameter(thisParam)));
                    arguments.push_back(std::unique_ptr<BoundExpression>(
                        new BoundAddressOfExpression(std::unique_ptr<BoundExpression>(boundMemberVariable), boundMemberVariable->GetType()->AddPointer(constructorNode->GetSpan()))));
                    std::unique_ptr<BoundFunctionCall> constructorCall = ResolveOverload(U"@constructor", containerScope, lookups, arguments, boundCompileUnit, boundFunction, 
                        constructorNode->GetSpan());
                    boundCompoundStatement->AddStatement(std::unique_ptr<BoundStatement>(new BoundExpressionStatement(std::move(constructorCall))));
                }
            }
        }
        if (!memberInitializerMap.empty())
        {
            MemberInitializerNode* initializer = memberInitializerMap.begin()->second;
            throw Exception("no member variable found for initializer named '" + ToUtf8(initializer->MemberId()->Str()) + "'", initializer->GetSpan(), classType->GetSpan());
        }
    }
    catch (const Exception& ex)
    {
        throw Exception("could not generate initialization for class '" + ToUtf8(classType->FullName()) + "'. Reason: " + ex.Message(), constructorNode->GetSpan(), ex.References());
    }
}

void GenerateClassTermination(DestructorSymbol* destructorSymbol, DestructorNode* destructorNode, BoundCompoundStatement* boundCompoundStatement, BoundFunction* boundFunction,
    BoundCompileUnit& boundCompileUnit, ContainerScope* containerScope, StatementBinder* statementBinder)
{
    Symbol* parent = destructorSymbol->Parent();
    Assert(parent->GetSymbolType() == SymbolType::classTypeSymbol || parent->GetSymbolType() == SymbolType::classTemplateSpecializationSymbol, "class type symbol expected");
    ClassTypeSymbol* classType = static_cast<ClassTypeSymbol*>(parent);
    try
    {
        ParameterSymbol* thisParam = destructorSymbol->GetThisParam();
        Assert(thisParam, "this parameter expected");
        if (classType->IsPolymorphic())
        {
            ParameterSymbol* thisParam = destructorSymbol->Parameters()[0];
            BoundExpression* classPtr = nullptr;
            ClassTypeSymbol* vmtPtrHolderClass = classType->VmtPtrHolderClass();
            if (vmtPtrHolderClass == classType)
            {
                classPtr = new BoundParameter(thisParam);
            }
            else
            {
                FunctionSymbol* thisToHolderConversion = boundCompileUnit.GetConversion(thisParam->GetType(), vmtPtrHolderClass->AddPointer(destructorNode->GetSpan()), destructorNode->GetSpan());
                if (!thisToHolderConversion)
                {
                    throw Exception("base class conversion not found", destructorNode->GetSpan(), classType->GetSpan());
                }
                classPtr = new BoundConversion(std::unique_ptr<BoundExpression>(new BoundParameter(thisParam)), thisToHolderConversion);
            }
            boundCompoundStatement->AddStatement(std::unique_ptr<BoundStatement>(new BoundSetVmtPtrStatement(std::unique_ptr<BoundExpression>(classPtr), classType)));
        }
        int n = classType->MemberVariables().size();
        for (int i = n - 1; i >= 0; --i)
        {
            MemberVariableSymbol* memberVariableSymbol = classType->MemberVariables()[i];
            if (memberVariableSymbol->GetType()->HasNontrivialDestructor())
            {
                std::vector<FunctionScopeLookup> memberDestructorCallLookups;
                memberDestructorCallLookups.push_back(FunctionScopeLookup(ScopeLookup::this_and_base_and_parent, containerScope));
                memberDestructorCallLookups.push_back(FunctionScopeLookup(ScopeLookup::this_, memberVariableSymbol->GetType()->BaseType()->ClassInterfaceOrNsScope()));
                memberDestructorCallLookups.push_back(FunctionScopeLookup(ScopeLookup::fileScopes, nullptr));
                std::vector<std::unique_ptr<BoundExpression>> memberDestructorCallArguments;
                BoundMemberVariable* boundMemberVariable = new BoundMemberVariable(memberVariableSymbol);
                boundMemberVariable->SetClassPtr(std::unique_ptr<BoundExpression>(new BoundParameter(destructorSymbol->GetThisParam())));
                memberDestructorCallArguments.push_back(std::unique_ptr<BoundExpression>(
                    new BoundAddressOfExpression(std::unique_ptr<BoundExpression>(boundMemberVariable), boundMemberVariable->GetType()->AddPointer(destructorNode->GetSpan()))));
                std::unique_ptr<BoundFunctionCall> memberDestructorCall = ResolveOverload(U"@destructor", containerScope, memberDestructorCallLookups, memberDestructorCallArguments,
                    boundCompileUnit, boundFunction, destructorNode->GetSpan());
                boundCompoundStatement->AddStatement(std::unique_ptr<BoundStatement>(new BoundExpressionStatement(std::move(memberDestructorCall))));
            }
        }
        if (classType->BaseClass() && classType->BaseClass()->HasNontrivialDestructor())
        {
            std::vector<FunctionScopeLookup> baseDestructorCallLookups;
            baseDestructorCallLookups.push_back(FunctionScopeLookup(ScopeLookup::this_and_base_and_parent, containerScope));
            baseDestructorCallLookups.push_back(FunctionScopeLookup(ScopeLookup::this_, classType->GetContainerScope()));
            baseDestructorCallLookups.push_back(FunctionScopeLookup(ScopeLookup::fileScopes, nullptr));
            std::vector<std::unique_ptr<BoundExpression>> baseDestructorCallArguments;
            FunctionSymbol* thisToBaseConversion = boundCompileUnit.GetConversion(thisParam->GetType(), classType->BaseClass()->AddPointer(destructorNode->GetSpan()), destructorNode->GetSpan());
            if (!thisToBaseConversion)
            {
                throw Exception("base class conversion not found", destructorNode->GetSpan(), classType->GetSpan());
            }
            BoundExpression* baseClassPointerConversion = new BoundConversion(std::unique_ptr<BoundExpression>(new BoundParameter(thisParam)), thisToBaseConversion);
            baseDestructorCallArguments.push_back(std::unique_ptr<BoundExpression>(baseClassPointerConversion));
            std::unique_ptr<BoundFunctionCall> baseDestructorCall = ResolveOverload(U"@destructor", containerScope, baseDestructorCallLookups, baseDestructorCallArguments, boundCompileUnit,
                boundFunction, destructorNode->GetSpan());
            boundCompoundStatement->AddStatement(std::unique_ptr<BoundStatement>(new BoundExpressionStatement(std::move(baseDestructorCall))));
        }
    }
    catch (const Exception& ex)
    {
        throw Exception("could not generate termination for class '" + ToUtf8(classType->FullName()) + "'. Reason: " + ex.Message(), destructorNode->GetSpan(), ex.References());
    }
}

Operation::Operation(const std::u32string& groupName_, int arity_, BoundCompileUnit& boundCompileUnit_) : groupName(groupName_), arity(arity_), boundCompileUnit(boundCompileUnit_)
{
}

Operation::~Operation()
{
}

SymbolTable* Operation::GetSymbolTable()
{
    return &boundCompileUnit.GetSymbolTable();
}

BoundCompileUnit& Operation::GetBoundCompileUnit()
{
    return boundCompileUnit;
}

void ArityOperation::Add(Operation* operation)
{
    operations.push_back(operation);
}

void ArityOperation::CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, std::unordered_set<FunctionSymbol*>& viableFunctions, 
    std::unique_ptr<Exception>& exception, const Span& span)
{
    for (Operation* operation : operations)
    {
        operation->CollectViableFunctions(containerScope, arguments, viableFunctions, exception, span);
    }
}

void OperationGroup::Add(Operation* operation)
{
    int arity = operation->Arity();
    if (arity >= arityOperations.size())
    {
        arityOperations.resize(arity + 1);
    }
    ArityOperation* arityOperation = arityOperations[arity].get();
    if (!arityOperation)
    {
        arityOperation = new ArityOperation();
        arityOperations[arity].reset(arityOperation);
    }
    arityOperation->Add(operation);
}

void OperationGroup::CollectViableFunctions(ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments, std::unordered_set<FunctionSymbol*>& viableFunctions, 
    std::unique_ptr<Exception>& exception, const Span& span)
{
    int arity = arguments.size();
    if (arity < arityOperations.size())
    {
        ArityOperation* arityOperation = arityOperations[arity].get();
        if (arityOperation)
        {
            arityOperation->CollectViableFunctions(containerScope, arguments, viableFunctions, exception, span);
        }
    }
}

OperationRepository::OperationRepository(BoundCompileUnit& boundCompileUnit_)
{
    Add(new LvalueReferenceCopyConstructorOperation(boundCompileUnit_));
    Add(new LvalueReferenceCopyAssignmentOperation(boundCompileUnit_));
    Add(new LvalueReferenceReturnOperation(boundCompileUnit_));
    Add(new PointerDefaultConstructorOperation(boundCompileUnit_));
    Add(new PointerCopyConstructorOperation(boundCompileUnit_));
    Add(new PointerCopyAssignmentOperation(boundCompileUnit_));
    Add(new PointerReturnOperation(boundCompileUnit_));
    Add(new PointerPlusOffsetOperation(boundCompileUnit_));
    Add(new OffsetPlusPointerOperation(boundCompileUnit_));
    Add(new PointerMinusOffsetOperation(boundCompileUnit_));
    Add(new PointerMinusPointerOperation(boundCompileUnit_));
    Add(new PointerEqualOperation(boundCompileUnit_));
    Add(new PointerLessOperation(boundCompileUnit_));
    Add(new PointerArrowOperation(boundCompileUnit_));
    Add(new ClassDefaultConstructorOperation(boundCompileUnit_));
}

void OperationRepository::Add(Operation* operation)
{
    OperationGroup* group = nullptr;
    auto it = operationGroupMap.find(operation->GroupName());
    if (it != operationGroupMap.cend())
    {
        group = it->second;
    }
    else
    {
        group = new OperationGroup();
        operationGroupMap.insert(std::make_pair(operation->GroupName(), group));
        operationGroups.push_back(std::unique_ptr<OperationGroup>(group));
    }
    group->Add(operation);
    operations.push_back(std::unique_ptr<Operation>(operation));
}

void OperationRepository::CollectViableFunctions(const std::u32string& groupName, ContainerScope* containerScope, const std::vector<std::unique_ptr<BoundExpression>>& arguments,
    std::unordered_set<FunctionSymbol*>& viableFunctions, std::unique_ptr<Exception>& exception, const Span& span)
{
    auto it = operationGroupMap.find(groupName);
    if (it != operationGroupMap.cend())
    {
        OperationGroup* operationGroup = it->second;
        operationGroup->CollectViableFunctions(containerScope, arguments, viableFunctions, exception, span);
    }
}

} } // namespace cmajor::binder
