// =================================
// Copyright (c) 2017 Seppo Laakko
// Distributed under the MIT license
// =================================

#ifndef CMAJOR_SYMBOLS_BASIC_TYPE_OPERATION_INCLUDED
#define CMAJOR_SYMBOLS_BASIC_TYPE_OPERATION_INCLUDED
#include <cmajor/symbols/BasicTypeSymbol.hpp>
#include <cmajor/symbols/FunctionSymbol.hpp>
#include <cmajor/symbols/VariableSymbol.hpp>
#include <cmajor/symbols/SymbolTable.hpp>
#include <cmajor/symbols/SymbolWriter.hpp>
#include <cmajor/symbols/SymbolReader.hpp>
#include <cmajor/ir/Emitter.hpp>
#include <llvm/IR/Constants.h>

namespace cmajor { namespace symbols {

struct BasicTypeNot
{
    static const char32_t* GroupName() { return U"operator!"; }
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* arg) { return builder.CreateNot(arg); }
};

struct BasicTypeUnaryPlus
{
    static const char32_t* GroupName() { return U"operator+"; }
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* arg) { return arg; }
};

struct BasicTypeIntUnaryMinus
{
    static const char32_t* GroupName() { return U"operator-"; }
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* arg) { return builder.CreateNeg(arg); }
};

struct BasicTypeFloatUnaryMinus
{
    static const char32_t* GroupName() { return U"operator-"; }
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* arg) { return builder.CreateFNeg(arg); }
};

struct BasicTypeComplement
{
    static const char32_t* GroupName() { return U"operator~"; }
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* arg) { return builder.CreateNot(arg); }
};

struct BasicTypeAdd
{
    static const char32_t* GroupName() { return U"operator+"; }
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* left, llvm::Value* right) { return builder.CreateAdd(left, right); }
};

struct BasicTypeFAdd
{
    static const char32_t* GroupName() { return U"operator+"; }
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* left, llvm::Value* right) { return builder.CreateFAdd(left, right); }
};

struct BasicTypeSub
{
    static const char32_t* GroupName() { return U"operator-"; }
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* left, llvm::Value* right) { return builder.CreateSub(left, right); }
};

struct BasicTypeFSub
{
    static const char32_t* GroupName() { return U"operator-"; }
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* left, llvm::Value* right) { return builder.CreateFSub(left, right); }
};

struct BasicTypeMul
{
    static const char32_t* GroupName() { return U"operator*"; }
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* left, llvm::Value* right) { return builder.CreateMul(left, right); }
};

struct BasicTypeFMul
{
    static const char32_t* GroupName() { return U"operator*"; }
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* left, llvm::Value* right) { return builder.CreateFMul(left, right); }
};

struct BasicTypeUDiv
{
    static const char32_t* GroupName() { return U"operator/"; }
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* left, llvm::Value* right) { return builder.CreateUDiv(left, right); }
};

struct BasicTypeSDiv
{
    static const char32_t* GroupName() { return U"operator/"; }
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* left, llvm::Value* right) { return builder.CreateSDiv(left, right); }
};

struct BasicTypeFDiv
{
    static const char32_t* GroupName() { return U"operator/"; }
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* left, llvm::Value* right) { return builder.CreateFDiv(left, right); }
};

struct BasicTypeURem
{
    static const char32_t* GroupName() { return U"operator%"; }
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* left, llvm::Value* right) { return builder.CreateURem(left, right); }
};

struct BasicTypeSRem
{
    static const char32_t* GroupName() { return U"operator%"; }
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* left, llvm::Value* right) { return builder.CreateSRem(left, right); }
};

struct BasicTypeAnd
{
    static const char32_t* GroupName() { return U"operator&"; }
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* left, llvm::Value* right) { return builder.CreateAnd(left, right); }
};

struct BasicTypeOr
{
    static const char32_t* GroupName() { return U"operator|"; }
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* left, llvm::Value* right) { return builder.CreateOr(left, right); }
};

