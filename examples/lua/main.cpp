#include "yasio/bindings/lyasio.h"
#include "yasio/cxx17/string_view.hpp"
#include "yasio/sol/sol.hpp"
#include <thread>

int main(int argc, char** argv)
{
  sol::state s;
  s.open_libraries();
  luaopen_yasio(s.lua_state());

  cxx17::string_view path  = argv[0];
  auto pos = path.find_last_of("/\\");
  if (pos != cxx17::string_view::npos)
    path.remove_suffix(path.size() - pos - 1);
  std::string package_path = s["package"]["path"];
  package_path.push_back(';');
  package_path.append(path.data());
  package_path.append("scripts/?.lua;./scripts/?.lua");
  s["package"]["path"]   = package_path; 

  package_path           = s["package"]["path"];
  sol::function function = s.script_file("scripts/example.lua");

  do
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  } while (!function.call(50.0 / 1000));

  return 0;
}
