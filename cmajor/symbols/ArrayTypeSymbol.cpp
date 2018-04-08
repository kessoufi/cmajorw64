// =================================
// Copyright (c) 2018 Seppo Laakko
// Distributed under the MIT license
// =================================

#include <cmajor/symbols/ArrayTypeSymbol.hpp>
#include <cmajor/symbols/SymbolWriter.hpp>
#include <cmajor/symbols/SymbolReader.hpp>
#include <cmajor/symbols/SymbolTable.hpp>
#include <cmajor/symbols/Exception.hpp>
#include <cmajor/util/Unicode.hpp>

namespace cmajor { namespace symbols {

using namespace cmajor::unicode;

ArrayTypeSymbol::ArrayTypeSymbol(const Span& span_, const std::u32string& name_) : TypeSymbol(SymbolType::arrayTypeSymbol, span_, name_), elementType(nullptr), size(-1), irType(nullptr)
{
}

ArrayTypeSymbol::ArrayTypeSymbol(const Span& span_, const std::u32string& name_, TypeSymbol* elementType_, int64_t size_) : 
    TypeSymbol(SymbolType::arrayTypeSymbol, span_, name_), elementType(elementType_), size(size_), irType(nullptr)
{
}

void ArrayTypeSymbol::Write(SymbolWriter& writer)
{
    TypeSymbol::Write(writer);
    //uint32_t elementTypeId = elementType->TypeId();
    const boost::uuids::uuid& elementTypeId = elementType->TypeId();
    writer.GetBinaryWriter().Write(elementTypeId);
    writer.GetBinaryWriter().Write(size);
}

void ArrayTypeSymbol::Read(SymbolReader& reader)
{
    TypeSymbol::Read(reader);
    //uint32_t elementTypeId = reader.GetBinaryReader().ReadUInt();
    boost::uuids::uuid elementTypeId;
    reader.GetBinaryReader().ReadUuid(elementTypeId);
    GetSymbolTable()->EmplaceTypeRequest(this, elementTypeId, 0);
    size = reader.GetBinaryReader().ReadLong();
}

void ArrayTypeSymbol::EmplaceType(TypeSymbol* typeSymbol, int index)
{
    if (index == 0)
    {
        elementType = typeSymbol;
    }
    else
    {
        throw Exception(GetModule(), "internal error: invalid array emplace type index " + std::to_string(index), GetSpan());
    }
}

llvm::Type* ArrayTypeSymbol::IrType(Emitter& emitter)
{
    if (size == -1)
    {
        throw Exception(GetModule(), "array '" + ToUtf8(FullName()) + "' size not defined", GetSpan());
    }
    if (!irType)
    {
        irType = llvm::ArrayType::get(elementType->IrType(emitter), size);
    }
    return irType;
}

llvm::Constant* ArrayTypeSymbol::CreateDefaultIrValue(Emitter& emitter)
{
    if (size == -1)
    {
        throw Exception(GetModule(), "array '" + ToUtf8(FullName()) + "' size not defined", GetSpan());
    }
    llvm::Type* irType = IrType(emitter);
    std::vector<llvm::Constant*> arrayOfDefaults;
    for (int64_t i = 0; i < size; ++i)
    {
        arrayOfDefaults.push_back(elementType->CreateDefaultIrValue(emitter));
    }
    return llvm::ConstantArray::get(llvm::cast<llvm::ArrayType>(irType), arrayOfDefaults);
}

llvm::DIType* ArrayTypeSymbol::CreateDIType(Emitter& emitter)
{
    // todo...
    std::vector<llvm::Metadata*> elements;
    return emitter.DIBuilder()->createArrayType(size, 8, elementType->GetDIType(emitter), emitter.DIBuilder()->getOrCreateArray(elements));
}

ValueType ArrayTypeSymbol::GetValueType() const
{
    return ValueType::arrayValue;
}

Value* ArrayTypeSymbol::MakeValue() const
{
    std::vector<std::unique_ptr<Value>> elementValues;
    return new ArrayValue(GetSpan(), const_cast<TypeSymbol*>(static_cast<const TypeSymbol*>(this)), std::move(elementValues));
}

ArrayLengthFunction::ArrayLengthFunction(const Span& span_, const std::u32string& name_) : FunctionSymbol(SymbolType::arrayLengthFunctionSymbol, span_, name_), arrayType(nullptr)
{
}

ArrayLengthFunction::ArrayLengthFunction(ArrayTypeSymbol* arrayType_) : FunctionSymbol(SymbolType::arrayLengthFunctionSymbol, arrayType_->GetSpan(), U"Length"), arrayType(arrayType_)
{
    SetGroupName(U"Length");
    SetAccess(SymbolAccess::public_);
    ParameterSymbol* arrayParam = new ParameterSymbol(arrayType->GetSpan(), U"array");
    arrayParam->SetType(arrayType);
    AddMember(arrayParam);
    TypeSymbol* longType = arrayType->GetSymbolTable()->GetTypeByName(U"long");
    SetReturnType(longType);
    ComputeName();
}

void ArrayLengthFunction::Write(SymbolWriter& writer)
{
    FunctionSymbol::Write(writer);
    writer.GetBinaryWriter().Write(arrayType->TypeId());
}

void ArrayLengthFunction::Read(SymbolReader& reader)
{
    FunctionSymbol::Read(reader);
    //uint32_t typeId = reader.GetBinaryReader().ReadUInt();
    boost::uuids::uuid typeId;
    reader.GetBinaryReader().ReadUuid(typeId);
    GetSymbolTable()->EmplaceTypeRequest(this, typeId, 1);
}

void ArrayLengthFunction::EmplaceType(TypeSymbol* typeSymbol, int index)
{
    if (index == 1)
    {
        Assert(typeSymbol->GetSymbolType() == SymbolType::arrayTypeSymbol, "array type expected");
        arrayType = static_cast<ArrayTypeSymbol*>(typeSymbol);
    }
    else
    {
        FunctionSymbol::EmplaceType(typeSymbol, index);
    }
}

void ArrayLengthFunction::GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags, const Span& span)
{
    emitter.SetCurrentDebugLocation(span);
    Assert(genObjects.size() == 1, "array length needs one object");
    llvm::Value* size = emitter.Builder().getInt64(arrayType->Size());
    emitter.Stack().Push(size);
}

std::unique_ptr<Value> ArrayLengthFunction::ConstructValue(const std::vector<std::unique_ptr<Value>>& argumentValues, const Span& span) const
{
    return std::unique_ptr<Value>(new LongValue(span, arrayType->Size()));
}

ArrayBeginFunction::ArrayBeginFunction(const Span& span_, const std::u32string& name_) : FunctionSymbol(SymbolType::arrayBeginFunctionSymbol, span_, name_), arrayType(nullptr)
{
}

ArrayBeginFunction::ArrayBeginFunction(ArrayTypeSymbol* arrayType_) : FunctionSymbol(SymbolType::arrayBeginFunctionSymbol, arrayType_->GetSpan(), U"@arrayBegin"), arrayType(arrayType_)
{
    SetGroupName(U"Begin");
    SetAccess(SymbolAccess::public_);
    ParameterSymbol* arrayParam = new ParameterSymbol(arrayType->GetSpan(), U"array");
    arrayParam->SetType(arrayType);
    AddMember(arrayParam);
    TypeSymbol* returnType = arrayType->ElementType()->AddPointer(arrayType->GetSpan());
    SetReturnType(returnType);
    ComputeName();
}

void ArrayBeginFunction::Write(SymbolWriter& writer)
{
    FunctionSymbol::Write(writer);
    writer.GetBinaryWriter().Write(arrayType->TypeId());
}

void ArrayBeginFunction::Read(SymbolReader& reader)
{
    FunctionSymbol::Read(reader);
    //uint32_t typeId = reader.GetBinaryReader().ReadUInt();
    boost::uuids::uuid typeId;
    reader.GetBinaryReader().ReadUuid(typeId);
    GetSymbolTable()->EmplaceTypeRequest(this, typeId, 1);
}

void ArrayBeginFunction::EmplaceType(TypeSymbol* typeSymbol, int index)
{
    if (index == 1)
    {
        Assert(typeSymbol->GetSymbolType() == SymbolType::arrayTypeSymbol, "array type expected");
        arrayType = static_cast<ArrayTypeSymbol*>(typeSymbol);
    }
    else
    {
        FunctionSymbol::EmplaceType(typeSymbol, index);
    }
}

void ArrayBeginFunction::GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags, const Span& span)
{
    Assert(genObjects.size() == 1, "array begin needs one object");
    genObjects[0]->Load(emitter, OperationFlags::addr);
    emitter.SetCurrentDebugLocation(span);
    llvm::Value* arrayPtr = emitter.Stack().Pop();
    ArgVector elementIndeces;
    elementIndeces.push_back(emitter.Builder().getInt64(0));
    elementIndeces.push_back(emitter.Builder().getInt64(0));
    llvm::Value* beginPtr = emitter.Builder().CreateGEP(arrayPtr, elementIndeces);
    emitter.Stack().Push(beginPtr);
}

