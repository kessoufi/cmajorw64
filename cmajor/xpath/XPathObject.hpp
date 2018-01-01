// =================================
// Copyright (c) 2018 Seppo Laakko
// Distributed under the MIT license
// =================================

#ifndef CMAJOR_XPATH_XPATH_OBJECT
#define CMAJOR_XPATH_XPATH_OBJECT
#include <cmajor/dom/Node.hpp>

namespace cmajor { namespace xpath {

enum class XPathObjectType
{
    nodeSet, boolean, number, string
};

class XPathObject
{
public:
    XPathObject(XPathObjectType type_);
    XPathObjectType Type() const { return type; }
    virtual ~XPathObject();
private:
    XPathObjectType type;
};

class XPathNodeSet : public XPathObject
{
public:
    XPathNodeSet();
    cmajor::dom::Node* operator[](int index) const { return nodes[index]; }
    int Length() const { return nodes.Length(); }
    void Add(cmajor::dom::Node* node);
private:
    cmajor::dom::NodeList nodes;
};

class XPathBoolean : public XPathObject
{
public:
    XPathBoolean(bool value_);
    bool Value() const { return value; }
private:
    bool value;
};

class XPathNumber : public XPathObject
{
public:
    XPathNumber(double value_);
    double Value() const { return value; }
private:
    double value;
};

class XPathString : public XPathObject
{
public:    
    XPathString(const std::u32string& value_);
    const std::u32string& Value() const { return value; }
private:
    std::u32string value;
};

} } // namespace cmajor::xpath

#endif // CMAJOR_XPATH_XPATH_OBJECT
