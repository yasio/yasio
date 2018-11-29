#ifndef _IBINARYSTREAM_H_
#define _IBINARYSTREAM_H_
#include <string>
#include <string_view>
#include <sstream>
#include <exception>
#include "endian_portable.h"
class obinarystream;
class ibinarystream
{
public:
    ibinarystream();
    ibinarystream(const char* data, int size);
    ibinarystream(const obinarystream*);
    ibinarystream(const ibinarystream& right) = delete;
    ibinarystream(ibinarystream&& right) = delete;

    ~ibinarystream();

    void assign(const char* data, int size);

    ibinarystream& operator=(const ibinarystream& right) = delete;
    ibinarystream& operator=(ibinarystream&& right) = delete;

    template<typename _Nty>
    void read_i(_Nty& ov);
    void read_i(float& ov);
    void read_i(double& ov);

    template<typename _Nty>
    _Nty read_i0() {
        _Nty value;
        read_i(value);
        return value;
    }

    void read_v(std::string&);
    void read_v16(std::string&);
    void read_v32(std::string&);

    void read_v(void*, int);
    void read_v16(void*, int);
    void read_v32(void*, int);

    void read_bytes(std::string& oav, int len);
    void read_bytes(void* oav, int len);

    std::string_view read_v();
    std::string_view read_bytes(int len);

    inline int size(void) { return size_;  }

    template<typename _LenT>
    std::string_view read_vx()
    {
        _LenT n = purelib::endian::ntohv(*(_LenT*)consume(sizeof(n)));

        if (n > 0) {
            return read_bytes(n);
        }

        return {};
    }

protected:
    // will throw std::logic_error
    const char* consume(size_t size) noexcept(false);

protected:
    const char*    ptr_;
    int            size_;
};

template <typename _Nty>
inline void ibinarystream::read_i(_Nty & ov)
{
    ov = purelib::endian::ntohv(*((_Nty*)consume(sizeof(_Nty))));
}

inline void ibinarystream::read_i(float& ov)
{
    ov = ntohf(*((uint32_t*)consume(sizeof(ov))));
}

inline void ibinarystream::read_i(double& ov)
{
    ov = ntohd(*((uint64_t*)consume(sizeof(uint64_t))));
}

#endif