ArrayEndFunction::ArrayEndFunction(const Span& span_, const std::u32string& name_) : FunctionSymbol(SymbolType::arrayEndFunctionSymbol, span_, name_), arrayType(nullptr)
{
}

ArrayEndFunction::ArrayEndFunction(ArrayTypeSymbol* arrayType_) : FunctionSymbol(SymbolType::arrayEndFunctionSymbol, arrayType_->GetSpan(), U"@arrayEnd"), arrayType(arrayType_)
{
    SetGroupName(U"End");
    SetAccess(SymbolAccess::public_);
    ParameterSymbol* arrayParam = new ParameterSymbol(arrayType->GetSpan(), U"array");
    arrayParam->SetType(arrayType);
    AddMember(arrayParam);
    TypeSymbol* returnType = arrayType->ElementType()->AddPointer(arrayType->GetSpan());
    SetReturnType(returnType);
    ComputeName();
}

void ArrayEndFunction::Write(SymbolWriter& writer)
{
    FunctionSymbol::Write(writer);
    writer.GetBinaryWriter().Write(arrayType->TypeId());
}

void ArrayEndFunction::Read(SymbolReader& reader)
{
    FunctionSymbol::Read(reader);
    //uint32_t typeId = reader.GetBinaryReader().ReadUInt();
    boost::uuids::uuid typeId;
    reader.GetBinaryReader().ReadUuid(typeId);
    GetSymbolTable()->EmplaceTypeRequest(this, typeId, 1);
}

