#include "xxlcall.h"

static lua_State*  s_lvm = nullptr;

void luax_setVM(lua_State* L)
{
    s_lvm = L;
}

lua_State* luax_getVM(void)
{
    return s_lvm;
}

void  luax_unref(int ref)
{
    lua_unref(s_lvm, ref);
}

int luax_rawgeti(int refid)
{
    int top = lua_gettop(s_lvm);
    lua_rawgeti(s_lvm, LUA_REGISTRYINDEX, refid);
    return top;
}

int luax_getobjfield(int objid, const char* field)
{
    int top = luax_rawgeti(objid);
    lua_getfield(luax_getVM(), -1, field);
    return top;
}

int luax_getglobal(const char* s)
{
    int top = lua_gettop(s_lvm);
    lua_getglobal(s_lvm, s);
    return top;
}

int luax_getglobal2(const char* func)
{
    auto top = lua_gettop(s_lvm);

    std::string source = func;

    const char* orig = source.c_str();
    const char* name = orig;
    auto offst = 0;
    auto end = 0;
    end = source.find_first_of('.', offst);
    if (end == std::string::npos)
    { // assume _G.func
        lua_getglobal(s_lvm, name);
        if (lua_isfunction(s_lvm, -1))
            return top;
        else
            return top;
    }

    // assume table
    source[end] = '\0';
    lua_getglobal(s_lvm, name);
    if (!lua_istable(s_lvm, -1))
        return top;
    offst = end + 1;

    // continue check sub table
    while ((end = source.find_first_of('.', offst)) != std::string::npos)
    { // assume table
        source[end] = '\0';
        name = orig + offst;
        lua_getfield(s_lvm, -1, name);
        if (!lua_istable(s_lvm, -1))
        {
            return top;
        }

        offst = end + 1;
    }

    // now assume function
    name = orig + offst;
    lua_getfield(s_lvm, -1, name);

    return top;
}

bool luax_assume_read(lua_State* L, const char* field)
{
    std::string source = field;

    const char* orig = source.c_str();
    const char* name = orig;
    auto offst = 0;
    auto end = 0;
    end = source.find_first_of('.', offst);
    if (end == std::string::npos)
    { // assume _G.func
        lua_getglobal(L, name);
        if (lua_isnumber(L, -1) || lua_isstring(L, -1) || lua_isboolean(L, -1) || lua_isfunction(L, -1))
            return true;
        else
            return false;
    }

    // assume table
    source[end] = '\0';
    lua_getglobal(L, name);
    if (!lua_istable(L, -1))
        return false;
    offst = end + 1;

    // continue check sub table
    while ((end = source.find_first_of('.', offst)) != std::string::npos)
    { // assume table
        source[end] = '\0';
        name = orig + offst;
        lua_getfield(L, -1, name);
        if (!lua_istable(L, -1))
        {
            return false;
        }

        offst = end + 1;
    }

    // now assume function
    name = orig + offst;
    lua_getfield(L, -1, name);

    return lua_isnumber(L, -1) || lua_isstring(L, -1) || lua_isboolean(L, -1) || lua_isfunction(L, -1);
}

bool luax_assume_write(lua_State* L, const char*& field)
{
    std::string source = field;

    const char* orig = source.c_str();
    const char* name = orig;
    auto offst = 0;
    auto end = 0;
    end = source.find_first_of('.', offst);
    if (end == std::string::npos)
    { // invalid field 
        return false;
    }

    // assume table
    source[end] = '\0';
    lua_getglobal(L, name);
    if (!lua_istable(L, -1))
        return false;
    offst = end + 1;

    // continue check sub table
    while ((end = source.find_first_of('.', offst)) != std::string::npos)
    { // assume table
        source[end] = '\0';
        name = orig + offst;
        lua_getfield(L, -1, name);
        if (!lua_istable(L, -1))
        {
            return false;
        }

        offst = end + 1;
    }

    // now assume function
    field += offst;
    // lua_getfield(L, -1, name);

    return true;
}

void luax_table_setvalue(const char* tableName, const char* tableItem, int value)
{
    lua_getglobal(s_lvm, tableName);
    lua_pushnumber(s_lvm, value);
    lua_setfield(s_lvm, -2, tableItem);
    lua_pop(s_lvm, 1);
}

void luax_table_setvalue(const char* tableName, const char* tableItem, const char* value, int len)
{
    lua_getglobal(s_lvm, tableName);
    lua_pushlstring(s_lvm, value, len);
    lua_setfield(s_lvm, -2, tableItem);
    lua_pop(s_lvm, 1);
}

void luax_dump_stack() {
    auto top = lua_gettop(s_lvm);
    for (auto i = 1; i <= top; i++) { /* repeat for each level */
        int t = lua_type(s_lvm, i);
        switch (t) {
        case LUA_TSTRING: /* strings */
            printf("`%s'\n", lua_tostring(s_lvm, i));
            break;
        case LUA_TBOOLEAN: /* booleans */
            printf(lua_toboolean(s_lvm, i) ? "true\n" : "false\n");
            break;
        case LUA_TNUMBER: /* numbers */
            printf("%g\n", lua_tonumber(s_lvm, i));
            break;
        default: /* other values */
            printf("%s\n", lua_typename(s_lvm, t));
            break;
        }
        printf(" \n"); /* put a separator */
    }
    printf("\n"); /* end the listing */
}

void luax_retrive_arg(lua_State* L, const char* func, int n, bool& arg, bool& ok)
{
    ok &= lua_isboolean(L, n);
    if (ok)
        arg = !!lua_toboolean(L, n);
}
void luax_retrive_arg(lua_State* L, const char* func, int n, int& arg, bool& ok)
{
    ok &= !!lua_isnumber(L, n);
    if (ok)
        arg = lua_tointeger(L, n);
}
void luax_retrive_arg(lua_State* L, const char* func, int n, long long& arg, bool& ok)
{
    ok &= !!lua_isnumber(L, n);
    if (ok)
        arg = lua_tointeger(L, n);
}
void luax_retrive_arg(lua_State* L, const char* func, int n, void*& arg, bool& ok)
{
    ok &= lua_islightuserdata(L, n);
    if (ok)
        arg = lua_touserdata(L, n);
}
void luax_retrive_arg(lua_State* L, const char* func, int n, float& arg, bool& ok)
{
    ok &= !!lua_isnumber(L, n);
    if (ok)
        arg = lua_tonumber(L, n);
}
void luax_retrive_arg(lua_State* L, const char* func, int n, const char*& arg, bool& ok)
{
    ok &= !!lua_isstring(L, n);
    if (ok)
        arg = lua_tostring(L, n);
}
void luax_retrive_arg(lua_State* L, const char* func, int n, const char*& arg, size_t& len, bool& ok)
{
    ok &= !!lua_isstring(L, n);
    if (ok)
        arg = lua_tolstring(L, n, &len);
}
void luax_retrive_arg(lua_State* L, const char* func, int n, std::string& arg, bool& ok)
{
    ok &= !!lua_isstring(L, n);
    if (ok) {
        size_t len = 0;
        const char* s = lua_tolstring(L, n, &len);
        arg.assign(s, len);
    }
}
void luax_retrive_arg(lua_State* L, const char* func, int n, SharedPtr<LuaFunc>& arg, bool& ok)
{
    ok &= lua_isfunction(L, n);
    if (ok) {
        lua_pushvalue(L, n);

        arg.reset(new LuaFunc(lua_ref(L, true)));
    }
}

/* end of CLASS LuaFunc */

