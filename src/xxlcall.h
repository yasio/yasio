#ifndef _XXLUA_CALL_HELPER_H_
#define _XXLUA_CALL_HELPER_H_
#include <type_traits>
#include <string>
#include <stdio.h>

#include "object_pool.h"
#include "ref_ptr.h"

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#ifdef _USING_IN_COCOS2DX
#include "cocos2d.h"
#define LUAX_LOG_FUNC cocos2d::log
#else
#define LUAX_LOG_FUNC(format,...) printf(((format) "\n"), ##__VA_ARGS__)
#endif


void       luax_setVM(lua_State *L);

lua_State* luax_getVM(void);

void       luax_unref(int ref);

void       luax_dump_stack(void);

/*
* lua interactive functions
*/
void       luax_table_setvalue(const char* tableName, const char* tableItem, int value);
void       luax_table_setvalue(const char* tableName, const char* tableItem, const char* value, int len);


bool       luax_assume_read(lua_State* L, const char* field);
bool       luax_assume_write(lua_State* L, const char*& field);

/*
usage:
[] (luax_value value) {
}
*/
template<typename _Fty> inline
void       luax_select_table(const char* table, const _Fty& callback)
{
    auto L = luax_getVM();
    lua_getglobal(L, table);
    callback(L);
    lua_pop(L, 1);
}

/*
usage:
[] (luax_value value) {
}
*/
template<typename _Fty> inline
void       luax_foreach(const char* table, const _Fty& callback)
{
    auto L = luax_getVM();

    auto top = lua_gettop(L);

    luax_assume_read(L, table);
    bool istable = lua_istable(L, -1);

    if (istable)
    {
        auto n = luaL_getn(L, -1);
        for (auto i = 1; i <= n; ++i)
        {
            lua_rawgeti(L, -1, i);
            callback(L);
            lua_pop(L, 1);
        }
    }

    lua_settop(L, top);
}

/*
* lua value iterator helper
*/
class luax_value
{
public:
    luax_value(lua_State* l) : l_(l) {}

    /// for array elements
    bool        is_string(void) const { return !!lua_isstring(l_, -1); }
    bool        is_number(void) const { return !!lua_isnumber(l_, -1); }
    bool        is_boolean(void) const { return !!lua_isboolean(l_, -1); }

    const char* to_string(void) const { return lua_tostring(l_, -1); }
    std::string to_lstring(void) const { size_t len = 0; const char* s = lua_tolstring(l_, -1, &len); return std::string(s, len); }
    double      to_number(void) const { return lua_tonumber(l_, -1); }
    bool        to_boolean(void) const { return !!lua_toboolean(l_, -1); }
    LUA_INTEGER to_integer(void) const { return lua_tointeger(l_, -1); }

    /// for table fields
    const char* getfield(const char* name, const char* default_value) const { const char* ret;  lua_getfield(l_, -1, name); ret = lua_tostring(l_, -1); lua_pop(l_, 1);  return ret; }
    double      getfield(const char* name, double default_value) const { LUA_NUMBER ret; lua_getfield(l_, -1, name); ret = lua_tonumber(l_, -1); lua_pop(l_, 1); return ret; }
    bool        getfield(const char* name, bool default_value) const { bool ret; lua_getfield(l_, -1, name); ret = !!lua_toboolean(l_, -1); lua_pop(l_, 1); return ret; }
    LUA_INTEGER getfield(const char* name, LUA_INTEGER default_value) const { LUA_INTEGER ret; lua_getfield(l_, -1, name); ret = lua_tointeger(l_, -1); lua_pop(l_, 1); return ret; }

    template<typename _Ty>
    void foreach(const _Ty& callback)
    {
        if (lua_istable(l_, -1))
        {
            auto n = luaL_getn(l_, -1);
            for (auto i = 1; i <= n; ++i)
            {
                lua_rawgeti(l_, -1, i);
                callback(l_);
                lua_pop(l_, 1);
            }
        }
    }

private:
    lua_State* l_;
};

/* powerful lua call, vardic template. */

#ifndef _POWERFUL_LUA_CALL_HELPER_
#define _POWERFUL_LUA_CALL_HELPER_

/* lua native support arg extract helpers */
/* CLASS LuaFunc */
#define  ENABLE_REFERENCE \
	private: \
	    unsigned  int referenceCount_; \
	public: \
		void retain() { ++referenceCount_; } \
		void release(){ --referenceCount_; if (referenceCount_ == 0) delete this; } \
    private:

template<size_t _IID>
class LuaRef
{
    ENABLE_REFERENCE
        int id_;
public:

    LuaRef(int refid)
    {
        referenceCount_ = 1;
        this->id_ = refid;
    }

    ~LuaRef()
    {
        if (!isNil())
            luaL_unref(luax_getVM(), LUA_REGISTRYINDEX, this->id_);
    }

    int Id() const
    {
        return id_;
    }

    bool isNil(void) const
    {
        return id_ == LUA_REFNIL;
    }

    static void * operator new(size_t /*size*/)
    {
        return getPool().get();
    }

    static void operator delete(void *p)
    {
        getPool().release(p);
    }

    static object_pool<LuaRef<_IID>, 128>& getPool() {
        static object_pool<LuaRef<_IID>, 128> s_pool;
        return s_pool;
    }
};

typedef LuaRef<0> LuaObj;
typedef LuaRef<1> LuaFunc;

/// helpers
extern int luax_getglobal(const char* s);

extern int luax_getglobal2(const char* func);

extern int luax_rawgeti(int refid);

extern int luax_getobjfield(int objid, const char* field);

/// FUNCTION TEMPLATE: luax_vcall
template<typename..._Args> inline
void luax_vcall(const char*  func, const _Args&...args);

template<typename _Result, typename..._Args> inline
_Result luax_vxcall(const char*  func, const _Args&...args);

/// TEMPLATE luax_vxcall alias
template<typename..._Args> inline
int luax_vicall(const char*  func, const _Args&...args);

template<typename..._Args> inline
float luax_vfcall(const char*  func, const _Args&...args);

template<typename..._Args> inline
double luax_vdcall(const char*  func, const _Args&...args);

template<typename..._Args> inline
std::string luax_vvcall(const char*  func, const _Args&...args);

/// FUNCTION TEMPLATE: luax_vpcall
template<typename..._Args> inline
void luax_pvcall(const char*  func, const _Args&...args);

template<typename _Result, typename..._Args> inline
_Result luax_pvxcall(const char*  func, const _Args&...args);

/// TEMPLATE luax_pvxcall alias
template<typename..._Args> inline
int luax_pvicall(const char*  func, const _Args&...args);

template<typename..._Args> inline
float luax_pvfcall(const char*  func, const _Args&...args);

template<typename..._Args> inline
double luax_pvdcall(const char*  func, const _Args&...args);

template<typename..._Args> inline
std::string luax_pvvcall(const char*  func, const _Args&...args);

/// arg push helper
inline
void luax_vpusharg(lua_State* L, int& carg, int& narg)
{
}

inline
void luax_vpusharg(lua_State* L, int& carg, int& narg, size_t arg)
{
    ++carg;
    if (lua_checkstack(L, 1))
        lua_pushinteger(L, arg), ++narg;
}

inline
void luax_vpusharg(lua_State* L, int& carg, int& narg, std::nullptr_t /*arg*/)
{
    ++carg;
    if (lua_checkstack(L, 1))
        lua_pushnil(L), ++narg;
}

inline
void luax_vpusharg(lua_State* L, int& carg, int& narg, bool arg)
{
    ++carg;
    if (lua_checkstack(L, 1))
        lua_pushboolean(L, arg), ++narg;
}

inline
void luax_vpusharg(lua_State* L, int& carg, int& narg, int arg)
{
    ++carg;
    if (lua_checkstack(L, 1))
        lua_pushinteger(L, arg), ++narg;
}

inline
void luax_vpusharg(lua_State* L, int& carg, int& narg, float arg)
{
    ++carg;
    if (lua_checkstack(L, 1))
        lua_pushnumber(L, arg), ++narg;
}

inline
void luax_vpusharg(lua_State* L, int& carg, int& narg, double arg)
{
    ++carg;
    if (lua_checkstack(L, 1))
        lua_pushnumber(L, arg), ++narg;
}

inline
void luax_vpusharg(lua_State* L, int& carg, int& narg, const char* arg)
{
    ++carg;
    if (lua_checkstack(L, 1))
        lua_pushstring(L, arg), ++narg;
}

inline
void luax_vpusharg(lua_State* L, int& carg, int& narg, const std::string& arg)
{
    ++carg;
    if (lua_checkstack(L, 1))
        lua_pushlstring(L, arg.c_str(), arg.length()), ++narg;
}

inline
void luax_vpusharg(lua_State* L, int& carg, int& narg, void* arg)
{
    ++carg;
    if (lua_checkstack(L, 1))
        lua_pushlightuserdata(L, arg), ++narg;
}

