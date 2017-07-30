// =================================
// Copyright (c) 2017 Seppo Laakko
// Distributed under the MIT license
// =================================

#include <cmajor/symbols/SymbolTable.hpp>
#include <cmajor/symbols/SymbolWriter.hpp>
#include <cmajor/symbols/SymbolReader.hpp>
#include <cmajor/symbols/BasicTypeSymbol.hpp>
#include <cmajor/symbols/BasicTypeOperation.hpp>
#include <cmajor/symbols/FunctionSymbol.hpp>
#include <cmajor/symbols/ClassTypeSymbol.hpp>
#include <cmajor/symbols/InterfaceTypeSymbol.hpp>
#include <cmajor/symbols/DelegateSymbol.hpp>
#include <cmajor/symbols/TypedefSymbol.hpp>
#include <cmajor/symbols/VariableSymbol.hpp>
#include <cmajor/symbols/ConstantSymbol.hpp>
#include <cmajor/symbols/EnumSymbol.hpp>
#include <cmajor/symbols/Exception.hpp>
#include <cmajor/ast/Identifier.hpp>
#include <cmajor/util/Unicode.hpp>
#include <boost/filesystem.hpp>

namespace cmajor { namespace symbols {

using namespace cmajor::unicode;

void TypeIdCounter::Init()
{
    instance.reset(new TypeIdCounter());
}

void TypeIdCounter::Done()
{
    instance.reset();
}

std::unique_ptr<TypeIdCounter> TypeIdCounter::instance;

TypeIdCounter::TypeIdCounter() : nextTypeId(1)
{
}

SymbolTable::SymbolTable() : 
    globalNs(Span(), std::u32string()), container(&globalNs), currentClass(nullptr), currentInterface(nullptr), mainFunctionSymbol(nullptr), parameterIndex(0), declarationBlockIndex(0)
{
    globalNs.SetSymbolTable(this);
}

void SymbolTable::Write(SymbolWriter& writer)
{
    globalNs.Write(writer);
    uint32_t n = derivedTypes.size();
    writer.GetBinaryWriter().WriteEncodedUInt(n);
    for (const auto& derivedType : derivedTypes)
    {
        writer.Write(derivedType.get());
    }
}

void SymbolTable::Read(SymbolReader& reader)
{
    reader.SetSymbolTable(this);
    globalNs.Read(reader);
    uint32_t n = reader.GetBinaryReader().ReadEncodedUInt();
    for (uint32_t i = 0; i < n; ++i)
    {
        DerivedTypeSymbol* derivedTypeSymbol = reader.ReadDerivedTypeSymbol();
        derivedTypeSymbol->SetParent(&globalNs);
        derivedTypes.push_back(std::unique_ptr<DerivedTypeSymbol>(derivedTypeSymbol));
    }
    ProcessTypeRequests();
    for (FunctionSymbol* conversion : reader.Conversions())
    {
        AddConversion(conversion);
    }
}

void SymbolTable::Import(SymbolTable& symbolTable)
{
    globalNs.Import(&symbolTable.globalNs, *this);
    for (const auto& pair : symbolTable.typeIdMap)
    {
        TypeSymbol* type = pair.second;
        typeIdMap[type->TypeId()] = type;
        typeNameMap[type->FullName()] = type;
    }
    for (auto& derivedType : symbolTable.derivedTypes)
    {
        derivedType->SetSymbolTable(this);
        derivedType->SetParent(&globalNs);
        derivedTypes.push_back(std::unique_ptr<DerivedTypeSymbol>(derivedType.release()));
    }
    int nd = derivedTypes.size();
    for (int i = 0; i < nd; ++i)
    {
        DerivedTypeSymbol* derivedTypeSymbol = derivedTypes[i].get();
        std::vector<DerivedTypeSymbol*>& derivedTypeVec = derivedTypeMap[derivedTypeSymbol->BaseType()];
        int n = derivedTypeVec.size();
        bool found = false;
        for (int i = 0; i < n; ++i)
        {
            DerivedTypeSymbol* prevDerivedTypeSymbol = derivedTypeVec[i];
            if (prevDerivedTypeSymbol->DerivationRec() == derivedTypeSymbol->DerivationRec())
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            derivedTypeVec.push_back(derivedTypeSymbol);
        }
    }
    Assert(conversionTable.IsEmpty(), "empty conversion table expected");
    conversionTable = std::move(symbolTable.conversionTable);
    for (ClassTypeSymbol* polymorphicClass : symbolTable.PolymorphicClasses())
    {
        AddPolymorphicClass(polymorphicClass);
    }
    symbolTable.Clear();
}

void SymbolTable::Clear()
{
    globalNs.Clear();
    typeIdMap.clear();
    typeNameMap.clear();
}

void SymbolTable::BeginContainer(ContainerSymbol* container_)
{
    containerStack.push(container);
    container = container_;
}

void SymbolTable::EndContainer()
{
    container = containerStack.top();
    containerStack.pop();
}

void SymbolTable::BeginNamespace(NamespaceNode& namespaceNode)
{
    std::u32string nsName = namespaceNode.Id()->Str();
    BeginNamespace(nsName, namespaceNode.GetSpan());
    MapNode(&namespaceNode, container);
}

void SymbolTable::BeginNamespace(const std::u32string& namespaceName, const Span& span)
{
    if (namespaceName.empty())
    {
        if (!globalNs.GetSpan().Valid())
        {
            globalNs.SetSpan(span);
        }
        BeginContainer(&globalNs);
    }
    else
    { 
        Symbol* symbol = container->GetContainerScope()->Lookup(namespaceName);
        if (symbol)
        {
            if (symbol->GetSymbolType() == SymbolType::namespaceSymbol)
            {
                NamespaceSymbol* ns = static_cast<NamespaceSymbol*>(symbol);
                BeginContainer(ns);
            }
            else
            {
                throw Exception("symbol '" + ToUtf8(symbol->Name()) + "' does not denote a namespace", symbol->GetSpan());
            }
        }
        else
        {
            NamespaceSymbol* ns = container->GetContainerScope()->CreateNamespace(namespaceName, span);
            BeginContainer(ns);
        }
    }
}

void SymbolTable::EndNamespace()
{
    EndContainer();
}

void SymbolTable::BeginFunction(FunctionNode& functionNode)
{
    FunctionSymbol* functionSymbol = new FunctionSymbol(functionNode.GetSpan(), functionNode.GroupId());
    functionSymbol->SetSymbolTable(this);
    functionSymbol->SetGroupName(functionNode.GroupId());
    if (functionSymbol->GroupName() == U"main")
    {
        if (mainFunctionSymbol)
        {
            throw Exception("already has main function", functionNode.GetSpan(), mainFunctionSymbol->GetSpan());
        }
        else
        {
            mainFunctionSymbol = functionSymbol;
        }
    }
    MapNode(&functionNode, functionSymbol);
    ContainerScope* functionScope = functionSymbol->GetContainerScope();
    ContainerScope* containerScope = container->GetContainerScope();
    functionScope->SetParent(containerScope);
    BeginContainer(functionSymbol);
    parameterIndex = 0;
    ResetDeclarationBlockIndex();
}

void SymbolTable::EndFunction()
{
    FunctionSymbol* functionSymbol = static_cast<FunctionSymbol*>(container);
    EndContainer();
    container->AddMember(functionSymbol);
}

void SymbolTable::AddParameter(ParameterNode& parameterNode)
{
    std::u32string& parameterName = ToUtf32("@p" + std::to_string(parameterIndex));
    if (parameterNode.Id())
    {
        parameterName = parameterNode.Id()->Str();
    }
    ParameterSymbol* parameterSymbol = new ParameterSymbol(parameterNode.GetSpan(), parameterName);
    parameterSymbol->SetSymbolTable(this);
    MapNode(&parameterNode, parameterSymbol);
    container->AddMember(parameterSymbol);
    ++parameterIndex;
}

void SymbolTable::BeginClass(ClassNode& classNode)
{
    ClassTypeSymbol* classTypeSymbol = new ClassTypeSymbol(classNode.GetSpan(), classNode.Id()->Str());
    currentClassStack.push(currentClass);
    currentClass = classTypeSymbol;
    classTypeSymbol->SetSymbolTable(this);
    MapNode(&classNode, classTypeSymbol);
    SetTypeIdFor(classTypeSymbol);
    ContainerScope* classScope = classTypeSymbol->GetContainerScope();
    ContainerScope* containerScope = container->GetContainerScope();
    classScope->SetParent(containerScope);
    container->AddMember(classTypeSymbol);
    BeginContainer(classTypeSymbol);
}

void SymbolTable::EndClass()
{
    currentClass = currentClassStack.top();
    currentClassStack.pop();
    EndContainer();
}

void SymbolTable::AddTemplateParameter(TemplateParameterNode& templateParameterNode)
{
    TemplateParameterSymbol* templateParameterSymbol = new TemplateParameterSymbol(templateParameterNode.GetSpan(), templateParameterNode.Id()->Str());
    templateParameterSymbol->SetSymbolTable(this);
    MapNode(&templateParameterNode, templateParameterSymbol);
    container->AddMember(templateParameterSymbol);
}

void SymbolTable::BeginInterface(InterfaceNode& interfaceNode)
{
    InterfaceTypeSymbol* interfaceTypeSymbol = new InterfaceTypeSymbol(interfaceNode.GetSpan(), interfaceNode.Id()->Str());
    currentInterfaceStack.push(currentInterface);
    currentInterface = interfaceTypeSymbol;
    interfaceTypeSymbol->SetSymbolTable(this);
    MapNode(&interfaceNode, interfaceTypeSymbol);
    SetTypeIdFor(interfaceTypeSymbol);
    ContainerScope* interfaceScope = interfaceTypeSymbol->GetContainerScope();
    ContainerScope* containerScope = container->GetContainerScope();
    interfaceScope->SetParent(containerScope);
    container->AddMember(interfaceTypeSymbol);
    BeginContainer(interfaceTypeSymbol);
}

void SymbolTable::EndInterface()
{
    currentInterface = currentInterfaceStack.top();
    currentInterfaceStack.pop();
    EndContainer();
}

void SymbolTable::BeginStaticConstructor(StaticConstructorNode& staticConstructorNode)
{
    StaticConstructorSymbol* staticConstructorSymbol = new StaticConstructorSymbol(staticConstructorNode.GetSpan(), U"@static_constructor");
    staticConstructorSymbol->SetSymbolTable(this);
    MapNode(&staticConstructorNode, staticConstructorSymbol);
    ContainerScope* staticConstructorScope = staticConstructorSymbol->GetContainerScope();
    ContainerScope* containerScope = container->GetContainerScope();
    staticConstructorScope->SetParent(containerScope);
    container->AddMember(staticConstructorSymbol);
    BeginContainer(staticConstructorSymbol);
    ResetDeclarationBlockIndex();
}

void SymbolTable::EndStaticConstructor()
{
    StaticConstructorSymbol* staticConstructorSymbol = static_cast<StaticConstructorSymbol*>(container);
    EndContainer();
    container->AddMember(staticConstructorSymbol);
}

void SymbolTable::BeginConstructor(ConstructorNode& constructorNode)
{
    ConstructorSymbol* constructorSymbol = new ConstructorSymbol(constructorNode.GetSpan(), U"@constructor");
    constructorSymbol->SetSymbolTable(this);
    MapNode(&constructorNode, constructorSymbol);
    ContainerScope* constructorScope = constructorSymbol->GetContainerScope();
    ContainerScope* containerScope = container->GetContainerScope();
    constructorScope->SetParent(containerScope);
    BeginContainer(constructorSymbol);
    parameterIndex = 0;
    ResetDeclarationBlockIndex();
    ParameterSymbol* thisParam = new ParameterSymbol(constructorNode.GetSpan(), U"this");
    thisParam->SetSymbolTable(this);
    TypeSymbol* thisParamType = nullptr;
    if (currentClass)
    {
        thisParamType = currentClass->AddPointer(constructorNode.GetSpan());
        thisParam->SetType(thisParamType);
        thisParam->SetBound();
        constructorSymbol->AddMember(thisParam);
    }
    else if (currentInterface)
    {
        throw Exception("interface type cannot have a constructor", constructorNode.GetSpan());
    }
}

void SymbolTable::EndConstructor()
{
    ConstructorSymbol* constructorSymbol = static_cast<ConstructorSymbol*>(container);
    EndContainer();
    container->AddMember(constructorSymbol);
}

void SymbolTable::BeginDestructor(DestructorNode& destructorNode)
{
    DestructorSymbol* destructorSymbol = new DestructorSymbol(destructorNode.GetSpan(), U"@destructor");
    destructorSymbol->SetSymbolTable(this);
    MapNode(&destructorNode, destructorSymbol);
    ContainerScope* destructorScope = destructorSymbol->GetContainerScope();
    ContainerScope* containerScope = container->GetContainerScope();
    destructorScope->SetParent(containerScope);
    BeginContainer(destructorSymbol);
    ResetDeclarationBlockIndex();
    ParameterSymbol* thisParam = new ParameterSymbol(destructorNode.GetSpan(), U"this");
    thisParam->SetSymbolTable(this);
    TypeSymbol* thisParamType = nullptr;
    if (currentClass)
    {
        thisParamType = currentClass->AddPointer(destructorNode.GetSpan());
        thisParam->SetType(thisParamType);
        thisParam->SetBound();
        destructorSymbol->AddMember(thisParam);
    }
    else if (currentInterface)
    {
        throw Exception("interface type cannot have a destructor", destructorNode.GetSpan());
    }
}

void SymbolTable::EndDestructor()
{
    DestructorSymbol* destructorSymbol = static_cast<DestructorSymbol*>(container);
    EndContainer();
    container->AddMember(destructorSymbol);
}

void SymbolTable::BeginMemberFunction(MemberFunctionNode& memberFunctionNode)
{
    MemberFunctionSymbol* memberFunctionSymbol = new MemberFunctionSymbol(memberFunctionNode.GetSpan(), memberFunctionNode.GroupId());
    memberFunctionSymbol->SetSymbolTable(this);
    memberFunctionSymbol->SetGroupName(memberFunctionNode.GroupId());
    MapNode(&memberFunctionNode, memberFunctionSymbol);
    ContainerScope* memberFunctionScope = memberFunctionSymbol->GetContainerScope();
    ContainerScope* containerScope = container->GetContainerScope();
    memberFunctionScope->SetParent(containerScope);
    BeginContainer(memberFunctionSymbol);
    parameterIndex = 0;
    ResetDeclarationBlockIndex();
    if ((memberFunctionNode.GetSpecifiers() & Specifiers::static_) == Specifiers::none)
    {
        ParameterSymbol* thisParam = new ParameterSymbol(memberFunctionNode.GetSpan(), U"this");
        thisParam->SetSymbolTable(this);
        TypeSymbol* thisParamType = nullptr;
        if (currentClass)
        {
            if (memberFunctionNode.IsConst())
            {
                thisParamType = currentClass->AddConst(memberFunctionNode.GetSpan())->AddPointer(memberFunctionNode.GetSpan());
            }
            else
            {
                thisParamType = currentClass->AddPointer(memberFunctionNode.GetSpan());
            }
        }
        else if (currentInterface)
        {
            thisParamType = currentInterface->AddPointer(memberFunctionNode.GetSpan());
        }
        else
        {
            Assert(false, "class or interface expected");
        }
        thisParam->SetType(thisParamType);
        thisParam->SetBound();
        memberFunctionSymbol->AddMember(thisParam);
    }
}

void SymbolTable::EndMemberFunction()
{
    MemberFunctionSymbol* memberFunctionSymbol = static_cast<MemberFunctionSymbol*>(container);
    EndContainer();
    container->AddMember(memberFunctionSymbol);
}

void SymbolTable::AddMemberVariable(MemberVariableNode& memberVariableNode)
{
    MemberVariableSymbol* memberVariableSymbol = new MemberVariableSymbol(memberVariableNode.GetSpan(), memberVariableNode.Id()->Str());
    if ((memberVariableNode.GetSpecifiers() & Specifiers::static_) != Specifiers::none)
    {
        memberVariableSymbol->SetStatic();
    }
    memberVariableSymbol->SetSymbolTable(this);
    MapNode(&memberVariableNode, memberVariableSymbol);
    container->AddMember(memberVariableSymbol);
}

void SymbolTable::BeginDelegate(DelegateNode& delegateNode)
{
    DelegateTypeSymbol* delegateTypeSymbol = new DelegateTypeSymbol(delegateNode.GetSpan(), delegateNode.Id()->Str());
    delegateTypeSymbol->SetSymbolTable(this);
    MapNode(&delegateNode, delegateTypeSymbol);
    SetTypeIdFor(delegateTypeSymbol);
    ContainerScope* delegateScope = delegateTypeSymbol->GetContainerScope();
    ContainerScope* containerScope = container->GetContainerScope();
    delegateScope->SetParent(containerScope);
    container->AddMember(delegateTypeSymbol);
    BeginContainer(delegateTypeSymbol);
    parameterIndex = 0;
}

void SymbolTable::EndDelegate()
{
    EndContainer();
}

void SymbolTable::BeginClassDelegate(ClassDelegateNode& classDelegateNode)
{
    ClassDelegateTypeSymbol* classDelegateTypeSymbol = new ClassDelegateTypeSymbol(classDelegateNode.GetSpan(), classDelegateNode.Id()->Str());
    classDelegateTypeSymbol->SetSymbolTable(this);
    MapNode(&classDelegateNode, classDelegateTypeSymbol);
    SetTypeIdFor(classDelegateTypeSymbol);
    ContainerScope* classDelegateScope = classDelegateTypeSymbol->GetContainerScope();
    ContainerScope* containerScope = container->GetContainerScope();
    classDelegateScope->SetParent(containerScope);
    container->AddMember(classDelegateTypeSymbol);
    BeginContainer(classDelegateTypeSymbol);
    parameterIndex = 0;
}

void SymbolTable::EndClassDelegate()
{
    EndContainer();
}

void SymbolTable::BeginDeclarationBlock(Node& node)
{
    DeclarationBlock* declarationBlock = new DeclarationBlock(node.GetSpan(), U"@locals" + ToUtf32(std::to_string(GetNextDeclarationBlockIndex())));
    declarationBlock->SetSymbolTable(this);
    MapNode(&node, declarationBlock);
    ContainerScope* declarationBlockScope = declarationBlock->GetContainerScope();
    ContainerScope* containerScope = container->GetContainerScope();
    declarationBlockScope->SetParent(containerScope);
    container->AddMember(declarationBlock);
    BeginContainer(declarationBlock);
}

void SymbolTable::EndDeclarationBlock()
{
    EndContainer();
}

void SymbolTable::AddLocalVariable(ConstructionStatementNode& constructionStatementNode)
{
    LocalVariableSymbol* localVariableSymbol = new LocalVariableSymbol(constructionStatementNode.GetSpan(), constructionStatementNode.Id()->Str());
    localVariableSymbol->SetSymbolTable(this);
    MapNode(&constructionStatementNode, localVariableSymbol);
    container->AddMember(localVariableSymbol);
}

void SymbolTable::AddLocalVariable(IdentifierNode& identifierNode)
{
    LocalVariableSymbol* localVariableSymbol = new LocalVariableSymbol(identifierNode.GetSpan(), identifierNode.Str());
    localVariableSymbol->SetSymbolTable(this);
    MapNode(&identifierNode, localVariableSymbol);
    container->AddMember(localVariableSymbol);
}

void SymbolTable::AddTypedef(TypedefNode& typedefNode)
{
    TypedefSymbol* typedefSymbol = new TypedefSymbol(typedefNode.GetSpan(), typedefNode.Id()->Str());
    typedefSymbol->SetSymbolTable(this);
    MapNode(&typedefNode, typedefSymbol);
    container->AddMember(typedefSymbol);
}

void SymbolTable::AddConstant(ConstantNode& constantNode)
{
    ConstantSymbol* constantSymbol = new ConstantSymbol(constantNode.GetSpan(), constantNode.Id()->Str());
    constantSymbol->SetSymbolTable(this);
    MapNode(&constantNode, constantSymbol);
    container->AddMember(constantSymbol);
}

void SymbolTable::BeginEnumType(EnumTypeNode& enumTypeNode)
{
    EnumTypeSymbol* enumTypeSymbol = new EnumTypeSymbol(enumTypeNode.GetSpan(), enumTypeNode.Id()->Str());
    enumTypeSymbol->SetSymbolTable(this);
    MapNode(&enumTypeNode, enumTypeSymbol);
    ContainerScope* enumTypeScope = enumTypeSymbol->GetContainerScope();
    ContainerScope* containerScope = container->GetContainerScope();
    enumTypeScope->SetParent(containerScope);
    container->AddMember(enumTypeSymbol);
    BeginContainer(enumTypeSymbol);
}

void SymbolTable::EndEnumType()
{
    EndContainer();
}

void SymbolTable::AddEnumConstant(EnumConstantNode& enumConstantNode)
{
    EnumConstantSymbol* enumConstantSymbol = new EnumConstantSymbol(enumConstantNode.GetSpan(), enumConstantNode.Id()->Str());
    enumConstantSymbol->SetSymbolTable(this);
    MapNode(&enumConstantNode, enumConstantSymbol);
    container->AddMember(enumConstantSymbol);
}

void SymbolTable::AddTypeSymbolToGlobalScope(TypeSymbol* typeSymbol)
{
    typeSymbol->SetSymbolTable(this);
    globalNs.AddMember(typeSymbol);
    SetTypeIdFor(typeSymbol);
    typeNameMap[typeSymbol->FullName()] = typeSymbol;
}

void SymbolTable::AddFunctionSymbolToGlobalScope(FunctionSymbol* functionSymbol)
{
    functionSymbol->SetSymbolTable(this);
    globalNs.AddMember(functionSymbol);
    if (functionSymbol->IsConversion())
    {
        conversionTable.AddConversion(functionSymbol);
    }
}

void SymbolTable::MapNode(Node* node, Symbol* symbol)
{
    nodeSymbolMap[node] = symbol;
    symbolNodeMap[symbol] = node;
}

Symbol* SymbolTable::GetSymbolNoThrow(Node* node) const
{
    auto it = nodeSymbolMap.find(node);
    if (it != nodeSymbolMap.cend())
    {
        return it->second;
    }
    else
    {
        return nullptr;
    }
}

Symbol* SymbolTable::GetSymbol(Node* node) const
{
    Symbol* symbol = GetSymbolNoThrow(node);
    if (symbol)
    {
        return symbol;
    }
    else
    {
        throw std::runtime_error("symbol for node not found");
    }
}

Node* SymbolTable::GetNodeNoThrow(Symbol* symbol) const
{
    auto it = symbolNodeMap.find(symbol);
    if (it != symbolNodeMap.cend())
    {
        return it->second;
    }
    else
    {
        return nullptr;
    }
}

Node* SymbolTable::GetNode(Symbol* symbol) const
{
    Node* node = GetNodeNoThrow(symbol);
    if (node)
    {
        return node;
    }
    else
    {
        throw std::runtime_error("node for symbol not found");
    }
}

void SymbolTable::AddTypeSymbolToTypeIdMap(TypeSymbol* typeSymbol)
{
    typeIdMap[typeSymbol->TypeId()] = typeSymbol;
}

void SymbolTable::SetTypeIdFor(TypeSymbol* typeSymbol)
{
    typeSymbol->SetTypeId(TypeIdCounter::Instance().GetNextTypeId());
}

void SymbolTable::EmplaceTypeRequest(Symbol* forSymbol, uint32_t typeId, int index)
{
    auto it = typeIdMap.find(typeId);
    if (it != typeIdMap.cend())
    {
        TypeSymbol* typeSymbol = it->second;
        forSymbol->EmplaceType(typeSymbol, index);
    }
    else
    {
        typeRequests.push_back(TypeRequest(forSymbol, typeId, index));
    }
}

void SymbolTable::ProcessTypeRequests()
{
    for (const TypeRequest& typeRequest : typeRequests)
    {
        Symbol* symbol = typeRequest.symbol;
        auto it = typeIdMap.find(typeRequest.typeId);
        if (it != typeIdMap.cend())
        {
            TypeSymbol* typeSymbol = it->second;
            int index = typeRequest.index;
            symbol->EmplaceType(typeSymbol, index);
        }
        else
        {
            throw std::runtime_error("cannot satisfy type request for symbol '" + ToUtf8(symbol->Name()) + "': type not found from symbol table");
        }
    }
    typeRequests.clear();
}

TypeSymbol* SymbolTable::GetTypeByNameNoThrow(const std::u32string& typeName) const
{
    auto it = typeNameMap.find(typeName);
    if (it != typeNameMap.cend())
    {
        return it->second;
    }
    else
    {
        return nullptr;
    }
}

TypeSymbol* SymbolTable::GetTypeByName(const std::u32string& typeName) const
{
    TypeSymbol* typeSymbol = GetTypeByNameNoThrow(typeName);
    if (typeSymbol)
    {
        return typeSymbol;
    }
    else
    {
        throw std::runtime_error("type '" + ToUtf8(typeName) + "' not found");
    }
}

TypeSymbol* SymbolTable::MakeDerivedType(TypeSymbol* baseType, const TypeDerivationRec& derivationRec, const Span& span)
{
    if (derivationRec.IsEmpty())
    {
        return baseType;
    }
    if (baseType->IsVoidType() && HasReferenceDerivation(derivationRec.derivations) && !HasPointerDerivation(derivationRec.derivations))
    {
        throw Exception("cannot have reference to void type", span);
    }
    std::vector<DerivedTypeSymbol*>& mappedDerivedTypes = derivedTypeMap[baseType];
    int n = mappedDerivedTypes.size();
    for (int i = 0; i < n; ++i)
    {
        DerivedTypeSymbol* derivedType = mappedDerivedTypes[i];
        if (derivedType->DerivationRec() == derivationRec)
        {
            return derivedType;
        }
    }
    DerivedTypeSymbol* derivedType = new DerivedTypeSymbol(baseType->GetSpan(), MakeDerivedTypeName(baseType, derivationRec), baseType, derivationRec);
    derivedType->SetParent(&globalNs);
    derivedType->SetSymbolTable(this);
    SetTypeIdFor(derivedType);
    mappedDerivedTypes.push_back(derivedType);
    derivedTypes.push_back(std::unique_ptr<DerivedTypeSymbol>(derivedType));
    return derivedType;
}

void SymbolTable::AddConversion(FunctionSymbol* conversion)
{
    conversionTable.AddConversion(conversion);
}

FunctionSymbol* SymbolTable::GetConversion(TypeSymbol* sourceType, TypeSymbol* targetType, const Span& span) const
{
    return conversionTable.GetConversion(sourceType, targetType, span);
}

void SymbolTable::AddPolymorphicClass(ClassTypeSymbol* polymorphicClass)
{
    if (!polymorphicClass->IsPolymorphic())
    {
        throw Exception("not a polymorphic class", polymorphicClass->GetSpan());
    }
    polymorphicClasses.insert(polymorphicClass);
}

void InitCoreSymbolTable(SymbolTable& symbolTable)
{
    BoolTypeSymbol* boolType = new BoolTypeSymbol(Span(), U"bool");
    SByteTypeSymbol* sbyteType = new SByteTypeSymbol(Span(), U"sbyte");
    ByteTypeSymbol* byteType = new ByteTypeSymbol(Span(), U"byte");
    ShortTypeSymbol* shortType = new ShortTypeSymbol(Span(), U"short");
    UShortTypeSymbol* ushortType = new UShortTypeSymbol(Span(), U"ushort");
    IntTypeSymbol* intType = new IntTypeSymbol(Span(), U"int");
    UIntTypeSymbol* uintType = new UIntTypeSymbol(Span(), U"uint");
    LongTypeSymbol* longType = new LongTypeSymbol(Span(), U"long");
    ULongTypeSymbol* ulongType = new ULongTypeSymbol(Span(), U"ulong");
    FloatTypeSymbol* floatType = new FloatTypeSymbol(Span(), U"float");
    DoubleTypeSymbol* doubleType = new DoubleTypeSymbol(Span(), U"double");
    CharTypeSymbol* charType = new CharTypeSymbol(Span(), U"char");
    WCharTypeSymbol* wcharType = new WCharTypeSymbol(Span(), U"wchar");
    UCharTypeSymbol* ucharType = new UCharTypeSymbol(Span(), U"uchar");
    VoidTypeSymbol* voidType = new VoidTypeSymbol(Span(), U"void");
    symbolTable.AddTypeSymbolToGlobalScope(boolType);
    symbolTable.AddTypeSymbolToGlobalScope(sbyteType);
    symbolTable.AddTypeSymbolToGlobalScope(byteType);
    symbolTable.AddTypeSymbolToGlobalScope(shortType);
    symbolTable.AddTypeSymbolToGlobalScope(ushortType);
    symbolTable.AddTypeSymbolToGlobalScope(intType);
    symbolTable.AddTypeSymbolToGlobalScope(uintType);
    symbolTable.AddTypeSymbolToGlobalScope(longType);
    symbolTable.AddTypeSymbolToGlobalScope(ulongType);
    symbolTable.AddTypeSymbolToGlobalScope(floatType);
    symbolTable.AddTypeSymbolToGlobalScope(doubleType);
    symbolTable.AddTypeSymbolToGlobalScope(charType);
    symbolTable.AddTypeSymbolToGlobalScope(wcharType);
    symbolTable.AddTypeSymbolToGlobalScope(ucharType);
    symbolTable.AddTypeSymbolToGlobalScope(voidType);
    symbolTable.AddTypeSymbolToGlobalScope(new NullPtrType(Span(), U"@nullptr_type"));
    MakeBasicTypeOperations(symbolTable, boolType, sbyteType, byteType, shortType, ushortType, intType, uintType, longType, ulongType, floatType, doubleType, charType, wcharType, ucharType, voidType);
}

void CreateClassFile(const std::string& executableFilePath, const SymbolTable& symbolTable)
{
    std::string classFilePath = boost::filesystem::path(executableFilePath).replace_extension(".cls").generic_string();
    const std::unordered_set<ClassTypeSymbol*>& polymorphicClasses = symbolTable.PolymorphicClasses();
    uint32_t n = polymorphicClasses.size();
    BinaryWriter writer(classFilePath);
    writer.WriteEncodedUInt(n);
    for (ClassTypeSymbol* polymorphicClass : polymorphicClasses)
    {
        uint32_t typeId = polymorphicClass->TypeId();
        const std::string& vmtObjectName = polymorphicClass->VmtObjectName();
        uint32_t baseClassTypeId = 0;
        if (polymorphicClass->BaseClass())
        {
            baseClassTypeId = polymorphicClass->BaseClass()->TypeId();
        }
        writer.WriteEncodedUInt(typeId);
        writer.Write(vmtObjectName);
        writer.WriteEncodedUInt(baseClassTypeId);
    }
}

void InitSymbolTable()
{
    TypeIdCounter::Init();
}

void DoneSymbolTable()
{
    TypeIdCounter::Done();
}

} } // namespace cmajor::symbols