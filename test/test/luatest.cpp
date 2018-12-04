#include "lmasio.h"
#include "crypto-support/crypto_wrapper.h"
#include <thread>
#pragma comment(lib, "lua51.lib")

void lua_open_crypto(lua_State* L)
{
    sol::state_view sol2(L);
    auto crypto = sol2.create_named_table("crypto");
    auto rsa = crypto.create_named("rsa");
    rsa.set_function("pri_encrypt", &crypto::rsa::pri::encrypt);
    rsa.set_function("pri_decrypt", &crypto::rsa::pri::decrypt);
    rsa.set_function("pub_encrypt", &crypto::rsa::pub::encrypt);
    rsa.set_function("pub_decrypt", &crypto::rsa::pub::decrypt);
    rsa.set_function("pri_encrypt2", &crypto::rsa::pri::encrypt2);
    rsa.set_function("pri_decrypt2", &crypto::rsa::pri::decrypt2);
    rsa.set_function("pub_encrypt2", &crypto::rsa::pub::encrypt2);
    rsa.set_function("pub_decrypt2", &crypto::rsa::pub::decrypt2);
}

int main(int argc, char** argv)
{
    sol::state s;
    s.open_libraries();
    lua_open_masio(s.lua_state());
    lua_open_crypto(s.lua_state());

    s.script_file("example.lua");

    do {
        std::this_thread::sleep_for(std::chrono::duration(std::chrono::milliseconds(50)));
    } while (!s["global_update"].call(50.0 / 1000));

    return 0;
}
