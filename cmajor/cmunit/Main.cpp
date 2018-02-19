#include <cmajor/ast/InitDone.hpp>
#include <cmajor/ast/Identifier.hpp>
#include <cmajor/ast/BasicType.hpp>
#include <cmajor/ast/Expression.hpp>
#include <cmajor/ast/TypeExpr.hpp>
#include <cmajor/ast/Literal.hpp>
#include <cmajor/parsing/InitDone.hpp>
#include <cmajor/util/InitDone.hpp>
#include <cmajor/symbols/InitDone.hpp>
#include <cmajor/symbols/Exception.hpp>
#include <cmajor/symbols/GlobalFlags.hpp>
#include <cmajor/symbols/Module.hpp>
#include <cmajor/symbols/SymbolCreatorVisitor.hpp>
#include <cmajor/binder/BoundStatement.hpp>
#include <cmajor/binder/ModuleBinder.hpp>
#include <cmajor/binder/AttributeBinder.hpp>
#include <cmajor/binder/TypeBinder.hpp>
#include <cmajor/binder/StatementBinder.hpp>
#include <cmajor/binder/ControlFlowAnalyzer.hpp>
#include <cmajor/build/Build.hpp>
#include <cmajor/emitter/Emitter.hpp>
#include <cmajor/build/Build.hpp>
#include <cmajor/parser/FileRegistry.hpp>
#include <cmajor/parser/Project.hpp>
#include <cmajor/parser/Solution.hpp>
#include <cmajor/parser/CompileUnit.hpp>
#include <cmajor/parsing/Exception.hpp>
#include <cmajor/dom/Document.hpp>
#include <cmajor/dom/Element.hpp>
#include <cmajor/dom/CharacterData.hpp>
#include <cmajor/dom/Parser.hpp>
#include <cmajor/xpath/InitDone.hpp>
#include <cmajor/xpath/XPathEvaluate.hpp>
#include <cmajor/util/Util.hpp>
#include <cmajor/util/Path.hpp>
#include <cmajor/util/Json.hpp>
#include <cmajor/util/Unicode.hpp>
#include <cmajor/util/Time.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <chrono>

struct InitDone
{
    InitDone()
    {
        cmajor::ast::Init();
        cmajor::parser::FileRegistry::Init();
        cmajor::symbols::Init();
        cmajor::parsing::Init();
        cmajor::util::Init();
        cmajor::xpath::Init();
    }
    ~InitDone()
    {
        cmajor::xpath::Done();
        cmajor::util::Done();
        cmajor::parsing::Done();
        cmajor::symbols::Done();
        cmajor::ast::Done();
    }
};

using namespace cmajor::ast;
using namespace cmajor::util;
using namespace cmajor::unicode;
using namespace cmajor::symbols;
using namespace cmajor::parser;
using namespace cmajor::parsing;
using namespace cmajor::build;
using namespace cmajor::binder;
using namespace cmajor::emitter;
using namespace cmajor::dom;
using namespace cmajor::xpath;

const char* version = "2.0.0";

void PrintHelp()
{
    std::cout << "Cmajor Unit Test Engine version " << version << std::endl;
    std::cout << "Usage: cmunit [options] { project.cmp | solution.cms }" << std::endl;
    std::cout << "Options:\n" <<
        "--help (-h)\n" <<
        "   print this help message\n" <<
        "--config=CONFIG | -c=CONFIG\n" <<
        "   run unit tests using configuration CONFIG (default is \"debug\")\n" <<
        "--file=FILE | -f=FILE\n" <<
        "   run only unit tests in file FILE\n" <<
        "--test=TEST | -t=TEST\n" <<
        "   run only unit test TEST\n" <<
        std::endl;
}

CompileUnitGrammar* compileUnitGrammar = nullptr;

void CreateSymbols(SymbolTable& symbolTable, CompileUnitNode* testUnit)
{
    SymbolCreatorVisitor symbolCreator(symbolTable);
    symbolTable.SetCurrentCompileUnit(testUnit);
    testUnit->Accept(symbolCreator);
}

std::unique_ptr<BoundCompileUnit> BindTypes(Module& module, CompileUnitNode* testUnit, AttributeBinder* attributeBinder)
{
    std::unique_ptr<BoundCompileUnit> boundCompileUnit(new BoundCompileUnit(module, testUnit, attributeBinder));
    boundCompileUnit->PushBindingTypes();
    TypeBinder typeBinder(*boundCompileUnit);
    testUnit->Accept(typeBinder);
    boundCompileUnit->PopBindingTypes();
    return boundCompileUnit;
}

void BindStatements(BoundCompileUnit& boundCompileUnit)
{
    StatementBinder statementBinder(boundCompileUnit);
    boundCompileUnit.GetCompileUnitNode()->Accept(statementBinder);
}

