// =================================
// Copyright (c) 2017 Seppo Laakko
// Distributed under the MIT license
// =================================

#ifndef CMAJOR_AST_INTERFACE_INCLUDED
#define CMAJOR_AST_INTERFACE_INCLUDED
#include <cmajor/ast/Function.hpp>

namespace cmajor { namespace ast {

class InterfaceNode : public Node
{
public:
    InterfaceNode(const Span& span_);
    InterfaceNode(const Span& span_, Specifiers specifiers_, IdentifierNode* id_);
    Node* Clone(CloneContext& cloneContext) const override;
    void Accept(Visitor& visitor) override;
    void Write(AstWriter& writer) override;
    void Read(AstReader& reader) override;
    Specifiers GetSpecifiers() const { return specifiers; }
    IdentifierNode* Id() const { return id.get(); }
    const NodeList<Node>& Members() const { return members; }
    void AddMember(Node* member);
private:
    Specifiers specifiers;
    std::unique_ptr<IdentifierNode> id;
    NodeList<Node> members;
};

} } // namespace cmajor::ast

#endif // CMAJOR_AST_INTERFACE_INCLUDED