void ArrayEndFunction::EmplaceType(TypeSymbol* typeSymbol, int index)
{
    if (index == 1)
    {
        Assert(typeSymbol->GetSymbolType() == SymbolType::arrayTypeSymbol, "array type expected");
        arrayType = static_cast<ArrayTypeSymbol*>(typeSymbol);
    }
    else
    {
        FunctionSymbol::EmplaceType(typeSymbol, index);
    }
}

void ArrayEndFunction::GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags, const Span& span)
{
    Assert(genObjects.size() == 1, "array end needs one object");
    genObjects[0]->Load(emitter, OperationFlags::addr);
    emitter.SetCurrentDebugLocation(span);
    llvm::Value* arrayPtr = emitter.Stack().Pop();
    ArgVector elementIndeces;
    elementIndeces.push_back(emitter.Builder().getInt64(0));
    elementIndeces.push_back(emitter.Builder().getInt64(arrayType->Size()));
    llvm::Value* endPtr = emitter.Builder().CreateGEP(arrayPtr, elementIndeces);
    emitter.Stack().Push(endPtr);
}

ArrayCBeginFunction::ArrayCBeginFunction(const Span& span_, const std::u32string& name_) : FunctionSymbol(SymbolType::arrayCBeginFunctionSymbol, span_, name_), arrayType(nullptr)
{
}

ArrayCBeginFunction::ArrayCBeginFunction(ArrayTypeSymbol* arrayType_) : FunctionSymbol(SymbolType::arrayCBeginFunctionSymbol, arrayType_->GetSpan(), U"@arrayCBegin"), arrayType(arrayType_)
{
    SetGroupName(U"CBegin");
    SetAccess(SymbolAccess::public_);
    ParameterSymbol* arrayParam = new ParameterSymbol(arrayType->GetSpan(), U"array");
    arrayParam->SetType(arrayType);
    AddMember(arrayParam);
    TypeSymbol* returnType = arrayType->ElementType()->AddConst(arrayType->GetSpan())->AddPointer(arrayType->GetSpan());
    SetReturnType(returnType);
    ComputeName();
}

void ArrayCBeginFunction::Write(SymbolWriter& writer)
{
    FunctionSymbol::Write(writer);
    writer.GetBinaryWriter().Write(arrayType->TypeId());
}

void ArrayCBeginFunction::Read(SymbolReader& reader)
{
    FunctionSymbol::Read(reader);
    //uint32_t typeId = reader.GetBinaryReader().ReadUInt();
    boost::uuids::uuid typeId;
    reader.GetBinaryReader().ReadUuid(typeId);
    GetSymbolTable()->EmplaceTypeRequest(this, typeId, 1);
}

void ArrayCBeginFunction::EmplaceType(TypeSymbol* typeSymbol, int index)
{
    if (index == 1)
    {
        Assert(typeSymbol->GetSymbolType() == SymbolType::arrayTypeSymbol, "array type expected");
        arrayType = static_cast<ArrayTypeSymbol*>(typeSymbol);
    }
    else
    {
        FunctionSymbol::EmplaceType(typeSymbol, index);
    }
}