void CreateMainUnit(std::vector<std::string>& objectFilePaths, Module& module, EmittingContext& emittingContext, AttributeBinder* attributeBinder, const std::string& testName, 
    int32_t numAssertions, const std::string& unitTestFilePath)
{
    CompileUnitNode mainCompileUnit(Span(), boost::filesystem::path(module.OriginalFilePath()).parent_path().append("__main__.cm").generic_string());
    mainCompileUnit.GlobalNs()->AddMember(new NamespaceImportNode(Span(), new IdentifierNode(Span(), U"System")));
    FunctionNode* mainFunction(new FunctionNode(Span(), Specifiers::public_, new IntNode(Span()), U"main", nullptr));
    mainFunction->SetProgramMain();
    CompoundStatementNode* mainFunctionBody = new CompoundStatementNode(Span());
    ConstructionStatementNode* constructExitCode = new ConstructionStatementNode(Span(), new IntNode(Span()), new IdentifierNode(Span(), U"exitCode"));
    mainFunctionBody->AddStatement(constructExitCode);
    InvokeNode* invokeStartUnitTest = new InvokeNode(Span(), new IdentifierNode(Span(), U"RtStartUnitTest"));
    invokeStartUnitTest->AddArgument(new IntLiteralNode(Span(), numAssertions));
    invokeStartUnitTest->AddArgument(new StringLiteralNode(Span(), unitTestFilePath));
    ExpressionStatementNode* rtStartUnitTestCall = new ExpressionStatementNode(Span(), invokeStartUnitTest);
    mainFunctionBody->AddStatement(rtStartUnitTestCall);
    ConstructionStatementNode* argc = new ConstructionStatementNode(Span(), new IntNode(Span()), new IdentifierNode(Span(), U"argc"));
    argc->AddArgument(new InvokeNode(Span(), new IdentifierNode(Span(), U"RtArgc")));
    mainFunctionBody->AddStatement(argc);
    ConstructionStatementNode* argv = new ConstructionStatementNode(Span(), new ConstNode(Span(), new PointerNode(Span(), new PointerNode(Span(), new CharNode(Span())))), new IdentifierNode(Span(), U"argv"));
    argv->AddArgument(new InvokeNode(Span(), new IdentifierNode(Span(), U"RtArgv")));
    mainFunctionBody->AddStatement(argv);
    CompoundStatementNode* tryBlock = new CompoundStatementNode(Span());
    InvokeNode* invokeTest = new InvokeNode(Span(), new IdentifierNode(Span(), ToUtf32(testName)));
    StatementNode* callMainStatement = new ExpressionStatementNode(Span(), invokeTest);
    tryBlock->AddStatement(callMainStatement);
    TryStatementNode* tryStatement = new TryStatementNode(Span(), tryBlock);
    CompoundStatementNode* catchBlock = new CompoundStatementNode(Span());
    InvokeNode* consoleError = new InvokeNode(Span(), new DotNode(Span(), new IdentifierNode(Span(), U"System.Console"), new IdentifierNode(Span(), U"Error")));
    DotNode* writeLine = new DotNode(Span(), consoleError, new IdentifierNode(Span(), U"WriteLine"));
    InvokeNode* printEx = new InvokeNode(Span(), writeLine);
    InvokeNode* exToString = new InvokeNode(Span(), new DotNode(Span(), new IdentifierNode(Span(), U"ex"), new IdentifierNode(Span(), U"ToString")));
    printEx->AddArgument(exToString);
    ExpressionStatementNode* printExStatement = new ExpressionStatementNode(Span(), printEx);
    catchBlock->AddStatement(printExStatement);
    InvokeNode* setEx = new InvokeNode(Span(), new IdentifierNode(Span(), U"RtSetUnitTestException"));
    ConstructionStatementNode* constructExStr = new ConstructionStatementNode(Span(), new IdentifierNode(Span(), U"string"), new IdentifierNode(Span(), U"@ex"));
    CloneContext cloneContext;
    constructExStr->AddArgument(exToString->Clone(cloneContext));
    catchBlock->AddStatement(constructExStr);
    InvokeNode* exStr = new InvokeNode(Span(), new DotNode(Span(), new IdentifierNode(Span(), U"@ex"), new IdentifierNode(Span(), U"Chars")));
    setEx->AddArgument(exStr);
    ExpressionStatementNode* setExStatement = new ExpressionStatementNode(Span(), setEx);
    catchBlock->AddStatement(setExStatement);
    AssignmentStatementNode* assignExitCodeStatement = new AssignmentStatementNode(Span(), new IdentifierNode(Span(), U"exitCode"), new IntLiteralNode(Span(), 1));
    catchBlock->AddStatement(assignExitCodeStatement);
    CatchNode* catchAll = new CatchNode(Span(), new ConstNode(Span(), new LValueRefNode(Span(), new IdentifierNode(Span(), U"System.Exception"))), new IdentifierNode(Span(), U"ex"), catchBlock);
    tryStatement->AddCatch(catchAll);
    mainFunctionBody->AddStatement(tryStatement);
    InvokeNode* invokeEndUnitTest = new InvokeNode(Span(), new IdentifierNode(Span(), U"RtEndUnitTest"));
    invokeEndUnitTest->AddArgument(new StringLiteralNode(Span(), testName));
    invokeEndUnitTest->AddArgument(new IdentifierNode(Span(), U"exitCode"));
    ExpressionStatementNode* endUnitTestCall = new ExpressionStatementNode(Span(), invokeEndUnitTest);
    mainFunctionBody->AddStatement(endUnitTestCall);
    ReturnStatementNode* returnStatement = new ReturnStatementNode(Span(), new IdentifierNode(Span(), U"exitCode"));
    mainFunctionBody->AddStatement(returnStatement);
    mainFunction->SetBody(mainFunctionBody);
    mainCompileUnit.GlobalNs()->AddMember(mainFunction);
    SymbolCreatorVisitor symbolCreator(module.GetSymbolTable());
    mainCompileUnit.Accept(symbolCreator);
    BoundCompileUnit boundMainCompileUnit(module, &mainCompileUnit, attributeBinder);
    boundMainCompileUnit.PushBindingTypes();
    TypeBinder typeBinder(boundMainCompileUnit);
    mainCompileUnit.Accept(typeBinder);
    boundMainCompileUnit.PopBindingTypes();
    StatementBinder statementBinder(boundMainCompileUnit);
    mainCompileUnit.Accept(statementBinder);
    if (boundMainCompileUnit.HasGotos())
    {
        AnalyzeControlFlow(boundMainCompileUnit);
    }
    GenerateCode(emittingContext, boundMainCompileUnit);
    objectFilePaths.push_back(boundMainCompileUnit.ObjectFilePath());
}