/// cocos2d-x object support
#define LUAX_VCALL_ADD_ANY_SUPPORT(type,prefix,ltype) \
	inline \
	void luax_vpusharg(lua_State* L, int& carg, int& narg, type* arg) \
{ \
	++carg; \
if (lua_checkstack(L, 1)) \
	object_to_luaval<type>(L,prefix ltype, arg),/*tolua_pushuserdata(L, arg),*/ ++narg; \
}

#define LUAX_VCALL_ADD_CCOBJ_SUPPORT(type) LUAX_VCALL_ADD_ANY_SUPPORT(type,"cc.", #type)
#define LUAX_VCALL_ADD_CCUI_SUPPORT(type,ltype) LUAX_VCALL_ADD_ANY_SUPPORT(type,"ccui.",ltype)

template<typename _Ty, typename..._Args> inline
void luax_vpusharg(lua_State* L, int& carg, int& narg, _Ty arg1, const _Args&...args)
{
    luax_vpusharg(L, carg, narg, arg1);
    luax_vpusharg(L, carg, narg, args...);
}

template<typename _Ty> inline
bool luax_dogetval(lua_State* L, _Ty& output_value);

template<> inline
bool luax_dogetval(lua_State* L, std::vector<int>& output_value)
{
    if (lua_istable(L, -1)) {
        int count = luaL_getn(L, -1);

        for (int i = 0; i < count; ++i)
        {
            lua_rawgeti(L, -1, i + 1);
            auto m = lua_tonumber(L, -1);
            output_value.push_back(m);
            lua_pop(L, 1);
        }

        return true;
    }

    return false;
}

template<> inline
bool luax_dogetval(lua_State* L, bool& output_value)
{
    if (lua_isboolean(L, -1)) {
        output_value = !!lua_toboolean(L, -1);
        return false;
    }
    return true;
}

template<> inline
bool luax_dogetval(lua_State* L, int& output_value)
{
    if (lua_isnumber(L, -1)) {
        output_value = lua_tointeger(L, -1);
        return true;
    }
    return false;
}

template<> inline
bool luax_dogetval<float>(lua_State* L, float& output_value)
{
    if (lua_isnumber(L, -1)) {
        output_value = lua_tonumber(L, -1);
        return true;
    }
    return false;
}

template<> inline
bool luax_dogetval<double>(lua_State* L, double& output_value)
{
    if (lua_isnumber(L, -1)) {
        output_value = lua_tonumber(L, -1);
        return true;
    }
    return false;
}

template<> inline
bool luax_dogetval<std::string>(lua_State* L, std::string& output_value)
{
    if (lua_isstring(L, -1)) {
        size_t len = 0;
        const char* str = lua_tolstring(L, -1, &len);
        output_value.assign(str, len);
        return true;
    }
    return false;
}

template<> inline
bool luax_dogetval<const char*>(lua_State* L, const char*& default_value)
{
    if (lua_isstring(L, -1)) {
        default_value = lua_tostring(L, -1);
        return true;
    }
    return false;
}

/// luax_dosetval
template<typename _Ty> inline
void luax_dosetval(lua_State* L, const char* k, const _Ty& val)
{
    lua_pushinteger(L, val);
    lua_setfield(L, -2, k);
}

template<> inline
void luax_dosetval(lua_State* L, const char* k, const float& val)
{
    lua_pushinteger(L, val);
    lua_setfield(L, -2, k);
}

template<> inline
void luax_dosetval(lua_State* L, const char* k, const double& val)
{
    lua_pushnumber(L, val);
    lua_setfield(L, -2, k);
}

template<> inline
void luax_dosetval(lua_State* L, const char* k, const char*const& val)
{
    lua_pushstring(L, val);
    lua_setfield(L, -2, k);
}

template<> inline
void luax_dosetval(lua_State* L, const char* k, const std::string& val)
{
    lua_pushlstring(L, val.c_str(), val.size());
    lua_setfield(L, -2, k);
}

#define LUAX_ERRLOG_1(ftemplate,func) LUAX_LOG_FUNC(#ftemplate " failed, the function:%s not exist!", func);
#define LUAX_ERRLOG_2(ftemplate,func) LUAX_LOG_FUNC(#ftemplate " failed, function:%s, argument exception:carg:%d,narg:%d", func, carg, narg);
#define LUAX_ERRLOG_3(ftemplate,func) do { \
                                         const char* errmsg = lua_tostring(L, -1); \
                                         LUAX_LOG_FUNC(#ftemplate " failed, function:%s, error info:%s", func, errmsg != nullptr ? errmsg : "unknown error!"); \
                                      } while (false)

