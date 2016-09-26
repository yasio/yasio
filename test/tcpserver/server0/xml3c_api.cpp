#include <cstdio>
#include <vector>
#include "xml3c_api.h"

#if defined(_USING_XERCESC)
#ifdef _WIN32
#define XML3C_URI_PREFIX "file:///"
#else
#define XML3C_URI_PREFIX "file://"
#endif
using namespace xercesc;
#endif
using namespace xml3c;

#if defined(_USING_XERCESC)
template<typename _Ch>
struct _xml3cstr_raii
{
    _xml3cstr_raii(_Ch* value) : _Myval(value)
    {
    }
    ~_xml3cstr_raii(void)
    {
        XMLString::release(&_Myval);
    }
    _Ch* _Myval;
};

std::string _xml3c_transcode(const XMLCh* source)
{
    _xml3cstr_raii<char> msg = XMLString::transcode(source);
    if(msg._Myval != nullptr)
    {
        return msg._Myval;
    }
    return "";
}

std::basic_string<XMLCh> _xml3c_transcode(const char* source)
{
    _xml3cstr_raii<XMLCh> msg = XMLString::transcode(source);
    if(msg._Myval != nullptr)
    {
        return msg._Myval;
    }
    return std::basic_string<XMLCh>();
}

bool _xml3c_transcode(const XMLCh* source, std::string& output)
{
    char* msg = XMLString::transcode(source);
    if(msg != nullptr)
    {
        output.assign(msg);
        XMLString::release(&msg);
        return true;
    }
    return false;
}

bool _xml3c_transcode(const char* source, std::basic_string<XMLCh> output)
{
    XMLCh* msg = XMLString::transcode(source);
    if(msg != nullptr)
    {
        output.assign(msg);
        XMLString::release(&msg);
        return true;
    }
    return false;
}

void _xml3c_serialize(xml3cNoderef _Mynode, XMLFormatTarget& _Target, bool _Formated = true)
{
    static const XMLCh tempLS[] = {chLatin_L, chLatin_S, chNull};

#if XERCES_VERSION_MAJOR >= 3
    DOMImplementation* impl = DOMImplementationRegistry::getDOMImplementation(tempLS);
    DOMLSSerializer*   writer = ((DOMImplementationLS*)impl)->createLSSerializer();
    DOMLSOutput*       output = ((DOMImplementationLS*)impl)->createLSOutput();

    output->setByteStream(&_Target);

    writer->getDomConfig()->setParameter(XMLUni::fgDOMWRTFormatPrettyPrint, _Formated);

    writer->write(&_Mynode, output);

    output->release();
    writer->release();
#else
    DOMImplementation* impl = DOMImplementationRegistry::getDOMImplementation(tempLS);
    DOMWriter* writer = ((DOMImplementationLS*)impl)->createDOMWriter();

    writer->setFeature((XMLCh*)L"format-pretty-print", _Formated);

    writer->writeNode(&_Target, _Mynode);

    writer->release();
#endif
}
#endif

element::element(xml3cNodePtr _Ptr) : _Mynode(_Ptr)
{
}

element element::clone(void) const
{
    if(_Mynode != nullptr)
    {
        return __xml3c_copy_node(_Mynode);
    }
    return nullptr;
}

std::string element::get_name(void) const
{
    if(_Mynode != nullptr) {
        return __xml3c_name_of(_Mynode);
    }
    return "";
}

std::string element::get_value(void) const
{
    std::string output;
    this->get_value(output);
#ifdef __cxx11
    return std::move(output);
#else
    return output;
#endif
}

bool element::get_value(std::string& output) const
{
    if(_Mynode != nullptr) {
#if defined(_USING_LIBXML2)
        pod_ptr<xmlChar>::type value(xmlNodeGetContent(_Mynode));
        //size_t n = nsc::strtrim(value.get());
        output.assign((const char*) value.get()/*, n*/);
        return true;
#else
        return _xml3c_transcode(_Mynode->getTextContent(), output);
        //if(_xml3c_transcode(_Mynode->getTextContent(), output))
        //{
        //    nsc::strtrim(output);
        //    return true;
        //}
#endif
    }
    return false;
}

std::string element::get_property_value(const char* name) const
{
    std::string output;
    this->get_property_value(name, output);
#ifdef __cxx11
    return std::move(output);
#else
    return output;
#endif
}

bool element::get_property_value(const char* name, std::string& output) const
{
    if(_Mynode != nullptr)
    {
#if defined(_USING_LIBXML2)
        pod_ptr<xmlChar>::type value (xmlGetProp(_Mynode, BAD_CAST name));
        if(value != nullptr)
        {
            // size_t n = nsc::strtrim(value.get());
            output.assign((const char*) value.get()/*, n*/);
            return true;
        } 
        return false;
#else 
        return _xml3c_transcode(((DOMElement*)_Mynode)->getAttribute(_xml3c_transcode(name).c_str()), output);
        //if(_xml3c_transcode(((DOMElement*)_Mynode)->getAttribute(_xml3c_transcode(name).c_str()), output))
        //{
        //    nsc::strtrim(output);
        //    return true;
        //}
#endif
    }
    return false;
}

