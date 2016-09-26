// unreal_string.h
#ifndef _UNREAL_STRING_H_
#define _UNREAL_STRING_H_

#include <ostream>
#include <string>
#include <memory>
#include <vector>
#include "xxfree.h"

namespace purelib {
    using namespace gc;

#define ULS_ELEM_SHIFT (sizeof(_Elem) >> 1)

    namespace native_utils {
        template<typename _Elem> inline
            size_t strlen(const _Elem* _Str)
        {
            const _Elem* pos = _Str;

            while (*(pos++));

            return (pos - _Str - 1);
        }

        template<typename _Elem> inline
            void reversesb(_Elem* _Str)
        {
            size_t start = 0;
            size_t last = strlen(_Str) - 1;
            while (start != last)
            {
                if (_Str[start] != _Str[last])
                {
                    std::swap(_Str[start], _Str[last]);
                }
                ++start, --last;
            }
        }

        template<typename _Elem> inline
            void reversesb(_Elem* _Str, size_t _Count)
        {
            size_t start = 0;
            size_t last = _Count - 1;
            while (start != last)
            {
                if (_Str[start] != _Str[last])
                {
                    std::swap(_Str[start], _Str[last]);
                }
                ++start, --last;
            }
        }

        template<typename _Elem, size_t _Size> inline
            void reversesba(_Elem(&_Str)[_Size])
        {
            size_t start = 0;
            size_t last = _Size - 2;
            while (start != last)
            {
                //  const char* type = typeid(_Str[start]).name();
                if (_Str[start] != _Str[last])
                {
                    std::swap(_Str[start], _Str[last]);
                }
                ++start, --last;
            }
        }

        template<typename _Elem> inline
            int strcmp(const _Elem* _Str1, const _Elem* _Str2)
        {
            int ret = 0;

            while (!(ret = (unsigned int)*_Str1 - (unsigned int)*_Str2) && *_Str2)
                ++_Str1, ++_Str2;

            return ret;
        }

        template<typename _Elem> inline
            _Elem* strcpy(_Elem* _DstStr, const _Elem* _SrcStr)
        {
            _Elem* _Rp = _DstStr;

            while ((*(_DstStr++) = *(_SrcStr++)));

            return _Rp;
        }
        template<typename _Elem> inline
            _Elem* strcpy_s(_Elem* _DstStr, size_t _DstSize, const _Elem* _SrcStr)
        {
            if (_DstSize <= (strlen(_SrcStr)))
            {
                return 0;
            }

            return strcpy(_DstStr, _SrcStr);
        }
        template<typename _Elem, size_t _DstSize> inline
            _Elem* strcpy_s(_Elem(&_DstStr)[_DstSize], const _Elem* _SrcStr)
        {
            return strcpy_s(_DstStr, _DstSize, _SrcStr);
        }


        template<typename _Elem> inline
            _Elem* strncpy(_Elem* _DstStr, const _Elem* _SrcStr, size_t _Count)
        {
            _Elem* _Rp = _DstStr;

            while (((_DstStr - _Rp) < _Count) &&
                (*(_DstStr++) = *(_SrcStr++)));

            *_DstStr = (_Elem)0;

            return _Rp;
        }

        template<typename _Elem> inline
            _Elem* strncpy_s(_Elem* _DstStr, size_t _DstSize, const _Elem* _SrcStr, size_t _Count)
        {
            if (_DstSize <= _Count)
            {
                return 0;
            }

            return strncpy(_DstStr, _SrcStr, _Count);
        }

        template<typename _Elem, size_t _DstSize> inline
            _Elem* strncpy_s(_Elem(&_DstStr)[_DstSize], const _Elem* _SrcStr, size_t _Count)
        {
            return strncpy_s(_DstStr, _DstSize, _SrcStr, _Count);
        }

        template<typename _Elem> inline
            _Elem* strcat(_Elem* _DstStr, const _Elem* _SrcStr)
        {
            _Elem * _Rp = _DstStr;

            while (*(_DstStr)) ++_DstStr;

            while ((*(_DstStr++) = *(_SrcStr++)));

            return (_Rp);
        }

        template<typename _Elem> inline
            _Elem* strcat_s(_Elem* _DstStr, size_t _DstSize, const _Elem* _SrcStr)
        {
            if (_DstSize <= strlen(_DstStr) + strlen(_SrcStr))
            {
                return 0;
            }

            return strcat(_DstStr, _SrcStr);
        }

