/******************************************************************************
* Module : xml3c_api                                                          *
*                                                                             *
* Purpose: Make C++ App load and modify XML-CONFIGURATION more conveniently.  *
*                                                                             *
* Author : X.D. Guo / xml3see / xml3s                                         *
*                                                                             *
* Comment:                                                                    *
*     Please compile with defining macro:                                     *
*        _USING_LIBXML2 or _USING_XERCESC or _USING_TINYXML2                  *
*                                                                             *
*     Version history:                                                        *
*       3.6.00: For more conveniently, use default value api style, remove    *
*               legacy api styles(empty or output parameter)                  *
*       3.5.70:                                                               *                   
*               (1)Add support for tinyxml2                                   *
*               (2)modify bug for the function:                               *
*                  element::set_property_value will lead heap overflow because*
*                  of recursive call infinity                                 *      
*       3.5.62: xml3c_api, Add vs2005 project file.                           *
*       3.5.61: xml3c_api, Add vs2008, 2010 project files.                    *
*       3.5.6: xml3c_api, support for compiler no c++2011 standard            *
*              such as vs2005, vs2008, or g++ no flag -std=c++0x/c++11        *
*       3.5.5: xml3c_api, support c++0x/11                                    *
*       3.5.3: xml3c_api, add two interface for getting children              *
*       3.5: xml3c_api, change naming style, optimize some code               *
*       3.3: xml3c_api, support XPATH based on 3rd library: libxml2 and the   *
*            3.0.0 or more version of xerces-c.                               *
*       3.2: xml3c_api, support XPATH based on 3rd library: libxml2_7 or      *
*            xercesc2_8/3_1                                                   *
*       3.1: xml3c_api, based on 3rd library: libxml2_7 or xercesc3           *
*            support XPATH.                                                   *
*       3.0: xml3c_api, based on 3rd library: libxml2_7 or xercesc            *
*       2.0: xml2c, based on 3rd library: xercesc                             *
*       1.0: xmlxx, based on 3rd library: libxml2_7                           *
*******************************************************************************/
#ifndef _XML3C_API_H_
#define _XML3C_API_H_

/// version
#ifndef _XML3C_VERSION
#define _XML3C_VERSION "3.6.0"
#endif

/// default: use tinyxml2
#undef _USING_LIBXML2
#undef _USING_XERCESC
#undef _USING_TINYXML2
#define _USING_TINYXML2 1

/// includes
#include <cstring>
#include <vector>
#include <functional>
#if defined(_USING_LIBXML2)
#include <libxml/xpath.h>
#elif defined(_USING_XERCESC)
#include <xercesc/dom/DOM.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/framework/MemBufFormatTarget.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLException.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#elif defined(_USING_TINYXML2)
#pragma warning(disable:4345)
#include <support/tinyxml2/tinyxml2.h>
#else
#error "error, the xml3c just supported by the 3rd library:libxml2 or xercesc£¬ please define the macro _USING_LIBXML2 or _USING_XERCESC!"
#endif

#include "nsconv.h"
#include "container_helper.h"


/// traversal macro defination
#define __xml3cts_algo(ptr, first, second, cond, stmt) \
        if(ptr != nullptr) { \
            ptr = ptr->first; \
        } \
        while (ptr != nullptr) { \
            if (cond) { \
                 stmt; \
            } \
            ptr = ptr->second; \
        }

/// typedefs
#if defined(_USING_LIBXML2) 
typedef xmlNodePtr                xml3cNodePtr;
typedef xmlDocPtr                 xml3cDocPtr;
typedef xmlXPathObject            xml3cXPathObject;
typedef xmlXPathObjectPtr         xml3cXPathObjectPtr;
#elif defined(_USING_XERCESC)
typedef xercesc::DOMNode*         xml3cNodePtr;
typedef xercesc::DOMNode&         xml3cNoderef;
typedef xercesc::DOMDocument*     xml3cDocPtr;
typedef xercesc::XercesDOMParser  xml3cParser;
typedef xercesc::DOMXPathResult   xml3cXPathObject;
typedef xercesc::DOMXPathResult*  xml3cXPathObjectPtr;
#else
typedef tinyxml2::XMLDocument*    xml3cDocPtr;
typedef tinyxml2::XMLNode*        xml3cNodePtr;
#endif