element element::get_parent(void) const
{
    if(_Mynode != nullptr)
    {
        return _Mynode->__xml3c_parent_endix;
    }
    return nullptr;
}

element element::get_prev_sibling(void) const
{
    xml3cNodePtr ptr = _Mynode;
    __xml3cts_algo(ptr, 
        __xml3c_prev_endix, 
        __xml3c_prev_endix,
        __xml3c_is_element(ptr),
        break
        );
    return ptr;
}

element element::get_next_sibling(void) const
{
    xml3cNodePtr ptr = _Mynode;
    __xml3cts_algo(ptr, 
        __xml3c_next_endix, 
        __xml3c_next_endix,
        __xml3c_is_element(ptr),
        break
        );
    return ptr;
}

element element::get_child(int index) const
{
    xml3cNodePtr ptr = _Mynode;
    __xml3cts_algo(ptr, 
        __xml3c_child_endix, 
        __xml3c_next_endix,
        __xml3c_is_element(ptr) && (0 == index--),
        break
        );
    return ptr;
}

element element::get_child(const char* name, int index) const
{
    xml3cNodePtr ptr = _Mynode;
    __xml3cts_algo(ptr, 
        __xml3c_child_endix, 
        __xml3c_next_endix,
        __xml3c_is_element(ptr) && name == ((element)ptr).get_name() && (0 == index--),
        break
        );
    return ptr;
}

void element::get_children(std::vector<element>& children) const
{
#ifdef __cxx11
    this->cforeach(std::bind(container_helper::push_back_to<std::vector<element> >, placeholders::element, std::reference_wrapper<std::vector<element>>(children)));
#else
    element_pusher<std::vector<element> > pusher(children);
    this->cforeach(pusher);
#endif
}

void element::get_children(const char* name, std::vector<element>& children) const
{
#ifdef __cxx11
    this->cforeach(name, std::bind(container_helper::push_back_to<std::vector<element> >, placeholders::element, std::reference_wrapper<std::vector<element>>(children)));
#else
    element_pusher<std::vector<element> > pusher(children);
    this->cforeach(name, pusher);
#endif
}

void element::set_value(const char* value)
{
    if(_Mynode != nullptr)
    {
        __xml3c_set_node_content(_Mynode, value);
    }
}

void element::set_value(const std::string& value)
{
    this->set_value(value.c_str());
}

void element::set_property_value(const char* name, const char* value) const
{
    if(_Mynode != nullptr) {
        __xml3c_set_propval(_Mynode, name, value);
    }
}

void element::set_property_value(const char* name, const std::string& value) const
{
    this->set_property_value(name, value.c_str());
}

element element::add_child(const element& e) const
{
    if(_Mynode != nullptr && e._Mynode != nullptr) {
        __xml3c_add_child(_Mynode, e._Mynode);
    }
    return nullptr;
}

element element::add_child(const char* name, const char* value) const
{
    if(_Mynode != nullptr)
    {
        element temp = this->add_child(document::_CreateElement(name, __xml3c_owner_of(_Mynode)));
        temp.set_value(value);
        return temp;
    }
    return nullptr;
}

void element::remove_child(int index)
{
    this->get_child(index).remove_self();
}

void element::remove_child(const char* name, int index)
{
    this->get_child(name, index).remove_self();
}

void element::remove_children(void)
{
    std::vector<element> children;
    this->get_children(children);
    std::for_each(
        children.begin(),
        children.end(),
        std::mem_fun_ref(&element::remove_self)
        );
}

void element::remove_children(const char* name)
{
    std::vector<element> children;
    this->get_children(name, children);
    std::for_each(
        children.begin(),
        children.end(),
        std::mem_fun_ref(&element::remove_self)
        );
}

void element::remove_self(void)
{
    if(_Mynode != nullptr) {
        xml3cRemoveNode(_Mynode);
        _Mynode = nullptr;
    }
}

std::string element::to_string(void) const
{
    if(_Mynode != nullptr) {
#if defined(_USING_LIBXML2)
        simple_ptr<xmlBuffer, xmlBufferFree> buf(xmlBufferCreate());
        xmlNodeDump(buf, _Mynode->doc, _Mynode, 1, 2);
        return std::string((const char*) buf->content);
#else
        MemBufFormatTarget target;
        _xml3c_serialize(*_Mynode, target, false);
        return std::string((const char*)target.getRawBuffer());
#endif
    }
    return "";
}