        template<typename _Elem, size_t _DstSize> inline
            _Elem* strcat_s(_Elem* _DstStr, const _Elem* _SrcStr)
        {
            return strcat_s(_DstStr, _DstSize, _SrcStr);
        }

        template<typename _Elem> inline
            _Elem toupper(_Elem ch)
        {
            return ((ch >= 'a' && ch <= 'z') ? (ch - 0x20) : ch);
        }

        template<typename _Elem> inline
            _Elem tolower(_Elem ch)
        {
            return ((ch >= 'A' && ch <= 'Z') ? (ch + 0x20) : ch);
        }

        template<typename _Elem> inline
            void strtoupper_noncopy(_Elem* _DstStr)
        {
            while (*_DstStr != 0x0)
            {
                if (*_DstStr >= 'a' && *_DstStr <= 'z')
                {
                    *_DstStr -= 0x20;
                }
                ++_DstStr;
            }
        }

        template<typename _Elem> inline
            void strtolower_noncopy(_Elem* _DstStr)
        {
            while (*_DstStr != 0x0)
            {
                if (*_DstStr >= 'A' && *_DstStr <= 'Z')
                {
                    *_DstStr += 0x20;
                }
                ++_DstStr;
            }
        }

    };

    // TEMPLATE CLASS unreal_string forward declare
    template<typename _Elem,
        typename _Cleaner = pseudo_cleaner<_Elem> >
    class unreal_string;

    typedef unreal_string<char, pod_cleaner<char> >          managed_cstring;
    typedef unreal_string<wchar_t, pod_cleaner<wchar_t> >    managed_wcstring;
    typedef unreal_string<char, array_cleaner<char> >        managed_string;
    typedef unreal_string<wchar_t, array_cleaner<wchar_t> >  managed_wstring;
    typedef unreal_string<char, pseudo_cleaner<char> >       unmanaged_string;
    typedef unreal_string<wchar_t, pseudo_cleaner<wchar_t> > unmanaged_wstring;

    template<typename _Elem,
        typename _Cleaner>
    class unreal_string
    {
        typedef unreal_string<_Elem, _Cleaner> _Myt;

    public:
        unreal_string(void)
        {
            _Tidy();
        }

        unreal_string(const _Elem* _Ptr)
        {
            _Tidy();
            this->assign(_Ptr);
        }

        template<size_t _Size>
        unreal_string(const _Elem(&_Ptr)[_Size])
        {
            _Tidy();
            this->assign(_Ptr, _Size);
        }

        unreal_string(const _Elem* _Ptr, size_t _Length)
        {
            _Tidy();
            this->assign(_Ptr, _Length);
        }

        unreal_string(const _Elem* _First, const _Elem* _Last)
        {
            _Tidy();
            this->assign(_First, _Last);
        }

        unreal_string(_Elem* _Ptr)
        {
            _Tidy();
            this->assign(_Ptr);
        }

        unreal_string(_Elem* _Ptr, size_t _Length)
        {
            _Tidy();
            this->assign(_Ptr, _Length);
        }

        unreal_string(_Elem* _First, _Elem* _Last)
        {
            _Tidy();
            this->assign(_First, _Last);
        }

        unreal_string(const std::basic_string<_Elem>& _Right)
        {
            _Tidy();
            this->assign(_Right);
        }

#if defined(_AFX) || defined(__CSTRINGT_H__)

#if defined(_AFXDLL)
#define _ULS_StrTraitMFC StrTraitMFC_DLL
#else
#define _ULS_StrTraitMFC StrTraitMFC
#endif

        unreal_string(const ATL::CStringT<_Elem, _ULS_StrTraitMFC< _Elem >>& _Right)
        {
            _Tidy();
            this->assign(_Right);
        }

        _Myt& assign(const ATL::CStringT<_Elem, _ULS_StrTraitMFC< _Elem >>& _Right)
        {
            _Tidy();
            static_assert(!_Cleaner::value, "atl-string can't assign to a managed unreal_string");

            this->_Bx._Const_Ptr = _Right.GetString();
            this->_Mysize = _Right.GetLength();
            return *this;
        }
#endif

        unreal_string(const std::vector<_Elem>& _Right)
        {
            _Tidy();
            this->assign(_Right);
        }

        unreal_string(const _Myt& _Right)
        {
            this->assign(_Right);
        }

        unreal_string(_Myt&& _Right)
        {
            _Assign_rv(std::forward<_Myt>(_Right));
        }

