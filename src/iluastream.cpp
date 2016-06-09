#include "iluastream.h"

//#ifdef _WIN32
//#pragma comment(lib, "ws2_32.lib")
//#endif

iluastream::iluastream(lua_State* l)
{
    this->L = l;
}

void iluastream::read_v(const char* field, std::string& ov)
{
    lua_getfield(L, -1, field);
    if (lua_isstring(L, -1)){
        size_t len = 0;
        const char* value = lua_tolstring(L, -1, &len);
        ov.assign(value, len);
    }
    lua_pop(L, 1);
}

void iluastream::read_v(const char* field, void* ov, int ovl)
{ 
    lua_getfield(L, -1, field);
    if (lua_isstring(L, -1)){
        size_t len = 0;
        const char* value = lua_tolstring(L, -1, &len);
        memcpy(ov, value, std::min((size_t)ovl, len));
    }
    lua_pop(L, 1);
}

void iluastream::read_v(int i, std::string& ov)
{
    lua_rawgeti(L, -1, i);
    if (lua_isstring(L, -1)){
        size_t len = 0;
        const char* value = lua_tolstring(L, -1, &len);
        ov.assign(value, len);
    }
    lua_pop(L, 1);
}

void iluastream::read_v(int i, void* ov, int ovl)
{
    lua_rawgeti(L, -1, i);
    if (lua_isstring(L, -1)){
        size_t len = 0;
        const char* value = lua_tolstring(L, -1, &len);
        memcpy(ov, value, std::min((size_t)ovl, len));
    }
    lua_pop(L, 1);
}
