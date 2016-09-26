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

    void vassign(const char* data, int size);

    ibinarystream& operator=(const ibinarystream& right) = delete;
    ibinarystream& operator=(ibinarystream&& right) = delete;

    template<typename _Nty>
    void read_i(_Nty& ov);
    void read_i(float& ov);
    void read_i(double& ov);

    int read_v(std::string& ov);
    int read_v(void* ov, int len);

    int read_array(std::string& oav, int len);
    int read_array(void* oav, int len);

    inline int remain(void) { return remain_;  }

protected:
    // will throw std::logic_error
    int consume(size_t size);

protected:
    const char*    ptr_;
    int            remain_;
};

template <typename _Nty>
inline void ibinarystream::read_i(_Nty & ov)
{
    ov = purelib::endian::ntohv(*((_Nty*)(ptr_)));
    consume(sizeof(_Nty));
}

inline void ibinarystream::read_i(float& ov)
{
    ov = ntohf(*((uint32_t*)(ptr_)));
    consume(sizeof(ov));
}

inline void ibinarystream::read_i(double& ov)
{
    ov = ntohd(*((uint64_t*)(ptr_)));
    consume(sizeof(uint64_t));
}

#endif
