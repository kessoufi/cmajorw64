// =================================
// Copyright (c) 2017 Seppo Laakko
// Distributed under the MIT license
// =================================

#ifndef CMAJOR_MODULE_BINDER_INCLUDED
#define CMAJOR_MODULE_BINDER_INCLUDED
#include <cmajor/binder/BoundCompileUnit.hpp>
#include <cmajor/symbols/SymbolTable.hpp>

namespace cmajor { namespace binder {

using namespace cmajor::symbols;

class ModuleBinder 
{
public:
    ModuleBinder(Module& module_, CompileUnitNode* compileUnitNode_);
    void BindClassTemplateSpecialization(ClassTemplateSpecializationSymbol* classTemplateSpecialization);
private:
    BoundCompileUnit boundCompileUnit;
    ContainerScope* containerScope;
    Span& span;
};

} } // namespace cmajor::binder

#endif // CMAJOR_MODULE_BINDER_INCLUDED
