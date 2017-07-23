// =================================
// Copyright (c) 2017 Seppo Laakko
// Distributed under the MIT license
// =================================

#include <cmajor/binder/OverloadResolution.hpp>
#include <cmajor/binder/BoundCompileUnit.hpp>
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
    return false;
}

bool FindConversions(BoundCompileUnit& boundCompileUnit, FunctionSymbol* function, const std::vector<std::unique_ptr<BoundExpression>>& arguments, FunctionMatch& functionMatch, ConversionType conversionType)
{
    int arity = arguments.size();
    Assert(arity == function->Arity(), "wrong arity");
    for (int i = 0; i < arity; ++i)
    {
        BoundExpression* argument = arguments[i].get();
        TypeSymbol* sourceType = argument->GetType();
        ParameterSymbol* parameter = function->Parameters()[i];
        TypeSymbol* targetType = parameter->GetType();
        if (TypesEqual(sourceType, targetType))
        {
            functionMatch.argumentMatches.push_back(ArgumentMatch());
        }
        else
        {
            if (arity == 2 && i == 0 && (function->GroupName() == U"@constructor" || function->GroupName() == U"operator="))
            {
                return false;
            }
            FunctionSymbol* conversionFun = boundCompileUnit.GetConversion(sourceType, targetType);
            if (conversionFun)
            {
                if (conversionFun->GetConversionType() == conversionType || conversionFun->GetConversionType() == ConversionType::implicit_)
                {
                    functionMatch.argumentMatches.push_back(ArgumentMatch(conversionFun, conversionFun->ConversionDistance()));
                    ++functionMatch.numConversions;
                }
                else
                {
                    if (arity == 2 && i == 1 && conversionType == ConversionType::implicit_ && conversionFun->GetConversionType() == ConversionType::explicit_)
                    {
                        functionMatch.castRequired = true;
                        functionMatch.castSourceType = sourceType;
                        functionMatch.castTargetType = targetType;
                    }
                    return false;
                }
            }
            else
            {
                return false;
            }
        }
    }
    return true;
}

std::string MakeOverloadName(const std::u32string& groupName, const std::vector<std::unique_ptr<BoundExpression>>& arguments)
{
    std::string overloadName = ToUtf8(groupName);
    overloadName.append(1, '(');
    bool first = true;
    for (const std::unique_ptr<BoundExpression>& argument : arguments)
    {
        if (first)
        {
            first = false;
        }
        else
        {
            overloadName.append(", ");
        }
        overloadName.append(ToUtf8(argument->GetType()->FullName()));
    }
    overloadName.append(1, ')');
    return overloadName;
}

std::unique_ptr<BoundFunctionCall> FailWithNoViableFunction(const std::u32string& groupName, const std::vector<std::unique_ptr<BoundExpression>>& arguments, const Span& span, OverloadResolutionFlags flags, 
    std::unique_ptr<Exception>& exception)
{
    std::string overloadName = MakeOverloadName(groupName, arguments);
    int arity = arguments.size();
    if ((flags & OverloadResolutionFlags::dontThrow) != OverloadResolutionFlags::none)
    {
        exception.reset(new Exception("overload resolution failed: '" + overloadName + "' not found. " +
            "No viable functions taking " + std::to_string(arity) + " arguments found in function group '" + ToUtf8(groupName) + "'", span));
        return std::unique_ptr<BoundFunctionCall>();
    }
    else
    {
        throw Exception("overload resolution failed: '" + overloadName + "' not found. " +
            "No viable functions taking " + std::to_string(arity) + " arguments found in function group '" + ToUtf8(groupName) + "'", span);
    }
}

std::unique_ptr<BoundFunctionCall> FailWithOverloadNotFound(const std::unordered_set<FunctionSymbol*> viableFunctions, const std::u32string& groupName, const std::vector<std::unique_ptr<BoundExpression>>& arguments,
    const std::vector<FunctionMatch>& failedFunctionMatches, const Span& span, OverloadResolutionFlags flags, std::unique_ptr<Exception>& exception)
{
    std::string overloadName = MakeOverloadName(groupName, arguments);
    bool castRequired = false;
    bool refRequired = false;
    TypeSymbol* castSourceType = nullptr;
    TypeSymbol* castTargetType = nullptr;
    if (!failedFunctionMatches.empty())
    {
        int n = int(failedFunctionMatches.size());
        for (int i = 0; i < n; ++i)
        {
            if (failedFunctionMatches[i].castRequired)
            {
                castRequired = true;
                castSourceType = failedFunctionMatches[i].castSourceType;
                castTargetType = failedFunctionMatches[i].castTargetType;
                break;
            }
        }
    }
    if (castRequired)
    {
        Assert(castSourceType, "cast source type not set");
        Assert(castTargetType, "cast target type not set");
        if ((flags & OverloadResolutionFlags::dontThrow) != OverloadResolutionFlags::none)
        {
            exception.reset(new CastOverloadException("overload resolution failed: '" + overloadName + "' not found, or there are no acceptable conversions for all argument types. " +
                std::to_string(viableFunctions.size()) + " viable functions examined. Note: cannot convert implicitly from '" +
                ToUtf8(castSourceType->FullName()) + "' to '" + ToUtf8(castTargetType->FullName()) + "' but explicit conversion (cast) exists.", span));
            return std::unique_ptr<BoundFunctionCall>();
        }
        else
        {
            throw CastOverloadException("overload resolution failed: '" + overloadName + "' not found, or there are no acceptable conversions for all argument types. " +
                std::to_string(viableFunctions.size()) + " viable functions examined. Note: cannot convert implicitly from '" +
                ToUtf8(castSourceType->FullName()) + "' to '" + ToUtf8(castTargetType->FullName()) + "' but explicit conversion (cast) exists.", span);
        }
    }
    else
    {
        if ((flags & OverloadResolutionFlags::dontThrow) != OverloadResolutionFlags::none)
        {
            exception.reset(new Exception("overload resolution failed: '" + overloadName + "' not found, or there are no acceptable conversions for all argument types. " +
                std::to_string(viableFunctions.size()) + " viable functions examined.", span));
            return std::unique_ptr<BoundFunctionCall>();
        }
        else
        {
            throw Exception("overload resolution failed: '" + overloadName + "' not found, or there are no acceptable conversions for all argument types. " +
                std::to_string(viableFunctions.size()) + " viable functions examined.", span);
        }
    }
}

