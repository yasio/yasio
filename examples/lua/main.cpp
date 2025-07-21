#include <thread>
#include "yasio/bindings/lyasio.hpp"
#include "yasio/string_view.hpp"
#if YASIO__HAS_CXX14
#  if YASIO__HAS_CXX17 // use sol2-3.x
#    include "sol/sol.hpp"
#  else
#    include "sol2/sol.hpp" // use sol2-2.x
#  endif
#else
#  include "kaguya/kaguya.hpp"
#endif

#if defined(_WIN32)
#  include <Windows.h>
#  include "yasio/wtimer_hres.hpp"
#endif

int main(int argc, char** argv)
{
#if defined(_WIN32)
  SetConsoleOutputCP(CP_UTF8);
#endif

#if defined(_WIN32)
  yasio::wtimer_hres whres;
#endif

#if YASIO__HAS_CXX14
  sol::state s;
  s.open_libraries();
  luaregister_yasio(s.lua_state());

  cxx17::string_view path = argv[0];
  auto pos                = path.find_last_of("/\\");
  if (pos != cxx17::string_view::npos)
    path.remove_suffix(path.size() - pos - 1);
  std::string package_path = s["package"]["path"];
  package_path.push_back(';');
  package_path.append(path.data(), path.length());
  package_path.append("scripts/?.lua;./scripts/?.lua");
  s["package"]["path"] = package_path;

  std::string new_package_path = s["package"]["path"];

  try
  {
    sol::table module_example   = s.load_file("scripts/example.lua").call();
    sol::function update_func = module_example["update"];
    do
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
    } while (!update_func(50.0 / 1000));
  }
  catch (const sol::error& e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    return -1;
  }
#else
  kaguya::State s;
  s.openlibs();
  luaregister_yasio(s.state());

  cxx17::string_view path = argv[0];
  auto pos                = path.find_last_of("/\\");
  if (pos != cxx17::string_view::npos)
    path.remove_suffix(path.size() - pos - 1);
  std::string package_path = s["package"]["path"];
  package_path.push_back(';');
  package_path.append(path.data(), path.length());
  package_path.append("scripts/?.lua;./scripts/?.lua");
  s["package"]["path"] = package_path;

  auto function = s.loadfile("scripts/example.lua");
  auto module_example   = function();
  auto update_func = module_example["update"];
  do
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  } while (!update_func(50.0 / 1000));
#endif
  return 0;
}
