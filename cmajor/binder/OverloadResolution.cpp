// =================================
// Copyright (c) 2017 Seppo Laakko
// Distributed under the MIT license
// =================================

#include <cmajor/binder/OverloadResolution.hpp>
#include <cmajor/binder/BoundCompileUnit.hpp>
#include <cmajor/binder/BoundFunction.hpp>
#include <cmajor/binder/BoundConstraint.hpp>
#include <cmajor/binder/Concept.hpp>
#include <cmajor/symbols/TemplateSymbol.hpp>
#include <cmajor/symbols/GlobalFlags.hpp>
#include <cmajor/symbols/Warning.hpp>
#include <cmajor/util/Unicode.hpp>

namespace cmajor { namespace binder {

using namespace cmajor::unicode;

bool BetterFunctionMatch::operator()(const FunctionMatch& left, const FunctionMatch& right) const
{
    int leftBetterArgumentMatches = 0;
    int rightBetterArgumentMatches = 0;
    int n = std::max(int(left.argumentMatches.size()), int(right.argumentMatches.size()));
    for (int i = 0; i < n; ++i)
    {
        ArgumentMatch leftMatch;
        if (i < int(left.argumentMatches.size()))
        {
            leftMatch = left.argumentMatches[i];
        }
        ArgumentMatch rightMatch;
        if (i < int(right.argumentMatches.size()))
        {
            rightMatch = right.argumentMatches[i];
        }
        if (BetterArgumentMatch(leftMatch, rightMatch))
        {
            ++leftBetterArgumentMatches;
        }
        else if (BetterArgumentMatch(rightMatch, leftMatch))
        {
            ++rightBetterArgumentMatches;
        }
    }
    if (leftBetterArgumentMatches > rightBetterArgumentMatches)
    {
        return true;
    }
    if (rightBetterArgumentMatches > leftBetterArgumentMatches)
    {
        return false;
    }
    if (left.numConversions < right.numConversions)
    {
        return true;
    }
    if (left.numConversions > right.numConversions)
    {
        return false;
    }
    if (left.numQualifyingConversions < right.numQualifyingConversions)
    {
        return true;
    }
    if (left.numQualifyingConversions > right.numQualifyingConversions)
    {
        return false;
    }
    if (!left.fun->IsFunctionTemplate() && right.fun->IsFunctionTemplate())
    {
        return true;
    }
    if (left.fun->IsFunctionTemplate() && !right.fun->IsFunctionTemplate())
    {
        return false;
    }
    if (!left.fun->IsTemplateSpecialization() && right.fun->IsTemplateSpecialization())
    {
        return true;
    }
    if (left.fun->IsTemplateSpecialization() && !right.fun->IsTemplateSpecialization())
    {
        return false;
    }
    if (left.boundConstraint && !right.boundConstraint)
    {
        return true;
    }
    if (!left.boundConstraint && right.boundConstraint)
    {
        return false;
    }
    if (left.boundConstraint && right.boundConstraint)
    {
        bool leftSubsumeRight = left.boundConstraint->Subsume(right.boundConstraint);
        bool rightSubsumeLeft = right.boundConstraint->Subsume(left.boundConstraint);
        if (leftSubsumeRight && !rightSubsumeLeft)
        {
            return true;
        }
        if (rightSubsumeLeft && !leftSubsumeRight)
        {
            return false;
        }
    }
    return false;
}

bool FindQualificationConversion(TypeSymbol* sourceType, TypeSymbol* targetType, BoundExpression* argument, ConversionType conversionType, const Span& span, FunctionMatch& functionMatch, ArgumentMatch& argumentMatch)
{
    int distance = 0;
    if (argumentMatch.conversionFun)
    {
        distance = argumentMatch.conversionFun->ConversionDistance();
    }
    if (targetType->IsRvalueReferenceType() && !sourceType->IsRvalueReferenceType())
    {
        ++functionMatch.numQualifyingConversions;
    }
    if (sourceType->IsConstType())
    {
        if (targetType->IsConstType() || !targetType->IsReferenceType())
        {
            ++distance;
        }
        else if (conversionType == ConversionType::implicit_)
        {
            if (sourceType->PointerCount() < 1)
            {
                functionMatch.cannotBindConstArgToNonConstParam = true;
                functionMatch.sourceType = sourceType;
                functionMatch.targetType = targetType;
                return false;
            }
            else
            {
                ++distance;
            }
        }
        else
        {
            distance = 255;
        }
    }
    else
    {
        if (targetType->IsConstType())
        {
            distance += 2;
        }
        else
        {
            distance += 3;
        }
    }
    if (sourceType->IsReferenceType() && !targetType->IsReferenceType())
    {
        argumentMatch.postReferenceConversionFlags = OperationFlags::deref;
        argumentMatch.conversionDistance = distance;
        ++functionMatch.numQualifyingConversions;
        return true;
    }
    else if (!sourceType->IsReferenceType() && (targetType->IsReferenceType() || targetType->IsClassTypeSymbol())) // conversion function logic removed!
    {
        if (targetType->IsConstType() || targetType->IsClassTypeSymbol()) // conversion function logic removed!
        {
            argumentMatch.postReferenceConversionFlags = OperationFlags::addr;
            argumentMatch.conversionDistance = distance;
            ++functionMatch.numQualifyingConversions;
            return true;
        }
        else if ((!sourceType->IsConstType() || sourceType->PointerCount() >= 1) && argument->IsLvalueExpression())
        {
            if (targetType->IsRvalueReferenceType() && !sourceType->IsRvalueReferenceType())
            {
                if (argument->GetFlag(BoundExpressionFlags::bindToRvalueReference))
                {
                    distance = 0;
                }
                else
                {
                    distance += 10;
                }
            }
            argumentMatch.postReferenceConversionFlags = OperationFlags::addr;
            argumentMatch.conversionDistance = distance;
            ++functionMatch.numQualifyingConversions;
            return true;
        }
        else 
        {
            functionMatch.cannotBindConstArgToNonConstParam = true;
            functionMatch.sourceType = sourceType;
            functionMatch.targetType = targetType;
        }
    }
    else if (sourceType->IsConstType() && !targetType->IsConstType())
    {
        ++functionMatch.numQualifyingConversions;
        ++distance;
        if (sourceType->IsLvalueReferenceType() && targetType->IsRvalueReferenceType())
        {
            ++distance;
            ++functionMatch.numQualifyingConversions;
        }
        argumentMatch.conversionDistance = distance;
        return true;
    }
    else if (!sourceType->IsConstType() && targetType->IsConstType())
    {
        ++functionMatch.numQualifyingConversions;
        ++distance;
        if (sourceType->IsLvalueReferenceType() && targetType->IsRvalueReferenceType())
        {
            ++distance;
            ++functionMatch.numQualifyingConversions;
        }
        argumentMatch.conversionDistance = distance;
        return true;
    }
    else if (sourceType->IsLvalueReferenceType() && targetType->IsRvalueReferenceType())
    {
        ++distance;
        ++functionMatch.numQualifyingConversions;
        argumentMatch.conversionDistance = distance;
        return true;
    }
    else if (sourceType->IsRvalueReferenceType() && targetType->IsLvalueReferenceType())
    {
        ++distance;
        ++functionMatch.numQualifyingConversions;
        argumentMatch.conversionDistance = distance;
        return true;
    }
    else if (argumentMatch.conversionFun)
    {
        argumentMatch.conversionDistance = distance;
        return true;
    }
    return false;
}

bool FindTemplateParameterMatch(TypeSymbol* sourceType, TypeSymbol* targetType, ConversionType conversionType, BoundExpression* argument,
    BoundCompileUnit& boundCompileUnit, FunctionMatch& functionMatch, ContainerScope* containerScope, BoundFunction* currentFunction, const Span& span)
{
    if (targetType->BaseType()->GetSymbolType() != SymbolType::templateParameterSymbol) return false;
    TemplateParameterSymbol* templateParameter = static_cast<TemplateParameterSymbol*>(targetType->BaseType());
    TypeSymbol* templateArgumentType = nullptr;
    auto it = functionMatch.templateParameterMap.find(templateParameter);
    if (it == functionMatch.templateParameterMap.cend())
    {
        templateArgumentType = sourceType->RemoveDerivations(targetType->DerivationRec(), span);
        if (templateArgumentType)
        {
            functionMatch.templateParameterMap[templateParameter] = templateArgumentType;
        }
        else
        {
            return false;
        }
    }
    else
    {
        templateArgumentType = it->second;
    }
    targetType = targetType->Unify(templateArgumentType, span);
    if (!targetType)
    {
        return false;
    }
    if (TypesEqual(sourceType, targetType))
    {
        functionMatch.argumentMatches.push_back(ArgumentMatch());
        return true;
    }
    else
    {
        bool qualificationConversionMatch = false;
        ArgumentMatch argumentMatch;
        if (TypesEqual(sourceType->PlainType(span), targetType->PlainType(span)))
        {
            qualificationConversionMatch = FindQualificationConversion(sourceType, targetType, argument, conversionType, span, functionMatch, argumentMatch);
        }
        if (qualificationConversionMatch)
        {
            functionMatch.argumentMatches.push_back(argumentMatch);
            return true;
        }
        else
        {
            FunctionSymbol* conversionFun = boundCompileUnit.GetConversion(sourceType, targetType, containerScope, currentFunction, span, argumentMatch);
            if (conversionFun)
            {
                if (conversionFun->GetConversionType() == conversionType || conversionFun->GetConversionType() == ConversionType::implicit_)
                {
                    ++functionMatch.numConversions;
                    argumentMatch.conversionFun = conversionFun;
                    if (argumentMatch.postReferenceConversionFlags == OperationFlags::none)
                    {
                        if (FindQualificationConversion(sourceType, targetType, argument, conversionType, span, functionMatch, argumentMatch))
                        {
                            functionMatch.argumentMatches.push_back(argumentMatch);
                            return true;
                        }
                        else
                        {
                            return false;
                        }
                    }
                    else
                    {
                        if (FindQualificationConversion(conversionFun->ConversionSourceType(), targetType, argument, conversionType, span, functionMatch, argumentMatch))
                        {
                            functionMatch.argumentMatches.push_back(argumentMatch);
                            return true;
                        }
                        else
                        {
                            return false;
                        }
                    }
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }
    }
    return false;
}

bool FindClassTemplateSpecializationMatch(TypeSymbol* sourceType, TypeSymbol* targetType, ConversionType conversionType, BoundExpression* argument,
    BoundCompileUnit& boundCompileUnit, FunctionMatch& functionMatch, ContainerScope* containerScope, BoundFunction* currentFunction, const Span& span)
{
    if (targetType->BaseType()->GetSymbolType() != SymbolType::classTemplateSpecializationSymbol)
    {
        return false;
    }
    ClassTemplateSpecializationSymbol* targetClassTemplateSpecialization = static_cast<ClassTemplateSpecializationSymbol*>(targetType->BaseType());
    int n = targetClassTemplateSpecialization->TemplateArgumentTypes().size();
    int numArgumentMatches = functionMatch.argumentMatches.size();
    if (sourceType->BaseType()->GetSymbolType() == SymbolType::classTemplateSpecializationSymbol)
    {
        ClassTemplateSpecializationSymbol* sourceClassTemplateSpecialization = static_cast<ClassTemplateSpecializationSymbol*>(sourceType->BaseType());
        int m = sourceClassTemplateSpecialization->TemplateArgumentTypes().size();
        if (n != m) return false;
        for (int i = 0; i < n; ++i)
        {
            TypeSymbol* sourceArgumentType = sourceClassTemplateSpecialization->TemplateArgumentTypes()[i];
            TypeSymbol* targetArgumentType = targetClassTemplateSpecialization->TemplateArgumentTypes()[i];
            if (!FindTemplateParameterMatch(sourceArgumentType, targetArgumentType, conversionType, argument, boundCompileUnit, functionMatch, containerScope, currentFunction, span))
            {
                return false;
            }
        }
    }
    functionMatch.argumentMatches.resize(numArgumentMatches);
    std::vector<TypeSymbol*> targetTemplateArguments;
    for (int i = 0; i < n; ++i)
    {
        TemplateParameterSymbol* templateParameter = static_cast<TemplateParameterSymbol*>(targetClassTemplateSpecialization->TemplateArgumentTypes()[i]->BaseType());
        auto it = functionMatch.templateParameterMap.find(templateParameter);
        if (it != functionMatch.templateParameterMap.cend())
        {
            TypeSymbol* templateArgumentType = it->second;
            targetTemplateArguments.push_back(templateArgumentType);
        }
        else
        {
            return false;
        }
    }
    TypeSymbol* plainTargetType = boundCompileUnit.GetSymbolTable().MakeClassTemplateSpecialization(targetClassTemplateSpecialization->GetClassTemplate(), targetTemplateArguments, span);
    targetType = boundCompileUnit.GetSymbolTable().MakeDerivedType(plainTargetType, targetType->DerivationRec(), span);
    if (TypesEqual(sourceType, targetType))
    {
        functionMatch.argumentMatches.push_back(ArgumentMatch());
        return true;
    }
    else
    {
        bool qualificationConversionMatch = false;
        ArgumentMatch argumentMatch;
        if (TypesEqual(sourceType->PlainType(span), targetType->PlainType(span)))
        {
            qualificationConversionMatch = FindQualificationConversion(sourceType, targetType, argument, conversionType, span, functionMatch, argumentMatch);
        }
        if (qualificationConversionMatch)
        {
            functionMatch.argumentMatches.push_back(argumentMatch);
            return true;
        }
        else
        {
            FunctionSymbol* conversionFun = boundCompileUnit.GetConversion(sourceType, targetType, containerScope, currentFunction, span, argumentMatch);
            if (conversionFun)
            {
                if (conversionFun->GetConversionType() == conversionType || conversionFun->GetConversionType() == ConversionType::implicit_)
                {
                    argumentMatch.conversionFun = conversionFun;
                    ++functionMatch.numConversions;
                    if (argumentMatch.preReferenceConversionFlags == OperationFlags::none)
                    {
                        if (FindQualificationConversion(sourceType, targetType, argument, conversionType, span, functionMatch, argumentMatch))
                        {
                            functionMatch.argumentMatches.push_back(argumentMatch);
                            return true;
                        }
                        else
                        {
                            return false;
                        }
                    }
                    else
                    {
                        if (FindQualificationConversion(conversionFun->ConversionSourceType(), targetType, argument, conversionType, span, functionMatch, argumentMatch))
                        {
                            functionMatch.argumentMatches.push_back(argumentMatch);
                            return true;
                        }
                        else
                        {
                            return false;
                        }
                    }
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }
    }
    return false;
}

bool FindConversions(BoundCompileUnit& boundCompileUnit, FunctionSymbol* function, std::vector<std::unique_ptr<BoundExpression>>& arguments, FunctionMatch& functionMatch, 
    ConversionType conversionType, ContainerScope* containerScope, BoundFunction* currentFunction, const Span& span)
{
    int arity = arguments.size();
    if (arity == 1 && function->GroupName() == U"@constructor" && arguments[0]->GetType()->IsReferenceType())
    {
        functionMatch.referenceMustBeInitialized = true;
        return false;
    }
    Assert(arity == function->Arity(), "wrong arity");
    for (int i = 0; i < arity; ++i)
    {
        BoundExpression* argument = arguments[i].get();
        TypeSymbol* sourceType = argument->GetType();
        ParameterSymbol* parameter = function->Parameters()[i];
        TypeSymbol* targetType = parameter->GetType();
        if (arity == 2 && function->GroupName() == U"operator=")
        {
            if (targetType->IsRvalueReferenceType() && !sourceType->IsRvalueReferenceType() && !argument->GetFlag(BoundExpressionFlags::bindToRvalueReference))
            {
                ++functionMatch.numQualifyingConversions;
            }
            if (i == 0)
            {
                if (targetType->IsConstType() && targetType->PointerCount() <= 1)
                {
                    functionMatch.cannotAssignToConstObject = true;
                    return false;
                }
            }
            if (i == 0 && TypesEqual(sourceType, targetType))    // exact match
            {
                if (sourceType->IsReferenceType() && !function->IsConstructorDestructorOrNonstaticMemberFunction())
                {
                    functionMatch.argumentMatches.push_back(ArgumentMatch(OperationFlags::none, nullptr, OperationFlags::deref, 1));
                    ++functionMatch.numConversions;
                    continue;
                }
            }
            else if (i == 1 && sourceType->IsReferenceType() && targetType->IsLvalueReferenceType() && TypesEqual(sourceType->RemoveReference(span), targetType->RemoveReference(span)))
            {
                if (!function->IsConstructorDestructorOrNonstaticMemberFunction())
                {
                    functionMatch.argumentMatches.push_back(ArgumentMatch(OperationFlags::none, nullptr, OperationFlags::deref, 1));
                    ++functionMatch.numConversions;
                    continue;
                }
            }
            else if (i == 1 && sourceType->IsReferenceType() && targetType->IsRvalueReferenceType() && TypesEqual(sourceType->RemoveReference(span), targetType->RemoveReference(span)))
            {
                if (!function->IsConstructorDestructorOrNonstaticMemberFunction())
                {
                    functionMatch.argumentMatches.push_back(ArgumentMatch(OperationFlags::none, nullptr, OperationFlags::none, 0));
                    continue;
                }
            }
            else if (i == 1 && !sourceType->IsReferenceType() && argument->IsLvalueExpression() && targetType->IsReferenceType() && TypesEqual(sourceType, targetType->RemoveReference(span)))
            {
                if (!function->IsConstructorDestructorOrNonstaticMemberFunction())
                {
                    functionMatch.argumentMatches.push_back(ArgumentMatch(OperationFlags::none, nullptr, OperationFlags::none , 0));
                    continue;
                }
            }
            else if (i == 1 && function->IsLvalueReferenceCopyAssignment() && TypesEqual(sourceType, targetType->RemoveReference(span)))
            {
                functionMatch.argumentMatches.push_back(ArgumentMatch(OperationFlags::none, nullptr, OperationFlags::none, 0));
                continue;
            }
            else if (i == 1 && function->IsLvalueReferenceCopyAssignment())
            {
                ArgumentMatch argumentMatch;
                FunctionSymbol* conversionFun = boundCompileUnit.GetConversion(sourceType, targetType->RemoveReference(span), containerScope, currentFunction, span, argumentMatch);
                if (conversionFun->GetConversionType() == conversionType || conversionFun->GetConversionType() == ConversionType::implicit_)
                {
                    ++functionMatch.numConversions;
                    functionMatch.argumentMatches.push_back(ArgumentMatch(OperationFlags::none, conversionFun, OperationFlags::none, 1));
                    continue;
                }
            }
        }
        if (TypesEqual(sourceType, targetType))    // exact match
        {
            functionMatch.argumentMatches.push_back(ArgumentMatch());
        }
        else
        {
            if (arity == 2 && (function->GroupName() == U"@constructor" || function->GroupName() == U"operator="))
            {
                if (i == 0)
                {
                    return false;
                }
            }
            bool qualificationConversionMatch = false;
            ArgumentMatch argumentMatch;
            if (TypesEqual(sourceType->PlainType(span), targetType->PlainType(span)))
            {
                qualificationConversionMatch = FindQualificationConversion(sourceType, targetType, argument, conversionType, span, functionMatch, argumentMatch);
                functionMatch.argumentMatches.push_back(argumentMatch);
            }
            if (!qualificationConversionMatch)
            {
                FunctionSymbol* conversionFun = boundCompileUnit.GetConversion(sourceType, targetType, containerScope, currentFunction, span, argumentMatch);
                if (conversionFun)
                {
                    if (conversionFun->GetConversionType() == conversionType || conversionFun->GetConversionType() == ConversionType::implicit_)
                    {
                        argumentMatch.conversionFun = conversionFun;
                        ++functionMatch.numConversions;
                        if (argumentMatch.preReferenceConversionFlags == OperationFlags::none)
                        {
                            if (FindQualificationConversion(sourceType, targetType, argument, conversionType, span, functionMatch, argumentMatch))
                            {
                                functionMatch.argumentMatches.push_back(argumentMatch);
                                continue;
                            }
                            else
                            {
                                return false;
                            }
                        }
                        else
                        {
                            if (FindQualificationConversion(conversionFun->ConversionSourceType(), targetType, argument, conversionType, span, functionMatch, argumentMatch))
                            {
                                functionMatch.argumentMatches.push_back(argumentMatch);
                                continue;
                            }
                            else
                            {
                                return false;
                            }
                        }
                    }
                    else
                    {
                        if (arity == 2 && i == 1 && conversionType == ConversionType::implicit_ && conversionFun->GetConversionType() == ConversionType::explicit_)
                        {
                            functionMatch.castRequired = true;
                            functionMatch.sourceType = sourceType;
                            functionMatch.targetType = targetType;
                        }
                        return false;
                    }
                }
                else
                {
                    if (function->IsFunctionTemplate())
                    {
                        if (FindTemplateParameterMatch(sourceType, targetType, conversionType, argument, boundCompileUnit, functionMatch, containerScope, currentFunction, span))
                        {
                            continue;
                        }
                        if (FindClassTemplateSpecializationMatch(sourceType, targetType, conversionType, argument, boundCompileUnit, functionMatch, containerScope, currentFunction, span))
                        {
                            continue;
                        }
                    }
                    return false;
                }
            }
        }
    }
    return true;
}

std::string MakeOverloadName(const std::u32string& groupName, const std::vector<std::unique_ptr<BoundExpression>>& arguments, const Span& span)
{
    std::string overloadName = ToUtf8(groupName);
    overloadName.append(1, '(');
    bool first = true;
    for (const std::unique_ptr<BoundExpression>& argument : arguments)
    {
        bool wasFirst = first;
        if (first)
        {
            first = false;
        }
        else
        {
            overloadName.append(", ");
        }
        if (wasFirst && (groupName == U"@constructor" || groupName == U"operator="))
        {
            overloadName.append(ToUtf8(argument->GetType()->RemovePointer(span)->FullName()));
        }
        else
        {
            overloadName.append(ToUtf8(argument->GetType()->FullName()));
        }
    }
    overloadName.append(1, ')');
    return overloadName;
}

std::unique_ptr<BoundFunctionCall> FailWithNoViableFunction(const std::u32string& groupName, const std::vector<std::unique_ptr<BoundExpression>>& arguments, 
    const Span& span, OverloadResolutionFlags flags, std::unique_ptr<Exception>& exception)
{
    std::string overloadName = MakeOverloadName(groupName, arguments, span);
    int arity = arguments.size();
    if (groupName == U"@constructor" && arity == 1 && arguments[0]->GetType()->IsReferenceType())
    {
        if ((flags & OverloadResolutionFlags::dontThrow) != OverloadResolutionFlags::none)
        {
            exception.reset(new NoViableFunctionException("overload resolution failed: '" + overloadName + "' not found. Note: reference must be initialized.", span, arguments[0]->GetSpan()));
            return std::unique_ptr<BoundFunctionCall>();
        }
        else
        {
            throw NoViableFunctionException("overload resolution failed: '" + overloadName + "' not found. Note: reference must be initialized.", span, arguments[0]->GetSpan());
        }
    }
    else
    {
        std::string note;
        if (exception)
        {
            note.append(": Note: ").append(exception->Message());
        }
        if ((flags & OverloadResolutionFlags::dontThrow) != OverloadResolutionFlags::none)
        {
            exception.reset(new NoViableFunctionException("overload resolution failed: '" + overloadName + "' not found. " +
                "No viable functions taking " + std::to_string(arity) + " arguments found in function group '" + ToUtf8(groupName) + "'" + note, span));
            return std::unique_ptr<BoundFunctionCall>();
        }
        else
        {
            throw NoViableFunctionException("overload resolution failed: '" + overloadName + "' not found. " +
                "No viable functions taking " + std::to_string(arity) + " arguments found in function group '" + ToUtf8(groupName) + "'" + note, span);
        }
    }
}

std::unique_ptr<BoundFunctionCall> FailWithOverloadNotFound(const std::unordered_set<FunctionSymbol*>& viableFunctions, const std::u32string& groupName, 
    const std::vector<std::unique_ptr<BoundExpression>>& arguments, const std::vector<FunctionMatch>& failedFunctionMatches, const Span& span, 
    OverloadResolutionFlags flags, std::unique_ptr<Exception>& exception)
{
    std::string overloadName = MakeOverloadName(groupName, arguments, span);
    bool referenceMustBeInitialized = false;
    bool castRequired = false;
    bool cannotBindConstArgToNonConstParam = false;
    bool cannotAssignToConstObject = false;
    TypeSymbol* sourceType = nullptr;
    TypeSymbol* targetType = nullptr;
    std::vector<Span> references;
    std::string note;
    if (exception)
    {
        note.append(" Note: ").append(exception->What());
    }
    if (!failedFunctionMatches.empty())
    {
        int n = failedFunctionMatches.size();
        for (int i = 0; i < n; ++i)
        {
            if (failedFunctionMatches[i].referenceMustBeInitialized)
            {
                referenceMustBeInitialized = true;
                break;
            }
        }
        if (!referenceMustBeInitialized)
        {
            for (int i = 0; i < n; ++i)
            {
                if (failedFunctionMatches[i].castRequired)
                {
                    castRequired = true;
                    sourceType = failedFunctionMatches[i].sourceType;
                    targetType = failedFunctionMatches[i].targetType;
                    references.push_back(failedFunctionMatches[i].fun->GetSpan());
                    break;
                }
            }
        }
        if (!referenceMustBeInitialized && !castRequired)
        {
            for (int i = 0; i < n; ++i)
            {
                if (failedFunctionMatches[i].cannotBindConstArgToNonConstParam)
                {
                    cannotBindConstArgToNonConstParam = true;
                    sourceType = failedFunctionMatches[i].sourceType;
                    targetType = failedFunctionMatches[i].targetType;
                    references.push_back(failedFunctionMatches[i].fun->GetSpan());
                    break;
                }
            }
        }
        if (!referenceMustBeInitialized && !castRequired && !cannotBindConstArgToNonConstParam)
        {
            for (int i = 0; i < n; ++i)
            {
                if (failedFunctionMatches[i].cannotAssignToConstObject)
                {
                    cannotAssignToConstObject = true;
                    references.push_back(failedFunctionMatches[i].fun->GetSpan());
                    break;
                }
            }
        }
        if (!referenceMustBeInitialized && !castRequired && !cannotBindConstArgToNonConstParam && !cannotAssignToConstObject)
        {
            for (int i = 0; i < n; ++i)
            {
                if (failedFunctionMatches[i].conceptCheckException)
                {
                    if (!note.empty())
                    {
                        note.append(".");
                    }
                    note.append(" Note: concept check failed: " + failedFunctionMatches[i].conceptCheckException->Message());
                    references.insert(references.end(), failedFunctionMatches[i].conceptCheckException->References().begin(), failedFunctionMatches[i].conceptCheckException->References().end());
                    break;
                }
            }
        }
    }
    if (referenceMustBeInitialized || groupName == U"@constructor" && arguments.size() == 1 && arguments[0]->GetType()->IsReferenceType())
    {
        if ((flags & OverloadResolutionFlags::dontThrow) != OverloadResolutionFlags::none)
        {
            references.push_back(arguments[0]->GetSpan());
            exception.reset(new Exception("overload resolution failed: '" + overloadName + "' not found. Note: reference must be initialized.", span, references));
            return std::unique_ptr<BoundFunctionCall>();
        }
        else
        {
            references.push_back(arguments[0]->GetSpan());
            throw Exception("overload resolution failed: '" + overloadName + "' not found. Note: reference must be initialized.", span, references);
        }
    }
    else if (castRequired)
    {
        Assert(sourceType, "source type not set");
        Assert(targetType, "target type not set");
        if ((flags & OverloadResolutionFlags::dontThrow) != OverloadResolutionFlags::none)
        {
            exception.reset(new CastOverloadException("overload resolution failed: '" + overloadName + "' not found, or there are no acceptable conversions for all argument types. " +
                std::to_string(viableFunctions.size()) + " viable functions examined. Note: cannot convert implicitly from '" +
                ToUtf8(sourceType->FullName()) + "' to '" + ToUtf8(targetType->FullName()) + "' but explicit conversion (cast) exists.", span, references));
            return std::unique_ptr<BoundFunctionCall>();
        }
        else
        {
            throw CastOverloadException("overload resolution failed: '" + overloadName + "' not found, or there are no acceptable conversions for all argument types. " +
                std::to_string(viableFunctions.size()) + " viable functions examined. Note: cannot convert implicitly from '" +
                ToUtf8(sourceType->FullName()) + "' to '" + ToUtf8(targetType->FullName()) + "' but explicit conversion (cast) exists.", span, references);
        }
    }
    else if (cannotBindConstArgToNonConstParam)
    {
        Assert(sourceType, "source type not set");
        Assert(targetType, "target type not set");
        if ((flags & OverloadResolutionFlags::dontThrow) != OverloadResolutionFlags::none)
        {
            exception.reset(new CannotBindConstToNonconstOverloadException("overload resolution failed: '" + overloadName + 
                "' not found, or there are no acceptable conversions for all argument types. " +
                std::to_string(viableFunctions.size()) + " viable functions examined. Note: cannot bind constant '" + ToUtf8(sourceType->FullName()) + "' argument " 
                " to nonconstant '" + ToUtf8(targetType->FullName()) +"' parameter", span, references));
            return std::unique_ptr<BoundFunctionCall>();
        }
        else
        {
            throw CannotBindConstToNonconstOverloadException("overload resolution failed: '" + overloadName + "' not found, or there are no acceptable conversions for all argument types. " +
                std::to_string(viableFunctions.size()) + " viable functions examined. Note: cannot bind constant '" + ToUtf8(sourceType->FullName()) + "' argument "
                " to nonconstant '" + ToUtf8(targetType->FullName()) + "' parameter", span, references);
        }
    }
    else if (cannotAssignToConstObject)
    {
        if ((flags & OverloadResolutionFlags::dontThrow) != OverloadResolutionFlags::none)
        {
            exception.reset(new CannotAssignToConstOverloadException("overload resolution failed: '" + overloadName +
                "' not found, or there are no acceptable conversions for all argument types. " +
                std::to_string(viableFunctions.size()) + " viable functions examined. Note: cannot assign to const object.", span, references));
            return std::unique_ptr<BoundFunctionCall>();
        }
        else
        {
            throw CannotAssignToConstOverloadException("overload resolution failed: '" + overloadName + "' not found, or there are no acceptable conversions for all argument types. " +
                std::to_string(viableFunctions.size()) + " viable functions examined. Note: cannot assign to const object. ", span, references);
        }
    }
    else
    {
        if ((flags & OverloadResolutionFlags::dontThrow) != OverloadResolutionFlags::none)
        {
            exception.reset(new Exception("overload resolution failed: '" + overloadName + "' not found, or there are no acceptable conversions for all argument types. " +
                std::to_string(viableFunctions.size()) + " viable functions examined." + note, span, references));
            return std::unique_ptr<BoundFunctionCall>();
        }
        else
        {
            throw Exception("overload resolution failed: '" + overloadName + "' not found, or there are no acceptable conversions for all argument types. " +
                std::to_string(viableFunctions.size()) + " viable functions examined." + note, span, references);
        }
    }
}

std::unique_ptr<BoundFunctionCall> FailWithAmbiguousOverload(const std::u32string& groupName, std::vector<std::unique_ptr<BoundExpression>>& arguments, 
    const std::vector<FunctionMatch>& functionMatches, const Span& span, OverloadResolutionFlags flags, std::unique_ptr<Exception>& exception)
{
    std::string overloadName = MakeOverloadName(groupName, arguments, span);
    std::string matchedFunctionNames;
    bool first = true;
    FunctionMatch equalMatch = std::move(functionMatches[0]);
    std::vector<FunctionMatch> equalMatches;
    equalMatches.push_back(std::move(equalMatch));
    int n = int(functionMatches.size());
    for (int i = 1; i < n; ++i)
    {
        FunctionMatch match = std::move(functionMatches[i]);
        if (!BetterFunctionMatch()(equalMatches[0], match))
        {
            equalMatches.push_back(std::move(match));
        }
    }
    std::vector<Span> references;
    for (const FunctionMatch& match : equalMatches)
    {
        if (first)
        {
            first = false;
        }
        else
        {
            matchedFunctionNames.append(", or ");
        }
        matchedFunctionNames.append(ToUtf8(match.fun->FullName()));
        references.push_back(match.fun->GetSpan());
    }
    if ((flags & OverloadResolutionFlags::dontThrow) != OverloadResolutionFlags::none)
    {
        exception.reset(new Exception("overload resolution for overload name '" + overloadName + "' failed: call is ambiguous: \n" + matchedFunctionNames, span, references));
        return std::unique_ptr<BoundFunctionCall>();
    }
    else
    {
        throw Exception("overload resolution for overload name '" + overloadName + "' failed: call is ambiguous: \n" + matchedFunctionNames, span, references);
    }
}

std::unique_ptr<BoundFunctionCall> CreateBoundFunctionCall(FunctionSymbol* bestFun, std::vector<std::unique_ptr<BoundExpression>>& arguments, BoundCompileUnit& boundCompileUnit, 
    BoundFunction* boundFunction, const FunctionMatch& bestMatch, ContainerScope* containerScope, const Span& span)
{
    std::unique_ptr<BoundFunctionCall> boundFunctionCall(new BoundFunctionCall(span, bestFun));
    int arity = arguments.size();
    for (int i = 0; i < arity; ++i)
    {
        std::unique_ptr<BoundExpression>& argument = arguments[i];
        if (i == 0 && !bestFun->IsConstructorDestructorOrNonstaticMemberFunction() && 
            (bestFun->GroupName() == U"@constructor" || bestFun->GroupName() == U"operator=" || bestFun->GroupName() == U"operator->") && 
            argument->GetBoundNodeType() == BoundNodeType::boundAddressOfExpression)
        {
            BoundAddressOfExpression* addrOf = static_cast<BoundAddressOfExpression*>(argument.get());
            std::unique_ptr<BoundExpression> subject(std::move(addrOf->Subject()));
            argument.reset(subject.release());
        }
        const ArgumentMatch& argumentMatch = bestMatch.argumentMatches[i];
        if (argumentMatch.preReferenceConversionFlags != OperationFlags::none)
        {
            if (argumentMatch.preReferenceConversionFlags == OperationFlags::addr)
            {
                if (!argument->IsLvalueExpression())
                {
                    BoundLocalVariable* backingStore = new BoundLocalVariable(boundFunction->GetFunctionSymbol()->CreateTemporary(argument->GetType(), span));
                    argument.reset(new BoundTemporary(std::move(argument), std::unique_ptr<BoundLocalVariable>(backingStore)));
                }
                TypeSymbol* type = nullptr;
                if (argument->GetType()->IsClassTypeSymbol() && argument->GetFlag(BoundExpressionFlags::bindToRvalueReference))
                {
                    type = argument->GetType()->AddRvalueReference(span);
                }
                else
                {
                    type = argument->GetType()->AddLvalueReference(span);
                }
                BoundAddressOfExpression* addressOfExpression = new BoundAddressOfExpression(std::move(argument), type);
                argument.reset(addressOfExpression);
            }
            else if (argumentMatch.preReferenceConversionFlags == OperationFlags::deref)
            {
                TypeSymbol* type = argument->GetType()->RemoveReference(span);
                BoundDereferenceExpression* dereferenceExpression = new BoundDereferenceExpression(std::move(argument), type);
                argument.reset(dereferenceExpression);
            }
        }
        if (argumentMatch.conversionFun)
        {
            FunctionSymbol* conversionFun = argumentMatch.conversionFun;
            if (conversionFun->GetSymbolType() == SymbolType::constructorSymbol)
            {
                BoundFunctionCall* constructorCall = new BoundFunctionCall(span, conversionFun);
                TypeSymbol* conversionTargetType = conversionFun->ConversionTargetType();
                LocalVariableSymbol* temporary = boundFunction->GetFunctionSymbol()->CreateTemporary(conversionTargetType, span);
                constructorCall->AddArgument(std::unique_ptr<BoundExpression>(new BoundAddressOfExpression(std::unique_ptr<BoundExpression>(new BoundLocalVariable(temporary)),
                    conversionTargetType->AddPointer(span))));
                if (conversionTargetType->IsClassTypeSymbol())
                {
                    ClassTypeSymbol* classType = static_cast<ClassTypeSymbol*>(conversionTargetType);
                    if (classType->Destructor())
                    {
                        std::unique_ptr<BoundFunctionCall> destructorCall(new BoundFunctionCall(span, classType->Destructor()));
                        destructorCall->AddArgument(std::unique_ptr<BoundExpression>(constructorCall->Arguments()[0]->Clone()));
                        boundFunction->AddTemporaryDestructorCall(std::move(destructorCall));
                    }
                }
                constructorCall->AddArgument(std::move(argument));
                BoundConstructAndReturnTemporaryExpression* conversion = new BoundConstructAndReturnTemporaryExpression(std::unique_ptr<BoundExpression>(constructorCall),
                    std::unique_ptr<BoundExpression>(new BoundLocalVariable(temporary)));
                argument.reset(conversion);
            }
            else if (conversionFun->GetSymbolType() == SymbolType::conversionFunctionSymbol && conversionFun->ReturnsClassOrClassDelegateByValue())
            {
                BoundFunctionCall* conversionFunctionCall = new BoundFunctionCall(span, conversionFun);
                conversionFunctionCall->AddArgument(std::move(argument));
                TypeSymbol* conversionTargetType = conversionFun->ConversionTargetType();
                LocalVariableSymbol* temporary = boundFunction->GetFunctionSymbol()->CreateTemporary(conversionTargetType, span);
                conversionFunctionCall->AddArgument(std::unique_ptr<BoundExpression>(new BoundAddressOfExpression(std::unique_ptr<BoundExpression>(new BoundLocalVariable(temporary)),
                    conversionTargetType->AddPointer(span))));
                BoundLocalVariable* conversionResult = new BoundLocalVariable(temporary);
                if (conversionTargetType->IsClassTypeSymbol())
                {
                    ClassTypeSymbol* classType = static_cast<ClassTypeSymbol*>(conversionTargetType);
                    if (classType->Destructor())
                    {
                        std::unique_ptr<BoundFunctionCall> destructorCall(new BoundFunctionCall(span, classType->Destructor()));
                        destructorCall->AddArgument(std::unique_ptr<BoundExpression>(conversionResult->Clone()));
                        boundFunction->AddTemporaryDestructorCall(std::move(destructorCall));
                    }
                }
                BoundClassOrClassDelegateConversionResult* conversion = new BoundClassOrClassDelegateConversionResult(std::unique_ptr<BoundExpression>(conversionResult),
                    std::unique_ptr<BoundFunctionCall>(conversionFunctionCall));
                argument.reset(conversion);
            }
            else
            {
                BoundConversion* conversion = new BoundConversion(std::move(argument), conversionFun);
                argument.reset(conversion);
            }
        }
        if (argumentMatch.postReferenceConversionFlags != OperationFlags::none)
        {
            if (argumentMatch.postReferenceConversionFlags == OperationFlags::addr)
            {
                if (!argument->IsLvalueExpression())
                {
                    BoundLocalVariable* backingStore = new BoundLocalVariable(boundFunction->GetFunctionSymbol()->CreateTemporary(argument->GetType(), span));
                    argument.reset(new BoundTemporary(std::move(argument), std::unique_ptr<BoundLocalVariable>(backingStore)));
                }
                TypeSymbol* type = nullptr;
                if (argument->GetType()->IsClassTypeSymbol() && argument->GetFlag(BoundExpressionFlags::bindToRvalueReference))
                {
                    type = argument->GetType()->AddRvalueReference(span);
                }
                else
                {
                    type = argument->GetType()->AddLvalueReference(span);
                }
                BoundAddressOfExpression* addressOfExpression = new BoundAddressOfExpression(std::move(argument), type);
                argument.reset(addressOfExpression);
            }
            else if (argumentMatch.postReferenceConversionFlags == OperationFlags::deref)
            {
                TypeSymbol* type = argument->GetType()->RemoveReference(span);
                BoundDereferenceExpression* dereferenceExpression = new BoundDereferenceExpression(std::move(argument), type);
                argument.reset(dereferenceExpression);
            }
        }
        if (argument->GetType()->IsClassTypeSymbol() || argument->GetType()->GetSymbolType() == SymbolType::classDelegateTypeSymbol)
        {
            if (argument->GetType()->IsClassTypeSymbol())
            {
                ClassTypeSymbol* classType = static_cast<ClassTypeSymbol*>(argument->GetType());
                if (!classType->CopyConstructor())
                {
                    try
                    {
                        boundCompileUnit.GenerateCopyConstructorFor(classType, containerScope, boundFunction, span);
                    }
                    catch (const Exception& ex)
                    {
                        std::vector<Span> references;
                        references.push_back(ex.Defined());
                        references.insert(references.end(), ex.References().begin(), ex.References().end());
                        throw Exception("cannot pass class '" + ToUtf8(classType->FullName()) + "' by value because: " + ex.Message(), argument->GetSpan(), references);
                    }
                }
                TypeSymbol* type = classType->AddConst(span)->AddLvalueReference(span);
                argument.reset(new BoundAddressOfExpression(std::move(argument), type));
            }
            else if (argument->GetType()->GetSymbolType() == SymbolType::classDelegateTypeSymbol)
            {
                TypeSymbol* type = argument->GetType()->AddConst(span)->AddLvalueReference(span);
                argument.reset(new BoundAddressOfExpression(std::move(argument), type));
            }
        }
        boundFunctionCall->AddArgument(std::move(argument));
    }
    return boundFunctionCall;
}

std::unique_ptr<BoundFunctionCall> SelectViableFunction(const std::unordered_set<FunctionSymbol*>& viableFunctions, const std::u32string& groupName, 
    std::vector<std::unique_ptr<BoundExpression>>& arguments, ContainerScope* containerScope, BoundCompileUnit& boundCompileUnit, BoundFunction* boundFunction, const Span& span,
    OverloadResolutionFlags flags, const std::vector<TypeSymbol*>& templateArgumentTypes, std::unique_ptr<Exception>& exception)
{
    std::vector<FunctionMatch> functionMatches;
    std::vector<FunctionMatch> failedFunctionMatches;
    std::vector<std::unique_ptr<Exception>> conceptCheckExceptions;
    std::vector<std::unique_ptr<BoundConstraint>> boundConstraints;
    for (FunctionSymbol* viableFunction : viableFunctions)
    {
        FunctionMatch functionMatch(viableFunction);
        if (viableFunction->IsFunctionTemplate())
        {
            int n = templateArgumentTypes.size();
            if (n > viableFunction->TemplateParameters().size())
            {
                continue;
            }
            else
            {
                for (int i = 0; i < n; ++i)
                {
                    TemplateParameterSymbol* templateParameter = viableFunction->TemplateParameters()[i];
                    functionMatch.templateParameterMap[templateParameter] = templateArgumentTypes[i];
                }
            }
        }
        else
        {
            if (!templateArgumentTypes.empty())
            {
                continue;
            }
        }
        if (FindConversions(boundCompileUnit, viableFunction, arguments, functionMatch, ConversionType::implicit_, containerScope, boundFunction, span))
        {
            if (viableFunction->IsFunctionTemplate())
            {
                bool allTemplateParametersFound = true;
                int n = viableFunction->TemplateParameters().size();
                for (int i = 0; i < n; ++i)
                {
                    TemplateParameterSymbol* templateParameterSymbol = viableFunction->TemplateParameters()[i];
                    auto it = functionMatch.templateParameterMap.find(templateParameterSymbol);
                    if (it == functionMatch.templateParameterMap.cend())
                    {
                        allTemplateParametersFound = false;
                        break;
                    }
                }
                if (allTemplateParametersFound)
                {
                    if (!viableFunction->Constraint())
                    {
                        Node* node = boundCompileUnit.GetSymbolTable().GetNodeNoThrow(viableFunction);
                        if (node)
                        {
                            Assert(node->GetNodeType() == NodeType::functionNode, "function node expected");
                            FunctionNode* functionNode = static_cast<FunctionNode*>(node);
                            ConstraintNode* constraint = functionNode->WhereConstraint();
                            if (constraint)
                            {
                                CloneContext cloneContext;
                                viableFunction->SetConstraint(static_cast<ConstraintNode*>(constraint->Clone(cloneContext)));
                            }
                        }
                    }
                    if (viableFunction->Constraint())
                    {
                        std::unique_ptr<Exception> conceptCheckException;
                        std::unique_ptr<BoundConstraint> boundConstraint;
                        Node* node = boundCompileUnit.GetSymbolTable().GetNodeNoThrow(viableFunction);
                        if (!node)
                        {
                            if (!viableFunction->GetFunctionNode())
                            {
                                viableFunction->ReadAstNodes();
                            }
                        }
                        bool candidateFound = CheckConstraint(viableFunction->Constraint(), viableFunction->UsingNodes(), boundCompileUnit, containerScope, boundFunction,
                            viableFunction->TemplateParameters(), functionMatch.templateParameterMap, boundConstraint, span, viableFunction, conceptCheckException);
                        if (candidateFound)
                        {
                            functionMatch.boundConstraint = boundConstraint.get();
                            functionMatches.push_back(functionMatch);
                            boundConstraints.push_back(std::move(boundConstraint));
                        }
                        else
                        {
                            functionMatch.conceptCheckException = conceptCheckException.get();
                            failedFunctionMatches.push_back(functionMatch);
                            conceptCheckExceptions.push_back(std::move(conceptCheckException));
                        }
                    }
                    else
                    {
                        functionMatches.push_back(functionMatch);
                    }
                }
                else
                {
                    failedFunctionMatches.push_back(functionMatch);
                }
            }
            else
            {
                functionMatches.push_back(functionMatch);
            }
        }
        else
        {
            failedFunctionMatches.push_back(functionMatch);
        }
    }
    if (functionMatches.empty())
    {
        return FailWithOverloadNotFound(viableFunctions, groupName, arguments, failedFunctionMatches, span, flags, exception);
    }
    else if (functionMatches.size() > 1)
    {
        std::sort(functionMatches.begin(), functionMatches.end(), BetterFunctionMatch());
        if (BetterFunctionMatch()(functionMatches[0], functionMatches[1]))
        {
            const FunctionMatch& bestMatch = functionMatches[0];
            FunctionSymbol* bestFun = bestMatch.fun;
            if (bestFun->IsSuppressed())
            {
                if ((flags & OverloadResolutionFlags::dontThrow) != OverloadResolutionFlags::none)
                {
                    exception.reset(new Exception("cannot call a suppressed member function '" + ToUtf8(bestFun->FullName()) + "'", span, bestFun->GetSpan()));
                    return std::unique_ptr<BoundFunctionCall>();
                }
                else
                {
                    throw Exception("cannot call a suppressed member function '" + ToUtf8(bestFun->FullName()) + "'", span, bestFun->GetSpan());
                }
            }
            bool instantiate = (flags & OverloadResolutionFlags::dontInstantiate) == OverloadResolutionFlags::none;
            if (bestFun->IsFunctionTemplate())
            {
                if (instantiate)
                {
                    bestFun = boundCompileUnit.InstantiateFunctionTemplate(bestFun, bestMatch.templateParameterMap, span);
                }
            }
            else if (!bestFun->IsGeneratedFunction() && bestFun->Parent()->GetSymbolType() == SymbolType::classTemplateSpecializationSymbol)
            {
                if (instantiate)
                {
                    boundCompileUnit.InstantiateClassTemplateMemberFunction(bestFun, containerScope, boundFunction, span);
                }
            }
            else if (!bestFun->IsGeneratedFunction() && GetGlobalFlag(GlobalFlags::release) && bestFun->IsInline())
            {
                if (instantiate)
                {
                    boundCompileUnit.InstantiateInlineFunction(bestFun, containerScope, span);
                }
            }
            if (boundFunction->GetFunctionSymbol()->DontThrow() && !boundFunction->GetFunctionSymbol()->HasTry() && !bestFun->DontThrow())
            {
                std::vector<Span> references;
                references.push_back(boundFunction->GetFunctionSymbol()->GetSpan());
                references.push_back(bestFun->GetSpan());
                if (GetGlobalFlag(GlobalFlags::strictNothrow))
                {
                    if ((flags & OverloadResolutionFlags::dontThrow) != OverloadResolutionFlags::none)
                    {
                        exception.reset(new Exception("a nothrow function cannot call a function that can throw unless it handles exceptions (compiled with --strict-nothrow)", span, references));
                        return std::unique_ptr<BoundFunctionCall>();
                    }
                    else
                    {
                        throw Exception("a nothrow function cannot call a function that can throw unless it handles exceptions (compiled with --strict-nothrow)", span, references);
                    }
                }
                else
                {
                    Warning warning(CompileWarningCollection::Instance().GetCurrentProjectName(), "a nothrow function calls a function that can throw and does not handle exceptions");
                    warning.SetDefined(span);
                    warning.SetReferences(references);
                    CompileWarningCollection::Instance().AddWarning(warning);
                }
            }
            return CreateBoundFunctionCall(bestFun, arguments, boundCompileUnit, boundFunction, bestMatch, containerScope, span);
        }
        else
        {
            return FailWithAmbiguousOverload(groupName, arguments, functionMatches, span, flags, exception);
        }
    }
    else
    {
        const FunctionMatch& bestMatch = functionMatches[0];
        FunctionSymbol* singleBest = bestMatch.fun;
        if (singleBest->IsSuppressed())
        {
            if ((flags & OverloadResolutionFlags::dontThrow) != OverloadResolutionFlags::none)
            {
                exception.reset(new Exception("cannot call a suppressed member function '" + ToUtf8(singleBest->FullName()) + "'", span, singleBest->GetSpan()));
                return std::unique_ptr<BoundFunctionCall>();
            }
            else
            {
                throw Exception("cannot call a suppressed member function '" + ToUtf8(singleBest->FullName()) + "'", span, singleBest->GetSpan());
            }
        }
        bool instantiate = (flags & OverloadResolutionFlags::dontInstantiate) == OverloadResolutionFlags::none;
        if (singleBest->IsFunctionTemplate())
        {
            if (instantiate)
            {
                singleBest = boundCompileUnit.InstantiateFunctionTemplate(singleBest, bestMatch.templateParameterMap, span);
            }
        }
        else if (!singleBest->IsGeneratedFunction() && singleBest->Parent()->GetSymbolType() == SymbolType::classTemplateSpecializationSymbol)
        {
            if (instantiate)
            {
                boundCompileUnit.InstantiateClassTemplateMemberFunction(singleBest, containerScope, boundFunction, span);
            }
        }
        else if (!singleBest->IsGeneratedFunction() && GetGlobalFlag(GlobalFlags::release) && singleBest->IsInline())
        {
            if (instantiate)
            {
                boundCompileUnit.InstantiateInlineFunction(singleBest, containerScope, span);
            }
        }
        if (boundFunction->GetFunctionSymbol()->DontThrow() && !boundFunction->GetFunctionSymbol()->HasTry() && !singleBest->DontThrow())
        {
            std::vector<Span> references;
            references.push_back(boundFunction->GetFunctionSymbol()->GetSpan());
            references.push_back(singleBest->GetSpan());
            if (GetGlobalFlag(GlobalFlags::strictNothrow))
            {
                if ((flags & OverloadResolutionFlags::dontThrow) != OverloadResolutionFlags::none)
                {
                    exception.reset(new Exception("a nothrow function cannot call a function that can throw unless it handles exceptions (compiled with --strict-nothrow)", span, references));
                    return std::unique_ptr<BoundFunctionCall>();
                }
                else
                {
                    throw Exception("a nothrow function cannot call a function that can throw unless it handles exceptions (compiled with --strict-nothrow)", span, references);
                }
            }
            else
            {
                Warning warning(CompileWarningCollection::Instance().GetCurrentProjectName(), "a nothrow function calls a function that can throw and does not handle exceptions");
                warning.SetDefined(span);
                warning.SetReferences(references);
                CompileWarningCollection::Instance().AddWarning(warning);
            }
        }
        return CreateBoundFunctionCall(singleBest, arguments, boundCompileUnit, boundFunction, bestMatch, containerScope, span);
    }
}

void CollectViableFunctionsFromSymbolTable(int arity, const std::u32string& groupName, const std::vector<FunctionScopeLookup>& functionScopeLookups, BoundCompileUnit& boundCompileUnit,
    std::unordered_set<FunctionSymbol*>& viableFunctions)
{
    std::unordered_set<ContainerScope*> scopesLookedUp;
    bool fileScopesLookedUp = false;
    for (const FunctionScopeLookup& functionScopeLookup : functionScopeLookups)
    {
        if (functionScopeLookup.scopeLookup == ScopeLookup::fileScopes && !fileScopesLookedUp)
        {
            fileScopesLookedUp = true;
            for (const std::unique_ptr<FileScope>& fileScope : boundCompileUnit.FileScopes())
            {
                fileScope->CollectViableFunctions(arity, groupName, scopesLookedUp, viableFunctions);
            }
        }
        else
        {
            functionScopeLookup.scope->CollectViableFunctions(arity, groupName, scopesLookedUp, functionScopeLookup.scopeLookup, viableFunctions);
        }
    }
}

std::unique_ptr<BoundFunctionCall> ResolveOverload(const std::u32string& groupName, ContainerScope* containerScope, const std::vector<FunctionScopeLookup>& functionScopeLookups,
    std::vector<std::unique_ptr<BoundExpression>>& arguments, BoundCompileUnit& boundCompileUnit, BoundFunction* currentFunction, const Span& span)
{
    std::unique_ptr<Exception> exception;
    std::vector<TypeSymbol*> templateArgumentTypes;
    return ResolveOverload(groupName, containerScope, functionScopeLookups, arguments, boundCompileUnit, currentFunction, span, OverloadResolutionFlags::none, templateArgumentTypes, exception);
}

std::unique_ptr<BoundFunctionCall> ResolveOverload(const std::u32string& groupName, ContainerScope* containerScope, const std::vector<FunctionScopeLookup>& functionScopeLookups, 
    std::vector<std::unique_ptr<BoundExpression>>& arguments, BoundCompileUnit& boundCompileUnit, BoundFunction* currentFunction, const Span& span, 
    OverloadResolutionFlags flags, const std::vector<TypeSymbol*>& templateArgumentTypes, std::unique_ptr<Exception>& exception)
{
    int arity = arguments.size();
    std::unordered_set<FunctionSymbol*> viableFunctions;
    boundCompileUnit.CollectViableFunctions(groupName, containerScope, arguments, currentFunction, viableFunctions, exception, span);
    if (viableFunctions.empty())
    {
        if ((flags & OverloadResolutionFlags::dontThrow) == OverloadResolutionFlags::none && exception)
        {
            throw *exception;
        }
        CollectViableFunctionsFromSymbolTable(arity, groupName, functionScopeLookups, boundCompileUnit, viableFunctions);
    }
    if (viableFunctions.empty())
    {
        return FailWithNoViableFunction(groupName, arguments, span, flags, exception);
    }
    else
    {
        return SelectViableFunction(viableFunctions, groupName, arguments, containerScope, boundCompileUnit, currentFunction, span, flags, templateArgumentTypes, exception);
    }
}

} } // namespace cmajor::binder