struct UnitTest
{
    UnitTest() : prevUnitTest(BeginUnitTest()) {}
    ~UnitTest() { EndUnitTest(prevUnitTest); }
    bool prevUnitTest;
};

void TestUnit(Project* project, CompileUnitNode* testUnit, const std::string& testName, Element* sourceFileElement)
{
    bool compileError = false;
    std::string compileErrorMessage;
    std::vector<int32_t> assertionLineNumbers;
    ResetUnitTestAssertionNumber();
    SetAssertionLineNumberVector(&assertionLineNumbers);
    std::string unitTestFilePath = Path::Combine(Path::GetDirectoryName(project->ExecutableFilePath()), "test.xml");
    try
    {
        std::string config = GetConfig();
        ReadTypeIdCounter(config);
        ReadFunctionIdCounter(config);
        ReadSystemFileIndex(config);
        Module module(project->Name(), project->ModuleFilePath());
        std::vector<ClassTypeSymbol*> classTypes;
        std::vector<ClassTemplateSpecializationSymbol*> classTemplateSpecializations;
        module.PrepareForCompilation(project->References(), project->SourceFilePaths(), classTypes, classTemplateSpecializations);
        cmajor::symbols::MetaInit(module.GetSymbolTable());
        AttributeBinder attributeBinder;
        ModuleBinder moduleBinder(module, testUnit, &attributeBinder);
        CreateSymbols(module.GetSymbolTable(), testUnit);
        for (ClassTemplateSpecializationSymbol* classTemplateSpecialization : classTemplateSpecializations)
        {
            moduleBinder.BindClassTemplateSpecialization(classTemplateSpecialization);
        }
        for (ClassTypeSymbol* classType : classTypes)
        {
            classType->SetSpecialMemberFunctions();
            classType->CreateLayouts();
        }
        std::vector<std::string> objectFilePaths;
        EmittingContext emittingContext;
        {
            UnitTest unitTest;
            std::unique_ptr<BoundCompileUnit> boundCompileUnit = BindTypes(module, testUnit, &attributeBinder);
            BindStatements(*boundCompileUnit);
            if (boundCompileUnit->HasGotos())
            {
                AnalyzeControlFlow(*boundCompileUnit);
            }
            GenerateCode(emittingContext, *boundCompileUnit);
            objectFilePaths.push_back(boundCompileUnit->ObjectFilePath());
        }
        int32_t numUnitTestAssertions = GetNumUnitTestAssertions();
        CreateMainUnit(objectFilePaths, module, emittingContext, &attributeBinder, testName, numUnitTestAssertions, unitTestFilePath);
        GenerateLibrary(objectFilePaths, project->LibraryFilePath());
        Link(project->ExecutableFilePath(), project->LibraryFilePath(), module.LibraryFilePaths(), module);
        CreateClassFile(project->ExecutableFilePath(), module.GetSymbolTable());
        WriteTypeIdCounter(config);
        WriteFunctionIdCounter(config);
        boost::filesystem::remove(unitTestFilePath);
        int exitCode = system(project->ExecutableFilePath().c_str());
        if (FileExists(unitTestFilePath))
        {
            std::unique_ptr<Document> testResultDoc = ReadDocument(unitTestFilePath);
            sourceFileElement->AppendChild(testResultDoc->DocumentElement()->CloneNode(true));
        }
        else
        {
            std::unique_ptr<cmajor::dom::Element> testElement(new cmajor::dom::Element(U"test"));
            testElement->SetAttribute(U"name", ToUtf32(testName));
            testElement->SetAttribute(U"exitCode", ToUtf32(std::to_string(exitCode)));
            for (int32_t i = 0; i < numUnitTestAssertions; ++i)
            {
                std::unique_ptr<cmajor::dom::Element> assertionElement(new cmajor::dom::Element(U"assertion"));
                assertionElement->SetAttribute(U"index", ToUtf32(std::to_string(i)));
                std::u32string assertionResultStr = U"empty";
                assertionElement->SetAttribute(U"result", assertionResultStr);
                if (i < assertionLineNumbers.size())
                {
                    assertionElement->SetAttribute(U"line", ToUtf32(std::to_string(assertionLineNumbers[i])));
                }
                testElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(assertionElement.release()));
            }
            sourceFileElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(testElement.release()));
        }
    }
    catch (const ParsingException& ex)
    {
        compileError = true;
        compileErrorMessage = ex.what();
    }
    catch (const Exception& ex)
    {
        compileError = true;
        compileErrorMessage = ex.What();
    }
    catch (const std::exception& ex)
    {
        compileError = true;
        compileErrorMessage = ex.what();
    }
    if (compileError)
    {
        std::unique_ptr<cmajor::dom::Element> testElement(new cmajor::dom::Element(U"test"));
        testElement->SetAttribute(U"name", ToUtf32(testName));
        testElement->SetAttribute(U"compileError", ToUtf32(compileErrorMessage));
        sourceFileElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(testElement.release()));
    }
}

