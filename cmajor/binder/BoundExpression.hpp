// =================================
// Copyright (c) 2017 Seppo Laakko
// Distributed under the MIT license
// =================================

#ifndef CMAJOR_BINDER_BOUND_EXPRESSION_INCLUDED
#define CMAJOR_BINDER_BOUND_EXPRESSION_INCLUDED
#include <cmajor/binder/BoundNode.hpp>
#include <cmajor/symbols/TypeSymbol.hpp>
#include <cmajor/symbols/VariableSymbol.hpp>
#include <cmajor/symbols/ConstantSymbol.hpp>
#include <cmajor/symbols/EnumSymbol.hpp>

namespace cmajor { namespace binder {

using namespace cmajor::symbols;

enum class BoundExpressionFlags : uint8_t
{
    none = 0, address = 1, value = 2
};

inline BoundExpressionFlags operator|(BoundExpressionFlags left, BoundExpressionFlags right)
{
    return BoundExpressionFlags(uint8_t(left) | uint8_t(right));
}

inline BoundExpressionFlags operator&(BoundExpressionFlags left, BoundExpressionFlags right)
{
    return BoundExpressionFlags(uint8_t(left) & uint8_t(right));
}

class BoundExpression : public BoundNode
{
public:
    BoundExpression(const Span& span_, BoundNodeType boundNodeType_, TypeSymbol* type_);
    virtual bool IsComplete() const { return true; }
    virtual bool IsLvalueExpression() const { return false; }
    virtual bool HasValue() const { return false; }
    const TypeSymbol* GetType() const { return type; }
    TypeSymbol* GetType() { return type; }
    bool GetFlag(BoundExpressionFlags flag) const { return (flags & flag) != BoundExpressionFlags::none; }
    void SetFlag(BoundExpressionFlags flag) { flags = flags | flag; }
private:
    TypeSymbol* type;
    BoundExpressionFlags flags;
};

class BoundParameter : public BoundExpression
{
public:
    BoundParameter(ParameterSymbol* parameterSymbol_);
    void Load(Emitter& emitter) override;
    void Store(Emitter& emitter) override;
    void Accept(BoundNodeVisitor& visitor) override;
    bool HasValue() const override { return true; }
    bool IsLvalueExpression() const override { return true; }
private:
    ParameterSymbol* parameterSymbol;
};

class BoundLocalVariable : public BoundExpression
{
public:
    BoundLocalVariable(LocalVariableSymbol* localVariableSymbol_);
    void Load(Emitter& emitter) override;
    void Store(Emitter& emitter) override;
    void Accept(BoundNodeVisitor& visitor) override;
    bool HasValue() const override { return true; }
    bool IsLvalueExpression() const override { return true; }
private:
    LocalVariableSymbol* localVariableSymbol;
};

class BoundMemberVariable : public BoundExpression
{
public:
    BoundMemberVariable(MemberVariableSymbol* memberVariableSymbol_);
    void Load(Emitter& emitter) override;
    void Store(Emitter& emitter) override;
    void Accept(BoundNodeVisitor& visitor) override;
    bool HasValue() const override { return true; }
    bool IsLvalueExpression() const override { return true; }
    void SetClassObject(std::unique_ptr<BoundExpression>&& classObject_);
    void SetStaticInitNeeded() { staticInitNeeded = true; }
private:
    MemberVariableSymbol* memberVariableSymbol;
    std::unique_ptr<BoundExpression> classObject;
    bool staticInitNeeded;
};

class BoundConstant : public BoundExpression
{
public:
    BoundConstant(ConstantSymbol* constantSymbol_);
    void Load(Emitter& emitter) override;
    void Store(Emitter& emitter) override;
    void Accept(BoundNodeVisitor& visitor) override;
    bool HasValue() const override { return true; }
private:
    ConstantSymbol* constantSymbol;
};

class BoundEnumConstant : public BoundExpression
{
public:
    BoundEnumConstant(EnumConstantSymbol* enumConstantSymbol_);
    void Load(Emitter& emitter) override;
    void Store(Emitter& emitter) override;
    void Accept(BoundNodeVisitor& visitor) override;
    bool HasValue() const override { return true; }
private:
    EnumConstantSymbol* enumConstantSymbol;
};

class BoundLiteral : public BoundExpression
{
public:
    BoundLiteral(std::unique_ptr<Value>&& value_, TypeSymbol* type_);
    void Load(Emitter& emitter) override;
    void Store(Emitter& emitter) override;
    void Accept(BoundNodeVisitor& visitor) override;
    bool HasValue() const override { return true; }
private:
    std::unique_ptr<Value> value;
};

class BoundFunctionCall : public BoundExpression
{
public:
    BoundFunctionCall(const Span& span_, FunctionSymbol* functionSymbol_);
    void Load(Emitter& emitter) override;
    void Store(Emitter& emitter) override;
    void Accept(BoundNodeVisitor& visitor) override;
    bool HasValue() const override;
    const FunctionSymbol* GetFunctionSymbol() const { return functionSymbol; }
    FunctionSymbol* GetFunctionSymbol() { return functionSymbol; }
    void AddArgument(std::unique_ptr<BoundExpression>&& argument);
    void SetArguments(std::vector<std::unique_ptr<BoundExpression>>&& arguments_);
private:
    FunctionSymbol* functionSymbol;
    std::vector<std::unique_ptr<BoundExpression>> arguments;
};

class BoundConversion : public BoundExpression
{
public:
    BoundConversion(std::unique_ptr<BoundExpression>&& sourceExpr_, FunctionSymbol* conversionFun_);
    void Load(Emitter& emitter) override;
    void Store(Emitter& emitter) override;
    void Accept(BoundNodeVisitor& visitor) override;
    bool HasValue() const override { return true; }
private:
    std::unique_ptr<BoundExpression> sourceExpr;
    FunctionSymbol* conversionFun;
};

class BoundTypeExpression : public BoundExpression
{
public:
    BoundTypeExpression(const Span& span_, TypeSymbol* type_);
    bool IsComplete() const override { return false; }
    void Load(Emitter& emitter) override;
    void Store(Emitter& emitter) override;
    void Accept(BoundNodeVisitor& visitor) override;
};

class BoundNamespaceExpression : public BoundExpression
{
public:
    BoundNamespaceExpression(const Span& span_, NamespaceSymbol* ns_);
    bool IsComplete() const override { return false; }
    void Load(Emitter& emitter) override;
    void Store(Emitter& emitter) override;
    void Accept(BoundNodeVisitor& visitor) override;
private:
    NamespaceSymbol* ns;
    std::unique_ptr<TypeSymbol> nsType;
};

class BoundFunctionGroupExpression : public BoundExpression
{
public:
    BoundFunctionGroupExpression(const Span& span_, FunctionGroupSymbol* functionGroupSymbol_);
    bool IsComplete() const override { return false; }
    void Load(Emitter& emitter) override;
    void Store(Emitter& emitter) override;
    void Accept(BoundNodeVisitor& visitor) override;
    const FunctionGroupSymbol* FunctionGroup() const { return functionGroupSymbol; }
    FunctionGroupSymbol* FunctionGroup() { return functionGroupSymbol; }
    void SetClassObject(std::unique_ptr<BoundExpression>&& classObject_);
    bool IsScopeQualified() const { return scopeQualified; }
    void SetScopeQualified() { scopeQualified = true; }
    ContainerScope* QualifiedScope() const { return qualifiedScope; }
    void SetQualifiedScope(ContainerScope* qualifiedScope_) { qualifiedScope = qualifiedScope_; }
private:
    FunctionGroupSymbol* functionGroupSymbol;
    std::unique_ptr<TypeSymbol> functionGroupType;
    std::unique_ptr<BoundExpression> classObject;
    bool scopeQualified;
    ContainerScope* qualifiedScope;
};

} } // namespace cmajor::binder

#endif // CMAJOR_BINDER_BOUND_EXPRESSION_INCLUDED
