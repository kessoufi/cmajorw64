// =================================
// Copyright (c) 2017 Seppo Laakko
// Distributed under the MIT license
// =================================

#include <cmajor/symbols/ConstantSymbol.hpp>
#include <cmajor/symbols/TypeSymbol.hpp>
#include <cmajor/symbols/SymbolTable.hpp>
#include <cmajor/symbols/SymbolWriter.hpp>
#include <cmajor/symbols/SymbolReader.hpp>
#include <cmajor/symbols/Exception.hpp>
#include <cmajor/symbols/SymbolCollector.hpp>
#include <cmajor/emitter/Emitter.hpp>
#include <cmajor/util/Unicode.hpp>
#include <llvm/IR/Module.h>

namespace cmajor { namespace symbols {

using namespace cmajor::unicode;

ConstantSymbol::ConstantSymbol(const Span& span_, const std::u32string& name_) : Symbol(SymbolType::constantSymbol, span_, name_), type(), evaluating(false)
{
}

void ConstantSymbol::Write(SymbolWriter& writer)
{
    Symbol::Write(writer);
    writer.GetBinaryWriter().Write(type->TypeId());
    bool hasExternalValue = type->IsBasicTypeSymbol() || type->IsEnumeratedType();
    writer.GetBinaryWriter().Write(hasExternalValue);
    if (hasExternalValue)
    {
        WriteValue(value.get(), writer.GetBinaryWriter());
    }
}

void ConstantSymbol::Read(SymbolReader& reader)
{
    Symbol::Read(reader);
    uint32_t typeId = reader.GetBinaryReader().ReadUInt();
    GetSymbolTable()->EmplaceTypeRequest(this, typeId, 0);
    bool hasExternalValue = reader.GetBinaryReader().ReadBool();
    if (hasExternalValue)
    {
        value = ReadValue(reader.GetBinaryReader(), GetSpan());
    }
}

void ConstantSymbol::EmplaceType(TypeSymbol* typeSymbol, int index)
{
    Assert(index == 0, "invalid emplace type index");
    type = typeSymbol;
}

void ConstantSymbol::Accept(SymbolCollector* collector)
{
    if (IsProject() && Access() == SymbolAccess::public_)
    {
        collector->AddConstant(this);
    }
}

void ConstantSymbol::Dump(CodeFormatter& formatter)
{
    formatter.WriteLine(ToUtf8(Name()));
    formatter.WriteLine("full name: " + ToUtf8(FullNameWithSpecifiers()));
    formatter.WriteLine("mangled name: " + ToUtf8(MangledName()));
    formatter.WriteLine("type: " + ToUtf8(type->FullName()));
    formatter.WriteLine("value: " + value->ToString());
}

void ConstantSymbol::SetSpecifiers(Specifiers specifiers)
{
    Specifiers accessSpecifiers = specifiers & Specifiers::access_;
    SetAccess(accessSpecifiers);
    if ((specifiers & Specifiers::static_) != Specifiers::none)
    {
        throw Exception("constant cannot be static", GetSpan());
    }
    if ((specifiers & Specifiers::virtual_) != Specifiers::none)
    {
        throw Exception("constant cannot be virtual", GetSpan());
    }
    if ((specifiers & Specifiers::override_) != Specifiers::none)
    {
        throw Exception("constant cannot be override", GetSpan());
    }
    if ((specifiers & Specifiers::abstract_) != Specifiers::none)
    {
        throw Exception("constant cannot be abstract", GetSpan());
    }
    if ((specifiers & Specifiers::inline_) != Specifiers::none)
    {
        throw Exception("constant cannot be inline", GetSpan());
    }
    if ((specifiers & Specifiers::explicit_) != Specifiers::none)
    {
        throw Exception("constant cannot be explicit", GetSpan());
    }
    if ((specifiers & Specifiers::external_) != Specifiers::none)
    {
        throw Exception("constant cannot be external", GetSpan());
    }
    if ((specifiers & Specifiers::suppress_) != Specifiers::none)
    {
        throw Exception("constant cannot be suppressed", GetSpan());
    }
    if ((specifiers & Specifiers::default_) != Specifiers::none)
    {
        throw Exception("constant cannot be default", GetSpan());
    }
    if ((specifiers & Specifiers::constexpr_) != Specifiers::none)
    {
        throw Exception("constant cannot be constexpr", GetSpan());
    }
    if ((specifiers & Specifiers::cdecl_) != Specifiers::none)
    {
        throw Exception("constant cannot be cdecl", GetSpan());
    }
    if ((specifiers & Specifiers::nothrow_) != Specifiers::none)
    {
        throw Exception("constant cannot be nothrow", GetSpan());
    }
    if ((specifiers & Specifiers::throw_) != Specifiers::none)
    {
        throw Exception("constant cannot be throw", GetSpan());
    }
    if ((specifiers & Specifiers::new_) != Specifiers::none)
    {
        throw Exception("constant cannot be new", GetSpan());
    }
    if ((specifiers & Specifiers::const_) != Specifiers::none)
    {
        throw Exception("constant cannot be const", GetSpan());
    }
    if ((specifiers & Specifiers::unit_test_) != Specifiers::none)
    {
        throw Exception("constant cannot be unit_test", GetSpan());
    }
}

std::string ConstantSymbol::Syntax() const
{
    std::string syntax = GetSpecifierStr();
    if (!syntax.empty())
    {
        syntax.append(1, ' ');
    }
    syntax.append("const ");
    syntax.append(ToUtf8(GetType()->DocName()));
    syntax.append(1, ' ');
    syntax.append(ToUtf8(DocName()));
    syntax.append(" = ");
    std::string valueStr = value->ToString();
    if (GetType()->IsUnsignedType())
    {
        valueStr.append(1, 'u');
    }
    syntax.append(valueStr);
    syntax.append(1, ';');
    return syntax;
}

void ConstantSymbol::SetValue(Value* value_)
{
    value.reset(value_);
}

llvm::Value* ConstantSymbol::ArrayIrObject(Emitter& emitter, bool create)
{
    if (!type->IsArrayType())
    {
        throw Exception("internal error: array object expected", GetSpan());
    }
    if (!value)
    {
        throw Exception("internal error: array value missing", GetSpan());
    }
    if (value->GetValueType() != ValueType::arrayValue)
    {
        throw Exception("internal error: array value expected", GetSpan());
    }
    ArrayValue* arrayValue = static_cast<ArrayValue*>(value.get());
    llvm::ArrayType* irArrayType = llvm::cast<llvm::ArrayType>(type->IrType(emitter));
    llvm::Constant* irArrayObject = emitter.Module()->getOrInsertGlobal(ToUtf8(MangledName()), irArrayType);
    if (create)
    {
        llvm::GlobalVariable* arrayObjectGlobal = llvm::cast<llvm::GlobalVariable>(irArrayObject);
        arrayObjectGlobal->setInitializer(llvm::cast<llvm::Constant>(arrayValue->IrValue(emitter)));
    }
    return irArrayObject;
}

} } // namespace cmajor::symbols
