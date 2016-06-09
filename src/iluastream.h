#ifndef _ILUASTREAM_H_
#define _ILUASTREAM_H_
#include <string>
#include <sstream>
#include <exception>

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

class iluastream
{
public:
    iluastream(lua_State* l);
    iluastream(const iluastream& right) = delete;
    iluastream(iluastream&& right) = delete;

    iluastream& operator=(const iluastream& right) = delete;
    iluastream& operator=(iluastream&& right) = delete;

    template<typename _Nty>
    void read_i(const char* field, _Nty& ov);

    template<typename _Nty>
    void read_f(const char* field, _Nty& ov);

    void read_v(const char* field, std::string& ov);
    void read_v(const char* field, void* ov, int len);

    template<typename _Nty>
    void read_i(int i, _Nty& ov);

    template<typename _Nty>
    void read_f(int i, _Nty& ov);

    void read_v(int i, std::string& ov);
    void read_v(int i, void* ov, int len);

protected:
    lua_State* L;
};

template <typename _Nty>
inline void iluastream::read_i(const char* field, _Nty & ov)
{
    lua_getfield(L, -1, field);
    if (lua_isnumber(L, -1)){
        ov = static_cast<_Nty>(lua_tointeger(L, -1));
    }
    lua_pop(L, 1);
}

template <>
inline void iluastream::read_i(const char* field, float & ov)
{
    lua_getfield(L, -1, field);
    if (lua_isnumber(L, -1)){
        ov = static_cast<float>(lua_tonumber(L, -1));
    }
    lua_pop(L, 1);
}

template <>
inline void iluastream::read_i(const char* field, double & ov)
{
    lua_getfield(L, -1, field);
    if (lua_isnumber(L, -1)){
        ov = lua_tonumber(L, -1);
    }
    lua_pop(L, 1);
}

template <>
inline void iluastream::read_i(const char* field, bool & ov)
{
    lua_getfield(L, -1, field);
    if (lua_isboolean(L, -1)){
        ov = !!(lua_toboolean(L, -1));
    }
    lua_pop(L, 1);
}

// item of array
template <typename _Nty>
inline void iluastream::read_i(int i, _Nty & ov)
{
    lua_rawgeti(L, -1, i);
    if (lua_isnumber(L, -1)){
        ov = static_cast<_Nty>(lua_tointeger(L, -1));
    }
    lua_pop(L, 1);
}

template <>
inline void iluastream::read_i(int i, bool & ov)
{
    lua_rawgeti(L, -1, i);
    if (lua_isboolean(L, -1)){
        ov = !!(lua_toboolean(L, -1));
    }
    lua_pop(L, 1);
}

template <>
inline void iluastream::read_i(int i, float & ov)
{
    lua_rawgeti(L, -1, i);
    if (lua_isnumber(L, -1)){
        ov = lua_tonumber(L, -1);
    }
    lua_pop(L, 1);
}

template <>
inline void iluastream::read_i(int i, double & ov)
{
    lua_rawgeti(L, -1, i);
    if (lua_isnumber(L, -1)){
        ov = lua_tonumber(L, -1);
    }
    lua_pop(L, 1);
}


#endif