/// diff macro functions defination
#if defined(_USING_LIBXML2)
#define xml3cRemoveNode(node)     xmlDOMWrapRemoveNode(nullptr, node->doc, node, 0)
#define __xml3c_name_of(ptr)      (const char*)ptr->name
#define __xml3c_owner_of(ptr)     ptr->doc
#define __xml3c_parent_endix      parent
#define __xml3c_prev_endix        prev
#define __xml3c_next_endix        next
#define __xml3c_child_endix       children
#define __xml3c_is_element(ptr)   XML_ELEMENT_NODE == ptr->type
#define __xml3c_copy_node(ptr)    xmlCopyNode(ptr, 1)
#define __xml3c_set_node_content(ptr,content) xmlNodeSetContent(ptr, BAD_CAST content)
#define __xml3c_set_propval(ptr,prop,value) xmlSetProp(ptr, BAD_CAST prop, BAD_CAST value);
#define __xml3c_add_child(ptr,child) xmlAddChild(ptr, child)
#elif defined(_USING_XERCESC)
#define xml3cRemoveNode(node)     node->getOwnerDocument()->removeChild(node)
#define __xml3c_name_of(ptr)      _xml3c_transcode(ptr->getNodeName())
#define __xml3c_owner_of(ptr)     ptr->getOwnerDocument()
#define __xml3c_parent_endix      getParentNode()
#define __xml3c_prev_endix        getPreviousSibling()
#define __xml3c_next_endix        getNextSibling()
#define __xml3c_child_endix       getFirstChild()
#define __xml3c_is_element(ptr)   xercesc::DOMNode::ELEMENT_NODE == ptr->getNodeType()
#define __xml3c_copy_node(ptr)    ptr->cloneNode(true)
#define __xml3c_set_node_content(ptr,content) ptr->setTextContent(_xml3c_transcode(content).c_str())
#define __xml3c_set_propval(ptr,prop,value) ((DOMElement*)ptr)->setAttribute(_xml3c_transcode(prop).c_str(), _xml3c_transcode(value).c_str())
#define __xml3c_add_child(ptr,child) ptr->appendChild(child)
#else
#define xml3cRemoveNode(node)     node->GetDocument()->DeleteNode(node)
#define __xml3c_name_of(ptr)      dynamic_cast<tinyxml2::XMLElement*>(ptr)->Name()
#define __xml3c_owner_of(ptr)     ptr->GetDocument()
#define __xml3c_parent_endix      Parent()
#define __xml3c_prev_endix        PreviousSiblingElement()
#define __xml3c_next_endix        NextSiblingElement()
#define __xml3c_child_endix       FirstChildElement()
#define __xml3c_is_element(ptr)   (true)
#define __xml3c_copy_node(ptr)    ptr->ShallowClone(ptr->GetDocument())
#define __xml3c_set_node_content(ptr,content) do { if( ptr->FirstChild() != nullptr ){ptr->FirstChild()->SetValue(value);} else {ptr->InsertFirstChild(ptr->GetDocument()->NewText(content));} } while(false)
#define __xml3c_set_propval(ptr,prop,value) dynamic_cast<tinyxml2::XMLElement*>(ptr)->SetAttribute(prop, value)
#define __xml3c_add_child(ptr,child) ptr->InsertEndChild(child)
#endif

using namespace thelib;
using namespace thelib::gc;

namespace xml3c {

class element;
class document;

class element 
{
    friend class document;
public:

    element(xml3cNodePtr _Ptr = nullptr);

    element         clone(void) const;

    std::string     get_name(void) const;
    std::string     get_value(const std::string& default_value = "") const;
    std::string     get_property_value(const char* name, const std::string& default_value = "") const; 
    element         get_parent(void) const;
    element         get_prev_sibling(void) const;
    element         get_next_sibling(void) const;
    element         get_child(int index = 0) const;
    element         operator[](int index) const;
    element         get_child(const char* name, int index = 0) const;
    void            get_children(std::vector<element>& children) const;
    void            get_children(const char* name, std::vector<element>& children) const;

    void            set_value(const char* value);
    void            set_value(const std::string& value);

    element         add_child(const element& element) const;
    element         add_child(const char* name, const char* value = nullptr) const;

    void            remove_child(int index = 0);
    void            remove_child(const char* name, int index = 0);
    void            remove_children(void);
    void            remove_children(const char* name);
    void            remove_self(void);

    std::string     to_string(void) const;

    operator        xml3cNodePtr&(void);
    operator        xml3cNodePtr const&(void) const;

    //  Function TEMPLATEs
    template<typename _Ty>
    _Ty             get_value(const _Ty& default_value = _Ty()) const;


    template<typename _Ty>
    _Ty             get_property_value(const char* name, const _Ty& default_value = _Ty()) const;

    template<typename _Ty>
    void            set_value(const _Ty& value);

    template<typename _Ty>
    void            set_property_value(const char* name, const _Ty& value);

    template<typename _Operation>
    void            cforeach(_Operation op) const;

    template<typename _Operation>
    void            cforeach(const char* name, _Operation op) const;

private:
    xml3cNodePtr    _Mynode;
}; /* CLASS element */


class document
{
    friend class element;
public:
    document(void);

    /** See also: open(const char* filename) **/
    document(const char* filename);