bool TestNameEquals(const std::string& testName, const std::string& testUnitName)
{
    std::vector<std::string> testNameComponents = Split(testName, '.');
    std::vector<std::string> testUnitNameComponents = Split(testUnitName, '.');
    int n = std::min(testNameComponents.size(), testUnitNameComponents.size());
    for (int i = 0; i < n; ++i)
    {
        int k = testNameComponents.size() - i - 1;
        int l = testUnitNameComponents.size() - i - 1;
        if (testNameComponents[k] != testUnitNameComponents[l]) return false;
    }
    return true;
}

std::vector<std::pair<std::unique_ptr<CompileUnitNode>, std::string>> SplitIntoTestUnits(CompileUnitNode* compileUnit)
{
    std::vector<std::pair<std::unique_ptr<CompileUnitNode>, std::string>> testUnits;
    CloneContext makeUnitTestUnitContext;
    makeUnitTestUnitContext.SetMakeTestUnits();
    std::unique_ptr<CompileUnitNode> environmentNode(static_cast<CompileUnitNode*>(compileUnit->Clone(makeUnitTestUnitContext)));
    for (std::unique_ptr<FunctionNode>& unitTestFunction : makeUnitTestUnitContext.UnitTestFunctions())
    {
        std::string unitTestName = ToUtf8(unitTestFunction->GroupId());
        CloneContext testUnitContext;
        std::pair<std::unique_ptr<CompileUnitNode>, std::string> testUnit = std::make_pair(
            std::unique_ptr<CompileUnitNode>(static_cast<CompileUnitNode*>(environmentNode->Clone(testUnitContext))), unitTestName);
        NamespaceNode* ns = testUnit.first->GlobalNs();
        FunctionNode* unitTestFun = unitTestFunction.release();
        ns->AddMember(unitTestFun);
        testUnits.push_back(std::move(testUnit));
    }
    return testUnits;
}

void TestSourceFile(Project* project, const std::string& sourceFilePath, const std::string& onlyTest, cmajor::dom::Element* projectElement)
{
    std::unique_ptr<cmajor::dom::Element> sourceFileElement(new cmajor::dom::Element(U"sourceFile"));
    sourceFileElement->SetAttribute(U"name", ToUtf32(Path::GetFileNameWithoutExtension(sourceFilePath)));
    if (!compileUnitGrammar)
    {
        compileUnitGrammar = CompileUnitGrammar::Create();
    }
    if (GetGlobalFlag(GlobalFlags::verbose))
    {
        std::cout << "> " << sourceFilePath << std::endl;
    }
    MappedInputFile sourceFile(sourceFilePath);
    int fileIndex = FileRegistry::Instance().RegisterFile(sourceFilePath);
    ParsingContext parsingContext;
    std::u32string s(ToUtf32(std::string(sourceFile.Begin(), sourceFile.End())));
    std::unique_ptr<CompileUnitNode> compileUnit(compileUnitGrammar->Parse(&s[0], &s[0] + s.length(), fileIndex, sourceFilePath, &parsingContext));
    std::vector<std::pair<std::unique_ptr<CompileUnitNode>, std::string>> testUnits = SplitIntoTestUnits(compileUnit.get());
    for (const auto& p : testUnits)
    {
        if (!onlyTest.empty())
        {
            if (!TestNameEquals(onlyTest, p.second))
            {
                continue;
            }
        }
        TestUnit(project, p.first.get(), p.second, sourceFileElement.get());
    }
    projectElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(sourceFileElement.release()));
}

bool SourceFileNameEquals(const std::string& fileName, const std::string& sourceFilePath)
{
    boost::filesystem::path filePath = fileName;
    std::vector<std::string> filePathComponents = Split(filePath.generic_string(), '/');
    std::vector<std::string> sourceFilePathComponents = Split(sourceFilePath, '/');
    int n = std::min(filePathComponents.size(), sourceFilePathComponents.size());
    for (int i = 0; i < n; ++i)
    {
        int k = filePathComponents.size() - i - 1;
        int l = sourceFilePathComponents.size() - i - 1;
        if (filePathComponents[k] != sourceFilePathComponents[l]) return false;
    }
    return true;
}

void TestProject(Project* project, const std::string& onlySourceFile, const std::string& onlyTest, cmajor::dom::Element* parentElement)
{
    std::unique_ptr<cmajor::dom::Element> projectElement(new cmajor::dom::Element(U"project"));
    projectElement->SetAttribute(U"name", project->Name());
    std::string config = GetConfig();
    if (GetGlobalFlag(GlobalFlags::verbose))
    {
        std::cout << "Testing project '" << ToUtf8(project->Name()) << "' (" << project->FilePath() << ") using " << config << " configuration." << std::endl;
    }
    SetCurrentProjectName(project->Name());
    SetCurrentTooName(U"cmc");
    for (const std::string& sourceFilePath : project->SourceFilePaths())
    {
        if (!onlySourceFile.empty())
        {
            if (!SourceFileNameEquals(onlySourceFile, sourceFilePath))
            {
                continue;
            }
        }
        TestSourceFile(project, sourceFilePath, onlyTest, projectElement.get());
    }
    if (GetGlobalFlag(GlobalFlags::verbose))
    {
        std::cout << "Project '" << ToUtf8(project->Name()) << "' (" << project->FilePath() << ") tested using " << config << " configuration." << std::endl;
    }
    parentElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(projectElement.release()));
}

SolutionGrammar* solutionGrammar = nullptr;
ProjectGrammar* projectGrammar = nullptr;

