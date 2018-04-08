// =================================
// Copyright (c) 2018 Seppo Laakko
// Distributed under the MIT license
// =================================

#include <cmajor/symbols/BasicTypeSymbol.hpp>
#include <cmajor/symbols/SymbolWriter.hpp>
#include <cmajor/symbols/SymbolReader.hpp>
#include <cmajor/symbols/SymbolTable.hpp>
#include <cmajor/symbols/SymbolCollector.hpp>
#include <cmajor/util/Unicode.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace cmajor { namespace symbols {

using namespace cmajor::unicode;

BasicTypeSymbol::BasicTypeSymbol(SymbolType symbolType_, const Span& span_, const std::u32string& name_) : 
    TypeSymbol(symbolType_, span_, name_), 
    defaultConstructor(nullptr), copyConstructor(nullptr), moveConstructor(nullptr), copyAssignment(nullptr), moveAssignment(nullptr), returnFun(nullptr), equalityOp(nullptr)
{
}

/*
void BasicTypeSymbol::Write(SymbolWriter& writer)
{
    TypeSymbol::Write(writer);
    uint32_t defaultConstructorId = 0;
    if (defaultConstructor)
    {
        defaultConstructorId = defaultConstructor->FunctionId();
    }
    writer.GetBinaryWriter().Write(defaultConstructorId);
    uint32_t copyConstructorId = 0;
    if (copyConstructor)
    {
        copyConstructorId = copyConstructor->FunctionId();
    }
    writer.GetBinaryWriter().Write(copyConstructorId);
    uint32_t moveConstructorId = 0;
    if (moveConstructor)
    {
        moveConstructorId = moveConstructor->FunctionId();
    }
    writer.GetBinaryWriter().Write(moveConstructorId);
    uint32_t copyAssignmentId = 0;
    if (copyAssignment)
    {
        copyAssignmentId = copyAssignment->FunctionId();
    }
    writer.GetBinaryWriter().Write(copyAssignmentId);
    uint32_t moveAssignmentId = 0;
    if (moveAssignment)
    {
        moveAssignmentId = moveAssignment->FunctionId();
    }
    writer.GetBinaryWriter().Write(moveAssignmentId);
    uint32_t returnId = 0;
    if (returnFun)
    {
        returnId = returnFun->FunctionId();
    }
    writer.GetBinaryWriter().Write(returnId);
    uint32_t equalityOpId = 0;
    if (equalityOp)
    {
        equalityOpId = equalityOp->FunctionId();
    }
    writer.GetBinaryWriter().Write(equalityOpId);
}

void BasicTypeSymbol::Read(SymbolReader& reader)
{
    TypeSymbol::Read(reader);
    uint32_t defaultConstructorId = reader.GetBinaryReader().ReadUInt();
    if (defaultConstructorId != 0)
    {
        GetSymbolTable()->EmplaceFunctionRequest(this, defaultConstructorId, 0);
    }
    uint32_t copyConstructorId = reader.GetBinaryReader().ReadUInt();
    if (copyConstructorId != 0)
    {
        GetSymbolTable()->EmplaceFunctionRequest(this, copyConstructorId, 1);
    }
    uint32_t moveConstructorId = reader.GetBinaryReader().ReadUInt();
    if (moveConstructorId != 0)
    {
        GetSymbolTable()->EmplaceFunctionRequest(this, moveConstructorId, 2);
    }
    uint32_t copyAssignmentId = reader.GetBinaryReader().ReadUInt();
    if (copyAssignmentId != 0)
    {
        GetSymbolTable()->EmplaceFunctionRequest(this, copyAssignmentId, 3);
    }
    uint32_t moveAssignmentId = reader.GetBinaryReader().ReadUInt();
    if (moveAssignmentId != 0)
    {
        GetSymbolTable()->EmplaceFunctionRequest(this, moveAssignmentId, 4);
    }
    uint32_t returnId = reader.GetBinaryReader().ReadUInt();
    if (returnId != 0)
    {
        GetSymbolTable()->EmplaceFunctionRequest(this, returnId, 5);
    }
    uint32_t equalityOpId = reader.GetBinaryReader().ReadUInt();
    if (equalityOpId != 0)
    {
        GetSymbolTable()->EmplaceFunctionRequest(this, equalityOpId, 6);
    }
}
*/