void ArrayCBeginFunction::GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags, const Span& span)
{
    Assert(genObjects.size() == 1, "array cbegin needs one object");
    genObjects[0]->Load(emitter, OperationFlags::addr);
    emitter.SetCurrentDebugLocation(span);
    llvm::Value* arrayPtr = emitter.Stack().Pop();
    ArgVector elementIndeces;
    elementIndeces.push_back(emitter.Builder().getInt64(0));
    elementIndeces.push_back(emitter.Builder().getInt64(0));
    llvm::Value* beginPtr = emitter.Builder().CreateGEP(arrayPtr, elementIndeces);
    emitter.Stack().Push(beginPtr);
}

ArrayCEndFunction::ArrayCEndFunction(const Span& span_, const std::u32string& name_) : FunctionSymbol(SymbolType::arrayCEndFunctionSymbol, span_, name_), arrayType(nullptr)
{
}

ArrayCEndFunction::ArrayCEndFunction(ArrayTypeSymbol* arrayType_) : FunctionSymbol(SymbolType::arrayCEndFunctionSymbol, arrayType_->GetSpan(), U"@arrayCEnd"), arrayType(arrayType_)
{
    SetGroupName(U"CEnd");
    SetAccess(SymbolAccess::public_);
    ParameterSymbol* arrayParam = new ParameterSymbol(arrayType->GetSpan(), U"array");
    arrayParam->SetType(arrayType);
    AddMember(arrayParam);
    TypeSymbol* returnType = arrayType->ElementType()->AddConst(arrayType->GetSpan())->AddPointer(arrayType->GetSpan());
    SetReturnType(returnType);
    ComputeName();
}

void ArrayCEndFunction::Write(SymbolWriter& writer)
{
    FunctionSymbol::Write(writer);
    writer.GetBinaryWriter().Write(arrayType->TypeId());
}

void ArrayCEndFunction::Read(SymbolReader& reader)
{
    FunctionSymbol::Read(reader);
    //uint32_t typeId = reader.GetBinaryReader().ReadUInt();
    boost::uuids::uuid typeId;
    reader.GetBinaryReader().ReadUuid(typeId);
    GetSymbolTable()->EmplaceTypeRequest(this, typeId, 1);
}

void ArrayCEndFunction::EmplaceType(TypeSymbol* typeSymbol, int index)
{
    if (index == 1)
    {
        Assert(typeSymbol->GetSymbolType() == SymbolType::arrayTypeSymbol, "array type expected");
        arrayType = static_cast<ArrayTypeSymbol*>(typeSymbol);
    }
    else
    {
        FunctionSymbol::EmplaceType(typeSymbol, index);
    }
}

void ArrayCEndFunction::GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags, const Span& span)
{
    Assert(genObjects.size() == 1, "array cend needs one object");
    genObjects[0]->Load(emitter, OperationFlags::addr);
    emitter.SetCurrentDebugLocation(span);
    llvm::Value* arrayPtr = emitter.Stack().Pop();
    ArgVector elementIndeces;
    elementIndeces.push_back(emitter.Builder().getInt64(0));
    elementIndeces.push_back(emitter.Builder().getInt64(arrayType->Size()));
    llvm::Value* endPtr = emitter.Builder().CreateGEP(arrayPtr, elementIndeces);
    emitter.Stack().Push(endPtr);
}

ArrayTypeDefaultConstructor::ArrayTypeDefaultConstructor(ArrayTypeSymbol* arrayType_, LocalVariableSymbol* loopVar_, FunctionSymbol* elementTypeDefaultConstructor_, const Span& span_) :
    FunctionSymbol(arrayType_->GetSpan(), U"@arrayDefaultCtor"), arrayType(arrayType_), loopVar(loopVar_), elementTypeDefaultConstructor(elementTypeDefaultConstructor_)
{
    SetGroupName(U"@constructor");
    SetAccess(SymbolAccess::public_); 
    ParameterSymbol* thisParam = new ParameterSymbol(span_, U"this");
    thisParam->SetType(arrayType->AddPointer(span_));
    AddMember(thisParam);
    ComputeName();
}