    /** See also: open(const char* filename, const char* rootname) **/
    document(const char* filename, const char* rootname);

    /** See also: open(const char* xmlstring, size_t length) **/
    document(const char* buffer, int length);

    ~document(void);

    /* @brief  : Open an exist XML document
    ** @params : 
    **        filename: specify the XML filename of to open.
    **
    ** @returns: No Explain...
    */
    bool            open(const char* filename);


    /* @brief  : Create a new XML document, if the document already exist, the old file will be overwritten.
    ** @params : 
    **        filename: specify the filename of new XML document
    **        rootname: specify the rootname of new XML document
    **
    ** @returns: No Explain...
    */
    bool            open(const char* filename, const char* rootname);


    /* @brief  : Open from xml formated string
    ** @params : 
    **        xmlstring: xml formated string, such as 
    **                   "<peoples><people><name>xxx</name><sex>female</sex></people></peoples>"
    **        length   : the length of xmlstring
    **
    ** @returns: No Explain...
    */
    bool            open(const char* xmlstring, int length);


    /* @brief  : Save document to disk with filename when it was opened
    ** @params : None
    **
    ** @returns: None
    */
    void            save(void) const;


    /* @brief  : Save document to disk with new filename
    ** @params : 
    **           filename: specify the new filename
    **
    ** @returns: None
    */
    void            save(const char* filename) const;


    /* @brief: Close the XML document and release all resource.
    ** @params : None
    **
    ** @returns: None
    */
    void            close(void);


    /* @brief: Is open succeed.
    ** @params : None
    **
    ** @returns: None
    */
    bool            is_open(void) const;


    /* @brief: Get the root element of the document.
    ** @params : None
    **
    ** @returns: the root element of the document
    */
    element&        root(void);


    /* @brief: Get element by XPATH
    ** @params : 
    **          xpath: such as: "/peoples/people/name"
    **          index: the index of the element set, default: the first.
    **
    ** @returns: the element of the document with spacified XPATH 
    */
    element         get_element(const char* xpath, int index = 0) const;


    /* @brief: Get all element by XPATH
    ** @params : 
    **          xpath: such as: "/peoples/people/name"
    **          elements: output parameters, save the all element with specified XPATH
    **
    ** @returns: None
    */
    void            get_element_list(const char* xpath, std::vector<element>& elements) const;


    /* 
    ** @brief: From DOM to String
    ** @params : None.
    **
    ** @returns: XML formated string
    */
    std::string     to_string(void) const;

#if !defined(_USING_TINYXML2)
    /* @brief: A function template, "foreach" all element which have specified XPATH.
    ** @params : 
    **           xpath: specify the xpath to "foreach"
    **           op: foreach operation
    **
    ** @returns: None
    ** @comment(usage):
    **          please see example in the file: xml3c_testapi.cpp 
    */
    template<typename _Operation>
    void            xforeach(const char* xpath, _Operation op) const;

private:
    xml3cXPathObjectPtr _GetXPathObj(const char* xpath) const;
#endif
private:
    static element  _CreateElement(const char* name, xml3cDocPtr = nullptr);

    static element  _CreateComment(const char* content, xml3cDocPtr = nullptr);


private:
    xml3cDocPtr     _Mydoc;
    element         _Myroot;
#if defined(_USING_XERCESC)
    xml3cParser*    _Myparser;
#endif
#if defined(_USING_TINYXML2)
    std::string     _Myfilename;
#endif
}; /* CLASS document */

#ifdef __cxx11
namespace placeholders {

    static auto element = std::placeholders::_1;
};
#else
template<typename _Cty>
class element_pusher
{
public:
    element_pusher(_Cty& container) : _Container(container)
    {
    }

    void operator()(const xml3c::element& elem)
    {
        _Container.push_back(elem);
    }

private:
    element_pusher& operator=(const element_pusher&);

    _Cty& _Container;
};

template<typename _Cty>
class value_pusher
{
public:
    value_pusher(_Cty& container) : _Container(container)
    {
    }

    void operator()(const xml3c::element& elem)
    {
        _Container.push_back(elem.get_value());
    }

private:
    value_pusher& operator=(const value_pusher&);

    _Cty& _Container;
};
template<typename _Cty>
class property_value_pusher
{
public:
    property_value_pusher(_Cty& container, const char* prop) : _Container(container), _Prop(prop)
    {
    }

    void operator()(const xml3c::element& elem)
    {
        _Container.push_back(elem.get_property_value(_Prop));
    }

private:
    property_value_pusher& operator=(const property_value_pusher&);

    const char* _Prop;
    _Cty&       _Container;
};
#endif

}; 
namespace xml3s = xml3c; /* namespace: xml3c */

#include "xml3c_api.inl"

#endif /* _XML3C_API_H_ */
/*
* Copyright (c) 2012-2013 by X.D. Guo  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
**/