void BasicTypeSymbol::Write(SymbolWriter& writer)
{
    TypeSymbol::Write(writer);
    boost::uuids::uuid defaultConstructorId = boost::uuids::nil_generator()();
    if (defaultConstructor)
    {
        defaultConstructorId = defaultConstructor->FunctionId();
    }
    writer.GetBinaryWriter().Write(defaultConstructorId);
    boost::uuids::uuid copyConstructorId = boost::uuids::nil_generator()();
    if (copyConstructor)
    {
        copyConstructorId = copyConstructor->FunctionId();
    }
    writer.GetBinaryWriter().Write(copyConstructorId);
    boost::uuids::uuid moveConstructorId = boost::uuids::nil_generator()();
    if (moveConstructor)
    {
        moveConstructorId = moveConstructor->FunctionId();
    }
    writer.GetBinaryWriter().Write(moveConstructorId);
    boost::uuids::uuid copyAssignmentId = boost::uuids::nil_generator()();
    if (copyAssignment)
    {
        copyAssignmentId = copyAssignment->FunctionId();
    }
    writer.GetBinaryWriter().Write(copyAssignmentId);
    boost::uuids::uuid moveAssignmentId = boost::uuids::nil_generator()();
    if (moveAssignment)
    {
        moveAssignmentId = moveAssignment->FunctionId();
    }
    writer.GetBinaryWriter().Write(moveAssignmentId);
    boost::uuids::uuid returnId = boost::uuids::nil_generator()();
    if (returnFun)
    {
        returnId = returnFun->FunctionId();
    }
    writer.GetBinaryWriter().Write(returnId);
    boost::uuids::uuid equalityOpId = boost::uuids::nil_generator()();
    if (equalityOp)
    {
        equalityOpId = equalityOp->FunctionId();
    }
    writer.GetBinaryWriter().Write(equalityOpId);
}

void BasicTypeSymbol::Read(SymbolReader& reader)
{
    TypeSymbol::Read(reader);
    boost::uuids::uuid defaultConstructorId;
    reader.GetBinaryReader().ReadUuid(defaultConstructorId);
    if (!defaultConstructorId.is_nil())
    {
        GetSymbolTable()->EmplaceFunctionRequest(this, defaultConstructorId, 0);
    }
    boost::uuids::uuid copyConstructorId;
    reader.GetBinaryReader().ReadUuid(copyConstructorId);
    if (!copyConstructorId.is_nil())
    {
        GetSymbolTable()->EmplaceFunctionRequest(this, copyConstructorId, 1);
    }
    boost::uuids::uuid moveConstructorId;
    reader.GetBinaryReader().ReadUuid(moveConstructorId);
    if (!moveConstructorId.is_nil())
    {
        GetSymbolTable()->EmplaceFunctionRequest(this, moveConstructorId, 2);
    }
    boost::uuids::uuid copyAssignmentId;
    reader.GetBinaryReader().ReadUuid(copyAssignmentId);
    if (!copyAssignmentId.is_nil())
    {
        GetSymbolTable()->EmplaceFunctionRequest(this, copyAssignmentId, 3);
    }
    boost::uuids::uuid moveAssignmentId;
    reader.GetBinaryReader().ReadUuid(moveAssignmentId);
    if (!moveAssignmentId.is_nil())
    {
        GetSymbolTable()->EmplaceFunctionRequest(this, moveAssignmentId, 4);
    }
    boost::uuids::uuid returnId;
    reader.GetBinaryReader().ReadUuid(returnId);
    if (!returnId.is_nil())
    {
        GetSymbolTable()->EmplaceFunctionRequest(this, returnId, 5);
    }
    boost::uuids::uuid equalityOpId;
    reader.GetBinaryReader().ReadUuid(equalityOpId);
    if (!equalityOpId.is_nil())
    {
        GetSymbolTable()->EmplaceFunctionRequest(this, equalityOpId, 6);
    }
}

void BasicTypeSymbol::EmplaceFunction(FunctionSymbol* functionSymbol, int index)
{
    switch (index)
    {
        case 0: defaultConstructor = functionSymbol; break;
        case 1: copyConstructor = functionSymbol; break;
        case 2: moveConstructor = functionSymbol; break;
        case 3: copyAssignment = functionSymbol; break;
        case 4: moveAssignment = functionSymbol; break;
        case 5: returnFun = functionSymbol; break;
        case 6: equalityOp = functionSymbol; break;
        default:
        {
            Assert(false, "invalid emplace function index");
        }
    }
}

void BasicTypeSymbol::Accept(SymbolCollector* collector)
{
    if (IsProject())
    {
        collector->AddBasicType(this);
    }
}

void BasicTypeSymbol::Dump(CodeFormatter& formatter)
{
    formatter.WriteLine(ToUtf8(Name()));
    //formatter.WriteLine("typeid: " + std::to_string(TypeId()));
    formatter.WriteLine("typeid: " + boost::uuids::to_string(TypeId()));
}

