// nsconv standard header, contain several methods for typecast between numeric and string
#ifndef _NSCONV_H_
#define _NSCONV_H_
#include <sstream>
#include <iomanip>
#ifdef _WIN32
#    include <ObjBase.h>
#endif
#include "simpleppdef.h"
#include "simple_ptr.h"
#include <string.h>
#include <vector>
#include "unreal_string.h"

namespace thelib {

namespace nsc {

typedef std::ios_base& (*ios_flag)(std::ios_base&);

template<typename _Nty, typename _Cty, typename _Alloc> inline
    std::basic_string<_Cty, std::char_traits<_Cty>, _Alloc> to_xstring(const _Nty& numeral, ios_flag radix = std::dec)
{
    std::basic_stringstream<_Cty, std::char_traits<_Cty>, _Alloc> swaper;

    swaper << radix << numeral;

    return swaper.str();
}

template<typename _Nty, typename _Cty, typename _Alloc> inline
    std::basic_string<_Cty, std::char_traits<_Cty>, _Alloc>& to_xstring(const _Nty& numeral, std::basic_string<_Cty, std::char_traits<_Cty>, _Alloc>& output, ios_flag radix = std::dec)
{
    std::basic_stringstream<_Cty, std::char_traits<_Cty>, _Alloc> swaper;

    swaper << radix << numeral;
    swaper >> output;

    return output;
}

// convert numeric[char/short/int/long/long long/float/double] to string[std::string/std::wstring].
template<typename _Nty> inline
std::string to_string(const _Nty& numeral, ios_flag radix = std::dec)
{ 
    std::stringstream swaper;

    swaper << radix << numeral;

    return swaper.str();
}

template<typename _Nty> inline
std::string& to_string(const _Nty& numeral, std::string& text, ios_flag radix = std::dec)
{ 
    std::stringstream swaper;

    swaper << radix << numeral;
    swaper >> text;

    return text;
}
template<typename _Nty> inline
std::wstring to_wstring(const _Nty& numeral, ios_flag radix = std::dec)
{ 
    std::wstringstream swaper;

    swaper << radix << numeral;

    return swaper.str();
}

template<typename _Nty> inline
std::wstring& to_wstring(const _Nty& numeral, std::wstring& text, ios_flag radix = std::dec)
{ 
    std::wstringstream swaper;

    swaper << radix << numeral;

    swaper >> text;

    return text;
} 
/*@method@ to_string @method@*/


// convert string[std::string/std::wstring] to numeric[char/short/int/long/long long/float/double].
template<typename _Nty, typename _Cty> inline
_Nty to_numeric(const std::basic_string<_Cty>& text, ios_flag radix = std::dec)
{ 
    _Nty numeral = _Nty();
    std::basic_stringstream<_Cty> swaper;

    swaper << radix << text;
    swaper >> numeral;

    return numeral;
}

template<typename _Nty, typename _Cty> inline
_Nty& to_numeric(const std::basic_string<_Cty>& text, _Nty& numeral, ios_flag radix = std::dec)
{
    std::basic_stringstream<_Cty> swaper;

    swaper << radix << text;
    swaper >> numeral;

    return numeral;
} /*@method@ to_numeric @method@*/

// convert string[NTCS] to numeric[char/short/int/long/long long/float/double].
template<typename _Nty, typename _Cty> inline
_Nty to_numeric(const _Cty* text, ios_flag radix = std::dec)
{ 
    _Nty numeral = _Nty();
    std::basic_stringstream<_Cty> swaper;

    swaper << radix << text;
    swaper >> numeral;

    return numeral;
}

template<typename _Nty, typename _Cty> inline
_Nty& to_numeric(const _Cty* text, _Nty& numeral, ios_flag radix = std::dec)
{
    std::basic_stringstream<_Cty> swaper;

    swaper << radix << text;
    swaper >> numeral;

    return numeral;
} /*@method@ to_numeric @method@*/

inline 
bool _Is_visible_char(int _Char)
{
    return (_Char > 0x20 && _Char < 0x7F);
}

template<typename _Cty> inline
size_t strtrim(_Cty* _Str)
{
    if(NULL == _Str || !*_Str) {
        return 0;
    }

    _Cty* _Ptr = _Str - 1;

    while( !_Is_visible_char(*(++_Ptr)) && *_Ptr ) ;

    _Cty* _First = _Ptr;
    _Cty* _Last = _Ptr;
    if(*_Ptr) {
        while(*(++_Ptr))
        {
            if(_Is_visible_char(*_Ptr)) {
                _Last = _Ptr;
            }
        }
    }

    size_t _Count = _Last - _First + 1;
    if(_Ptr != _Str) {
        ::memmove(_Str, _First, _Count);
        _Str[_Count] = '\0';
    }

    return _Count;
}

template<typename _Cty> inline
int strtrim(_Cty* _Str, int _StrLen)
{
    if(NULL == _Str || !*_Str) {
        return 0;
    }
    _Cty* _Ptr = _Str - 1;

    while( !_Is_visible_char(*(++_Ptr)) && --_StrLen ) ;

    if(_StrLen > 0) {
        while( !_Is_visible_char(_Ptr[_StrLen - 1]) ) --_StrLen ;
        _Ptr[_StrLen] = (_Cty)'\0';
    }

    if(_Ptr != _Str) {
        ::memmove(_Str, _Ptr, _StrLen);
        _Str[_StrLen] = (_Cty)'\0';
    }

    return _StrLen;
}

template<typename _Cty>
std::basic_string<_Cty>& strtrim(std::basic_string<_Cty>& _String)
{
    _String.resize(strtrim(const_cast<_Cty*>(_String.c_str()), _String.length()));
    return _String;
}

#ifdef __cxx11
template<typename _Cty> inline
std::basic_string<_Cty>& strtrim(std::basic_string<_Cty>&& _String)
{
    return strtrim(_String);
}
#endif


/* 
* nsc::split API (override+6)
*/
template<typename _Cty, typename _Fty> inline
void split(const _Cty* s, const _Cty delim, _Fty op)
{
    const _Cty* _Start = s; // the start of every string
    const _Cty* _Ptr = s;   // source string iterator
    while( *_Ptr != '\0' )
    {
        if(delim == *_Ptr && _Ptr != _Start) 
        {
            op(std::basic_string<_Cty>(_Start, _Ptr));
            _Start = _Ptr + 1;
        }
        ++_Ptr;
    }
    if(_Start != _Ptr) {
        op(std::basic_string<_Cty>(_Start, _Ptr));
    }
}

template<typename _Cty, typename _Fty> inline
void split(const _Cty* s, const _Cty* delims, _Fty op)
{
    const _Cty* _Start = s; // the start of every string
    const _Cty* _Ptr = s;   // source string iterator
    size_t      _Lend = strlen(delims);
    while( (_Ptr = strstr(_Ptr, delims)) != nullptr )
    {
        if(_Ptr != _Start) 
        {
            op(std::basic_string<_Cty>(_Start, _Ptr));
            _Start = _Ptr + _Lend;
        }
        _Ptr += _Lend;
    }
    if(*_Start) {
        op(std::basic_string<_Cty>(_Start));
    }
}

template<typename _Fty> inline
void split(const std::string& s, const char* delims, _Fty op)
{
   size_t last = 0;
   size_t index = s.find_first_of(delims, last);
   while (index != std::string::npos)
   {
      op(s.substr(last, index - last));
      last = index + 1;
      index = s.find_first_of(delims, last);
   }
   if (index > last)
   {
      op(s.substr(last, index - last));
   }
}

template<typename _Cty> inline
std::vector<std::string>& split(const _Cty* s, const _Cty delim, std::vector<std::basic_string<_Cty> >& output)
{
    nsc::split(s, delim, [&output](const std::basic_string<_Cty>& value)->void{
        output.push_back(value);
    });
    return output;
}

template<typename _Cty> inline
std::vector<std::string>& split(const _Cty* s, const _Cty* delims, std::vector<std::basic_string<_Cty> >& output)
{
    nsc::split(s, delims, [&output](const std::basic_string<_Cty>& value)->void{
        output.push_back(value);
    });
    return output;
}

inline
std::vector<std::string>& split(const std::string& s, const char* delim, std::vector< std::string >& output)
{
    nsc::split(s, delim, [&output](const std::string& value)->void {
        output.push_back(value);
    });
    return output;
}

inline
std::string& replace(std::string& string, const std::string& replaced_key, const std::string& replacing_key) 
{
    std::string::size_type pos = 0;
    while( (pos = string.find(replaced_key, pos)) != std::string::npos )
    {
        (void)string.replace(pos, replaced_key.length(), replacing_key);
        pos += replacing_key.length();
    }
    return string;
}

inline
std::string& replace(std::string&& string, const std::string& replaced_key, const std::string& replacing_key) 
{
    return replace(string, replaced_key, replacing_key);
}

inline
std::string rsubstr(const std::string& string, size_t off)
{
    if(string.length() >= off)
    {
        return string.substr(string.length() - off);
    }
    return "";
}

template<typename _Cty> inline
void strtoupper(_Cty* source)
{
    while( *source != '\0' )
    {
        *source = toupper(*source);
        ++source;
    }
}

template<typename _Cty> inline
void strtolower(_Cty* source)
{
    while( *source != '\0' )
    {
        *source = tolower(*source);
        ++source;
    }
}

#ifdef _WIN32

enum code_page {
    code_page_acp = CP_ACP,
    code_page_utf8 = CP_UTF8
};

inline managed_cstring transcode(const wchar_t* wcb, code_page cp = code_page_acp)
{
    int cch = WideCharToMultiByte(cp, 0, wcb, -1, NULL, 0, NULL, NULL);
    char* target = (char*)malloc(cch);
    WideCharToMultiByte(cp, 0, wcb, -1, target, cch, NULL, NULL);
    return  managed_cstring(target);
}

inline managed_wcstring transcode(const char* mcb, code_page cp = code_page_acp)
{
    int cch = MultiByteToWideChar(cp, 0, mcb, -1, NULL, 0);
    wchar_t* target = (wchar_t*)malloc(cch * sizeof(wchar_t));
    MultiByteToWideChar(cp, 0, mcb, -1, target, cch);
    return managed_wcstring(target);
}

inline managed_cstring to_utf8(const char* ascii_text)
{
    return transcode(transcode(ascii_text).c_str(), code_page_utf8);
}

inline managed_cstring to_ascii(const char* utf8_text)
{
    return transcode(transcode(utf8_text, code_page_utf8).c_str());
}

/* utils GUID
**
*/
#include "xxbswap.h"

#define GUID_BUF_SIZE ( sizeof(_GUID) * 2 + sizeof(void*) )

inline void create_guid(LPTSTR outs)
{
    _GUID guid;
    CoCreateGuid(&guid);

    wsprintf(outs, TEXT("%08X-%04X-%04X-%04X-%04X%08X"), 
        guid.Data1,
        guid.Data2,
        guid.Data3,
        __bswap16(*(reinterpret_cast<unsigned __int16*>(guid.Data4))),
        __bswap16(*(reinterpret_cast<unsigned __int16*>(guid.Data4 + 2))),
        __bswap32(*(reinterpret_cast<unsigned __int32*>(guid.Data4 + 4)))
        );
}

inline void create_guid_v2(LPTSTR outs)
{
    _GUID guid;
    CoCreateGuid(&guid);

    wsprintf(outs, TEXT("%08X%04X%04X%016I64X"), 
        guid.Data1,
        guid.Data2,
        guid.Data3,
        __bswap64(*(reinterpret_cast<unsigned __int64*>(guid.Data4)))
        );
}

template<typename _Cty> inline
std::basic_string<_Cty> create_guid(void)
{
    std::basic_stringstream<_Cty> swaper;

    _GUID g;
    CoCreateGuid(&g);

    swaper << std::hex 
        << std::setw(8) << std::setfill(_Cty('0')) << g.Data1
        << _Cty('-')
        << std::setw(4) << std::setfill(_Cty('0')) << g.Data2
        << _Cty('-') 
        << std::setw(4) << std::setfill(_Cty('0')) << g.Data3
        << _Cty('-')
        << std::setw(8) << std::setfill(_Cty('0')) << *(reinterpret_cast<unsigned __int32*>(g.Data4))
        << std::setw(4) << std::setfill(_Cty('0')) << *(reinterpret_cast<unsigned __int16*>(g.Data4)+2);

    return swaper.str();
}

template<typename _Cty> inline
void create_guid(std::basic_string<_Cty>& outs)
{
    std::basic_stringstream<_Cty> swaper;

    _GUID g;
    CoCreateGuid(&g);

    swaper << std::hex 
        << std::setw(8) << std::setfill(_Cty('0')) << g.Data1
        << _Cty('-')
        << std::setw(4) << std::setfill(_Cty('0')) << g.Data2
        << _Cty('-') 
        << std::setw(4) << std::setfill(_Cty('0')) << g.Data3
        << _Cty('-')
        << std::setw(8) << std::setfill(_Cty('0')) << *(reinterpret_cast<unsigned __int32*>(g.Data4))
        << std::setw(4) << std::setfill(_Cty('0')) << *(reinterpret_cast<unsigned __int16*>(g.Data4)+2);

    outs = swaper.str();
}
#endif


}; // namespace: simplepp_1_13_201307::nsc
}; // namespace: simplepp_1_13_201307

#endif /* _NSCONV_ */
/*
* Copyright (c) 2012-2013 by X.D. Guo  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
**/