void ArrayTypeDefaultConstructor::GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags, const Span& span)
{
    Assert(genObjects.size() == 1, "default constructor needs one object");
    genObjects[0]->Load(emitter, OperationFlags::addr);
    llvm::Value* ptr = emitter.Stack().Pop();
    emitter.Builder().CreateStore(emitter.Builder().getInt64(0), loopVar->IrObject());
    llvm::Value* size = emitter.Builder().getInt64(arrayType->Size());
    llvm::BasicBlock* loop = llvm::BasicBlock::Create(emitter.Context(), "loop", emitter.Function());
    llvm::BasicBlock* init = llvm::BasicBlock::Create(emitter.Context(), "init", emitter.Function());
    llvm::BasicBlock* next = llvm::BasicBlock::Create(emitter.Context(), "next", emitter.Function());
    emitter.Builder().CreateBr(loop);
    emitter.SetCurrentBasicBlock(loop);
    llvm::Value* index = emitter.Builder().CreateLoad(loopVar->IrObject());
    llvm::Value* less = emitter.Builder().CreateICmpULT(index, size);
    emitter.Builder().CreateCondBr(less, init, next);
    emitter.SetCurrentBasicBlock(init);
    ArgVector elementIndeces;
    elementIndeces.push_back(emitter.Builder().getInt64(0));
    elementIndeces.push_back(index);
    llvm::Value* elementPtr = emitter.Builder().CreateGEP(ptr, elementIndeces);
    LlvmValue elementPtrValue(elementPtr);
    std::vector<GenObject*> elementGenObjects;
    elementGenObjects.push_back(&elementPtrValue);
    elementTypeDefaultConstructor->GenerateCall(emitter, elementGenObjects, OperationFlags::none, span);
    llvm::Value* nextI = emitter.Builder().CreateAdd(index, emitter.Builder().getInt64(1));
    emitter.Builder().CreateStore(nextI, loopVar->IrObject());
    emitter.Builder().CreateBr(loop);
    emitter.SetCurrentBasicBlock(next);
}

ArrayTypeCopyConstructor::ArrayTypeCopyConstructor(ArrayTypeSymbol* arrayType_, LocalVariableSymbol* loopVar_, FunctionSymbol* elementTypeCopyConstructor_, const Span& span_) :
    FunctionSymbol(arrayType_->GetSpan(), U"@arrayCopyCtor"), arrayType(arrayType_), loopVar(loopVar_), elementTypeCopyConstructor(elementTypeCopyConstructor_)
{
    SetGroupName(U"@constructor");
    SetAccess(SymbolAccess::public_);
    ParameterSymbol* thisParam = new ParameterSymbol(span_, U"this");
    thisParam->SetType(arrayType->AddPointer(span_));
    AddMember(thisParam);
    ParameterSymbol* thatParam = new ParameterSymbol(span_, U"that");
    thatParam->SetType(arrayType->AddConst(span_)->AddLvalueReference(span_));
    AddMember(thatParam);
    ComputeName();
}

void ArrayTypeCopyConstructor::GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags, const Span& span)
{
    Assert(genObjects.size() == 2, "copy constructor needs two objects");
    genObjects[0]->Load(emitter, OperationFlags::addr);
    llvm::Value* ptr = emitter.Stack().Pop();
    genObjects[1]->Load(emitter, OperationFlags::none);
    llvm::Value* sourcePtr = emitter.Stack().Pop();
    emitter.Builder().CreateStore(emitter.Builder().getInt64(0), loopVar->IrObject());
    llvm::Value* size = emitter.Builder().getInt64(arrayType->Size());
    llvm::BasicBlock* loop = llvm::BasicBlock::Create(emitter.Context(), "loop", emitter.Function());
    llvm::BasicBlock* init = llvm::BasicBlock::Create(emitter.Context(), "init", emitter.Function());
    llvm::BasicBlock* next = llvm::BasicBlock::Create(emitter.Context(), "next", emitter.Function());
    emitter.Builder().CreateBr(loop);
    emitter.SetCurrentBasicBlock(loop);
    llvm::Value* index = emitter.Builder().CreateLoad(loopVar->IrObject());
    llvm::Value* less = emitter.Builder().CreateICmpULT(index, size);
    emitter.Builder().CreateCondBr(less, init, next);
    emitter.SetCurrentBasicBlock(init);
    ArgVector elementIndeces;
    elementIndeces.push_back(emitter.Builder().getInt64(0));
    elementIndeces.push_back(index);
    llvm::Value* elementPtr = emitter.Builder().CreateGEP(ptr, elementIndeces);
    LlvmValue elementPtrValue(elementPtr);
    std::vector<GenObject*> elementGenObjects;
    elementGenObjects.push_back(&elementPtrValue);
    llvm::Value* sourceElementPtr = emitter.Builder().CreateGEP(sourcePtr, elementIndeces);
    llvm::Value* sourceElementValue = sourceElementPtr;
    TypeSymbol* elementType = arrayType->ElementType();
    if (elementType->IsBasicTypeSymbol() || elementType->IsPointerType() || elementType->GetSymbolType() == SymbolType::delegateTypeSymbol)
    {
        sourceElementValue = emitter.Builder().CreateLoad(sourceElementPtr);
    }
    LlvmValue sourceValue(sourceElementValue);
    elementGenObjects.push_back(&sourceValue);
    elementTypeCopyConstructor->GenerateCall(emitter, elementGenObjects, OperationFlags::none, span);
    llvm::Value* nextI = emitter.Builder().CreateAdd(index, emitter.Builder().getInt64(1));
    emitter.Builder().CreateStore(nextI, loopVar->IrObject());
    emitter.Builder().CreateBr(loop);
    emitter.SetCurrentBasicBlock(next);
}