element::operator xml3cNodePtr&(void)
{
    return _Mynode;
}

element::operator xml3cNodePtr const&(void) const
{
    return _Mynode;
}

/*----------------------------- friendly division line ----------------------------------------*/

document::document(void) 
    : _Mydoc()
    , _Myroot()
#if defined(_USING_XERCESC)
    , _Myparser()
#endif
{
}

document::document(const char* filename)   
    : _Mydoc()
    , _Myroot()
#if defined(_USING_XERCESC)
    , _Myparser()
#endif
{
    this->open(filename);
}

document::document(const char* filename, const char* rootname)
    : _Mydoc()
    , _Myroot()
#if defined(_USING_XERCESC)
    , _Myparser()
#endif
{
    this->open(filename, rootname);
}

document::document(const char* memory, int length)
    : _Mydoc()
    , _Myroot()
#if defined(_USING_XERCESC)
    , _Myparser()
#endif
{
    this->open(memory, length);
}

document::~document(void)
{
    this->close();
}

bool document::open(const char* filename)
{
    if(nullptr == _Myroot) {
#if defined(_USING_LIBXML2)
        xmlKeepBlanksDefault(0);
        _Mydoc = xmlReadFile(filename, "UTF-8", XML_PARSE_RECOVER | XML_PARSE_NOBLANKS | XML_PARSE_NODICT);
        _Myroot = xmlDocGetRootElement(_Mydoc);
#else 
        _Myparser = new XercesDOMParser;
        _Myparser->setValidationScheme(XercesDOMParser::Val_Always);
        try {
            _Myparser->parse(filename);
            _Mydoc = _Myparser->getDocument();
            if(_Mydoc != nullptr)
            {
                _Myroot = _Mydoc->getDocumentElement();
            }
        }
        catch(...)
        {
            ;
        }

        if(nullptr == _Myroot) {
            delete _Myparser;
            _Myparser = nullptr;
            _Mydoc = nullptr;
        }
#endif
    }
    return is_open();
}

bool document::open(const char* filename, const char* rootname)
{
    if(nullptr == _Myroot._Mynode) {
#if defined(_USING_LIBXML2)
        _Mydoc = xmlNewDoc(BAD_CAST "1.0");
        _Myroot._Mynode = xmlNewNode(NULL, BAD_CAST rootname);
        xmlDocSetRootElement(_Mydoc, _Myroot._Mynode);

        xmlChar*& ref = const_cast<xmlChar*&>(_Mydoc->URL);
        size_t url_len = strlen(filename);
        ref = (xmlChar*)malloc( url_len + 1);
        memcpy(ref, BAD_CAST filename, url_len + 1);
#else
        _Mydoc = DOMImplementationRegistry::getDOMImplementation(nullptr)->createDocument();
        _Myroot._Mynode = _CreateElement(rootname, _Mydoc);
        _Mydoc->setDocumentURI(_xml3c_transcode(filename).c_str());
        _Mydoc->appendChild(_Myroot);
#endif
    }
    return is_open();
}

bool document::open(const char* membuf, int length)
{
    if(nullptr == _Myroot) {
#if defined(_USING_LIBXML2)
        xmlKeepBlanksDefault(0);
        _Mydoc = xmlReadMemory(membuf, length, "memory.libxml3c.xml", "UTF-8", 1);
        _Myroot = xmlDocGetRootElement(_Mydoc);
#else 
        MemBufInputSource* memBufIS = nullptr;
        _Myparser = new XercesDOMParser;
        _Myparser->setValidationScheme(XercesDOMParser::Val_Auto);
        try {
            memBufIS = new MemBufInputSource(
                (const XMLByte*)membuf
                , (XMLSize_t)length
                , (const char *const)nullptr);

            _Myparser->parse(*memBufIS);

            _Mydoc = _Myparser->getDocument();
            if(_Mydoc != nullptr)
            {
                _Myroot = _Mydoc->getDocumentElement();
            }
        }
        catch(...)
        {
            ;
        }

        delete memBufIS;

        if(nullptr == _Myroot) {
            delete _Myparser;
            _Myparser = nullptr;
            _Mydoc = nullptr;
        }
#endif
    }
    return is_open();
}

void document::save(void) const
{
#if defined(_USING_LIBXML2)
    this->save((const char*)_Mydoc->URL);
#else
    std::string uri;
    _xml3c_transcode(_Mydoc->getDocumentURI(), uri);
    if(strstr(uri.c_str(), XML3C_URI_PREFIX) != nullptr)
    {
        this->save(uri.c_str() + sizeof(XML3C_URI_PREFIX) - 1);
    }
    else {
        this->save(uri.c_str());
    }
#endif
}

