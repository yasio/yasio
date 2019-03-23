#include "yasio/lyasio.h"
#include "yasio/detail/string_view.hpp"
#include "yasio/detail/sol.hpp"
#include <thread>
#pragma comment(lib, "lua51.lib")

int main(int argc, char **argv)
{
  sol::state s;
  s.open_libraries();
  luaopen_yasio(s.lua_state());

  sol::function function = s.script_file("example.lua");

  do
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  } while (!function.call(50.0 / 1000));

  return 0;
}