ArrayTypeMoveConstructor::ArrayTypeMoveConstructor(ArrayTypeSymbol* arrayType_, LocalVariableSymbol* loopVar_, FunctionSymbol* elementTypeMoveConstructor_, const Span& span_) :
    FunctionSymbol(arrayType_->GetSpan(), U"@arrayMoveCtor"), arrayType(arrayType_), loopVar(loopVar_), elementTypeMoveConstructor(elementTypeMoveConstructor_)
{
    SetGroupName(U"@constructor");
    SetAccess(SymbolAccess::public_);
    ParameterSymbol* thisParam = new ParameterSymbol(span_, U"this");
    thisParam->SetType(arrayType->AddPointer(span_));
    AddMember(thisParam);
    ParameterSymbol* thatParam = new ParameterSymbol(span_, U"that");
    thatParam->SetType(arrayType->AddRvalueReference(span_));
    AddMember(thatParam);
    ComputeName();
}

void ArrayTypeMoveConstructor::GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags, const Span& span)
{
    Assert(genObjects.size() == 2, "move constructor needs two objects");
    genObjects[0]->Load(emitter, OperationFlags::addr);
    llvm::Value* ptr = emitter.Stack().Pop();
    genObjects[1]->Load(emitter, OperationFlags::none);
    llvm::Value* sourcePtr = emitter.Stack().Pop();
    emitter.Builder().CreateStore(emitter.Builder().getInt64(0), loopVar->IrObject());
    llvm::Value* size = emitter.Builder().getInt64(arrayType->Size());
    llvm::BasicBlock* loop = llvm::BasicBlock::Create(emitter.Context(), "loop", emitter.Function());
    llvm::BasicBlock* init = llvm::BasicBlock::Create(emitter.Context(), "init", emitter.Function());
    llvm::BasicBlock* next = llvm::BasicBlock::Create(emitter.Context(), "next", emitter.Function());
    emitter.Builder().CreateBr(loop);
    emitter.SetCurrentBasicBlock(loop);
    llvm::Value* index = emitter.Builder().CreateLoad(loopVar->IrObject());
    llvm::Value* less = emitter.Builder().CreateICmpULT(index, size);
    emitter.Builder().CreateCondBr(less, init, next);
    emitter.SetCurrentBasicBlock(init);
    ArgVector elementIndeces;
    elementIndeces.push_back(emitter.Builder().getInt64(0));
    elementIndeces.push_back(index);
    llvm::Value* elementPtr = emitter.Builder().CreateGEP(ptr, elementIndeces);
    LlvmValue elementPtrValue(elementPtr);
    std::vector<GenObject*> elementGenObjects;
    elementGenObjects.push_back(&elementPtrValue);
    llvm::Value* sourceElementPtr = emitter.Builder().CreateGEP(sourcePtr, elementIndeces);
    LlvmValue sourcePtrValue(sourceElementPtr);
    elementGenObjects.push_back(&sourcePtrValue);
    elementTypeMoveConstructor->GenerateCall(emitter, elementGenObjects, OperationFlags::none, span);
    llvm::Value* nextI = emitter.Builder().CreateAdd(index, emitter.Builder().getInt64(1));
    emitter.Builder().CreateStore(nextI, loopVar->IrObject());
    emitter.Builder().CreateBr(loop);
    emitter.SetCurrentBasicBlock(next);
}