void document::save(const char* filename) const
{
    if( is_open () ) {
#if defined(_USING_LIBXML2)
        xmlSaveFormatFile(filename, this->_Mydoc, 1); 
#else
        LocalFileFormatTarget target(filename);
        _xml3c_serialize(*_Mydoc, target);
#endif
    }
}

void document::close(void)
{
    if(is_open()) {
#if defined(_USING_LIBXML2)
        _Myroot = nullptr;
        xmlFreeDoc(_Mydoc);
        _Mydoc = nullptr;
#else
        if(_Myparser != nullptr)
        {
            _Myroot._Mynode = nullptr;
            _Mydoc = nullptr;
            delete _Myparser;
            _Myparser = nullptr;
        }
        else {
            _Myroot._Mynode = nullptr;
            _Mydoc->release();
            _Mydoc = nullptr;
        }
#endif
    }
}

bool document::is_open(void) const
{
    return _Myroot != nullptr;
}

element& document::root(void)
{
    return _Myroot;
}

element document::get_element(const char* xpath, int index) const
{
#if defined(_USING_LIBXML2)
    simple_ptr< xmlXPathObject, xmlXPathFreeObject> xpath_obj( _GetXPathObj(xpath) );

    if(xpath_obj != nullptr && index < xpath_obj->nodesetval->nodeNr)
    {
        return xpath_obj->nodesetval->nodeTab[index];
    }
    return nullptr;
#else
#if XERCES_VERSION_MAJOR >= 3
    element result;
    xml3cXPathObjectPtr xpath_obj = _GetXPathObj(xpath);
    if(xpath_obj != nullptr && xpath_obj->snapshotItem(index))
    {
        return xpath_obj->getNodeValue();
    }
    xpath_obj->release();
    return result;
#else
    throw std::logic_error("It's just supported by xerces-c which has 3.0.0 or more version for xml3c using xpath to operate xml!");
#endif
#endif
}

void document::get_element_list(const char* xpath, std::vector<element>& elements) const
{
#ifdef __cxx11
    this->xforeach(xpath, std::bind(container_helper::push_back_to<std::vector<element> >, placeholders::element, std::reference_wrapper<std::vector<element>>(elements)));
#else
    element_pusher<std::vector<element> > pusher(elements);
    this->xforeach(xpath, pusher);
#endif
}

std::string document::to_string(void) const
{
    if(is_open())
    {
#if defined(_USING_LIBXML2)
        pod_ptr<xmlChar>::type buf;
        xmlDocDumpMemory(_Mydoc, &buf, NULL);
        return std::string((const char*) buf.get());
#else
        return ( (element)_Mydoc ).to_string();
#endif
    }
    return "";
}

element document::_CreateElement(const char* name, xml3cDocPtr doc)
{

#ifdef _USING_LIBXML2
    (void)doc;
    return xmlNewNode(nullptr, BAD_CAST name);  
#else
    if(doc != nullptr) {
        return doc->createElement(_xml3c_transcode(name).c_str());
    }
    return nullptr;
#endif
}

element document::_CreateComment(const char* content, xml3cDocPtr doc)
{
#if defined(_USING_LIBXML2)
    (void)doc;
    return xmlNewComment(BAD_CAST content);
#else
    if(doc != nullptr)
    {
        return doc->createComment(_xml3c_transcode(content).c_str());
    }
    return nullptr;
#endif
}

xml3cXPathObjectPtr document::_GetXPathObj(const char* xpath) const
{
    if(xpath != nullptr && _Mydoc != nullptr)
    {
#if defined(_USING_LIBXML2)
        simple_ptr<xmlXPathContext, xmlXPathFreeContext> ctx(xmlXPathNewContext(_Mydoc));
        return xmlXPathEvalExpression(BAD_CAST xpath, ctx);
#else
        return (DOMXPathResult*)_Mydoc->evaluate(_xml3c_transcode(xpath).c_str(), 
            _Mydoc->getDocumentElement(), 
            nullptr, 
            DOMXPathResult::ORDERED_NODE_SNAPSHOT_TYPE, 
            nullptr
            ); 
#endif
    }
    return nullptr;
}

/*----------------------------- friendly division line ----------------------------------------*/

void xml3c::initialize(void)
{
#if defined(_USING_LIBXML2)
    xmlInitParser();
#else
    try {
        XMLPlatformUtils::Initialize();
    }
    catch (const XMLException& toCatch) {
        char *pMessage = XMLString::transcode(toCatch.getMessage());
        fprintf(stderr, "xml3c_api3_1: Error during XMLPlatformUtils::Initialize(). \n"
            "  Message is: %s\n", pMessage);
        XMLString::release(&pMessage);
        return;
    }
#endif
}
void xml3c::cleanup(void)
{
#if defined(_USING_LIBXML2)
    xmlCleanupParser();
#else
    XMLPlatformUtils::Terminate();
#endif
}


