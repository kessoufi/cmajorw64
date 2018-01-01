// =================================
// Copyright (c) 2018 Seppo Laakko
// Distributed under the MIT license
// =================================

#ifndef CMAJOR_SYMBOLS_WARNING_INCLUDED
#define CMAJOR_SYMBOLS_WARNING_INCLUDED
#include <cmajor/parsing/Scanner.hpp>
#include <cmajor/util/Json.hpp>

namespace cmajor { namespace symbols {

using cmajor::parsing::Span;
using cmajor::util::JsonValue;

class Warning
{
public:
    Warning(const std::u32string& project_, const std::string& message_);
    const std::u32string& Project() const { return project; }
    const std::string& Message() const { return message; }
    const Span& Defined() const { return defined; }
    void SetDefined(const Span& defined_) { defined = defined_; }
    const std::vector<Span>& References() const { return references; }
    void SetReferences(const std::vector<Span>& references_);
    std::unique_ptr<JsonValue> ToJson() const;
private:
    std::u32string project;
    std::string message;
    Span defined;
    std::vector<Span> references;
};

class CompileWarningCollection
{
public:
    static void Init();
    static void Done();
    static CompileWarningCollection& Instance();
    void SetCurrentProjectName(const std::u32string& currentProjectName_);
    const std::u32string& GetCurrentProjectName() const { return currentProjectName; }
    void AddWarning(const Warning& warning);
    const std::vector<Warning>& Warnings() const { return warnings; }
private:
    static std::unique_ptr<CompileWarningCollection> instance;
    CompileWarningCollection();
    std::u32string currentProjectName;
    std::vector<Warning> warnings;
};

void InitWarning();
void DoneWarning();

} } // namespace cmajor::symbols

#endif // CMAJOR_SYMBOLS_WARNING_INCLUDED