struct BasicTypeXor
{
    static const char32_t* GroupName() { return U"operator^"; }
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* left, llvm::Value* right) { return builder.CreateXor(left, right); }
};

struct BasicTypeShl
{
    static const char32_t* GroupName() { return U"operator<<"; }
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* left, llvm::Value* right) { return builder.CreateShl(left, right); }
};

struct BasicTypeAShr
{
    static const char32_t* GroupName() { return U"operator>>"; }
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* left, llvm::Value* right) { return builder.CreateAShr(left, right); }
};

struct BasicTypeLShr
{
    static const char32_t* GroupName() { return U"operator>>"; }
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* left, llvm::Value* right) { return builder.CreateLShr(left, right); }
};

struct DefaultInt1
{
    static llvm::Value* Generate(llvm::IRBuilder<>& builder) { return builder.getInt1(false); }
};

struct DefaultInt8
{
    static llvm::Value* Generate(llvm::IRBuilder<>& builder) { return builder.getInt8(0); }
};

struct DefaultInt16
{
    static llvm::Value* Generate(llvm::IRBuilder<>& builder) { return builder.getInt16(0); }
};

struct DefaultInt32
{
    static llvm::Value* Generate(llvm::IRBuilder<>& builder) { return builder.getInt32(0); }
};

struct DefaultInt64
{
    static llvm::Value* Generate(llvm::IRBuilder<>& builder) { return builder.getInt64(0); }
};

struct DefaultFloat
{
    static llvm::Value* Generate(llvm::IRBuilder<>& builder) { return llvm::ConstantFP::get(llvm::Type::getFloatTy(builder.getContext()), 0.0); }
};

struct DefaultDouble
{
    static llvm::Value* Generate(llvm::IRBuilder<>& builder) { return llvm::ConstantFP::get(llvm::Type::getDoubleTy(builder.getContext()), 0.0); }
};

struct BasicTypeIntegerEquality
{
    static const char32_t* GroupName() { return U"operator=="; }
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* left, llvm::Value* right) { return builder.CreateICmpEQ(left, right); }
};

struct BasicTypeFloatingEquality
{
    static const char32_t* GroupName() { return U"operator=="; }
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* left, llvm::Value* right) { return builder.CreateFCmpOEQ(left, right); }
};

struct BasicTypeUnsignedIntegerLessThan
{
    static const char32_t* GroupName() { return U"operator<"; }
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* left, llvm::Value* right) { return builder.CreateICmpULT(left, right); }
};

struct BasicTypeSignedIntegerLessThan
{
    static const char32_t* GroupName() { return U"operator<"; }
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* left, llvm::Value* right) { return builder.CreateICmpSLT(left, right); }
};

struct BasicTypeFloatingLessThan
{
    static const char32_t* GroupName() { return U"operator<"; }
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* left, llvm::Value* right) { return builder.CreateFCmpOLT(left, right); }
};

struct BasicTypeSignExtension
{
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* operand, llvm::Type* destinationType) { return builder.CreateSExt(operand, destinationType); }
};

struct BasicTypeZeroExtension
{
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* operand, llvm::Type* destinationType) { return builder.CreateZExt(operand, destinationType); }
};

struct BasicTypeFloatingExtension
{
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* operand, llvm::Type* destinationType) { return builder.CreateFPExt(operand, destinationType); }
};

struct BasicTypeTruncation
{
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* operand, llvm::Type* destinationType) { return builder.CreateTrunc(operand, destinationType); }
};

struct BasicTypeFloatingTruncation
{
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* operand, llvm::Type* destinationType) { return builder.CreateFPTrunc(operand, destinationType); }
};

struct BasicTypeBitCast
{
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* operand, llvm::Type* destinationType) { return builder.CreateBitCast(operand, destinationType); }
};

struct BasicTypeUnsignedIntToFloating
{
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* operand, llvm::Type* destinationType) { return builder.CreateUIToFP(operand, destinationType); }
};