void TestProject(const std::string& projectFileName, const std::string& onlySourceFile, const std::string& onlyTest, cmajor::dom::Element* parentElement)
{
    std::string config = GetConfig();
    if (!projectGrammar)
    {
        projectGrammar = ProjectGrammar::Create();
    }
    MappedInputFile projectFile(projectFileName);
    std::u32string p(ToUtf32(std::string(projectFile.Begin(), projectFile.End())));
    std::unique_ptr<Project> project(projectGrammar->Parse(&p[0], &p[0] + p.length(), 0, projectFileName, config));
    project->ResolveDeclarations();
    TestProject(project.get(), onlySourceFile, onlyTest, parentElement);
}

void TestSolution(const std::string& solutionFileName, const std::string& onlySourceFile, const std::string& onlyTest, cmajor::dom::Element* cmunitElement)
{
    std::unique_ptr<cmajor::dom::Element> solutionElement(new cmajor::dom::Element(U"solution"));
    if (!solutionGrammar)
    {
        solutionGrammar = SolutionGrammar::Create();
    }
    if (!projectGrammar)
    {
        projectGrammar = ProjectGrammar::Create();
    }
    MappedInputFile solutionFile(solutionFileName);
    std::u32string s(ToUtf32(std::string(solutionFile.Begin(), solutionFile.End())));
    std::unique_ptr<Solution> solution(solutionGrammar->Parse(&s[0], &s[0] + s.length(), 0, solutionFileName));
    solutionElement->SetAttribute(U"name", solution->Name());
    solution->ResolveDeclarations();
    std::string config = GetConfig();
    if (GetGlobalFlag(GlobalFlags::verbose))
    {
        std::cout << "Testing solution '" << ToUtf8(solution->Name()) << "' (" << solution->FilePath() << ") using " << config << " configuration." << std::endl;
    }
    for (const std::string& projectFilePath : solution->ProjectFilePaths())
    {
        TestProject(projectFilePath, onlySourceFile, onlyTest, solutionElement.get());
    }
    if (GetGlobalFlag(GlobalFlags::verbose))
    {
        std::cout << "Solution '" << ToUtf8(solution->Name()) << "' (" << solution->FilePath() << ") tested using " << config << " configuration." << std::endl;
    }
    cmunitElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(solutionElement.release()));
}