        template<typename _OtherElem, typename _OtherCleaner>
        unreal_string(const unreal_string<_OtherElem, _OtherCleaner>& _Right)
        {
            this->assign(_Right);
        }

        template<typename _OtherElem, typename _OtherCleaner>
        unreal_string(unreal_string<_OtherElem, _OtherCleaner>&& _Right)
        {
            _Assign_rv(std::forward<unreal_string<_OtherElem, _OtherCleaner>>(_Right));
        }

        ~unreal_string(void)
        {
            _Mysize = 0;
            _Cleaner::cleanup(this->_Bx._Ptr);
            this->_Bx._Const_Ptr = nullptr;
        }

        _Myt& assign(const _Elem* _Ptr)
        { // shallow copy
            if (_Ptr != nullptr) {
                this->_Bx._Const_Ptr = _Ptr;
                this->_Mysize = native_utils::strlen(_Ptr);
            }
            return *this;
        }

        _Myt& assign(const _Elem* _Ptr, size_t _Length)
        { // shallow copy
            if (_Ptr != nullptr) {
                this->_Bx._Const_Ptr = _Ptr;
                this->_Mysize = _Length;
            }
            return *this;
        }

        _Myt& assign(const _Elem* _First, const _Elem* _Last)
        { // shallow copy
            if (_First != nullptr && _First <= _Last) {
                this->_Bx._Const_Ptr = _First;
                this->_Mysize = _Last - _First;
            }
            return *this;
        }

        _Myt& assign(_Elem* _Ptr)
        { // shallow copy
            if (_Ptr != nullptr) {
                this->_Bx._Ptr = _Ptr;
                this->_Mysize = native_utils::strlen(_Ptr);
            }
            return *this;
        }

        _Myt& assign(_Elem* _Ptr, size_t _Length)
        { // shallow copy
            if (_Ptr != nullptr) {
                this->_Bx._Ptr = _Ptr;
                this->_Mysize = _Length;
            }
            return *this;
        }

        _Myt& assign(_Elem* _First, _Elem* _Last)
        { // shallow copy
            if (_First != nullptr && _First <= _Last) {
                this->_Bx._Ptr = _First;
                this->_Mysize = _Last - _First;
            }
            return *this;
        }

        _Myt& assign(const std::basic_string<_Elem>& _Right)
        {
            static_assert(!_Cleaner::value, "atl-string can't assign to a managed unreal_string");

            this->_Bx._Const_Ptr = _Right.c_str();
            this->_Mysize = _Right.size();
            return *this;
        }

        _Myt& assign(const std::vector<_Elem>& _Right)
        {
            static_assert(!_Cleaner::value, "stl-string can't assign to a managed unreal_string");

            this->_Bx._Const_Ptr = _Right.data();
            this->_Mysize = _Right.size();
            return *this;
        }

        _Myt& assign(const _Myt& _Right)
        {
            static_assert(!_Cleaner::value, "the managed unreal string can't assign to this managed unreal_string object!");

            _Tidy();
            ::memcpy(this, &_Right, sizeof(*this));
            return *this;
        }

        _Myt& assign(_Myt&& _Right)
        {
            if (this != &_Right)
                _Assign_rv(std::forward<_Myt>(_Right));
            return *this;
        }

        template<typename _OtherElem, typename _OtherCleaner>
        _Myt& assign(const unreal_string<_OtherElem, _OtherCleaner>& _Right)
        {
            static_assert(std::is_same<_Elem, _OtherElem>::value, "can't assign unreal string assign, type not same!");
            static_assert(!_Cleaner::value || !_OtherCleaner::value, "the unreal string can't assign to this managed unreal_string object!");

            _Tidy();
            ::memcpy(this, &_Right, sizeof(*this));
            return *this;
        }

        template<typename _OtherElem, typename _OtherCleaner>
        _Myt& assign(unreal_string<_OtherElem, _OtherCleaner>&& _Right)
        {
            if (this != &_Right)
                _Assign_rv(std::forward<unreal_string<_OtherElem, _OtherCleaner>>(_Right));
            return *this;
        }

        template<typename _OtherElem, typename _OtherCleaner>
        void _Assign_rv(unreal_string<_OtherElem, _OtherCleaner>&& _Right)
        {
            ::memcpy(this, &_Right, sizeof(*this));

            if (this->managed())
                _Right._Tidy();
        }