BoolTypeSymbol::BoolTypeSymbol(const Span& span_, const std::u32string& name_) : BasicTypeSymbol(SymbolType::boolTypeSymbol, span_, name_)
{
}

ValueType BoolTypeSymbol::GetValueType() const
{
    return ValueType::boolValue;
}

Value* BoolTypeSymbol::MakeValue() const
{
    return new BoolValue(GetSpan(), false);
}

llvm::DIType* BoolTypeSymbol::CreateDIType(Emitter& emitter) 
{
    return emitter.DIBuilder()->createBasicType("bool", 8, llvm::dwarf::DW_ATE_boolean); 
}

SByteTypeSymbol::SByteTypeSymbol(const Span& span_, const std::u32string& name_) : BasicTypeSymbol(SymbolType::sbyteTypeSymbol, span_, name_)
{
}

ValueType SByteTypeSymbol::GetValueType() const
{
    return ValueType::sbyteValue;
}

Value* SByteTypeSymbol::MakeValue() const
{
    return new SByteValue(GetSpan(), 0);
}

llvm::DIType* SByteTypeSymbol::CreateDIType(Emitter& emitter)
{
    return emitter.DIBuilder()->createBasicType("sbyte", 8, llvm::dwarf::DW_ATE_signed);
}

ByteTypeSymbol::ByteTypeSymbol(const Span& span_, const std::u32string& name_) : BasicTypeSymbol(SymbolType::byteTypeSymbol, span_, name_)
{
}

ValueType ByteTypeSymbol::GetValueType() const
{
    return ValueType::byteValue;
}

Value* ByteTypeSymbol::MakeValue() const
{
    return new ByteValue(GetSpan(), 0);
}

llvm::DIType* ByteTypeSymbol::CreateDIType(Emitter& emitter)
{
    return emitter.DIBuilder()->createBasicType("byte", 8, llvm::dwarf::DW_ATE_unsigned);
}

ShortTypeSymbol::ShortTypeSymbol(const Span& span_, const std::u32string& name_) : BasicTypeSymbol(SymbolType::shortTypeSymbol, span_, name_)
{
}

ValueType ShortTypeSymbol::GetValueType() const
{
    return ValueType::shortValue;
}

Value* ShortTypeSymbol::MakeValue() const
{
    return new ShortValue(GetSpan(), 0);
}

llvm::DIType* ShortTypeSymbol::CreateDIType(Emitter& emitter)
{
    return emitter.DIBuilder()->createBasicType("short", 16, llvm::dwarf::DW_ATE_signed);
}

UShortTypeSymbol::UShortTypeSymbol(const Span& span_, const std::u32string& name_) : BasicTypeSymbol(SymbolType::ushortTypeSymbol, span_, name_)
{
}

ValueType UShortTypeSymbol::GetValueType() const
{
    return ValueType::ushortValue;
}

Value* UShortTypeSymbol::MakeValue() const
{
    return new UShortValue(GetSpan(), 0);
}

llvm::DIType* UShortTypeSymbol::CreateDIType(Emitter& emitter)
{
    return emitter.DIBuilder()->createBasicType("ushort", 16, llvm::dwarf::DW_ATE_unsigned);
}

IntTypeSymbol::IntTypeSymbol(const Span& span_, const std::u32string& name_) : BasicTypeSymbol(SymbolType::intTypeSymbol, span_, name_)
{
}

ValueType IntTypeSymbol::GetValueType() const
{
    return ValueType::intValue;
}

Value* IntTypeSymbol::MakeValue() const
{
    return new IntValue(GetSpan(), 0);
}

llvm::DIType* IntTypeSymbol::CreateDIType(Emitter& emitter)
{
    return emitter.DIBuilder()->createBasicType("int", 32, llvm::dwarf::DW_ATE_signed);
}

UIntTypeSymbol::UIntTypeSymbol(const Span& span_, const std::u32string& name_) : BasicTypeSymbol(SymbolType::uintTypeSymbol, span_, name_)
{
}

ValueType UIntTypeSymbol::GetValueType() const
{
    return ValueType::uintValue;
}

Value* UIntTypeSymbol::MakeValue() const
{
    return new UIntValue(GetSpan(), 0);
}

llvm::DIType* UIntTypeSymbol::CreateDIType(Emitter& emitter)
{
    return emitter.DIBuilder()->createBasicType("uint", 32, llvm::dwarf::DW_ATE_unsigned);
}

LongTypeSymbol::LongTypeSymbol(const Span& span_, const std::u32string& name_) : BasicTypeSymbol(SymbolType::longTypeSymbol, span_, name_)
{
}