std::unique_ptr<cmajor::dom::Document> GenerateHtmlReport(cmajor::dom::Document* testDoc)
{
    std::unique_ptr<cmajor::dom::Document> reportDoc(new cmajor::dom::Document());
    std::unique_ptr<cmajor::dom::Element> htmlElement(new cmajor::dom::Element(U"html"));
    std::unique_ptr<cmajor::dom::Element> headElement(new cmajor::dom::Element(U"head"));
    std::unique_ptr<cmajor::dom::Element> titleElement(new cmajor::dom::Element(U"title"));
    titleElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Text(U"Unit Test Report")));
    std::unique_ptr<cmajor::dom::Element> styleElement(new cmajor::dom::Element(U"style"));
    std::u32string style = 
        U"body { max-width: 800px; } h1, h2, h3, h4, h5, h6 { color: #005ab4; font-family: sans-serif; } table { boder-collapse: collapse; } table, th, td { text-align: left; border: 1px solid #dddddd; padding: 8px; }";
    styleElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Text(style)));
    std::unique_ptr<cmajor::dom::Element> bodyElement(new cmajor::dom::Element(U"body"));
    std::unique_ptr<cmajor::dom::Element> h1Element(new cmajor::dom::Element(U"h1"));
    h1Element->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Text(U"Unit Test Report")));
    bodyElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(h1Element.release()));
    std::unique_ptr<cmajor::dom::Element> p1Element(new cmajor::dom::Element(U"p"));
    cmajor::dom::Element* cmunitElement = testDoc->DocumentElement();
    std::u32string start = cmunitElement->GetAttribute(U"start");
    p1Element->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Text(U"start: " + start)));
    p1Element->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Element(U"br")));
    std::u32string end = cmunitElement->GetAttribute(U"end");
    p1Element->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Text(U"end: " + end)));
    p1Element->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Element(U"br")));
    std::u32string duration = cmunitElement->GetAttribute(U"duration");
    p1Element->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Text(U"duration: " + duration)));
    p1Element->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Element(U"br")));
    bodyElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(p1Element.release()));

    std::unique_ptr<cmajor::dom::Element> h2Element(new cmajor::dom::Element(U"h2"));
    h2Element->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Text(U"Results")));
    bodyElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(h2Element.release()));

    std::unique_ptr<cmajor::dom::Element> tableElement(new cmajor::dom::Element(U"table"));

    std::unique_ptr<cmajor::dom::Element> trHeaderElement(new cmajor::dom::Element(U"tr"));
    std::unique_ptr<cmajor::dom::Element> thNameElement(new cmajor::dom::Element(U"th"));
    thNameElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Text(U"name")));
    trHeaderElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(thNameElement.release()));

    std::unique_ptr<cmajor::dom::Element> thCountElement(new cmajor::dom::Element(U"th"));
    thCountElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Text(U"count")));
    trHeaderElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(thCountElement.release()));

    std::unique_ptr<cmajor::dom::Element> thPassedElement(new cmajor::dom::Element(U"th"));
    thPassedElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Text(U"passed")));
    trHeaderElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(thPassedElement.release()));

    std::unique_ptr<cmajor::dom::Element> thFailedElement(new cmajor::dom::Element(U"th"));
    thFailedElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Text(U"failed")));
    trHeaderElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(thFailedElement.release()));

    std::unique_ptr<cmajor::dom::Element> thEmptyElement(new cmajor::dom::Element(U"th"));
    thEmptyElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Text(U"empty")));
    trHeaderElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(thEmptyElement.release()));

    std::unique_ptr<cmajor::dom::Element> thExitCodeElement(new cmajor::dom::Element(U"th"));
    thExitCodeElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Text(U"exit code")));
    trHeaderElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(thExitCodeElement.release()));

    std::unique_ptr<cmajor::dom::Element> thCompileErrorElement(new cmajor::dom::Element(U"th"));
    thCompileErrorElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Text(U"compile error")));
    trHeaderElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(thCompileErrorElement.release()));

    std::unique_ptr<cmajor::dom::Element> thExceptionElement(new cmajor::dom::Element(U"th"));
    thExceptionElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Text(U"exception")));
    trHeaderElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(thExceptionElement.release()));

    std::vector<std::pair<std::u32string, cmajor::dom::Element*>> failedAssertions;

    tableElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(trHeaderElement.release()));

    std::unique_ptr<XPathObject> tests = Evaluate(U"//test", testDoc);
    if (tests && tests->Type() == XPathObjectType::nodeSet)
    {
        XPathNodeSet* testElements = static_cast<XPathNodeSet*>(tests.get());
        int n = testElements->Length();
        for (int i = 0; i < n; ++i)
        {
            cmajor::dom::Node* testNode = (*testElements)[i];
            if (testNode->GetNodeType() == cmajor::dom::NodeType::elementNode)
            {
                cmajor::dom::Element* testElement = static_cast<cmajor::dom::Element*>(testNode);
                std::u32string name = testElement->GetAttribute(U"name");
                std::u32string fullTestName = name;
                cmajor::dom::ParentNode* sourceFileNode = testElement->Parent();
                if (sourceFileNode)
                {
                    if (sourceFileNode->GetNodeType() == cmajor::dom::NodeType::elementNode)
                    {
                        cmajor::dom::Element* sourceFileElement = static_cast<cmajor::dom::Element*>(sourceFileNode);
                        name = sourceFileElement->GetAttribute(U"name");
                        fullTestName = name + U"." + fullTestName;
                        cmajor::dom::ParentNode* projectNode = sourceFileElement->Parent();
                        if (projectNode)
                        {
                            if (projectNode->GetNodeType() == cmajor::dom::NodeType::elementNode)
                            {
                                cmajor::dom::Element* projectElement = static_cast<cmajor::dom::Element*>(projectNode);
                                name = projectElement->GetAttribute(U"name");
                                fullTestName = name + U"." + fullTestName;
                                cmajor::dom::ParentNode* parentNode = projectElement->Parent();
                                if (parentNode)
                                {
                                    if (parentNode->GetNodeType() == cmajor::dom::NodeType::elementNode)
                                    {
                                        cmajor::dom::Element* parentElement = static_cast<cmajor::dom::Element*>(parentNode);
                                        if (parentElement->Name() == U"solution")
                                        {
                                            cmajor::dom::Element* solutionElement = parentElement;
                                            name = solutionElement->GetAttribute(U"name");
                                            fullTestName = name + U"." + fullTestName;
                                        }

                                        std::unique_ptr<cmajor::dom::Element> trElement(new cmajor::dom::Element(U"tr"));
                                        std::unique_ptr<cmajor::dom::Element> tdNameElement(new cmajor::dom::Element(U"td"));
                                        cmajor::dom::Element* nameElement = tdNameElement.get();
                                        tdNameElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Text(fullTestName)));
                                        trElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(tdNameElement.release()));
                                        std::unique_ptr<cmajor::dom::Element> tdCountElement(new cmajor::dom::Element(U"td"));
                                        tdCountElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Text(testElement->GetAttribute(U"count"))));
                                        trElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(tdCountElement.release()));
                                        std::unique_ptr<XPathObject> assertions = Evaluate(U"assertion", testElement);
                                        int passed = 0;
                                        int failed = 0;
                                        int empty = 0;
                                        int count = 0;
                                        if (assertions && assertions->Type() == cmajor::xpath::XPathObjectType::nodeSet)
                                        {
                                            XPathNodeSet* assertionElements = static_cast<XPathNodeSet*>(assertions.get());
                                            count = assertionElements->Length();
                                            for (int i = 0; i < count; ++i)
                                            {
                                                cmajor::dom::Node* assertionNode = (*assertionElements)[i];
                                                if (assertionNode->GetNodeType() == cmajor::dom::NodeType::elementNode)
                                                {
                                                    cmajor::dom::Element* assertionElement = static_cast<cmajor::dom::Element*>(assertionNode);
                                                    std::u32string result = assertionElement->GetAttribute(U"result");
                                                    if (result == U"passed")
                                                    {
                                                        ++passed;
                                                    }
                                                    else if (result == U"failed")
                                                    {
                                                        ++failed;
                                                        failedAssertions.push_back(std::make_pair(fullTestName, assertionElement));
                                                    }
                                                    else
                                                    {
                                                        ++empty;
                                                    }
                                                }
                                            }
                                        }
                                        std::u32string fontAttr = U"font-family: sans-serif; font-size: 13px; font-weight: bold;";
                                        if (count > 0)
                                        {
                                            if (passed == count)
                                            {
                                                nameElement->SetAttribute(U"style", U"background-color: #00e600; " + fontAttr);
                                            }
                                            else if (failed > 0)
                                            {
                                                nameElement->SetAttribute(U"style", U"background-color: #ff3300; " + fontAttr);
                                            }
                                            else
                                            {
                                                nameElement->SetAttribute(U"style", U"background-color: #ffff00; " + fontAttr);
                                            }
                                        }
                                        else
                                        {
                                            nameElement->SetAttribute(U"style", U"background-color: #ffff00; " + fontAttr);
                                        }
                                        std::unique_ptr<cmajor::dom::Element> tdPassedElement(new cmajor::dom::Element(U"td"));
                                        tdPassedElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Text(ToUtf32(std::to_string(passed)))));
                                        trElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(tdPassedElement.release()));

                                        std::unique_ptr<cmajor::dom::Element> tdFailedElement(new cmajor::dom::Element(U"td"));
                                        tdFailedElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Text(ToUtf32(std::to_string(failed)))));
                                        trElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(tdFailedElement.release()));

                                        if (count == 0)
                                        {
                                            std::unique_ptr<cmajor::dom::Element> tdEmptyElement(new cmajor::dom::Element(U"td"));
                                            tdEmptyElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Text(testElement->GetAttribute(U"count"))));
                                            trElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(tdEmptyElement.release()));
                                        }
                                        else
                                        {
                                            std::unique_ptr<cmajor::dom::Element> tdEmptyElement(new cmajor::dom::Element(U"td"));
                                            tdEmptyElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Text(ToUtf32(std::to_string(empty)))));
                                            trElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(tdEmptyElement.release()));
                                        }

                                        std::unique_ptr<cmajor::dom::Element> tdExitCodeElement(new cmajor::dom::Element(U"td"));
                                        tdExitCodeElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Text(testElement->GetAttribute(U"exitCode"))));
                                        trElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(tdExitCodeElement.release()));
                                        std::unique_ptr<cmajor::dom::Element> tdCompileErrorElement(new cmajor::dom::Element(U"td")); 
                                        tdCompileErrorElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Text(testElement->GetAttribute(U"compileError"))));
                                        trElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(tdCompileErrorElement.release()));
                                        std::unique_ptr<cmajor::dom::Element> tdExceptionElement(new cmajor::dom::Element(U"td")); 
                                        tdExceptionElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Text(testElement->GetAttribute(U"exception"))));
                                        trElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(tdExceptionElement.release()));
                                        tableElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(trElement.release()));
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    bodyElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(tableElement.release()));

    if (!failedAssertions.empty())
    {
        std::unique_ptr<cmajor::dom::Element> h2Element(new cmajor::dom::Element(U"h2"));
        h2Element->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Text(U"Failed Assertions")));
        bodyElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(h2Element.release()));

        std::unique_ptr<cmajor::dom::Element> failedAssertionsTable(new cmajor::dom::Element(U"table"));

        std::unique_ptr<cmajor::dom::Element> trHeaderElement(new cmajor::dom::Element(U"tr"));

        std::unique_ptr<cmajor::dom::Element> thNameElement(new cmajor::dom::Element(U"th"));
        thNameElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Text(U"name")));
        trHeaderElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(thNameElement.release()));

        std::unique_ptr<cmajor::dom::Element> thIndexElement(new cmajor::dom::Element(U"th"));
        thIndexElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Text(U"index")));
        trHeaderElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(thIndexElement.release()));

        std::unique_ptr<cmajor::dom::Element> thLineElement(new cmajor::dom::Element(U"th"));
        thLineElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Text(U"line")));
        trHeaderElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(thLineElement.release()));

        failedAssertionsTable->AppendChild(std::unique_ptr<cmajor::dom::Node>(trHeaderElement.release()));

        int n = failedAssertions.size();
        for (int i = 0; i < n; ++i)
        {
            const std::pair<std::u32string, cmajor::dom::Element*>& p = failedAssertions[i];
            std::unique_ptr<cmajor::dom::Element> trAssertionElement(new cmajor::dom::Element(U"tr"));
            cmajor::dom::Element* assertionElement = p.second;

            std::unique_ptr<cmajor::dom::Element> tdNameElement(new cmajor::dom::Element(U"td"));
            std::u32string attr = U"background-color: #ff3300; font-family: sans-serif; font-size: 13px; font-weight: bold;";
            tdNameElement->SetAttribute(U"style", attr);
            tdNameElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Text(p.first)));
            trAssertionElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(tdNameElement.release()));

            std::unique_ptr<cmajor::dom::Element> tdIndexElement(new cmajor::dom::Element(U"td"));
            tdIndexElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Text(assertionElement->GetAttribute(U"index"))));
            trAssertionElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(tdIndexElement.release()));

            std::unique_ptr<cmajor::dom::Element> tdLineElement(new cmajor::dom::Element(U"td"));
            tdLineElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(new cmajor::dom::Text(assertionElement->GetAttribute(U"line"))));
            trAssertionElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(tdLineElement.release()));

            failedAssertionsTable->AppendChild(std::unique_ptr<cmajor::dom::Node>(trAssertionElement.release()));
        }

        bodyElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(failedAssertionsTable.release()));
    }

    headElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(titleElement.release()));
    headElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(styleElement.release()));
    htmlElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(headElement.release()));
    htmlElement->AppendChild(std::unique_ptr<cmajor::dom::Node>(bodyElement.release()));
    reportDoc->AppendChild(std::unique_ptr<cmajor::dom::Node>(htmlElement.release()));
    return reportDoc;
}