ArrayTypeCopyAssignment::ArrayTypeCopyAssignment(ArrayTypeSymbol* arrayType_, LocalVariableSymbol* loopVar_, FunctionSymbol* elementTypeCopyAssignment_, const Span& span_) :
    FunctionSymbol(arrayType_->GetSpan(), U"@arrayCopyAssignment"), arrayType(arrayType_), loopVar(loopVar_), elementTypeCopyAssignment(elementTypeCopyAssignment_)
{
    SetGroupName(U"operator=");
    SetAccess(SymbolAccess::public_);
    ParameterSymbol* thisParam = new ParameterSymbol(span_, U"this");
    thisParam->SetType(arrayType->AddPointer(span_));
    AddMember(thisParam);
    ParameterSymbol* thatParam = new ParameterSymbol(span_, U"that");
    thatParam->SetType(arrayType->AddConst(span_)->AddLvalueReference(span_));
    AddMember(thatParam);
    TypeSymbol* voidType = arrayType->GetSymbolTable()->GetTypeByName(U"void");
    SetReturnType(voidType);
    ComputeName();
}

void ArrayTypeCopyAssignment::GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags, const Span& span)
{
    Assert(genObjects.size() == 2, "copy assignment needs two objects");
    genObjects[0]->Load(emitter, OperationFlags::addr);
    llvm::Value* ptr = emitter.Stack().Pop();
    genObjects[1]->Load(emitter, OperationFlags::none);
    llvm::Value* sourcePtr = emitter.Stack().Pop();
    emitter.Builder().CreateStore(emitter.Builder().getInt64(0), loopVar->IrObject());
    llvm::Value* size = emitter.Builder().getInt64(arrayType->Size());
    llvm::BasicBlock* loop = llvm::BasicBlock::Create(emitter.Context(), "loop", emitter.Function());
    llvm::BasicBlock* init = llvm::BasicBlock::Create(emitter.Context(), "init", emitter.Function());
    llvm::BasicBlock* next = llvm::BasicBlock::Create(emitter.Context(), "next", emitter.Function());
    emitter.Builder().CreateBr(loop);
    emitter.SetCurrentBasicBlock(loop);
    llvm::Value* index = emitter.Builder().CreateLoad(loopVar->IrObject());
    llvm::Value* less = emitter.Builder().CreateICmpULT(index, size);
    emitter.Builder().CreateCondBr(less, init, next);
    emitter.SetCurrentBasicBlock(init);
    ArgVector elementIndeces;
    elementIndeces.push_back(emitter.Builder().getInt64(0));
    elementIndeces.push_back(index);
    llvm::Value* elementPtr = emitter.Builder().CreateGEP(ptr, elementIndeces);
    LlvmValue elementPtrValue(elementPtr);
    std::vector<GenObject*> elementGenObjects;
    elementGenObjects.push_back(&elementPtrValue);
    llvm::Value* sourceElementPtr = emitter.Builder().CreateGEP(sourcePtr, elementIndeces);
    llvm::Value* sourceElementValue = sourceElementPtr;
    TypeSymbol* elementType = arrayType->ElementType();
    if (elementType->IsBasicTypeSymbol() || elementType->IsPointerType() || elementType->GetSymbolType() == SymbolType::delegateTypeSymbol)
    {
        sourceElementValue = emitter.Builder().CreateLoad(sourceElementPtr);
    }
    LlvmValue sourceValue(sourceElementValue);
    elementGenObjects.push_back(&sourceValue);
    elementTypeCopyAssignment->GenerateCall(emitter, elementGenObjects, OperationFlags::none, span);
    llvm::Value* nextI = emitter.Builder().CreateAdd(index, emitter.Builder().getInt64(1));
    emitter.Builder().CreateStore(nextI, loopVar->IrObject());
    emitter.Builder().CreateBr(loop);
    emitter.SetCurrentBasicBlock(next);
}

ArrayTypeMoveAssignment::ArrayTypeMoveAssignment(ArrayTypeSymbol* arrayType_, LocalVariableSymbol* loopVar_, FunctionSymbol* elementTypeMoveAssignment_, const Span& span_) :
    FunctionSymbol(arrayType_->GetSpan(), U"@arrayMoveAssignment"), arrayType(arrayType_), loopVar(loopVar_), elementTypeMoveAssignment(elementTypeMoveAssignment_)
{
    SetGroupName(U"operator=");
    SetAccess(SymbolAccess::public_);
    ParameterSymbol* thisParam = new ParameterSymbol(span_, U"this");
    thisParam->SetType(arrayType->AddPointer(span_));
    AddMember(thisParam);
    ParameterSymbol* thatParam = new ParameterSymbol(span_, U"that");
    thatParam->SetType(arrayType->AddRvalueReference(span_));
    AddMember(thatParam);
    TypeSymbol* voidType = arrayType->GetSymbolTable()->GetTypeByName(U"void");
    SetReturnType(voidType);
    ComputeName();
}

