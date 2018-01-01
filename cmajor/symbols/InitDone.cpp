// =================================
// Copyright (c) 2018 Seppo Laakko
// Distributed under the MIT license
// =================================

#include <cmajor/symbols/InitDone.hpp>
#include <cmajor/symbols/Symbol.hpp>
#include <cmajor/symbols/FunctionSymbol.hpp>
#include <cmajor/symbols/Module.hpp>
#include <cmajor/symbols/Warning.hpp>
#include <cmajor/symbols/SymbolTable.hpp>

namespace cmajor { namespace symbols {

void Init()
{
    InitWarning();
    InitSymbol();
    InitFunctionSymbol();
    InitModule();
    InitSymbolTable();
}

void Done()
{
    DoneSymbolTable();
    DoneModule();
    DoneFunctionSymbol();
    DoneSymbol();
}

} } // namespace cmajor::symbols
