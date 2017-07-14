// =================================
// Copyright (c) 2017 Seppo Laakko
// Distributed under the MIT license
// =================================

#ifndef CMAJOR_SYMBOLS_ENUM_SYMBOL_INCLUDED
#define CMAJOR_SYMBOLS_ENUM_SYMBOL_INCLUDED
#include <cmajor/symbols/TypeSymbol.hpp>
#include <cmajor/symbols/Value.hpp>

namespace cmajor { namespace symbols {

class EnumTypeSymbol : public TypeSymbol
{
public:
    EnumTypeSymbol(const Span& span_, const std::u32string& name_);
    void Write(SymbolWriter& writer) override;
    void Read(SymbolReader& reader) override;
    void EmplaceType(TypeSymbol* typeSymbol_, int index) override;
    std::string TypeString() const override { return "enumerated type"; }
    void SetSpecifiers(Specifiers specifiers);
    const TypeSymbol* UnderlyingType() const { return underlyingType; }
    void SetUnderlyingType(TypeSymbol* underlyingType_) { underlyingType = underlyingType_; }
    llvm::Type* IrType(Emitter& emitter) const override { return underlyingType->IrType(emitter); }
private:
    TypeSymbol* underlyingType;
};

class EnumConstantSymbol : public Symbol
{
public:
    EnumConstantSymbol(const Span& span_, const std::u32string& name_);
    SymbolAccess DeclaredAccess() const override { return SymbolAccess::public_; }
    std::string TypeString() const override { return "enumeration constant"; }
    bool Evaluating() const { return evaluating; }
    void SetEvaluating() { evaluating = true; }
    void ResetEvaluating() { evaluating = false; }
private:
    std::unique_ptr<Value> value;
    bool evaluating;
};

} } // namespace cmajor::symbols

#endif // CMAJOR_SYMBOLS_ENUM_SYMBOL_INCLUDED