template<typename..._Args> inline
void luax_dovcall(int top, const char* func, const _Args&...args)
{
    auto L = luax_getVM();

    int carg = 0, narg = 0;

    if (!lua_isfunction(L, -1))
    {
        LUAX_ERRLOG_1(luax_dovcall, func);
        goto _L_exit;
    }

    luax_vpusharg(L, carg, narg, args...);

    if (carg != narg) {
        LUAX_ERRLOG_2(luax_dovcall, func);
        goto _L_exit;
    }

    if (lua_pcall(L, narg, 0, 0) != 0)
    {
        LUAX_ERRLOG_3(luax_dovcall, func);
        goto _L_exit;
    }

_L_exit:
    lua_settop(L, top); // resume stack
}

template<typename _Result, typename..._Args> inline
_Result luax_dovxcall(int top, const char* func, const _Args&...args)
{
    auto L = luax_getVM();

    _Result result = _Result();
    int carg = 0, narg = 0;

    if (!lua_isfunction(L, -1))
    {
        LUAX_ERRLOG_1(luax_dovxcall, func);
        goto _L_end;
    }

    luax_vpusharg(L, carg, narg, args...);

    if (carg != narg) {
        LUAX_ERRLOG_2(luax_dovxcall, func);
        goto _L_end;
    }

    if (lua_pcall(L, narg, 1, 0) != 0)
    {
        LUAX_ERRLOG_3(luax_dovxcall, func);
        goto _L_end;
    }

    luax_dogetval(L, result);

_L_end:
    lua_settop(L, top); // resume stack

    return result;
}

template<typename _Result, typename..._Args> inline
void luax_dovxcall2(int top, const char* func, _Result& result, const _Args&...args)
{
    auto L = luax_getVM();

    int carg = 0, narg = 0;

    if (!lua_isfunction(L, -1))
    {
        LUAX_ERRLOG_1(luax_dovxcall2, func);
        goto _L_end;
    }

    luax_vpusharg(L, carg, narg, args...);

    if (carg != narg) {
        LUAX_ERRLOG_2(luax_dovxcall2, func);
        goto _L_end;
    }

    if (lua_pcall(L, narg, 1, 0) != 0)
    {
        LUAX_ERRLOG_3(luax_dovxcall2, func);
        goto _L_end;
    }

    luax_dogetval(L, result);

_L_end:
    lua_settop(L, top); // resume stack
}

inline const char* luax_refid2a(int refid)
{
#define LUAX_LAMBDA_FUNC "LUA anonymous function:"
    static char s_str[64] = LUAX_LAMBDA_FUNC;
    sprintf(s_str + sizeof(LUAX_LAMBDA_FUNC) - 1, "%d", refid);
    return s_str;
}

template<typename..._Args> inline
void luax_vcall(int refid, const _Args&...args)
{
    luax_dovcall(luax_rawgeti(refid), luax_refid2a(refid), args...);
}

template<typename..._Args> inline
void luax_vcall(const ref_ptr<LuaFunc>& refid, const _Args&...args)
{
    luax_dovcall(luax_rawgeti(refid->Id()), luax_refid2a(refid->Id()), args...);
}

template<typename _Result, typename..._Args> inline
_Result luax_vxcall(const ref_ptr<LuaFunc>& refid, const _Args&...args)
{
    return luax_dovxcall<_Result>(luax_rawgeti(refid->Id()), luax_refid2a(refid->Id()), args...);
}

// Lua object func call helpers
template<typename..._Args> inline
void luax_vcall(int lobj, const char* objfunc, const _Args&...args)
{
    luax_dovcall(luax_getobjfield(lobj, objfunc), objfunc, args...);
}

template<typename..._Args> inline
void luax_vcall(const ref_ptr<LuaObj>& lobj, const char* objfunc, const _Args&...args)
{
    luax_dovcall(luax_getobjfield(lobj->Id(), objfunc), objfunc, args...);
}

template<typename _Result, typename..._Args> inline
_Result luax_vxcall(const ref_ptr<LuaObj>& lobj, const char* objfunc, const _Args&...args)
{
    return luax_dovxcall<_Result>(luax_getobjfield(lobj->Id(), objfunc), objfunc, args...);
}
/* End of Lua object func call helpers */

template<typename..._Args> inline
void luax_vcall(const char* func, const _Args&...args)
{
    luax_dovcall(luax_getglobal(func), func, args...);
}

