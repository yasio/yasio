#ifndef _OLUASTREAM_H_
#define _OLUASTREAM_H_
#include <string>
#include <sstream>
#include <vector>

#include "luax/LuaSupport.h"

class oluastream
{
public:
    oluastream(lua_State* luaS);
    oluastream(const oluastream& right) = delete;
    oluastream(oluastream&& right) = delete;

    oluastream& operator=(const oluastream& right) = delete;
    oluastream& operator=(oluastream&& right) = delete;

    template<typename _Nty>
    void write_i(const char* field, const _Nty& value);

    void write_v(const char* field, const std::string&);

    void write_v(const char* field, const void* v, int vl);

    /// index
    template<typename _Nty>
    void write_i(const int field, const _Nty& value);

    void write_v(const int field, const std::string&);

    void write_v(const int field, const void* v, int vl);

protected:
    lua_State*    L;
};

template <typename _Nty>
inline void oluastream::write_i(const char* field, const _Nty& value)
{
    lua_pushinteger(L, value);
    lua_setfield(L, -2, field); // -2
}

template <>
inline void oluastream::write_i(const char* field, const bool& value)
{
    lua_pushboolean(L, value);
    lua_setfield(L, -2, field); // -2
}

template <>
inline void oluastream::write_i(const char* field, const float& value)
{
    lua_pushnumber(L, value);
    lua_setfield(L, -2, field); // -2
}

template <>
inline void oluastream::write_i(const char* field, const double& value)
{
    lua_pushnumber(L, value);
    lua_setfield(L, -2, field); // -2
}

/// array
template <typename _Nty>
inline void oluastream::write_i(const int field, const _Nty& value)
{
    lua_pushinteger(L, value);
    lua_rawseti(L, -2, field); // -2
}

template <>
inline void oluastream::write_i(const int field, const bool& value)
{
    lua_pushboolean(L, value);
    lua_rawseti(L, -2, field); // -2
}

template <>
inline void oluastream::write_i(const int field, const float& value)
{
    lua_pushnumber(L, value);
    lua_rawseti(L, -2, field); // -2
}

template <>
inline void oluastream::write_i(const int field, const double& value)
{
    lua_pushnumber(L, value);
    lua_rawseti(L, -2, field); // -2
}

#endif
