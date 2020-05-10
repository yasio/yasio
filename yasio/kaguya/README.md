## kaguya is a C++ library binding to Lua, require C++11
see: https://github.com/satoren/kaguya

**pitfall: kaguya can't handle unaligned pointer from ```lua_newuserdata``` will cause crash at ```apple clang``` release optimized mode.**  
  * If you want use luabinding provided by yasio, we strong recommand upgrade your build system to c++14, then yasio will auto choose the mostly modern lua binding solution sol: https://github.com/ThePhD/sol2  
  * Another reason you don't choose ```kaguya``` is it's lost maintain more than 3 years  
  * From the reference: http://lua-users.org/lists/lua-l/2019-07/msg00197.html, if you still want use c++11, you must define compiler marco **```LUAI_USER_ALIGNMENT_T=max_align_t```** for lua_newuserdata can return properly aligned pointer  