template<typename _Result, typename..._Args> inline
_Result luax_vxcall(const char* func, const _Args&...args)
{
    return luax_dovxcall<_Result>(luax_getglobal(func), func, args...);
}

// TEMPLATE luax_vxcall alias
template<typename..._Args> inline
int luax_vicall(const char*  func, const _Args&...args)
{
    return luax_vxcall<int>(func, args...);
}

template<typename..._Args> inline
float luax_vfcall(const char*  func, const _Args&...args)
{
    return luax_vxcall<float>(func, args...);
}

template<typename..._Args> inline
double luax_vdcall(const char*  func, const _Args&...args)
{
    return luax_vxcall<double>(func, args...);
}

template<typename..._Args> inline
std::string luax_vvcall(const char*  func, const _Args&...args)
{
    return luax_vxcall<std::string>(func, args...);
}

// support any talbe prefix
template<typename..._Args> inline
void luax_pvcall(const char* func, const _Args&...args)
{
    luax_dovcall(luax_getglobal2(func), func, args...);
}

template<typename _Result, typename..._Args> inline
_Result luax_pvxcall(const char*  func, const _Args&...args)
{
    return luax_dovxcall<_Result>(luax_getglobal2(func), func, args...);
}

template<typename..._Args> inline
int luax_pvicall(const char*  func, const _Args&...args)
{
    return luax_pvxcall<int>(func, args...);
}

template<typename..._Args> inline
float luax_pvfcall(const char*  func, const _Args&...args)
{
    return luax_pvxcall<float>(func, args...);
}

template<typename..._Args> inline
double luax_pvdcall(const char*  func, const _Args&...args)
{
    return luax_pvxcall<double>(func, args...);
}

template<typename..._Args> inline
std::string luax_pvvcall(const char*  func, const _Args&...args)
{
    return luax_pvxcall<std::string>(func, args...);
}

// pvxcall use output parameter to store return value.
template<typename _Result, typename..._Args> inline
void luax_pvxcall2(const char*  func, _Result& result, const _Args&...args)
{
    luax_dovxcall2<_Result>(luax_getglobal2(func), func, result, args...);
}

#endif /* powerful lua call, vardic template. */

#ifndef _POWERFUL_LUAX_FIELD_QUERY

/* powerful get fields of table */
template<typename _Ty> inline
_Ty luax_vgetval(const char* field, const _Ty& default_value = _Ty())
{
    auto L = luax_getVM();

    auto top = lua_gettop(L); // store stack

    _Ty result;

    if (!luax_assume_read(L, field))
    {
        LUAX_LOG_FUNC("luax_vgetval field %s not exist, use default value!", field);
        result = default_value;
        goto err_exit;
    }

    luax_dogetval(L, result);

    lua_settop(L, top); // resume stack

    return result;

err_exit:
    lua_settop(L, top); // resume stack

    return std::move(result);
}

template<typename _Ty, size_t _Size> inline
const char* luax_vgetval(const char* field, const _Ty(&default_value)[_Size])
{
    return luax_vgetval(field, (const char*)default_value);
}

template<typename _Ty> inline
void luax_vsetval(const char* field, const _Ty& value)
{
    auto L = luax_getVM();

    auto top = lua_gettop(L); // store stack

    if (luax_assume_write(L, field))
    {
        luax_dosetval<_Ty>(L, field, value);
    }
    else {
        LUAX_LOG_FUNC("luax_vsetval field %s not exist!", field);
    }

    lua_settop(L, top); // resume stack
}

#endif  /* powerful get fields of table. */

void luax_retrive_arg(lua_State* L, const char* func, int n, bool& arg, bool& ok);
void luax_retrive_arg(lua_State* L, const char* func, int n, int& arg, bool& ok);
void luax_retrive_arg(lua_State* L, const char* func, int n, long long& arg, bool& ok);
void luax_retrive_arg(lua_State* L, const char* func, int n, void*& arg, bool& ok);
void luax_retrive_arg(lua_State* L, const char* func, int n, float& arg, bool& ok);
void luax_retrive_arg(lua_State* L, const char* func, int n, const char*& arg, bool& ok);
void luax_retrive_arg(lua_State* L, const char* func, int n, const char*& arg, size_t& len, bool& ok);
void luax_retrive_arg(lua_State* L, const char* func, int n, std::string& arg, bool& ok);
void luax_retrive_arg(lua_State* L, const char* func, int n, ref_ptr<LuaFunc>& arg, bool& ok);

#endif