        void reserve(size_t capacity)
        {
            if (this->_Capacity < capacity)
            {
                this->_Capacity = capacity;
                this->_Bx._Ptr = (_Elem*)realloc(this->_Bx._Ptr/*nullptr ok*/, _Capacity << ULS_ELEM_SHIFT);
            }
        }
        
        void cappend(const _Elem* _Ptr, size_t _Count)
        {
            static_assert(_Cleaner::value, "only managed_cstring is support append operation!");

            if (_Count > 0) {
                if (this->_Bx._Ptr == nullptr) {
                    _Capacity = (_Count * 3) >> 1; // 1.5
                    _Mysize = _Count;
                    this->_Bx._Ptr = (_Elem*)malloc(_Capacity << ULS_ELEM_SHIFT);
                    ::memcpy(this->_Bx._Ptr, _Ptr, _Count << ULS_ELEM_SHIFT);
                }
                else {
                    auto _Newsize = _Mysize + _Count;
                    if (_Newsize > _Capacity)
                    {
                        _Capacity = (_Newsize * 3) >> 1;
                        this->_Bx._Ptr = (_Elem*)realloc(this->_Bx._Ptr, _Capacity << ULS_ELEM_SHIFT);
                    }

                    ::memcpy(this->_Bx._Ptr + ( (_Mysize) << ULS_ELEM_SHIFT ), _Ptr, _Count << ULS_ELEM_SHIFT);

                    _Mysize = _Newsize;
                }
            }
        }

        _Elem* deatch(void)
        { // detach without memory destory for managed string or unmanaged string
            auto _Ptr = this->_Bx._Ptr;
            memset(this, 0x0, sizeof(*this));
            return _Ptr;
        }

        _Myt& operator=(_Elem *_Ptr)
        {	// assign [_Ptr, <null>)
            return (this->assign(_Ptr));
        }

        _Myt& operator=(const _Elem *_Ptr)
        {	// assign [_Ptr, <null>)
            return (this->assign(_Ptr));
        }

        _Myt& operator=(const std::basic_string<_Elem>& _Right)
        {	// assign [_Ptr, <null>)
            return (this->assign(_Right));
        }

        _Myt& operator=(const _Myt& _Right)
        {
            return this->assign(_Right);
        }

        _Myt& operator=(_Myt&& _Right)
        {
            return this->assign(_Right);
        }

        template<typename _OtherElem, typename _OtherCleaner>
        _Myt& operator=(const unreal_string<_OtherElem, _OtherCleaner>& _Right)
        {
            return this->assign(_Right);
        }

        template<typename _OtherElem, typename _OtherCleaner>
        _Myt& operator=(unreal_string<_OtherElem, _OtherCleaner>&& _Right)
        {
            return this->assign(_Right);
        }

        _Myt& operator+=(const _Myt& _Right);

        _Myt& operator+=(const _Elem *_Ptr);

        _Myt& operator+=(_Elem _Ch);

        static const bool managed(void)
        { // return if free memory automatically
            return _Cleaner::value;
        }

        void  set_size(size_t _Newsize)
        {
            this->_Mysize = _Newsize;
        }

        void shrink(size_t _Newsize)
        {
            if (_Mysize > _Newsize)
            {
                this->_Mysize = _Newsize;
            }
        }

        _Elem& at(size_t _Off)
        {	// subscript nonmutable sequence
            return (this->_Bx._Ptr[_Off]);
        }

        const _Elem& at(size_t _Off) const
        {	// subscript nonmutable sequence
            return (this->_Bx._Const_Ptr[_Off]);
        }

        _Elem& operator[](size_t _Off)
        {	// subscript nonmutable sequence
            return (this->_Bx._Ptr[_Off]);
        }

        const _Elem& operator[](size_t _Off) const
        {	// subscript nonmutable sequence
            return (this->_Bx._Const_Ptr[_Off]);
        }

        std::basic_string<_Elem> to_string(void) const
        {   // deep copy: return a c++ stl string
            static const _Elem s_empty[] = { (_Elem)0 };
            if (!this->empty()) {
                return std::basic_string<_Elem>(this->_Bx._Const_Ptr, this->size());
            }
            return s_empty;
        }

        const _Elem* c_str(void) const
        {	// return pointer to null-terminated nonmutable array
            if (this->_Bx._Const_Ptr != nullptr && this->_Mysize > 0) {
                return this->_Bx._Const_Ptr;
            }
            static const _Elem s_empty[] = { (_Elem)0 };
            return s_empty;
        }

