/**
 * htmlparser.h
 * Copyright (C) 2016 Farci <farci@rehpost.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SPINTEST_HTMLPARSER_H
#define SPINTEST_HTMLPARSER_H

#include <libxml/tree.h>
#include <libxml/HTMLparser.h>
#include <cassert>
#include <cstring>

namespace Spin
{

class CHTMLAttr
{
public:
    struct SRes
    {
        const char* m_pKey;
        const char* m_pValue;
    };

public:
    CHTMLAttr(xmlAttrPtr _pAttr)
        : m_pAttr(_pAttr)
    {

    }

    ~CHTMLAttr()
    {

    }

    inline const char* getKey() const
    {
        return reinterpret_cast<const char*>(m_pAttr->name);
    }

    inline const char* getValue() const
    {
        assert(m_pAttr->children != nullptr);
        return reinterpret_cast<const char*>(m_pAttr->children->content);
    }

    inline const CHTMLAttr& next()
    {
        m_pAttr = m_pAttr->next;
        return *this;
    }

    inline bool operator == (const CHTMLAttr& _rOther) const
    {
        return m_pAttr == _rOther.m_pAttr;
    }

    inline bool operator != (const CHTMLAttr& _rOther) const
    {
        return !(*this == _rOther);
    }

    inline const CHTMLAttr& operator ++ ()
    {
        m_pAttr = m_pAttr->next;
        return *this;
    }

    inline SRes* operator->()
    {
        m_res.m_pKey   = reinterpret_cast<const char*>(m_pAttr->name);
        m_res.m_pValue = reinterpret_cast<const char*>(m_pAttr->children->content);

        return &m_res;
    }

    inline SRes* operator*()
    {
        m_res.m_pKey   = reinterpret_cast<const char*>(m_pAttr->name);
        m_res.m_pValue = reinterpret_cast<const char*>(m_pAttr->children->content);

        return &m_res;
    }

private:
    xmlAttrPtr m_pAttr;
    SRes       m_res;
};

class CHTMLNode
{
public:
    CHTMLNode(xmlNodePtr _pNode)
        : m_pNode(_pNode)
    {

    }

    ~CHTMLNode()
    {

    }

    inline const char* getName() const
    {
        return reinterpret_cast<const char*>(m_pNode->name);
    }

    inline const char* getContent() const
    {
        xmlNodePtr pNodeChildren = m_pNode->children;
        if (pNodeChildren          != NULL &&
            pNodeChildren->content != NULL    )
        {
            return reinterpret_cast<const char*>(m_pNode->children->content);
        }
        return "";
    }

    inline const char* getAttributeValue(const char* _pAttr) const
    {
        for (xmlAttrPtr pCurrentAttr = m_pNode->properties; pCurrentAttr; pCurrentAttr = pCurrentAttr->next)
        {
            if (strcmp(_pAttr, reinterpret_cast<const char*>(pCurrentAttr->name)) == 0)
            {
                return reinterpret_cast<const char*>(pCurrentAttr->children->content);
            }
        }

        return nullptr;
    }

    inline bool hasAttributeContainingValue(const char* _pAttr, const char* _pValue) const
    {
        for (xmlAttrPtr pCurrentAttr = m_pNode->properties; pCurrentAttr; pCurrentAttr = pCurrentAttr->next)
        {
            if (strcmp(_pAttr, reinterpret_cast<const char*>(pCurrentAttr->name)) == 0)
            {
                return strstr(reinterpret_cast<const char*>(pCurrentAttr->children->content), _pValue) != NULL;
            }
        }

        return false;
    }

    inline bool hasAttributeIsValue(const char* _pAttr, const char* _pValue) const
    {
        for (xmlAttrPtr pCurrentAttr = m_pNode->properties; pCurrentAttr; pCurrentAttr = pCurrentAttr->next)
        {
            if (strcmp(_pAttr, reinterpret_cast<const char*>(pCurrentAttr->name)) == 0)
            {
                return strcmp(reinterpret_cast<const char*>(pCurrentAttr->children->content), _pValue) == 0;
            }
        }

        return false;
    }

    inline CHTMLNode getParent() const
    {
        return CHTMLNode(m_pNode->parent);
    }

    inline CHTMLNode getFirstChild() const
    {
        return CHTMLNode(m_pNode->children);
    }

    inline CHTMLAttr getAttrCBegin() const
    {
        return CHTMLAttr(m_pNode->properties);
    }

    inline CHTMLAttr getAttrCEnd() const
    {
        return CHTMLAttr(nullptr);
    }

    inline CHTMLNode& next()
    {
        m_pNode = m_pNode->next;
        return *this;
    }

    inline bool isLastChild()
    {
        return m_pNode->next == NULL;
    }

private:
    xmlNodePtr m_pNode;
};


template <typename TCallee>
class CHTMLParser
{
public:
    typedef bool (*TCallback)(TCallee*, CHTMLNode*);

public:
    CHTMLParser();
    ~CHTMLParser();

    void parse(const char* _pHTMLString, TCallback _callback, TCallee* _pCallee);

private:
    void innerParse(xmlNode* _pNode);

private:
    TCallback m_callback;
    xmlDoc*   m_pXMLDoc;
    TCallee*  m_pCallee;

};

template <typename TCallee>
CHTMLParser<TCallee>::CHTMLParser()
    : m_callback()
    , m_pXMLDoc (nullptr)
    , m_pCallee (nullptr)
{

}

template <typename TCallee>
CHTMLParser<TCallee>::~CHTMLParser()
{
    if (m_pXMLDoc != nullptr) xmlFreeDoc(m_pXMLDoc);
}

template <typename TCallee>
void CHTMLParser<TCallee>::parse(const char* _pHTMLString, TCallback _callback, TCallee* _pCallee)
{
    if (m_pXMLDoc != nullptr) xmlFreeDoc(m_pXMLDoc);

    m_callback = _callback;
    m_pCallee  = _pCallee;

    m_pXMLDoc = htmlReadDoc(
        reinterpret_cast<const xmlChar*>(_pHTMLString),
        NULL,
        NULL,
        HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);

    xmlNodePtr pNode = xmlDocGetRootElement(m_pXMLDoc);

    innerParse(pNode);
}

template <typename TCallee>
void CHTMLParser<TCallee>::innerParse(xmlNodePtr _pNode)
{
    for (xmlNodePtr pCurrentNode = _pNode; pCurrentNode; pCurrentNode = pCurrentNode->next)
    {
        if (pCurrentNode->type == XML_ELEMENT_NODE)
        {
            CHTMLNode htmlNode(pCurrentNode);
            if (!(*m_callback)(m_pCallee, &htmlNode))
            {
                continue;
            }
        }

        innerParse(pCurrentNode->children);
    }
}

}

#endif // SPINTEST_HTMLPARSER_H
