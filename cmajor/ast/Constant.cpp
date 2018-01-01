// =================================
// Copyright (c) 2018 Seppo Laakko
// Distributed under the MIT license
// =================================

#include <cmajor/ast/Constant.hpp>
#include <cmajor/ast/Identifier.hpp>
#include <cmajor/ast/Visitor.hpp>
#include <cmajor/ast/AstWriter.hpp>
#include <cmajor/ast/AstReader.hpp>

namespace cmajor { namespace ast {

ConstantNode::ConstantNode(const Span& span_) : Node(NodeType::constantNode, span_), specifiers(Specifiers::none)
{
}

ConstantNode::ConstantNode(const Span& span_, Specifiers specifiers_, Node* typeExpr_, IdentifierNode* id_, Node* value_) :
    Node(NodeType::constantNode, span_), specifiers(specifiers_), typeExpr(typeExpr_), id(id_), value(value_)
{
    typeExpr->SetParent(this);
    id->SetParent(this);
    value->SetParent(this);
}

Node* ConstantNode::Clone(CloneContext& cloneContext) const
{
    return new ConstantNode(GetSpan(), specifiers, typeExpr->Clone(cloneContext), static_cast<IdentifierNode*>(id->Clone(cloneContext)), value->Clone(cloneContext));
}

void ConstantNode::Accept(Visitor& visitor)
{
    visitor.Visit(*this);
}

void ConstantNode::Write(AstWriter& writer)
{
    Node::Write(writer);
    writer.Write(specifiers);
    writer.Write(typeExpr.get());
    writer.Write(id.get());
    writer.Write(value.get());
}

void ConstantNode::Read(AstReader& reader)
{
    Node::Read(reader);
    specifiers = reader.ReadSpecifiers();
    typeExpr.reset(reader.ReadNode());
    typeExpr->SetParent(this);
    id.reset(reader.ReadIdentifierNode());
    id->SetParent(this);
    value.reset(reader.ReadNode());
    value->SetParent(this);
}

} } // namespace cmajor::ast
