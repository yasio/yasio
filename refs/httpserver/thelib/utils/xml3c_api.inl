#ifndef _XML3C_API_INL_
#define _XML3C_API_INL_

#if defined(_XML3C_API_H_)
namespace xml3c {

template<typename _Ty>
_Ty element::get_value(const _Ty& default_value) const
{
    return nsc::to_numeric<_Ty>(this->get_value(nsc::to_string(default_value)));
}

template<typename _Ty>
_Ty element::get_property_value(const char* name, const _Ty& default_value) const
{
    return nsc::to_numeric<_Ty>(this->get_property_value(name, nsc::to_string(default_value)));
}

template<typename _Ty>
void element::set_value(const _Ty& value)
{
    this->set_value(nsc::to_string(value));
}

template<typename _Ty>
void element::set_property_value(const char* name, const _Ty& value)
{
    if(_Mynode != nullptr) {
        __xml3c_set_propval(_Mynode, name, nsc::to_string(value).c_str());
    }
    //this->set_property_value(name, nsc::to_string<char>(value));
}

template<typename _Handler>
void element::cforeach(_Handler handler) const
{
    xml3cNodePtr ptr = _Mynode;
    __xml3cts_algo(ptr, 
        __xml3c_child_endix, 
        __xml3c_next_endix,
        __xml3c_is_element(ptr),
        handler(ptr)
        );
}

template<typename _Handler>
void element::cforeach(const char* name, _Handler handler) const
{
    xml3cNodePtr ptr = _Mynode;
    __xml3cts_algo(ptr, 
        __xml3c_child_endix, 
        __xml3c_next_endix,
        __xml3c_is_element(ptr) && name == ((element)ptr).get_name(),
        handler(ptr)
        );
}

#if !defined(_USING_TINYXML2)
template<typename _Handler>
void document::xforeach(const char* xpath, _Handler handler) const
{
#if defined(_USING_LIBXML2)
    simple_ptr< xmlXPathObject, xmlXPathFreeObject> xpath_obj( _GetXPathObj(xpath) );

    if(nullptr == xpath_obj)
    {
        return; //  internal error;
    }

    for(int index = 0; index < xpath_obj->nodesetval->nodeNr; ++index)
    {
        handler(xpath_obj->nodesetval->nodeTab[index]);
    }

#else
#if XERCES_VERSION_MAJOR >= 3
    xml3cXPathObjectPtr xpath_obj = _GetXPathObj(xpath);
    if(nullptr == xpath_obj)
    {
        return; //  internal error;
    }
    size_t index = 0;
    while(xpath_obj->snapshotItem(index++))
    {
        handler(xpath_obj->getNodeValue());
    }
    xpath_obj->release();
#else
    throw std::logic_error("It's just supported by xerces-c which has 3.0.0 or more version for xml3c using xpath to operate xml!");
#endif
#endif
}
#endif

};
#endif

#endif /* _XML3C_API_INL_ */
/*
* Copyright (c) 2012-2019 by X.D. Guo  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
**/
