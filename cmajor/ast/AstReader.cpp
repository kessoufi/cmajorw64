// =================================
// Copyright (c) 2017 Seppo Laakko
// Distributed under the MIT license
// =================================

#include <cmajor/ast/AstReader.hpp>
#include <cmajor/ast/Identifier.hpp>
#include <cmajor/ast/Statement.hpp>

namespace cmajor { namespace ast {

AstReader::AstReader(const std::string& fileName_) : binaryReader(fileName_)
{
}

Node* AstReader::ReadNode()
{
    NodeType nodeType = static_cast<NodeType>(binaryReader.ReadByte());
    Span span = ReadSpan();
    Node* node = NodeFactory::Instance().CreateNode(nodeType, span);
    node->Read(*this);
    return node;
}

IdentifierNode* AstReader::ReadIdentifierNode()
{
    Node* node = ReadNode();
    if (node->GetNodeType() == NodeType::identifierNode)
    {
        return static_cast<IdentifierNode*>(node);
    }
    else
    {
        throw std::runtime_error("identifier node expected");
    }
}

LabelNode* AstReader::ReadLabelNode()
{
    Node* node = ReadNode();
    if (node->GetNodeType() == NodeType::labelNode)
    {
        return static_cast<LabelNode*>(node);
    }
    else
    {
        throw std::runtime_error("label node expected");
    }
}

StatementNode* AstReader::ReadStatementNode()
{
    Node* node = ReadNode();
    if (node->IsStatementNode())
    {
        return static_cast<StatementNode*>(node);
    }
    else
    {
        throw std::runtime_error("statement node expected");
    }
}

DefaultStatementNode* AstReader::ReadDefaultStatementNode()
{
    Node* node = ReadNode();
    if (node->GetNodeType() == NodeType::defaultStatementNode)
    {
        return static_cast<DefaultStatementNode*>(node);
    }
    else
    {
        throw std::runtime_error("default statement node expected");
    }
}

CompoundStatementNode* AstReader::ReadCompoundStatementNode()
{
    Node* node = ReadNode();
    if (node->GetNodeType() == NodeType::compoundStatementNode)
    {
        return static_cast<CompoundStatementNode*>(node);
    }
    else
    {
        throw std::runtime_error("compound statement node expected");
    }
}

Specifiers AstReader::ReadSpecifiers()
{
    return static_cast<Specifiers>(binaryReader.ReadUInt());
}

Span AstReader::ReadSpan()
{
    bool valid = binaryReader.ReadBool();
    if (!valid)
    {
        return Span();
    }
    else
    {
        uint32_t fileIndex = binaryReader.ReadEncodedUInt();
        uint32_t lineNumber = binaryReader.ReadEncodedUInt();
        uint32_t start = binaryReader.ReadEncodedUInt();
        uint32_t end = binaryReader.ReadEncodedUInt();
        return Span(fileIndex, lineNumber, start, end);
    }
}

} } // namespace cmajor::ast