#include <thread>
#include "yasio/bindings/lyasio.h"
#include "yasio/cxx17/string_view.hpp"
#if YASIO__HAS_CXX14
#  if !YASIO__HAS_CXX20
#    include "sol/sol.hpp"
#  else
#    include "sol3/sol.hpp"
#  endif
#else
#  include "kaguya/kaguya.hpp"
#endif

int main(int argc, char** argv)
{
#if YASIO__HAS_CXX14
  sol::state s;
  s.open_libraries();
  luaopen_yasio(s.lua_state());

  cxx17::string_view path = argv[0];
  auto pos                = path.find_last_of("/\\");
  if (pos != cxx17::string_view::npos)
    path.remove_suffix(path.size() - pos - 1);
  std::string package_path = s["package"]["path"];
  package_path.push_back(';');
  package_path.append(path.data());
  package_path.append("scripts/?.lua;./scripts/?.lua");
  s["package"]["path"] = package_path;

  package_path           = s["package"]["path"];
  sol::function function = s.script_file("scripts/example.lua");

  do
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  } while (!function.call(50.0 / 1000));
#else
  kaguya::State s;
  s.openlibs();
  luaopen_yasio(s.state());

  cxx17::string_view path = argv[0];
  auto pos                = path.find_last_of("/\\");
  if (pos != cxx17::string_view::npos)
    path.remove_suffix(path.size() - pos - 1);
  std::string package_path = s["package"]["path"];
  package_path.push_back(';');
  package_path.append(path.data());
  package_path.append("scripts/?.lua;./scripts/?.lua");
  s["package"]["path"] = package_path;

  auto function = s.loadfile("scripts/example.lua");
  auto update   = function();
  do
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  } while (!update(50.0 / 1000));
#endif
  return 0;
}
