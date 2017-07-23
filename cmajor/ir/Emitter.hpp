// =================================
// Copyright (c) 2017 Seppo Laakko
// Distributed under the MIT license
// =================================

#ifndef CMAJOR_IR_EMITTER_INCLUDED
#define CMAJOR_IR_EMITTER_INCLUDED
#include <cmajor/ir/ValueStack.hpp>
#include <llvm/IR/IRBuilder.h>

namespace cmajor { namespace ir {

typedef llvm::SmallVector<llvm::Value*, 4> ArgVector;

class Emitter
{
public:
    Emitter(llvm::LLVMContext& context_);
    virtual ~Emitter();
    llvm::LLVMContext& Context() { return context; }
    llvm::IRBuilder<>& Builder() { return builder; }
    llvm::Module* Module() { return module; }
    ValueStack& Stack() { return stack; }
    void SetModule(llvm::Module* module_) { module = module_; }
private:
    llvm::LLVMContext& context;
    llvm::IRBuilder<> builder;
    llvm::Module* module;
    ValueStack stack;
};

} } // namespace cmajor::ir

#endif // CMAJOR_IR_EMITTER_INCLUDED