        const _Elem* data(void) const
        {	// return pointer to nonmutable array
            return this->c_str();
        }

        const _Elem*& ldata(void)
        { // return internal pointer which can be change by exnternal, use careful
            return this->_Bx._Const_Ptr;
        }

        size_t& lsize()
        {
            return _Mysize;
        }

        size_t& llength()
        {
            return _Mysize;
        }

        size_t length(void) const
        {	// return length of sequence
            return _Mysize;
        }

        size_t size(void) const
        {	// return length of sequence
            return _Mysize;
        }

        void clear(void)
        {
            _Mysize = 0;
        }

        bool empty(void) const
        {
            return 0 == this->_Mysize || nullptr == this->_Bx._Ptr;
        }

        int compare(const _Elem* _Ptr) const
        { // regard right as null-terminated string
            if (this->empty())
                return (_Ptr == nullptr || *_Ptr == 0x0) ? 0 : -1;
            else {

                if (_Ptr == nullptr)
                    return 1;

                int diff = 0;
                size_t _N0 = 0;

                for (;;)
                {
                    if (*_Ptr == 0x0) {
                        diff = _Mysize - _N0;
                        break;
                    }

                    if (_N0 >= _Mysize)
                    { // Left terminated, right not terminated.
                        diff = -1;
                        break;
                    }

                    diff = (this->_Bx._Ptr[_N0++]) - *_Ptr++;
                    if (diff != 0) {
                        break;
                    }
                }

                return diff;
            }
        }

        int compare(const _Elem* _Right, size_t _Count) const
        {
            if (this->empty())
                return (_Right == nullptr || _Count == 0 || *_Right == 0x0) ? 0 : -1;
            else {
                if (_Right != nullptr)
                {
                    int diff = memcmp(this->_Bx._Ptr, _Right, (_Mysize < _Count ? _Mysize : _Count) << (sizeof(_Elem) >> 1));
                    return (diff != 0) ? diff : (static_cast<int>(_Mysize)-static_cast<int>(_Count));
                }
                else {
                    return 1;
                }
            }
        }

        int compare(const std::basic_string<_Elem>& _Right) const
        {
            return this->compare(_Right.c_str(), _Right.length());
        }

        template<typename _OtherCleaner> inline
            int compare(const unreal_string<_Elem, _OtherCleaner>& _Right) const
        {
            return this->compare(_Right.c_str(), _Right.length());
        }

        operator std::basic_string<_Elem>(void) const
        {
            return this->to_string();
        }

        std::string substr(std::string::size_type _Off, std::string::size_type _Count = std::string::npos) const
        {
            return this->to_string().substr(_Off, _Count);
        }

        void free_any(void) const
        {
            if (_Bx._Ptr != nullptr) {
                free(_Bx._Ptr);
            }
        }

        void delete_any(void) const
        {
            if (_Bx._Ptr != nullptr) {
                delete[](_Bx._Ptr);
            }
        }

    protected:
        void _Tidy(void)
        {
            ::memset(this, 0x0, sizeof(*this));
        }

    private:
        enum
        {	// length of internal buffer, [1, 16]
            _BUF_SIZE = 16 / sizeof(_Elem) < 1 ? 1
            : 16 / sizeof(_Elem)
        };
        enum
        {	// roundup mask for allocated buffers, [0, 15]
            _ALLOC_MASK = sizeof(_Elem) <= 1 ? 15
            : sizeof(_Elem) <= 2 ? 7
            : sizeof(_Elem) <= 4 ? 3
            : sizeof(_Elem) <= 8 ? 1 : 0
        };
        union _Bxty
        {	// storage for small buffer or pointer to larger one
            //_Elem _Buf[_BUF_SIZE];
            const _Elem* _Const_Ptr; // always shallow copy
            _Elem*       _Ptr;       // managed pointer
            //char _Alias[_BUF_SIZE];	// to permit aliasing
        } _Bx;
        size_t       _Mysize;
        size_t       _Capacity;
    };

    /// operator +
    template<typename _Elem,
        typename _Cleaner> inline
        std::basic_string<_Elem> operator+(
        const unreal_string<_Elem, _Cleaner>& _Left,
        const unreal_string<_Elem, _Cleaner>& _Right)
    {    // unreal_string + unreal_string
        return (_Left.to_string() + _Right.to_string());
    }

