// =================================
// Copyright (c) 2018 Seppo Laakko
// Distributed under the MIT license
// =================================

#ifndef CMAJOR_SYMBOLS_CONCEPT_SYMBOL_INCLUDED
#define CMAJOR_SYMBOLS_CONCEPT_SYMBOL_INCLUDED
#include <cmajor/symbols/ContainerSymbol.hpp>
#include <cmajor/symbols/TemplateSymbol.hpp>
#include <unordered_map>

namespace cmajor { namespace symbols {

class ConceptSymbol;

class ConceptGroupSymbol : public Symbol
{
public:
    ConceptGroupSymbol(const Span& span_, const std::u32string& name_);
    std::string TypeString() const override { return "concept_group"; }
    bool IsExportSymbol() const override { return false; }
    SymbolAccess DeclaredAccess() const override { return SymbolAccess::public_; }
    void AddConcept(ConceptSymbol* concept);
    ConceptSymbol* GetConcept(int arity);
private:
    std::unordered_map<int, ConceptSymbol*> arityConceptMap;
};

class ConceptSymbol : public ContainerSymbol
{
public:
    ConceptSymbol(const Span& span_, const std::u32string& name_);
    void Write(SymbolWriter& writer) override;
    void Read(SymbolReader& reader) override;
    void EmplaceConcept(ConceptSymbol* concept) override;
    void Accept(SymbolCollector* collector) override;
    void Dump(CodeFormatter& formatter) override;
    void AddMember(Symbol* member) override;
    std::string TypeString() const override { return "concept"; }
    std::u32string SimpleName() const override { return groupName; }
    void ComputeName();
    void SetSpecifiers(Specifiers specifiers);
    //void SetTypeId(uint32_t typeId_) { typeId = typeId_; }
    void SetTypeId(const boost::uuids::uuid& typeId_) { typeId = typeId_; }
    //uint32_t TypeId() const { return typeId; }
    const boost::uuids::uuid& TypeId() const { return typeId; }
    const std::u32string& GroupName() const { return groupName; }
    void SetGroupName(const std::u32string& groupName_) { groupName = groupName_; }
    int Arity() const { return templateParameters.size(); }
    ConceptNode* GetConceptNode() { return conceptNode.get(); }
    ConceptSymbol* RefinedConcept() const { return refinedConcept; }
    void SetRefinedConcept(ConceptSymbol* refinedConcept_) { refinedConcept = refinedConcept_; }
    const std::vector<TemplateParameterSymbol*>& TemplateParameters() const { return templateParameters; }
private:
    //uint32_t typeId;
    boost::uuids::uuid typeId;
    std::u32string groupName;
    std::vector<TemplateParameterSymbol*> templateParameters;
    std::unique_ptr<ConceptNode> conceptNode;
    ConceptSymbol* refinedConcept;
};

} } // namespace cmajor::symbols

#endif // CMAJOR_SYMBOLS_CONCEPT_SYMBOL_INCLUDED
