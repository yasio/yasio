## kaguya is a C++ library binding to Lua, require C++11
**kaguya can't handle unaligned pointer from ```lua_newuserdata``` will cause crash at clang release optimized mode without compiler definiation ```LUAI_USER_ALIGNMENT_T=max_align_t```.**  
* If you wan't use luabinding provided by yasio, we strong recommand upgrade your build system to c++14, then yasio will auto choose the mostly modern lua binding solution sol: https://github.com/ThePhD/sol2  
* Another reason you don't choose ```kaguya``` is it's lost maintain more than 3 years  
* I already send email to lua team, I think ```lua_newuserdata``` should return aligned pointer  
see: https://github.com/satoren/kaguya