    template<typename _Elem,
        typename _Cleaner> inline
        std::basic_string<_Elem> operator+(
        const unreal_string<_Elem, _Cleaner>& _Left,
        const std::string& _Right)
    {    // unreal_string + std::string
        return (_Left.to_string() + _Right);
    }

    template<typename _Elem,
        typename _Cleaner> inline
        std::basic_string<_Elem> operator+(
        const unreal_string<_Elem, _Cleaner>& _Left,
        const _Elem* _Right)
    {    // unreal_string + NTCS
        return (_Left.to_string() + _Right);
    }

    template<typename _Elem,
        typename _Cleaner> inline
        std::basic_string<_Elem> operator+(
        const std::string& _Left,
        const unreal_string<_Elem, _Cleaner>& _Right)
    {    // std::string + unreal_string
        return (_Left + _Right.c_str());
    }

    template<typename _Elem,
        typename _Cleaner> inline
        std::basic_string<_Elem> operator+(
        const _Elem* _Left,
        const unreal_string<_Elem, _Cleaner>& _Right)
    {    // NTCS + unreal_string
        return (std::string(_Left) + _Right.c_str());
    }

    /// operator comparands
    template<typename _Elem,
        typename _Cleaner> inline
        bool operator==(
        const unreal_string<_Elem, _Cleaner>& _Left,
        const unreal_string<_Elem, _Cleaner>& _Right)
    {    // test for unreal_string equality
        return (_Left.compare(_Right) == 0);
    }

    template<typename _Elem,
        typename _Cleaner> inline
        bool operator==(
        const _Elem* _Left,
        const unreal_string<_Elem, _Cleaner>& _Right)
    {    // test for NTCS vs. unreal_string equality
        return (_Right.compare(_Left) == 0);
    }

    template<typename _Elem,
        typename _Cleaner> inline
        bool operator==(
        const unreal_string<_Elem, _Cleaner>& _Left,
        const _Elem* _Right)
    {    // test for unreal_string vs. NTCS equality
        return (_Left.compare(_Right) == 0);
    }

    template<typename _Elem,
        typename _Cleaner> inline
        bool operator==(
        const std::basic_string<_Elem>& _Left,
        const unreal_string<_Elem, _Cleaner>& _Right)
    {    // test for std::string vs. unreal_string equality
        return (_Right.compare(_Left.c_str(), _Left.length()) == 0);
    }

    template<typename _Elem,
        typename _Cleaner> inline
        bool operator==(
        const unreal_string<_Elem, _Cleaner>& _Left,
        const std::basic_string<_Elem>& _Right)
    {    // test for unreal_string vs. std::string equality
        return (_Left.compare(_Right.c_str(), _Right.length()) == 0);
    }

    template<typename _Elem,
        typename _Cleaner> inline
        bool operator!=(
        const unreal_string<_Elem, _Cleaner>& _Left,
        const unreal_string<_Elem, _Cleaner>& _Right)
    {    // test for unreal_string inequality
        return (!(_Left == _Right));
    }

    template<typename _Elem,
        typename _Cleaner> inline
        bool operator!=(
        const _Elem* _Left,
        const unreal_string<_Elem, _Cleaner>& _Right)
    {    // test for NTCS vs. unreal_string inequality
        return (!(_Left == _Right));
    }

    template<typename _Elem,
        typename _Cleaner> inline
        bool operator!=(
        const unreal_string<_Elem, _Cleaner>& _Left,
        const _Elem* _Right)
    {    // test for unreal_string vs. NTCS inequality
        return (!(_Left == _Right));
    }

    template<typename _Elem,
        typename _Cleaner> inline
        bool operator!=(
        const std::basic_string<_Elem>& _Left,
        const unreal_string<_Elem, _Cleaner>& _Right)
    {    // test for std::string vs. unreal_string inequality
        return (!(_Left == _Right));
    }

    template<typename _Elem,
        typename _Cleaner> inline
        bool operator!=(
        const unreal_string<_Elem, _Cleaner>& _Left,
        const std::basic_string<_Elem>& _Right)
    {    // test for unreal_string vs. std::string inequality
        return (!(_Left == _Right));
    }

    template<typename _Elem,
        typename _Cleaner> inline
        bool operator<(
        const unreal_string<_Elem, _Cleaner>& _Left,
        const unreal_string<_Elem, _Cleaner>& _Right)
    {    // test if unreal_string < unreal_string
        return (_Left.compare(_Right) < 0);
    }

