#include "lmasio.h"
#include <thread>
#pragma comment(lib, "lua51.lib")

int main(int argc, char** argv)
{
    sol::state s;
    s.open_libraries();
    lua_open_masio(s.lua_state());

    s.script_file("example.lua");

    while (1)
    {
        std::this_thread::sleep_for(std::chrono::duration(std::chrono::milliseconds(50)));
        if (s["global_update"].call(50.0 / 1000))
            break;
    }
    return 0;
}