int main(int argc, const char** argv)
{
    try
    {
        InitDone initDone;
        std::unique_ptr<cmajor::dom::Element> cmunitElement(new cmajor::dom::Element(U"cmunit"));
        cmunitElement->SetAttribute(U"start", ToUtf32(GetCurrentDateTime().ToString()));
        std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();
        SetGlobalFlag(GlobalFlags::unitTest);
        std::vector<std::string> projectsAndSolutions;
        std::string onlySourceFile;
        std::string onlyTest;
        for (int i = 1; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (!arg.empty() && arg[0] == '-')
            {
                if (arg == "--help" || arg == "-h")
                {
                    PrintHelp();
                    return 0;
                }
                else if (arg == "--verbose" || arg == "-v")
                {
                    SetGlobalFlag(GlobalFlags::verbose);
                }
                else if (arg.find('=') != std::string::npos)
                {
                    std::vector<std::string> components = Split(arg, '=');
                    if (components.size() == 2)
                    {
                        if (components[0] == "--config" || components[0] == "-c")
                        {
                            if (components[1] == "release")
                            {
                                SetGlobalFlag(GlobalFlags::release);
                            }
                            else if (components[1] != "debug")
                            {
                                throw std::runtime_error("unknown configuration '" + components[1] + "'");
                            }
                        }
                        else if (components[0] == "--file" || components[0] == "-f")
                        {
                            onlySourceFile = components[1];
                            boost::filesystem::path p = onlySourceFile;
                            if (p.extension().empty())
                            {
                                onlySourceFile.append(".cm");
                            }
                        }
                        else if (components[0] == "--test" || components[0] == "-t")
                        {
                            onlyTest = components[1];
                        }
                    }
                    else
                    {
                        throw std::runtime_error("unknown option '" + arg + "'");
                    }
                }
                else
                {
                    throw std::runtime_error("unknown option '" + arg + "'");
                }
            }
            else
            {
                projectsAndSolutions.push_back(arg);
            }
        }
        if (projectsAndSolutions.empty())
        {
            PrintHelp();
            throw std::runtime_error("no projects and/or solutions given");
        }
        else
        {
            int n = projectsAndSolutions.size();
            for (int i = 0; i < n; ++i)
            {
                const std::string& projectOrSolution = projectsAndSolutions[i];
                boost::filesystem::path fp(projectOrSolution);
                if (fp.extension() == ".cms")
                {
                    if (!boost::filesystem::exists(fp))
                    {
                        throw std::runtime_error("solution file '" + fp.generic_string() + " not found");
                    }
                    else
                    {
                        TestSolution(GetFullPath(fp.generic_string()), onlySourceFile, onlyTest, cmunitElement.get());
                    }
                }
                else if (fp.extension() == ".cmp")
                {
                    if (!boost::filesystem::exists(fp))
                    {
                        throw std::runtime_error("project file '" + fp.generic_string() + " not found");
                    }
                    else
                    {
                        TestProject(GetFullPath(fp.generic_string()), onlySourceFile, onlyTest, cmunitElement.get());
                    }
                }
                else
                {
                    throw std::runtime_error("Argument '" + fp.generic_string() + "' has invalid extension. Not Cmajor solution (.cms) or project (.cmp) file.");
                }
            }
        }
        cmunitElement->SetAttribute(U"end", ToUtf32(GetCurrentDateTime().ToString()));
        std::chrono::time_point<std::chrono::steady_clock> end = std::chrono::steady_clock::now();
        int durationSecs = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
        int hours = durationSecs / 3600;
        int mins = (durationSecs / 60) % 60;
        int secs = durationSecs % 60;
        std::string durationStr;
        durationStr.append(1, '0' + (hours / 10));
        durationStr.append(1, '0' + (hours % 10));
        durationStr.append(1, ':');
        durationStr.append(1, '0' + (mins / 10));
        durationStr.append(1, '0' + (mins % 10));
        durationStr.append(1, ':');
        durationStr.append(1, '0' + (secs / 10));
        durationStr.append(1, '0' + (secs % 10));
        cmunitElement->SetAttribute(U"duration", ToUtf32(durationStr));
        cmajor::dom::Document testDoc;
        testDoc.AppendChild(std::unique_ptr<cmajor::dom::Node>(cmunitElement.release()));
        std::string cmunitFileName = "cmunit";
        cmunitFileName.append("-").append(GetCurrentDateTime().ToString(true, true, false, false)).append(".xml");
        std::ofstream cmunitXmlFile(cmunitFileName);
        cmajor::util::CodeFormatter formatter(cmunitXmlFile);
        formatter.SetIndentSize(2);
        testDoc.Write(formatter);
        std::unique_ptr<cmajor::dom::Document> reportDoc = GenerateHtmlReport(&testDoc);
        std::string cmunitReportFileName = Path::ChangeExtension(cmunitFileName, ".html");
        std::ofstream cmunitHtmlFile(cmunitReportFileName);
        cmajor::util::CodeFormatter htmlFormatter(cmunitHtmlFile);
        htmlFormatter.SetIndentSize(2);
        reportDoc->Write(htmlFormatter);
    }
    catch (const ParsingException& ex)
    {
        std::cerr << ex.what() << std::endl;
        return 1;
    }
    catch (const Exception& ex)
    {
        std::cerr << ex.What() << std::endl;
        return 1;
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
        return 1;
    }
    return 0;
}