ValueType LongTypeSymbol::GetValueType() const
{
    return ValueType::longValue;
}

Value* LongTypeSymbol::MakeValue() const
{
    return new LongValue(GetSpan(), 0);
}

llvm::DIType* LongTypeSymbol::CreateDIType(Emitter& emitter)
{
    return emitter.DIBuilder()->createBasicType("long", 64, llvm::dwarf::DW_ATE_signed);
}

ULongTypeSymbol::ULongTypeSymbol(const Span& span_, const std::u32string& name_) : BasicTypeSymbol(SymbolType::ulongTypeSymbol, span_, name_)
{
}

ValueType ULongTypeSymbol::GetValueType() const
{
    return ValueType::ulongValue;
}

Value* ULongTypeSymbol::MakeValue() const
{
    return new ULongValue(GetSpan(), 0);
}

llvm::DIType* ULongTypeSymbol::CreateDIType(Emitter& emitter)
{
    return emitter.DIBuilder()->createBasicType("ulong", 64, llvm::dwarf::DW_ATE_unsigned);
}

FloatTypeSymbol::FloatTypeSymbol(const Span& span_, const std::u32string& name_) : BasicTypeSymbol(SymbolType::floatTypeSymbol, span_, name_)
{
}

ValueType FloatTypeSymbol::GetValueType() const
{
    return ValueType::floatValue;
}

Value* FloatTypeSymbol::MakeValue() const
{
    return new FloatValue(GetSpan(), 0.0);
}

llvm::DIType* FloatTypeSymbol::CreateDIType(Emitter& emitter)
{
    return emitter.DIBuilder()->createBasicType("float", 32, llvm::dwarf::DW_ATE_float);
}

DoubleTypeSymbol::DoubleTypeSymbol(const Span& span_, const std::u32string& name_) : BasicTypeSymbol(SymbolType::doubleTypeSymbol, span_, name_)
{
}

ValueType DoubleTypeSymbol::GetValueType() const
{
    return ValueType::doubleValue;
}

Value* DoubleTypeSymbol::MakeValue() const
{
    return new DoubleValue(GetSpan(), 0.0);
}

llvm::DIType* DoubleTypeSymbol::CreateDIType(Emitter& emitter)
{
    return emitter.DIBuilder()->createBasicType("double", 64, llvm::dwarf::DW_ATE_float);
}

CharTypeSymbol::CharTypeSymbol(const Span& span_, const std::u32string& name_) : BasicTypeSymbol(SymbolType::charTypeSymbol, span_, name_)
{
}

ValueType CharTypeSymbol::GetValueType() const
{
    return ValueType::charValue;
}

Value* CharTypeSymbol::MakeValue() const
{
    return new CharValue(GetSpan(), '\0');
}

llvm::DIType* CharTypeSymbol::CreateDIType(Emitter& emitter)
{
    return emitter.DIBuilder()->createBasicType("char", 8, llvm::dwarf::DW_ATE_unsigned_char);
}

WCharTypeSymbol::WCharTypeSymbol(const Span& span_, const std::u32string& name_) : BasicTypeSymbol(SymbolType::wcharTypeSymbol, span_, name_)
{
}

ValueType WCharTypeSymbol::GetValueType() const
{
    return ValueType::wcharValue;
}

Value* WCharTypeSymbol::MakeValue() const
{
    return new WCharValue(GetSpan(), '\0');
}

llvm::DIType* WCharTypeSymbol::CreateDIType(Emitter& emitter)
{
    return emitter.DIBuilder()->createBasicType("wchar", 16, llvm::dwarf::DW_ATE_unsigned_char);
}

UCharTypeSymbol::UCharTypeSymbol(const Span& span_, const std::u32string& name_) : BasicTypeSymbol(SymbolType::ucharTypeSymbol, span_, name_)
{
}

ValueType UCharTypeSymbol::GetValueType() const
{
    return ValueType::ucharValue;
}

Value* UCharTypeSymbol::MakeValue() const
{
    return new UCharValue(GetSpan(), '\0');
}

llvm::DIType* UCharTypeSymbol::CreateDIType(Emitter& emitter)
{
    return emitter.DIBuilder()->createBasicType("uchar", 32, llvm::dwarf::DW_ATE_unsigned_char);
}

VoidTypeSymbol::VoidTypeSymbol(const Span& span_, const std::u32string& name_) : BasicTypeSymbol(SymbolType::voidTypeSymbol, span_, name_)
{
}

llvm::DIType* VoidTypeSymbol::CreateDIType(Emitter& emitter)
{
    return emitter.DIBuilder()->createUnspecifiedType("void");
}

} } // namespace cmajor::symbols