std::unique_ptr<BoundFunctionCall> FailWithAmbiguousOverload(const std::u32string& groupName, std::vector<std::unique_ptr<BoundExpression>>& arguments, const std::vector<FunctionMatch>& functionMatches, 
    const Span& span, OverloadResolutionFlags flags, std::unique_ptr<Exception>& exception)
{
    std::string overloadName = MakeOverloadName(groupName, arguments);
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

std::unique_ptr<BoundFunctionCall> CreateBoundFunctionCall(FunctionSymbol* bestFun, std::vector<std::unique_ptr<BoundExpression>>& arguments, BoundCompileUnit& boundCompileUnit, const FunctionMatch& bestMatch,
    const Span& span)
{
    std::unique_ptr<BoundFunctionCall> boundFunctionCall(new BoundFunctionCall(span, bestFun));
    int arity = arguments.size();
    for (int i = 0; i < arity; ++i)
    {
        std::unique_ptr<BoundExpression>& argument = arguments[i];
        const ArgumentMatch& argumentMatch = bestMatch.argumentMatches[i];
        if (argumentMatch.conversionFun)
        {
            BoundConversion* conversion = new BoundConversion(std::move(argument), argumentMatch.conversionFun);
            argument.reset(conversion);
        }
        boundFunctionCall->AddArgument(std::move(argument));
    }
    return boundFunctionCall;
}

std::unique_ptr<BoundFunctionCall> SelectViableFunction(const std::unordered_set<FunctionSymbol*> viableFunctions, const std::u32string& groupName, std::vector<std::unique_ptr<BoundExpression>>& arguments,
    BoundCompileUnit& boundCompileUnit, const Span& span, OverloadResolutionFlags flags, std::unique_ptr<Exception>& exception)
{
    std::vector<FunctionMatch> functionMatches;
    std::vector<FunctionMatch> failedFunctionMatches;
    for (FunctionSymbol* viableFunction : viableFunctions)
    {
        FunctionMatch functionMatch(viableFunction);
        if (FindConversions(boundCompileUnit, viableFunction, arguments, functionMatch, ConversionType::implicit_))
        {
            functionMatches.push_back(functionMatch);
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
            return CreateBoundFunctionCall(bestFun, arguments, boundCompileUnit, bestMatch, span);
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
        return CreateBoundFunctionCall(singleBest, arguments, boundCompileUnit, bestMatch, span);
    }
}

void CollectViableFunctions(int arity, const std::u32string& groupName, const std::vector<FunctionScopeLookup>& functionScopeLookups, BoundCompileUnit& boundCompileUnit, 
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

std::unique_ptr<BoundFunctionCall> ResolveOverload(const std::u32string& groupName, const std::vector<FunctionScopeLookup>& functionScopeLookups, std::vector<std::unique_ptr<BoundExpression>>& arguments,
    BoundCompileUnit& boundCompileUnit, const Span& span)
{
    std::unique_ptr<Exception> exception;
    return ResolveOverload(groupName, functionScopeLookups, arguments, boundCompileUnit, span, OverloadResolutionFlags::none, exception);
}

std::unique_ptr<BoundFunctionCall> ResolveOverload(const std::u32string& groupName, const std::vector<FunctionScopeLookup>& functionScopeLookups, std::vector<std::unique_ptr<BoundExpression>>& arguments,
    BoundCompileUnit& boundCompileUnit, const Span& span, OverloadResolutionFlags flags, std::unique_ptr<Exception>& exception)
{
    int arity = arguments.size();
    std::unordered_set<FunctionSymbol*> viableFunctions;
    CollectViableFunctions(arity, groupName, functionScopeLookups, boundCompileUnit, viableFunctions);
    if (viableFunctions.empty())
    {
        return FailWithNoViableFunction(groupName, arguments, span, flags, exception);
    }
    else
    {
        return SelectViableFunction(viableFunctions, groupName, arguments, boundCompileUnit, span, flags, exception);
    }
}

} } // namespace cmajor::binder
