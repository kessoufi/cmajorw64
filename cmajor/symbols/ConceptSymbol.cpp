// =================================
// Copyright (c) 2018 Seppo Laakko
// Distributed under the MIT license
// =================================

#include <cmajor/symbols/ConceptSymbol.hpp>
#include <cmajor/symbols/Exception.hpp>
#include <cmajor/symbols/SymbolWriter.hpp>
#include <cmajor/symbols/SymbolReader.hpp>
#include <cmajor/symbols/SymbolTable.hpp>
#include <cmajor/symbols/SymbolCollector.hpp>
#include <cmajor/util/Unicode.hpp>

namespace cmajor { namespace symbols {

using namespace cmajor::unicode;

ConceptGroupSymbol::ConceptGroupSymbol(const Span& span_, const std::u32string& name_) : Symbol(SymbolType::conceptGroupSymbol, span_, name_)
{
}

void ConceptGroupSymbol::AddConcept(ConceptSymbol* concept)
{
    Assert(concept->GroupName() == Name(), "wrong concept group");
    int arity = concept->Arity();
    auto it = arityConceptMap.find(arity);
    if (it != arityConceptMap.cend())
    {
        throw Exception("concept group '" + ToUtf8(FullName()) + "' already has concept with arity " + std::to_string(arity), GetSpan());
    }
    arityConceptMap[arity] = concept;
}

ConceptSymbol* ConceptGroupSymbol::GetConcept(int arity) const
{
    auto it = arityConceptMap.find(arity);
    if (it != arityConceptMap.cend())
    {
        return it->second;
    }
    else
    {
        throw Exception("concept with arity " + std::to_string(arity) + " not found from concept group '" + ToUtf8(FullName()) + "'", GetSpan());
    }
}

ConceptSymbol::ConceptSymbol(const Span& span_, const std::u32string& name_) : ContainerSymbol(SymbolType::conceptSymbol, span_, name_), refinedConcept(nullptr), typeId(0)
{
}

void ConceptSymbol::Write(SymbolWriter& writer)
{
    ContainerSymbol::Write(writer);
    Assert(typeId != 0, "type id not initialized");
    writer.GetBinaryWriter().Write(typeId);
    writer.GetBinaryWriter().Write(groupName);
    uint32_t refineConceptId = 0;
    if (refinedConcept)
    {
        refineConceptId = refinedConcept->TypeId();
    }
    writer.GetBinaryWriter().Write(refineConceptId);
    Node* node = GetSymbolTable()->GetNode(this);
    Assert(node->IsConceptNode(), "concept node expected");
    writer.GetAstWriter().Write(node);
}

void ConceptSymbol::Read(SymbolReader& reader)
{
    ContainerSymbol::Read(reader);
    typeId = reader.GetBinaryReader().ReadUInt();
    GetSymbolTable()->AddTypeOrConceptSymbolToTypeIdMap(this);
    groupName = reader.GetBinaryReader().ReadUtf32String();
    uint32_t refinedConcepId = reader.GetBinaryReader().ReadUInt();
    if (refinedConcepId != 0)
    {
        GetSymbolTable()->EmplaceConceptRequest(this, refinedConcepId);
    }
    conceptNode.reset(reader.GetAstReader().ReadConceptNode());
}

void ConceptSymbol::EmplaceConcept(ConceptSymbol* concept)
{
    refinedConcept = concept;
}

void ConceptSymbol::Accept(SymbolCollector* collector)
{
    if (IsProject() && Access() == SymbolAccess::public_)
    {
        collector->AddConcept(this);
    }
}

void ConceptSymbol::Dump(CodeFormatter& formatter)
{
    formatter.WriteLine(ToUtf8(Name()));
    formatter.WriteLine("group name: " + ToUtf8(groupName));
    formatter.WriteLine("full name: " + ToUtf8(FullNameWithSpecifiers()));
    formatter.WriteLine("mangled name: " + ToUtf8(MangledName()));
    formatter.WriteLine("typeid: " + std::to_string(typeId));
    if (refinedConcept)
    {
        formatter.WriteLine("refined concept: " + ToUtf8(refinedConcept->FullName()));
    }
}

void ConceptSymbol::AddMember(Symbol* member)
{
    ContainerSymbol::AddMember(member);
    if (member->GetSymbolType() == SymbolType::templateParameterSymbol)
    {
        templateParameters.push_back(static_cast<TemplateParameterSymbol*>(member));
    }
}

void ConceptSymbol::ComputeName()
{
    std::u32string name = groupName;
    bool first = true;
    name.append(1, '<');
    for (TemplateParameterSymbol* templateParameter : templateParameters)
    {
        if (first)
        {
            first = false;
        }
        else
        {
            name.append(U", ");
        }
        name.append(templateParameter->Name());
    }
    name.append(1, '>');
    SetName(name);
    ComputeMangledName();
}

void ConceptSymbol::SetSpecifiers(Specifiers specifiers)
{
    Specifiers accessSpecifiers = specifiers & Specifiers::access_;
    SetAccess(accessSpecifiers);
    if ((specifiers & Specifiers::static_) != Specifiers::none)
    {
        throw Exception("concept symbol cannot be static", GetSpan());
    }
    if ((specifiers & Specifiers::virtual_) != Specifiers::none)
    {
        throw Exception("concept symbol cannot be virtual", GetSpan());
    }
    if ((specifiers & Specifiers::override_) != Specifiers::none)
    {
        throw Exception("concept symbol cannot be override", GetSpan());
    }
    if ((specifiers & Specifiers::abstract_) != Specifiers::none)
    {
        throw Exception("concept symbol cannot be abstract", GetSpan());
    }
    if ((specifiers & Specifiers::inline_) != Specifiers::none)
    {
        throw Exception("concept symbol cannot be inline", GetSpan());
    }
    if ((specifiers & Specifiers::explicit_) != Specifiers::none)
    {
        throw Exception("concept symbol cannot be explicit", GetSpan());
    }
    if ((specifiers & Specifiers::external_) != Specifiers::none)
    {
        throw Exception("concept symbol cannot be external", GetSpan());
    }
    if ((specifiers & Specifiers::suppress_) != Specifiers::none)
    {
        throw Exception("concept symbol cannot be suppressed", GetSpan());
    }
    if ((specifiers & Specifiers::default_) != Specifiers::none)
    {
        throw Exception("concept symbol cannot be default", GetSpan());
    }
    if ((specifiers & Specifiers::constexpr_) != Specifiers::none)
    {
        throw Exception("concept symbol cannot be constexpr", GetSpan());
    }
    if ((specifiers & Specifiers::cdecl_) != Specifiers::none)
    {
        throw Exception("concept symbol cannot be decl", GetSpan());
    }
    if ((specifiers & Specifiers::nothrow_) != Specifiers::none)
    {
        throw Exception("concept symbol cannot be nothrow", GetSpan());
    }
    if ((specifiers & Specifiers::throw_) != Specifiers::none)
    {
        throw Exception("concept symbol cannot be throw", GetSpan());
    }
    if ((specifiers & Specifiers::new_) != Specifiers::none)
    {
        throw Exception("concept symbol cannot be new", GetSpan());
    }
    if ((specifiers & Specifiers::const_) != Specifiers::none)
    {
        throw Exception("concept symbol cannot be const", GetSpan());
    }
    if ((specifiers & Specifiers::unit_test_) != Specifiers::none)
    {
        throw Exception("concept symbol cannot be unit_test", GetSpan());
    }
}

} } // namespace cmajor::symbols
