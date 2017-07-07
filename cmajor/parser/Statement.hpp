#ifndef Statement_hpp_21397
#define Statement_hpp_21397

#include <cmajor/parsing/Grammar.hpp>
#include <cmajor/parsing/Keyword.hpp>
#include <cmajor/ast/Statement.hpp>
#include <cmajor/parser/ParsingContext.hpp>

namespace cmajor { namespace parser {

using namespace cmajor::ast;
class StatementGrammar : public cmajor::parsing::Grammar
{
public:
    static StatementGrammar* Create();
    static StatementGrammar* Create(cmajor::parsing::ParsingDomain* parsingDomain);
    StatementNode* Parse(const char32_t* start, const char32_t* end, int fileIndex, const std::string& fileName, ParsingContext* ctx);
private:
    StatementGrammar(cmajor::parsing::ParsingDomain* parsingDomain_);
    virtual void CreateRules();
    virtual void GetReferencedGrammars();
    class StatementRule;
    class LabelIdRule;
    class LabelRule;
    class LabeledStatementRule;
    class ControlStatementRule;
    class CompoundStatementRule;
    class ReturnStatementRule;
    class IfStatementRule;
    class WhileStatementRule;
    class DoStatementRule;
    class ForStatementRule;
    class ForInitStatementRule;
    class ForLoopStatementExprRule;
    class RangeForStatementRule;
    class BreakStatementRule;
    class ContinueStatementRule;
    class GotoStatementRule;
    class SwitchStatementRule;
    class CaseStatementRule;
    class DefaultStatementRule;
    class GotoCaseStatementRule;
    class GotoDefaultStatementRule;
    class AssignmentStatementExprRule;
    class AssignmentStatementRule;
    class ConstructionStatementRule;
    class DeleteStatementRule;
    class DestroyStatementRule;
    class IncrementStatementExprRule;
    class IncrementStatementRule;
    class DecrementStatementExprRule;
    class DecrementStatementRule;
    class ExpressionStatementRule;
    class EmptyStatementRule;
    class ThrowStatementRule;
    class TryStatementRule;
    class CatchRule;
};

} } // namespace cmajor.parser

#endif // Statement_hpp_21397