void ArrayTypeMoveAssignment::GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags, const Span& span)
{
    Assert(genObjects.size() == 2, "move assignment needs two objects");
    genObjects[0]->Load(emitter, OperationFlags::addr);
    llvm::Value* ptr = emitter.Stack().Pop();
    genObjects[1]->Load(emitter, OperationFlags::none);
    llvm::Value* sourcePtr = emitter.Stack().Pop();
    emitter.Builder().CreateStore(emitter.Builder().getInt64(0), loopVar->IrObject());
    llvm::Value* size = emitter.Builder().getInt64(arrayType->Size());
    llvm::BasicBlock* loop = llvm::BasicBlock::Create(emitter.Context(), "loop", emitter.Function());
    llvm::BasicBlock* init = llvm::BasicBlock::Create(emitter.Context(), "init", emitter.Function());
    llvm::BasicBlock* next = llvm::BasicBlock::Create(emitter.Context(), "next", emitter.Function());
    emitter.Builder().CreateBr(loop);
    emitter.SetCurrentBasicBlock(loop);
    llvm::Value* index = emitter.Builder().CreateLoad(loopVar->IrObject());
    llvm::Value* less = emitter.Builder().CreateICmpULT(index, size);
    emitter.Builder().CreateCondBr(less, init, next);
    emitter.SetCurrentBasicBlock(init);
    ArgVector elementIndeces;
    elementIndeces.push_back(emitter.Builder().getInt64(0));
    elementIndeces.push_back(index);
    llvm::Value* elementPtr = emitter.Builder().CreateGEP(ptr, elementIndeces);
    LlvmValue elementPtrValue(elementPtr);
    std::vector<GenObject*> elementGenObjects;
    elementGenObjects.push_back(&elementPtrValue);
    llvm::Value* sourceElementPtr = emitter.Builder().CreateGEP(sourcePtr, elementIndeces);
    TypeSymbol* elementType = arrayType->ElementType();
    LlvmValue sourcePtrValue(sourceElementPtr);
    elementGenObjects.push_back(&sourcePtrValue);
    elementTypeMoveAssignment->GenerateCall(emitter, elementGenObjects, OperationFlags::none, span);
    llvm::Value* nextI = emitter.Builder().CreateAdd(index, emitter.Builder().getInt64(1));
    emitter.Builder().CreateStore(nextI, loopVar->IrObject());
    emitter.Builder().CreateBr(loop);
    emitter.SetCurrentBasicBlock(next);
}

ArrayTypeElementAccess::ArrayTypeElementAccess(ArrayTypeSymbol* arrayType_, const Span& span_) : FunctionSymbol(arrayType_->GetSpan(), U"@arrayElementAccess"), arrayType(arrayType_)
{
    SetGroupName(U"operator[]");
    SetAccess(SymbolAccess::public_);
    ParameterSymbol* arrayParam = new ParameterSymbol(span_, U"array");
    arrayParam->SetType(arrayType);
    AddMember(arrayParam);
    ParameterSymbol* indexParam = new ParameterSymbol(span_, U"index");
    indexParam->SetType(arrayType->GetSymbolTable()->GetTypeByName(U"long"));
    AddMember(indexParam);
    TypeSymbol* returnType = arrayType->ElementType();
    if (!returnType->IsBasicTypeSymbol() && !returnType->IsPointerType() && returnType->GetSymbolType() != SymbolType::delegateTypeSymbol)
    {
        returnType = returnType->AddLvalueReference(span_);
    }
    SetReturnType(returnType);
    ComputeName();
}

void ArrayTypeElementAccess::GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags, const Span& span)
{
    Assert(genObjects.size() == 2, "element access needs two objects");
    genObjects[0]->Load(emitter, OperationFlags::addr);
    llvm::Value* ptr = emitter.Stack().Pop();
    genObjects[1]->Load(emitter, OperationFlags::none);
    emitter.SetCurrentDebugLocation(span);
    llvm::Value* indexValue = emitter.Stack().Pop();
    ArgVector elementIndeces;
    elementIndeces.push_back(emitter.Builder().getInt64(0));
    elementIndeces.push_back(indexValue);
    llvm::Value* elementPtr = emitter.Builder().CreateGEP(ptr, elementIndeces);
    TypeSymbol* elementType = arrayType->ElementType();
    if ((flags & OperationFlags::addr) == OperationFlags::none && (elementType->IsBasicTypeSymbol() || elementType->IsPointerType() || elementType->GetSymbolType() == SymbolType::delegateTypeSymbol))
    {
        llvm::Value* elementValue = emitter.Builder().CreateLoad(elementPtr);
        emitter.Stack().Push(elementValue);
    }
    else
    {
        emitter.Stack().Push(elementPtr);
    }
}

} } // namespace cmajor::symbols