    template<typename _Elem,
        typename _Cleaner> inline
        bool operator<(
        const _Elem* _Left,
        const unreal_string<_Elem, _Cleaner>& _Right)
    {    // test if NTCS < unreal_string
        return (_Right.compare(_Left) > 0);
    }

    template<typename _Elem,
        typename _Cleaner> inline
        bool operator<(
        const unreal_string<_Elem, _Cleaner>& _Left,
        const _Elem* _Right)
    {    // test if unreal_string < NTCS
        return (_Left.compare(_Right) < 0);
    }

    template<typename _Elem,
        typename _Cleaner> inline
        bool operator<(
        const std::basic_string<_Elem>& _Left,
        const unreal_string<_Elem, _Cleaner>& _Right)
    {    // test if std::string < unreal_string
        return (_Left.compare(_Right.c_str()) > 0);
    }

    template<typename _Elem,
        typename _Cleaner> inline
        bool operator<(
        const unreal_string<_Elem, _Cleaner>& _Left,
        const std::basic_string<_Elem>& _Right)
    {    // test if unreal_string < std::string
        return (_Right.compare(_Left.c_str()) < 0);
    }

    template<typename _Elem,
        typename _Cleaner> inline
        bool operator>(
        const unreal_string<_Elem, _Cleaner>& _Left,
        const unreal_string<_Elem, _Cleaner>& _Right)
    {    // test if unreal_string > unreal_string
        return (_Right < _Left);
    }

    template<typename _Elem,
        typename _Cleaner> inline
        bool operator>(
        const _Elem* _Left,
        const unreal_string<_Elem, _Cleaner>& _Right)
    {    // test if NTCS > unreal_string
        return (_Right < _Left);
    }

    template<typename _Elem,
        typename _Cleaner> inline
        bool operator>(
        const unreal_string<_Elem, _Cleaner>& _Left,
        const _Elem* _Right)
    {    // test if unreal_string > NTCS
        return (_Right < _Left);
    }

    template<typename _Elem,
        typename _Cleaner> inline
        bool operator>(
        const std::basic_string<_Elem>& _Left,
        const unreal_string<_Elem, _Cleaner>& _Right)
    {    // test if std::string > unreal_string
        return (_Right < _Left);
    }

    template<typename _Elem,
        typename _Cleaner> inline
        bool operator>(
        const unreal_string<_Elem, _Cleaner>& _Left,
        const std::basic_string<_Elem>& _Right)
    {    // test if unreal_string > std::string
        return (_Right < _Left);
    }

    template<typename _Elem,
        typename _Cleaner> inline
        bool operator<=(
        const unreal_string<_Elem, _Cleaner>& _Left,
        const unreal_string<_Elem, _Cleaner>& _Right)
    {    // test if unreal_string <= unreal_string
        return (!(_Right < _Left));
    }

    template<typename _Elem,
        typename _Cleaner> inline
        bool operator<=(
        const _Elem* _Left,
        const unreal_string<_Elem, _Cleaner>& _Right)
    {    // test if NTCS <= unreal_string
        return (!(_Right < _Left));
    }

    template<typename _Elem,
        typename _Cleaner> inline
        bool operator<=(
        const unreal_string<_Elem, _Cleaner>& _Left,
        const _Elem* _Right)
    {    // test if unreal_string <= NTCS
        return (!(_Right < _Left));
    }

    template<typename _Elem,
        typename _Cleaner> inline
        bool operator<=(
        const std::basic_string<_Elem>& _Left,
        const unreal_string<_Elem, _Cleaner>& _Right)
    {    // test if std::string <= unreal_string
        return (!(_Right < _Left));
    }

    template<typename _Elem,
        typename _Cleaner> inline
        bool operator<=(
        const unreal_string<_Elem, _Cleaner>& _Left,
        const std::basic_string<_Elem>& _Right)
    {    // test if unreal_string <= std::string
        return (!(_Right < _Left));
    }

    template<typename _Elem,
        typename _Cleaner> inline
        bool operator>=(
        const unreal_string<_Elem, _Cleaner>& _Left,
        const unreal_string<_Elem, _Cleaner>& _Right)
    {    // test if unreal_string >= unreal_string
        return (!(_Left < _Right));
    }

    template<typename _Elem,
        typename _Cleaner> inline
        bool operator>=(
        const _Elem* _Left,
        const unreal_string<_Elem, _Cleaner>& _Right)
    {    // test if NTCS >= unreal_string
        return (!(_Left < _Right));
    }