struct BasicTypeSignedIntToFloating
{
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* operand, llvm::Type* destinationType) { return builder.CreateSIToFP(operand, destinationType); }
};

struct BasicTypeFloatingToUnsignedInt
{
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* operand, llvm::Type* destinationType) { return builder.CreateFPToUI(operand, destinationType); };
};

struct BasicTypeFloatingToSignedInt
{
    static llvm::Value* Generate(llvm::IRBuilder<>& builder, llvm::Value* operand, llvm::Type* destinationType) { return builder.CreateFPToSI(operand, destinationType); };
};

template<typename UnOp>
class BasicTypeUnaryOperation : public FunctionSymbol
{
public:
    BasicTypeUnaryOperation(SymbolType symbolType);
    BasicTypeUnaryOperation(SymbolType symbolType, TypeSymbol* type);
    SymbolAccess DeclaredAccess() const override { return SymbolAccess::public_; }
    void GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags) override;
    bool IsBasicTypeOperation() const override { return true; }
};

template<typename UnOp>
BasicTypeUnaryOperation<UnOp>::BasicTypeUnaryOperation(SymbolType symbolType) : FunctionSymbol(symbolType, Span(), UnOp::GroupName())
{
}

template<typename UnOp>
BasicTypeUnaryOperation<UnOp>::BasicTypeUnaryOperation(SymbolType symbolType, TypeSymbol* type) : FunctionSymbol(symbolType, Span(), UnOp::GroupName())
{
    SetGroupName(UnOp::GroupName());
    SetAccess(SymbolAccess::public_);
    ParameterSymbol* operandParam = new ParameterSymbol(Span(), U"operand");
    operandParam->SetType(type);
    AddMember(operandParam);
    SetReturnType(type);
    ComputeName();
}

template<typename UnOp>
void BasicTypeUnaryOperation<UnOp>::GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags)
{
    Assert(genObjects.size() == 1, "unary operation needs one object");
    genObjects[0]->Load(emitter, OperationFlags::none);
    llvm::Value* arg = emitter.Stack().Pop();
    emitter.Stack().Push(UnOp::Generate(emitter.Builder(), arg));
}

