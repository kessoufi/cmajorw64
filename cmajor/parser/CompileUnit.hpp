#ifndef CompileUnit_hpp_28778
#define CompileUnit_hpp_28778

#include <cmajor/parsing/Grammar.hpp>
#include <cmajor/parsing/Keyword.hpp>
#include <cmajor/ast/CompileUnit.hpp>
#include <cmajor/parser/ParsingContext.hpp>

namespace cmajor { namespace parser {

using namespace cmajor::ast;
class CompileUnitGrammar : public cmajor::parsing::Grammar
{
public:
    static CompileUnitGrammar* Create();
    static CompileUnitGrammar* Create(cmajor::parsing::ParsingDomain* parsingDomain);
    CompileUnitNode* Parse(const char32_t* start, const char32_t* end, int fileIndex, const std::string& fileName, ParsingContext* ctx);
private:
    CompileUnitGrammar(cmajor::parsing::ParsingDomain* parsingDomain_);
    virtual void CreateRules();
    virtual void GetReferencedGrammars();
    class CompileUnitRule;
    class NamespaceContentRule;
    class UsingDirectivesRule;
    class UsingDirectiveRule;
    class UsingAliasDirectiveRule;
    class UsingNamespaceDirectiveRule;
    class DefinitionsRule;
    class DefinitionRule;
    class NamespaceDefinitionRule;
    class TypedefDeclarationRule;
    class ConceptDefinitionRule;
    class FunctionDefinitionRule;
    class ClassDefinitionRule;
    class InterfaceDefinitionRule;
    class EnumTypeDefinitionRule;
    class ConstantDefinitionRule;
    class DelegateDefinitionRule;
    class ClassDelegateDefinitionRule;
};

} } // namespace cmajor.parser

#endif // CompileUnit_hpp_28778