    template<typename _Elem,
        typename _Cleaner> inline
        bool operator>=(
        const unreal_string<_Elem, _Cleaner>& _Left,
        const _Elem* _Right)
    {    // test if unreal_string >= NTCS
        return (!(_Left < _Right));
    }

    template<typename _Elem,
        typename _Cleaner> inline
        bool operator>=(
        const std::basic_string<_Elem>& _Left,
        const unreal_string<_Elem, _Cleaner>& _Right)
    {    // test if std::string >= unreal_string
        return (!(_Left < _Right));
    }

    template<typename _Elem,
        typename _Cleaner> inline
        bool operator>=(
        const unreal_string<_Elem, _Cleaner>& _Left,
        const std::basic_string<_Elem>& _Right)
    {    // test if unreal_string >= std::string
        return (!(_Left < _Right));
    }

    /// operator <<
    template<typename _Elem,
        typename _Cleaner> inline
        std::basic_ostream<_Elem>& operator<<(
        std::basic_ostream<_Elem>& _Ostr,
        const unreal_string<_Elem, _Cleaner>& _Str)
    {    // insert a unreal_string
        if (!_Str.empty()) {
            _Ostr << _Str.c_str();
        }
        return (_Ostr);
    }

}; // namespace: purelib


/* unreal_string hash, support unordered container. */
namespace std {
    // FUNCTION _Hash_seq
    inline size_t _FNV1a_hash(const void* _First, size_t _Count)
    {	// FNV-1a hash function for bytes in [_First, _First+_Count)
#if defined(_M_X64) || defined(_LP64) || defined(__x86_64) || defined(_WIN64)
        static_assert(sizeof(size_t) == 8, "This code is for 64-bit size_t.");
        const size_t _FNV_offset_basis = 14695981039346656037ULL;
        const size_t _FNV_prime = 1099511628211ULL;

#else /* defined(_M_X64), etc. */
        static_assert(sizeof(size_t) == 4, "This code is for 32-bit size_t.");
        const size_t _FNV_offset_basis = 2166136261U;
        const size_t _FNV_prime = 16777619U;
#endif /* defined(_M_X64), etc. */

        size_t _Val = _FNV_offset_basis;
        for (size_t _Next = 0; _Next < _Count; ++_Next)
        {	// fold in another byte
            _Val ^= (size_t)static_cast<const unsigned char *>(_First)[_Next];
            _Val *= _FNV_prime;
        }

#if 0 // Follow code are removed from VS2015
#if defined(_M_X64) || defined(_LP64) || defined(__x86_64) || defined(_WIN64)
        static_assert(sizeof(size_t) == 8, "This code is for 64-bit size_t.");
        _Val ^= _Val >> 32;

#else /* defined(_M_X64), etc. */
        static_assert(sizeof(size_t) == 4, "This code is for 32-bit size_t.");
#endif /* defined(_M_X64), etc. */
#endif
        return (_Val);
    }

    // optional hash functions
#if 0  
    inline size_t _BKDR_hash(const unsigned char* key, size_t len)
    {
        static size_t seed = 16777619U; // 31 131 1313 13131 131313 etc..
        size_t hash = 0;

        while (len-- > 0)
        {
            hash = hash * seed + (*key++);
        }

        return (hash & 0x7FFFFFFF);
    }

    inline size_t _Times33_hash(const unsigned char* key, size_t len)
    {
        size_t nHash = 0;

        while (len-- > 0)
            nHash = (nHash << 5) + nHash + *key++;

        return nHash;
    }
#endif

    template<class _Elem,
    class _Cleaner>
    struct hash < purelib::unreal_string<_Elem, _Cleaner> >
    {	// hash functor for basic_string
        typedef purelib::unreal_string<_Elem, _Cleaner> _Kty;

        size_t operator()(const _Kty& _Keyval) const
        {	// hash _Keyval to size_t value by pseudorandomizing transform
#if defined(__APPLE__)
            return std::__do_string_hash(_Keyval.data(), _Keyval.data() + _Keyval.size());
#elif defined(__linux__)
            return std::_Hash_impl::hash(_Keyval.data(), _Keyval.size() << (sizeof(_Elem) >> 1) );
#else
            return _FNV1a_hash(_Keyval.c_str(), _Keyval.size() << ULS_ELEM_SHIFT);
#endif
        }
    };
}

#endif 
/* _UNREAL_STRING_H_ */

/*
* Copyright (c) 2012-2016 by halx99 ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
V3.0:2011 */

