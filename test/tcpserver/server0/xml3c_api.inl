#ifndef _XML3C_API_INL_
#define _XML3C_API_INL_

#if defined(_XML3C_API_H_)
namespace xml3c {

template<typename _Ty>
_Ty element::get_value(void) const
{
    if(sizeof(_Ty) > sizeof(char)) {
        return nsc::to_numeric<_Ty>(this->get_value());
    }
    return (_Ty)nsc::to_numeric<uint32_t>(this->get_value());
}

template<typename _Ty>
bool element::get_value(_Ty& output) const
{
    std::string val;
    bool ret = this->get_value(val);

    if(sizeof(_Ty) > sizeof(char)) {
        nsc::to_numeric(output, val);
    }
    else { // for char or unsigned char
        int tmpval = (int)output;
        nsc::to_numeric(tmpval, val);
        output = (_Ty)tmpval;
    }

    return ret;
}

template<typename _Ty>
_Ty element::get_property_value(const char* name) const
{
    if(sizeof(_Ty) > sizeof(char)) {
        return nsc::to_numeric<_Ty>(this->get_property_value(name));
    }
    return (_Ty)nsc::to_numeric<uint32_t>(this->get_property_value(name));
}

template<typename _Ty>
bool element::get_property_value(const char* name, _Ty& output) const
{
    std::string val;
    bool ret = this->get_property_value(name, val);

    if(sizeof(_Ty) > sizeof(char)) {
        nsc::to_numeric(output, val);
    }
    else { // for char or unsigned char
        int tmpval = (int)output;
        nsc::to_numeric(tmpval, val);
        output = (_Ty)tmpval;
    }

    return ret;
}

template<typename _Ty>
void element::set_value(const _Ty& value)
{
    this->set_value(nsc::to_string<char>(value));
}

template<typename _Ty>
void element::set_property_value(const char* name, const _Ty& value)
{
    this->set_property_value(name, nsc::to_string<char>(value));
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

};
#endif

#endif /* _XML3C_API_INL_ */
/*
* Copyright (c) 2012-2019 by X.D. Guo  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
**/