class BasicTypeUnaryPlusOperation : public BasicTypeUnaryOperation<BasicTypeUnaryPlus>
{
public:
    BasicTypeUnaryPlusOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeIntUnaryMinusOperation : public BasicTypeUnaryOperation<BasicTypeIntUnaryMinus>
{
public:
    BasicTypeIntUnaryMinusOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeFloatUnaryMinusOperation : public BasicTypeUnaryOperation<BasicTypeFloatUnaryMinus>
{
public:
    BasicTypeFloatUnaryMinusOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeComplementOperation : public BasicTypeUnaryOperation<BasicTypeComplement>
{
public:
    BasicTypeComplementOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeNotOperation : public BasicTypeUnaryOperation<BasicTypeNot>
{
public:
    BasicTypeNotOperation(const Span& span_, const std::u32string& name_);
};

template<typename BinOp>
class BasicTypeBinaryOperation : public FunctionSymbol
{
public:
    BasicTypeBinaryOperation(SymbolType symbolType);
    BasicTypeBinaryOperation(SymbolType symbolType, TypeSymbol* type);
    SymbolAccess DeclaredAccess() const override { return SymbolAccess::public_; }
    void GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags) override;
    bool IsBasicTypeOperation() const override { return true; }
};

template<typename BinOp>
BasicTypeBinaryOperation<BinOp>::BasicTypeBinaryOperation(SymbolType symbolType) : FunctionSymbol(symbolType, Span(), BinOp::GroupName())
{
}

template<typename BinOp>
BasicTypeBinaryOperation<BinOp>::BasicTypeBinaryOperation(SymbolType symbolType, TypeSymbol* type) : FunctionSymbol(symbolType, Span(), BinOp::GroupName())
{
    SetGroupName(BinOp::GroupName());
    SetAccess(SymbolAccess::public_);
    ParameterSymbol* leftParam = new ParameterSymbol(Span(), U"left");
    leftParam->SetType(type);
    AddMember(leftParam);
    ParameterSymbol* rightParam = new ParameterSymbol(Span(), U"right");
    rightParam->SetType(type);
    AddMember(rightParam);
    SetReturnType(type);
    ComputeName();
}

template<typename BinOp>
void BasicTypeBinaryOperation<BinOp>::GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags)
{
    Assert(genObjects.size() == 2, "binary operation needs two objects");
    genObjects[0]->Load(emitter, OperationFlags::none);
    llvm::Value* left = emitter.Stack().Pop();
    genObjects[1]->Load(emitter, OperationFlags::none);
    llvm::Value* right = emitter.Stack().Pop();
    emitter.Stack().Push(BinOp::Generate(emitter.Builder(), left, right));
}

class BasicTypeAddOperation : public BasicTypeBinaryOperation<BasicTypeAdd>
{
public:
    BasicTypeAddOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeFAddOperation : public BasicTypeBinaryOperation<BasicTypeFAdd>
{
public:
    BasicTypeFAddOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeSubOperation : public BasicTypeBinaryOperation<BasicTypeSub>
{
public:
    BasicTypeSubOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeFSubOperation : public BasicTypeBinaryOperation<BasicTypeFSub>
{
public:
    BasicTypeFSubOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeMulOperation : public BasicTypeBinaryOperation<BasicTypeMul>
{
public:
    BasicTypeMulOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeFMulOperation : public BasicTypeBinaryOperation<BasicTypeFMul>
{
public:
    BasicTypeFMulOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeSDivOperation : public BasicTypeBinaryOperation<BasicTypeSDiv>
{
public:
    BasicTypeSDivOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeUDivOperation : public BasicTypeBinaryOperation<BasicTypeUDiv>
{
public:
    BasicTypeUDivOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeFDivOperation : public BasicTypeBinaryOperation<BasicTypeFDiv>
{
public:
    BasicTypeFDivOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeSRemOperation : public BasicTypeBinaryOperation<BasicTypeSRem>
{
public:
    BasicTypeSRemOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeURemOperation : public BasicTypeBinaryOperation<BasicTypeURem>
{
public:
    BasicTypeURemOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeAndOperation : public BasicTypeBinaryOperation<BasicTypeAnd>
{
public:
    BasicTypeAndOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeOrOperation : public BasicTypeBinaryOperation<BasicTypeOr>
{
public:
    BasicTypeOrOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeXorOperation : public BasicTypeBinaryOperation<BasicTypeXor>
{
public:
    BasicTypeXorOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeShlOperation : public BasicTypeBinaryOperation<BasicTypeShl>
{
public:
    BasicTypeShlOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeAShrOperation : public BasicTypeBinaryOperation<BasicTypeAShr>
{
public:
    BasicTypeAShrOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeLShrOperation : public BasicTypeBinaryOperation<BasicTypeLShr>
{
public:
    BasicTypeLShrOperation(const Span& span_, const std::u32string& name_);
};

template<typename DefaultOp>
class BasicTypeDefaultCtor : public FunctionSymbol
{
public:
    BasicTypeDefaultCtor(SymbolType symbolType);
    BasicTypeDefaultCtor(SymbolType symbolType, TypeSymbol* type);
    SymbolAccess DeclaredAccess() const override { return SymbolAccess::public_; }
    void GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags) override;
    bool IsBasicTypeOperation() const override { return true; }
};

template<typename DefaultOp>
BasicTypeDefaultCtor<DefaultOp>::BasicTypeDefaultCtor(SymbolType symbolType) : FunctionSymbol(symbolType, Span(), U"@constructor")
{
}

template<typename DefaultOp>
BasicTypeDefaultCtor<DefaultOp>::BasicTypeDefaultCtor(SymbolType symbolType, TypeSymbol* type) : FunctionSymbol(symbolType, Span(), U"@constructor")
{
    SetGroupName(U"@constructor");
    SetAccess(SymbolAccess::public_);
    ParameterSymbol* thisParam = new ParameterSymbol(Span(), U"this");
    thisParam->SetType(type->AddPointer(Span()));
    AddMember(thisParam);
    ComputeName();
}

template<typename DefaultOp>
void BasicTypeDefaultCtor<DefaultOp>::GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags)
{
    Assert(genObjects.size() == 1, "default constructor needs one object");
    emitter.Stack().Push(DefaultOp::Generate(emitter.Builder()));
    genObjects[0]->Store(emitter, OperationFlags::none);
}

class BasicTypeDefaultInt1Operation : public BasicTypeDefaultCtor<DefaultInt1>
{
public:
    BasicTypeDefaultInt1Operation(const Span& span_, const std::u32string& name_);
};

class BasicTypeDefaultInt8Operation : public BasicTypeDefaultCtor<DefaultInt8>
{
public:
    BasicTypeDefaultInt8Operation(const Span& span_, const std::u32string& name_);
};

class BasicTypeDefaultInt16Operation : public BasicTypeDefaultCtor<DefaultInt16>
{
public:
    BasicTypeDefaultInt16Operation(const Span& span_, const std::u32string& name_);
};

class BasicTypeDefaultInt32Operation : public BasicTypeDefaultCtor<DefaultInt32>
{
public:
    BasicTypeDefaultInt32Operation(const Span& span_, const std::u32string& name_);
};

class BasicTypeDefaultInt64Operation : public BasicTypeDefaultCtor<DefaultInt64>
{
public:
    BasicTypeDefaultInt64Operation(const Span& span_, const std::u32string& name_);
};

class BasicTypeDefaultFloatOperation : public BasicTypeDefaultCtor<DefaultFloat>
{
public:
    BasicTypeDefaultFloatOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeDefaultDoubleOperation : public BasicTypeDefaultCtor<DefaultDouble>
{
public:
    BasicTypeDefaultDoubleOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeCopyCtor : public FunctionSymbol
{
public:
    BasicTypeCopyCtor(TypeSymbol* type);
    BasicTypeCopyCtor(const Span& span_, const std::u32string& name_);
    SymbolAccess DeclaredAccess() const override { return SymbolAccess::public_; }
    void GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags) override;
    bool IsBasicTypeOperation() const override { return true; }
};

class BasicTypeCopyAssignment : public FunctionSymbol
{
public:
    BasicTypeCopyAssignment(TypeSymbol* type, TypeSymbol* voidType);
    BasicTypeCopyAssignment(const Span& span_, const std::u32string& name_);
    SymbolAccess DeclaredAccess() const override { return SymbolAccess::public_; }
    void GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags) override;
    bool IsBasicTypeOperation() const override { return true; }
};

class BasicTypeReturn : public FunctionSymbol
{
public:
    BasicTypeReturn(TypeSymbol* type);
    BasicTypeReturn(const Span& span_, const std::u32string& name_);
    SymbolAccess DeclaredAccess() const override { return SymbolAccess::public_; }
    void GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags) override;
    bool IsBasicTypeOperation() const override { return true; }
};

template <typename ConversionOp>
class BasicTypeConversion : public FunctionSymbol
{
public:
    BasicTypeConversion(SymbolType symbolType);
    BasicTypeConversion(SymbolType symbolType, const std::u32string& name_, ConversionType conversionType_, uint8_t conversionDistance, TypeSymbol* sourceType_, TypeSymbol* targetType_);
    void Write(SymbolWriter& writer) override;
    void Read(SymbolReader& reader) override;
    void EmplaceType(TypeSymbol* typeSymbol_, int index) override;
    SymbolAccess DeclaredAccess() const override { return SymbolAccess::public_; }
    void GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags) override;
    bool IsBasicTypeOperation() const override { return true; }
    ConversionType GetConversionType() const override { return conversionType; }
    uint8_t ConversionDistance() const override { return conversionDistance; }
    TypeSymbol* ConversionSourceType() const override { return sourceType; }
    TypeSymbol* ConversionTargetType() const override { return targetType; }
private:
    ConversionType conversionType;
    uint8_t conversionDistance;
    TypeSymbol* sourceType;
    TypeSymbol* targetType;
};

template <typename ConversionOp>
BasicTypeConversion<ConversionOp>::BasicTypeConversion(SymbolType symbolType) :
    FunctionSymbol(symbolType, Span(), U"@conversion"), conversionType(ConversionType::implicit_), conversionDistance(0), sourceType(nullptr), targetType(nullptr)
{
    SetGroupName(U"@conversion");
    SetConversion();
}

template <typename ConversionOp>
BasicTypeConversion<ConversionOp>::BasicTypeConversion(SymbolType symbolType, const std::u32string& name_, ConversionType conversionType_, uint8_t conversionDistance_, TypeSymbol* sourceType_, TypeSymbol* targetType_) :
    FunctionSymbol(symbolType, Span(), name_), conversionType(conversionType_), conversionDistance(conversionDistance_), sourceType(sourceType_), targetType(targetType_)
{
    SetGroupName(U"@conversion");
    SetConversion();
}

template <typename ConversionOp>
void BasicTypeConversion<ConversionOp>::Write(SymbolWriter& writer)
{
    FunctionSymbol::Write(writer);
    writer.GetBinaryWriter().Write(static_cast<uint8_t>(conversionType));
    writer.GetBinaryWriter().Write(conversionDistance);
    writer.GetBinaryWriter().WriteEncodedUInt(sourceType->TypeId());
    writer.GetBinaryWriter().WriteEncodedUInt(targetType->TypeId());
}

template <typename ConversionOp>
void BasicTypeConversion<ConversionOp>::Read(SymbolReader& reader)
{
    FunctionSymbol::Read(reader);
    conversionType = static_cast<ConversionType>(reader.GetBinaryReader().ReadByte());
    conversionDistance = reader.GetBinaryReader().ReadByte();
    uint32_t sourceTypeId = reader.GetBinaryReader().ReadEncodedUInt();
    GetSymbolTable()->EmplaceTypeRequest(this, sourceTypeId, 1);
    uint32_t targetTypeId = reader.GetBinaryReader().ReadEncodedUInt();
    GetSymbolTable()->EmplaceTypeRequest(this, targetTypeId, 2);
}

template <typename ConversionOp>
void BasicTypeConversion<ConversionOp>::EmplaceType(TypeSymbol* typeSymbol_, int index)
{
    if (index == 1)
    {
        sourceType = typeSymbol_;
    }
    else if (index == 2)
    {
        targetType = typeSymbol_;
    }
    else
    {
        FunctionSymbol::EmplaceType(typeSymbol_, index);
    }
}

template <typename ConversionOp>
void BasicTypeConversion<ConversionOp>::GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags)
{
    llvm::Value* value = emitter.Stack().Pop();
    emitter.Stack().Push(ConversionOp::Generate(emitter.Builder(), value, targetType->IrType(emitter)));
}

template <typename ConversionOp>
class BasicTypeImplicitConversion : public BasicTypeConversion<ConversionOp>
{
public:
    BasicTypeImplicitConversion(SymbolType symbolType);
    BasicTypeImplicitConversion(SymbolType symbolType, const std::u32string& name_, uint8_t conversionDistance, TypeSymbol* sourceType_, TypeSymbol* targetType_);
};

template <typename ConversionOp>
BasicTypeImplicitConversion<ConversionOp>::BasicTypeImplicitConversion(SymbolType symbolType) : BasicTypeConversion<ConversionOp>(symbolType)
{
}

template <typename ConversionOp>
BasicTypeImplicitConversion<ConversionOp>::BasicTypeImplicitConversion(SymbolType symbolType, const std::u32string& name_, uint8_t conversionDistance, TypeSymbol* sourceType_, TypeSymbol* targetType_) :
    BasicTypeConversion<ConversionOp>(symbolType, name_, ConversionType::implicit_, conversionDistance, sourceType_, targetType_)
{
}

template <typename ConversionOp>
class BasicTypeExplicitConversion : public BasicTypeConversion<ConversionOp>
{
public:
    BasicTypeExplicitConversion(SymbolType symbolType);
    BasicTypeExplicitConversion(SymbolType symbolType, const std::u32string& name_, TypeSymbol* sourceType_, TypeSymbol* targetType_);
};

template <typename ConversionOp>
BasicTypeExplicitConversion<ConversionOp>::BasicTypeExplicitConversion(SymbolType symbolType) : BasicTypeConversion<ConversionOp>(symbolType)
{
}

template <typename ConversionOp>
BasicTypeExplicitConversion<ConversionOp>::BasicTypeExplicitConversion(SymbolType symbolType, const std::u32string& name_, TypeSymbol* sourceType_, TypeSymbol* targetType_) :
    BasicTypeConversion<ConversionOp>(symbolType, name_, ConversionType::explicit_, 255, sourceType_, targetType_)
{
}

class BasicTypeImplicitSignExtensionOperation : public BasicTypeImplicitConversion<BasicTypeSignExtension>
{
public:
    BasicTypeImplicitSignExtensionOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeImplicitZeroExtensionOperation : public BasicTypeImplicitConversion<BasicTypeZeroExtension>
{
public:
    BasicTypeImplicitZeroExtensionOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeExplicitSignExtensionOperation : public BasicTypeExplicitConversion<BasicTypeSignExtension>
{
public:
    BasicTypeExplicitSignExtensionOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeExplicitZeroExtensionOperation : public BasicTypeExplicitConversion<BasicTypeZeroExtension>
{
public:
    BasicTypeExplicitZeroExtensionOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeTruncationOperation : public BasicTypeExplicitConversion<BasicTypeTruncation>
{
public:
    BasicTypeTruncationOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeBitCastOperation : public BasicTypeExplicitConversion<BasicTypeBitCast>
{
public:
    BasicTypeBitCastOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeImplicitUnsignedIntToFloatingOperation : public BasicTypeImplicitConversion<BasicTypeUnsignedIntToFloating>
{
public:
    BasicTypeImplicitUnsignedIntToFloatingOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeImplicitSignedIntToFloatingOperation : public BasicTypeImplicitConversion<BasicTypeSignedIntToFloating>
{
public:
    BasicTypeImplicitSignedIntToFloatingOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeExplicitUnsignedIntToFloatingOperation : public BasicTypeExplicitConversion<BasicTypeUnsignedIntToFloating>
{
public:
    BasicTypeExplicitUnsignedIntToFloatingOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeExplicitSignedIntToFloatingOperation : public BasicTypeExplicitConversion<BasicTypeSignedIntToFloating>
{
public:
    BasicTypeExplicitSignedIntToFloatingOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeFloatingToUnsignedIntOperation : public BasicTypeExplicitConversion<BasicTypeFloatingToUnsignedInt>
{
public:
    BasicTypeFloatingToUnsignedIntOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeFloatingToSignedIntOperation : public BasicTypeExplicitConversion<BasicTypeFloatingToSignedInt>
{
public:
    BasicTypeFloatingToSignedIntOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeFloatingExtensionOperation : public BasicTypeImplicitConversion<BasicTypeFloatingExtension>
{
public:
    BasicTypeFloatingExtensionOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeFloatingTruncationOperation : public BasicTypeExplicitConversion<BasicTypeFloatingTruncation>
{
public:
    BasicTypeFloatingTruncationOperation(const Span& span_, const std::u32string& name_);
};

template<typename ComparisonOp>
class BasicTypeComparisonOperation : public FunctionSymbol
{
public:
    BasicTypeComparisonOperation(SymbolType symbolType);
    BasicTypeComparisonOperation(SymbolType symbolType, TypeSymbol* type, TypeSymbol* boolType);
    SymbolAccess DeclaredAccess() const override { return SymbolAccess::public_; }
    void GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags) override;
    bool IsBasicTypeOperation() const override { return true; }
};

template<typename ComparisonOp>
BasicTypeComparisonOperation<ComparisonOp>::BasicTypeComparisonOperation(SymbolType symbolType) : FunctionSymbol(symbolType, Span(), ComparisonOp::GroupName())
{
}

template<typename ComparisonOp>
BasicTypeComparisonOperation<ComparisonOp>::BasicTypeComparisonOperation(SymbolType symbolType, TypeSymbol* type, TypeSymbol* boolType) : FunctionSymbol(symbolType, Span(), ComparisonOp::GroupName())
{
    SetGroupName(ComparisonOp::GroupName());
    SetAccess(SymbolAccess::public_);
    ParameterSymbol* leftParam = new ParameterSymbol(Span(), U"left");
    leftParam->SetType(type);
    AddMember(leftParam);
    ParameterSymbol* rightParam = new ParameterSymbol(Span(), U"right");
    rightParam->SetType(type);
    AddMember(rightParam);
    SetReturnType(boolType);
    ComputeName();
}

template<typename ComparisonOp>
void BasicTypeComparisonOperation<ComparisonOp>::GenerateCall(Emitter& emitter, std::vector<GenObject*>& genObjects, OperationFlags flags)
{
    Assert(genObjects.size() == 2, "comparison operation needs two objects");
    genObjects[0]->Load(emitter, OperationFlags::none);
    llvm::Value* left = emitter.Stack().Pop();
    genObjects[1]->Load(emitter, OperationFlags::none);
    llvm::Value* right = emitter.Stack().Pop();
    emitter.Stack().Push(ComparisonOp::Generate(emitter.Builder(), left, right));
}

class BasicTypeIntegerEqualityOperation : public BasicTypeComparisonOperation<BasicTypeIntegerEquality>
{
public:
    BasicTypeIntegerEqualityOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeFloatingEqualityOperation : public BasicTypeComparisonOperation<BasicTypeFloatingEquality>
{
public:
    BasicTypeFloatingEqualityOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeUnsignedIntegerLessThanOperation : public BasicTypeComparisonOperation<BasicTypeUnsignedIntegerLessThan>
{
public:
    BasicTypeUnsignedIntegerLessThanOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeSignedIntegerLessThanOperation : public BasicTypeComparisonOperation<BasicTypeSignedIntegerLessThan>
{
public:
    BasicTypeSignedIntegerLessThanOperation(const Span& span_, const std::u32string& name_);
};

class BasicTypeFloatingLessThanOperation : public BasicTypeComparisonOperation<BasicTypeFloatingLessThan>
{
public:
    BasicTypeFloatingLessThanOperation(const Span& span_, const std::u32string& name_);
};

void MakeBasicTypeOperations(SymbolTable& symbolTable,
    BoolTypeSymbol* boolType, SByteTypeSymbol* sbyteType, ByteTypeSymbol* byteType, ShortTypeSymbol* shortType, UShortTypeSymbol* ushortType, IntTypeSymbol* intType, UIntTypeSymbol* uintType,
    LongTypeSymbol* longType, ULongTypeSymbol* ulongType, FloatTypeSymbol* floatType, DoubleTypeSymbol* doubleType, CharTypeSymbol* charType, WCharTypeSymbol* wcharType, UCharTypeSymbol* ucharType,
    VoidTypeSymbol* voidType);

} } // namespace cmajor::symbols

#endif // CMAJOR_SYMBOLS_BASIC_TYPE_OPERATION_INCLUDED