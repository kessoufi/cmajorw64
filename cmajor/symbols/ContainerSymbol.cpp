// =================================
// Copyright (c) 2017 Seppo Laakko
// Distributed under the MIT license
// =================================

#include <cmajor/symbols/ContainerSymbol.hpp>
#include <cmajor/symbols/FunctionSymbol.hpp>
#include <cmajor/symbols/VariableSymbol.hpp>
#include <cmajor/symbols/SymbolWriter.hpp>
#include <cmajor/symbols/SymbolReader.hpp>
#include <cmajor/symbols/Exception.hpp>
#include <cmajor/util/Unicode.hpp>

namespace cmajor { namespace symbols {

using namespace cmajor::unicode;

ContainerSymbol::ContainerSymbol(SymbolType symbolType_, const Span& span_, const std::u32string& name_) : Symbol(symbolType_, span_, name_)
{
}

void ContainerSymbol::Write(SymbolWriter& writer)
{
    Symbol::Write(writer);
    std::vector<Symbol*> exportSymbols;
    for (const std::unique_ptr<Symbol>& member : members)
    {
        if (member->IsExportSymbol())
        {
            exportSymbols.push_back(member.get());
        }
    }
    uint32_t n = uint32_t(exportSymbols.size());
    writer.GetBinaryWriter().WriteEncodedUInt(n);
    for (uint32_t i = 0; i < n; ++i)
    {
        writer.Write(exportSymbols[i]);
    }
}

void ContainerSymbol::Read(SymbolReader& reader)
{
    Symbol::Read(reader);
    uint32_t n = reader.GetBinaryReader().ReadEncodedUInt();
    for (uint32_t i = 0; i < n; ++i)
    {
        Symbol* symbol = reader.ReadSymbol();
        AddMember(symbol);
    }
}

void ContainerSymbol::AddMember(Symbol* member)
{
    member->SetParent(this);
    members.push_back(std::unique_ptr<Symbol>(member));
    switch (member->GetSymbolType())
    {
        case SymbolType::functionSymbol:
        case SymbolType::staticConstructorSymbol:
        case SymbolType::constructorSymbol:
        case SymbolType::destructorSymbol:
        case SymbolType::memberFunctionSymbol:
        {
            FunctionSymbol* functionSymbol = static_cast<FunctionSymbol*>(member);
            FunctionGroupSymbol* functionGroupSymbol = MakeFunctionGroupSymbol(functionSymbol->GroupName(), functionSymbol->GetSpan());
            functionGroupSymbol->AddFunction(functionSymbol);
            functionSymbol->GetContainerScope()->SetParent(GetContainerScope());
            break;
        }
        default: 
        {
            containerScope.Install(member);
            break;
        }
    }
}

FunctionGroupSymbol* ContainerSymbol::MakeFunctionGroupSymbol(const std::u32string& groupName, const Span& span)
{
    Symbol* symbol = containerScope.Lookup(groupName);
    if (!symbol)
    {
        FunctionGroupSymbol* functionGroupSymbol = new FunctionGroupSymbol(span, groupName);
        functionGroupSymbol->SetSymbolTable(GetSymbolTable());
        AddMember(functionGroupSymbol);
        return functionGroupSymbol;
    }
    if (symbol->GetSymbolType() == SymbolType::functionGroupSymbol)
    {
        return static_cast<FunctionGroupSymbol*>(symbol);
    }
    else
    {
        throw Exception("name of symbol '" + ToUtf8(symbol->FullName()) + "' conflicts with a function group '" + ToUtf8(groupName) + "'", symbol->GetSpan(), span);
    }
}

DeclarationBlock::DeclarationBlock(const Span& span_, const std::u32string& name_) : ContainerSymbol(SymbolType::declarationBlock, span_, name_)
{
}

void DeclarationBlock::AddMember(Symbol* member) 
{
    ContainerSymbol::AddMember(member);
    if (member->GetSymbolType() == SymbolType::localVariableSymbol)
    {
        FunctionSymbol* fun = Function();
        if (fun)
        {
            fun->AddLocalVariable(static_cast<LocalVariableSymbol*>(member));
        }
    }
}


} } // namespace cmajor::symbols
