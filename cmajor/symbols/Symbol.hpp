// =================================
// Copyright (c) 2017 Seppo Laakko
// Distributed under the MIT license
// =================================

#ifndef CMAJOR_SYMBOLS_SYMBOL_INCLUDED
#define CMAJOR_SYMBOLS_SYMBOL_INCLUDED
#include <cmajor/ast/Specifier.hpp>
#include <cmajor/ast/CompileUnit.hpp>
#include <cmajor/parsing/Scanner.hpp>
#include <cmajor/util/CodeFormatter.hpp>
#include <llvm/IR/Value.h>
#include <unordered_set>
#include <stdint.h>

namespace cmajor { namespace symbols {

using cmajor::parsing::Span;
using namespace cmajor::ast;
using namespace cmajor::util;

class SymbolWriter;
class SymbolReader;
class ContainerScope;
class ContainerSymbol;
class TypeSymbol;
class ConceptSymbol;
class ClassTypeSymbol;
class InterfaceTypeSymbol;
class NamespaceSymbol;
class FunctionSymbol;
class SymbolTable;
class Module;
class SymbolCollector;

enum class SymbolType : uint8_t
{
    boolTypeSymbol, sbyteTypeSymbol, byteTypeSymbol, shortTypeSymbol, ushortTypeSymbol, intTypeSymbol, uintTypeSymbol, longTypeSymbol, ulongTypeSymbol, floatTypeSymbol, doubleTypeSymbol, 
    charTypeSymbol, wcharTypeSymbol, ucharTypeSymbol, voidTypeSymbol, nullPtrTypeSymbol,
    derivedTypeSymbol,
    namespaceSymbol, functionSymbol, staticConstructorSymbol, constructorSymbol, destructorSymbol, memberFunctionSymbol, functionGroupSymbol, 
    classGroupTypeSymbol, classTypeSymbol, classTemplateSpecializationSymbol, interfaceTypeSymbol, conceptGroupSymbol, conceptSymbol,
    delegateTypeSymbol, classDelegateTypeSymbol, declarationBlock, typedefSymbol, constantSymbol, enumTypeSymbol, enumConstantSymbol,
    templateParameterSymbol, boundTemplateParameterSymbol, parameterSymbol, localVariableSymbol, memberVariableSymbol,
    basicTypeUnaryPlus, basicTypeIntUnaryMinus, basicTypeFloatUnaryMinus, basicTypeComplement, basicTypeAdd, basicTypeFAdd, basicTypeSub, basicTypeFSub, basicTypeMul, basicTypeFMul, 
    basicTypeSDiv, basicTypeUDiv, basicTypeFDiv, basicTypeSRem, basicTypeURem, basicTypeAnd, basicTypeOr, basicTypeXor, basicTypeShl, basicTypeAShr, basicTypeLShr,
    basicTypeNot, basicTypeIntegerEquality, basicTypeUnsignedIntegerLessThan, basicTypeSignedIntegerLessThan, basicTypeFloatingEquality, basicTypeFloatingLessThan,
    defaultInt1, defaultInt8, defaultInt16, defaultInt32, defaultInt64, defaultFloat, defaultDouble, basicTypeCopyCtor, basicTypeMoveCtor, basicTypeCopyAssignment, basicTypeMoveAssignment, 
    basicTypeReturn,
    basicTypeImplicitSignExtension, basicTypeImplicitZeroExtension, basicTypeExplicitSignExtension, basicTypeExplicitZeroExtension, basicTypeTruncation, basicTypeBitCast,
    basicTypeImplicitUnsignedIntToFloating, basicTypeImplicitSignedIntToFloating, basicTypeExplicitUnsignedIntToFloating, basicTypeExplicitSignedIntToFloating, 
    basicTypeFloatingToUnsignedInt, basicTypeFloatingToSignedInt, basicTypeFloatingExtension, basicTypeFloatingTruncation, 
    enumTypeDefaultConstructor, enumTypeCopyConstructor, enumTypeMoveConstructor, enumTypeCopyAssignment, enumTypeMoveAssignment, enumTypeReturn, enumTypeEquality, 
    enumTypeToUnderlyingType, underlyingToEnumType,
    namespaceTypeSymbol, functionGroupTypeSymbol, memberExpressionTypeSymbol, valueSymbol,
    maxSymbol
};

std::string SymbolTypeStr(SymbolType symbolType);

enum class SymbolAccess : uint8_t
{
    private_ = 0, protected_ = 1, internal_ = 2, public_ = 3
};

enum class SymbolFlags : uint8_t
{
    none = 0, 
    access = 1 << 0 | 1 << 1,
    static_ = 1 << 2,
    nothrow_ = 1 << 3,
    project = 1 << 4,
    bound = 1 << 5,
    export_ = 1 << 6,
    exportComputed = 1 << 7
};

inline SymbolFlags operator&(SymbolFlags left, SymbolFlags right)
{
    return SymbolFlags(uint8_t(left) & uint8_t(right));
}

inline SymbolFlags operator|(SymbolFlags left, SymbolFlags right)
{
    return SymbolFlags(uint8_t(left) | uint8_t(right));
}

inline SymbolFlags operator~(SymbolFlags flags)
{
    return SymbolFlags(~uint8_t(flags));
}

std::string SymbolFlagStr(SymbolFlags symbolFlags);
std::string SymbolFlagStr(SymbolFlags symbolFlags, bool noAccess);

class Symbol
{
public:
    Symbol(SymbolType symbolType_, const Span& span_, const std::u32string& name_);
    virtual ~Symbol();
    virtual void Write(SymbolWriter& writer);
    virtual void Read(SymbolReader& reader);
    virtual void EmplaceType(TypeSymbol* typeSymbol, int index) {}
    virtual void EmplaceConcept(ConceptSymbol* conceptSymbol) {}
    virtual void EmplaceFunction(FunctionSymbol* functionSymbol, int index) {}
    virtual bool IsExportSymbol() const { return IsProject(); }
    virtual bool IsContainerSymbol() const { return false; }
    virtual bool IsFunctionSymbol() const { return false; }
    virtual bool IsTypeSymbol() const { return false; }
    virtual bool IsClassTypeSymbol() const { return false; }
    virtual void Accept(SymbolCollector* collector) {}
    virtual const ContainerScope* GetContainerScope() const { return nullptr; }
    virtual ContainerScope* GetContainerScope() { return nullptr; }
    virtual std::u32string FullName() const;
    virtual std::u32string FullNameWithSpecifiers() const;
    virtual std::u32string SimpleName() const { return Name(); }
    virtual SymbolAccess DeclaredAccess() const { return Access(); }
    virtual std::string TypeString() const { return "symbol";  }
    virtual llvm::Value* IrObject() { return irObject; }
    virtual void ComputeMangledName();
    virtual void ComputeExportClosure();
    virtual void Dump(CodeFormatter& formatter) {}
    void SetMangledName(const std::u32string& mangledName_);
    SymbolAccess Access() const { return SymbolAccess(flags & SymbolFlags::access);  }
    void SetAccess(SymbolAccess access_) { flags = flags | SymbolFlags(access_); }
    void SetAccess(Specifiers accessSpecifiers);
    bool IsSameParentOrAncestorOf(const Symbol* that) const;
    SymbolType GetSymbolType() const { return symbolType; }
    const Span& GetSpan() const { return span; }
    void SetSpan(const Span& span_) { span = span_; }
    const std::u32string& Name() const { return name; }
    void SetName(const std::u32string& name_) { name = name_; }
    SymbolFlags GetSymbolFlags() const { return flags; }
    bool IsStatic() const { return GetFlag(SymbolFlags::static_); }
    void SetStatic() { SetFlag(SymbolFlags::static_); }
    bool IsNothrow() const { return GetFlag(SymbolFlags::nothrow_); }
    void SetNothrow() { SetFlag(SymbolFlags::nothrow_); }
    bool IsProject() const { return GetFlag(SymbolFlags::project); }
    void SetProject() { SetFlag(SymbolFlags::project); }
    bool IsBound() const { return GetFlag(SymbolFlags::bound); }
    void SetBound() { SetFlag(SymbolFlags::bound); }
    bool MarkedExport() const { return GetFlag(SymbolFlags::export_); }
    void MarkExport() { SetFlag(SymbolFlags::export_); }
    bool ExportComputed() const { return GetFlag(SymbolFlags::exportComputed); }
    void SetExportComputed() { SetFlag(SymbolFlags::exportComputed); }
    bool GetFlag(SymbolFlags flag) const { return (flags & flag) != SymbolFlags::none; }
    void SetFlag(SymbolFlags flag) { flags = flags | flag; }
    void ResetFlag(SymbolFlags flag) { flags = flags & ~flag; }
    const Symbol* Parent() const { return parent; }
    Symbol* Parent() { return parent; }
    void SetParent(Symbol* parent_) { parent = parent_; }
    const NamespaceSymbol* Ns() const;
    NamespaceSymbol* Ns();
    const ClassTypeSymbol* ClassNoThrow() const;
    ClassTypeSymbol* ClassNoThrow();
    const ContainerSymbol* ClassOrNsNoThrow() const;
    ContainerSymbol* ClassOrNsNoThrow() ;
    const ContainerSymbol* ClassInterfaceOrNsNoThrow() const;
    ContainerSymbol* ClassInterfaceOrNsNoThrow();
    const ContainerSymbol* ClassInterfaceEnumOrNsNoThrow() const;
    ContainerSymbol* ClassInterfaceEnumOrNsNoThrow();
    const ClassTypeSymbol* Class() const;
    ClassTypeSymbol* Class();
    const ClassTypeSymbol* ContainingClassNoThrow() const;
    ClassTypeSymbol* ContainingClassNoThrow();
    const InterfaceTypeSymbol* InterfaceNoThrow() const;
    InterfaceTypeSymbol* InterfaceNoThrow();
    const InterfaceTypeSymbol* ContainingInterfaceNoThrow() const;
    InterfaceTypeSymbol* ContainingInterfaceNoThrow() ;
    const FunctionSymbol* FunctionNoThrow() const;
    FunctionSymbol* FunctionNoThrow();
    const FunctionSymbol* Function() const;
    FunctionSymbol* Function();
    const FunctionSymbol* ContainingFunctionNoThrow() const;
    FunctionSymbol* ContainingFunctionNoThrow() ;
    const ContainerScope* ClassOrNsScope() const;
    ContainerScope* ClassOrNsScope();
    const ContainerScope* ClassInterfaceOrNsScope() const;
    ContainerScope* ClassInterfaceOrNsScope() ;
    const ContainerScope* ClassInterfaceEnumOrNsScope() const;
    ContainerScope* ClassInterfaceEnumOrNsScope();
    const SymbolTable* GetSymbolTable() const { return symbolTable; }
    SymbolTable* GetSymbolTable() { return symbolTable; }
    void SetSymbolTable(SymbolTable* symbolTable_) { symbolTable = symbolTable_; }
    const Module* GetModule() const { return module; }
    Module* GetModule() { return module; }
    void SetModule(Module* module_) { module = module_; }
    const CompileUnitNode* GetCompileUnit() const { return compileUnit; }
    void SetCompileUnit(CompileUnitNode* compileUnit_) { compileUnit = compileUnit_; }
    void SetIrObject(llvm::Value* irObject_) { irObject = irObject_; }
    const std::u32string& MangledName() const { return mangledName; }
private:
    SymbolType symbolType;
    Span span;
    std::u32string name;
    SymbolFlags flags;
    std::u32string mangledName;
    Symbol* parent;
    SymbolTable* symbolTable;
    Module* module;
    CompileUnitNode* compileUnit;
    llvm::Value* irObject;
};

class SymbolCreator
{
public:
    virtual ~SymbolCreator();
    virtual Symbol* CreateSymbol(const Span& span, const std::u32string& name) = 0;
};

class SymbolFactory
{
public:
    static void Init();
    static void Done();
    static SymbolFactory& Instance() { Assert(instance, "symbol factory not initialized"); return *instance; }
    Symbol* CreateSymbol(SymbolType symbolType, const Span& span, const std::u32string& name);
    void Register(SymbolType symbolType, SymbolCreator* creator);
private:
    static std::unique_ptr<SymbolFactory> instance;
    std::vector<std::unique_ptr<SymbolCreator>> symbolCreators;
    SymbolFactory();
};

void InitSymbol();
void DoneSymbol();

} } // namespace cmajor::symbols

#endif // CMAJOR_SYMBOLS_SYMBOL_INCLUDED
