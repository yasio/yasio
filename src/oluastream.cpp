#include "oluastream.h"
#include <iostream>
#include <fstream>

oluastream::oluastream(lua_State* s) : L(s)
{
}

void oluastream::write_v(const char* field, const std::string & value)
{
    lua_pushlstring(L, value.c_str(), value.length());
    lua_setfield(L, -2, field);
}

void oluastream::write_v(const char* field, const void* v, int size)
{
    lua_pushlstring(L, (const char*)v, size);
    lua_setfield(L, -2, field);
}

void oluastream::write_v(const int field, const std::string& value)
{
    lua_pushlstring(L, value.c_str(), value.length());
    lua_rawseti(L, -2, field);
}

void oluastream::write_v(const int field, const void* v, int vl)
{
    lua_pushlstring(L, (const char*)v, vl);
    lua_rawseti(L, -2, field);
}

