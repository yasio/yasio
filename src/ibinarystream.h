#ifndef _IBINARYSTREAM_H_
#define _IBINARYSTREAM_H_
#include <string>
#include <sstream>
#include <exception>
#include "endian_portable.h"

class ibinarystream
{
public:
    ibinarystream();
    ibinarystream(const char* data, int size);
    ibinarystream(const ibinarystream& right) = delete;
    ibinarystream(ibinarystream&& right) = delete;

    void assign(const char* data, int size);

    ibinarystream& operator=(const ibinarystream& right) = delete;
    ibinarystream& operator=(ibinarystream&& right) = delete;

    template<typename _Nty>
    int read_i(_Nty& ov);
    int read_i(float& ov);
    int read_i(double& ov);

    int read_v(std::string&);
    int read_v16(std::string&);
    int read_v32(std::string&);

    int read_v(void*, int);
    int read_v16(void*, int);
    int read_v32(void*, int);

    int read_bytes(std::string& oav, int len);
    int read_bytes(void* oav, int len);

    inline int remain(void) { return remain_;  }

    template<typename _LenT>
    int read_vv(std::string& ov)
    {
        _LenT n = purelib::endian::ntohv(*(_LenT*)(ptr_));

        (void)consume(sizeof(n));

        if (n > 0) {
            ov.resize(n);
            return read_bytes(&ov.front(), n);
        }

        return remain_;
    }

    template<typename _LenT>
    int read_vv(void* ov, int len)
    {
        _LenT n = purelib::endian::ntohv(*(_LenT*)(ptr_));

        (void)consume(sizeof(n));

        if (n > 0) {
            // ov.resize(n);
            if (len < static_cast<int>(n))
                n = static_cast<_LenT>(len);
            return read_bytes(ov, n);
        }

        return remain_;
    }

protected:
    // will throw std::logic_error
    int consume(size_t size);

protected:
    const char*    ptr_;
    int            remain_;
};

template <typename _Nty>
inline int ibinarystream::read_i(_Nty & ov)
{
    ov = purelib::endian::ntohv(*((_Nty*)(ptr_)));
    return consume(sizeof(_Nty));
}

inline int ibinarystream::read_i(float& ov)
{
    ov = ntohf(*((uint32_t*)(ptr_)));
    return consume(sizeof(ov));
}

inline int ibinarystream::read_i(double& ov)
{
    ov = ntohd(*((uint64_t*)(ptr_)));
    return  consume(sizeof(uint64_t));
}

#endif
