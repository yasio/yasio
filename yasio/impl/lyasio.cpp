//////////////////////////////////////////////////////////////////////////////////////////
// A cross platform socket APIs, support ios & android & wp8 & window store
// universal app
//////////////////////////////////////////////////////////////////////////////////////////
/*
The MIT License (MIT)

Copyright (c) 2012-2019 halx99

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include "yasio/ibinarystream.h"
#include "yasio/obinarystream.h"
#include "yasio/yasio.h"
#include "yasio/lyasio.h"

namespace lyasio
{
class ibstream : public ibinarystream
{
public:
  ibstream(std::vector<char> blob) : ibinarystream(), blob_(std::move(blob))
  {
    this->assign(blob_.data(), blob_.size());
  }
  ibstream(const obinarystream *obs) : ibinarystream(), blob_(obs->buffer())
  {
    this->assign(blob_.data(), blob_.size());
  }

private:
  std::vector<char> blob_;
};
} // namespace lyasio

#if _HAS_CXX17_FULL_FEATURES

#  include "yasio/detail/sol.hpp"

extern "C" {

YASIO_API int luaopen_yasio(lua_State *L)
{
  using namespace purelib::inet;
  sol::state_view sol2(L);

  auto yasio = sol2.create_named_table("yasio");

  yasio.new_usertype<io_hostent>(
      "io_hostent", sol::constructors<io_hostent(), io_hostent(const std::string &, u_short)>(),
      "host", &io_hostent::host_, "port", &io_hostent::port_);

  yasio.new_usertype<io_event>(
      "io_event", "channel_index", &io_event::channel_index, "kind", &io_event::kind, "status",
      &io_event::status, "transport", &io_event::transport, "take_packet", [](io_event *event) {
        return std::unique_ptr<lyasio::ibstream>(new lyasio::ibstream(event->take_packet()));
      });

  yasio.new_usertype<io_service>(
      "io_service", "start_service",
      sol::overload(
          static_cast<void (io_service::*)(std::vector<io_hostent>, io_event_callback_t)>(
              &io_service::start_service),
          static_cast<void (io_service::*)(const io_hostent *channel_eps, io_event_callback_t cb)>(
              &io_service::start_service)),
      "stop_service", &io_service::stop_service, "set_option",
      [](io_service *service, int opt, sol::variadic_args va) {
        switch (opt)
        {
          case YOPT_CHANNEL_REMOTE_HOST:
            service->set_option(opt, static_cast<int>(va[0]), va[1].as<const char *>());
            break;
          case YOPT_CHANNEL_REMOTE_PORT:
          case YOPT_CHANNEL_LOCAL_PORT:
            service->set_option(opt, static_cast<int>(va[0]), static_cast<int>(va[1]));
            break;
          case YOPT_CHANNEL_REMOTE_ENDPOINT:
            service->set_option(opt, static_cast<int>(va[0]), va[1].as<const char *>(),
                                static_cast<int>(va[2]));
            break;
          case YOPT_TCP_KEEPALIVE:
            service->set_option(opt, static_cast<int>(va[0]), static_cast<int>(va[1]),
                                static_cast<int>(va[2]));
            break;
          case YOPT_LFBFD_PARAMS:
            service->set_option(opt, static_cast<int>(va[0]), static_cast<int>(va[1]),
                                static_cast<int>(va[2]), static_cast<int>(va[3]));
            break;
          case YOPT_RESOLV_FUNCTION: // lua does not support set custom
                                     // resolv function
            break;
          case YOPT_IO_EVENT_CALLBACK:
            (void)0;
            {
              sol::function fn           = va[0];
              io_event_callback_t fnwrap = [=](event_ptr e) mutable -> void { fn(std::move(e)); };
              service->set_option(opt, std::addressof(fnwrap));
            }
            break;
          default:
            service->set_option(opt, static_cast<int>(va[0]));
        }
      },
      "dispatch_events", &io_service::dispatch_events, "open", &io_service::open, "write",
      sol::overload(
          [](io_service *service, transport_ptr transport, stdport::string_view s) {
            service->write(transport, std::vector<char>(s.data(), s.data() + s.length()));
          },
          [](io_service *service, transport_ptr transport, obinarystream *obs) {
            service->write(transport, obs->take_buffer());
          }));

  // ##-- obstream
  yasio.new_usertype<obinarystream>(
      "obstream", "push32", &obinarystream::push32, "pop32",
      sol::overload(static_cast<void (obinarystream ::*)()>(&obinarystream::pop32),
                    static_cast<void (obinarystream ::*)(uint32_t)>(&obinarystream::pop32)),
      "push24", &obinarystream::push24, "pop24",
      sol::overload(static_cast<void (obinarystream ::*)()>(&obinarystream::pop24),
                    static_cast<void (obinarystream ::*)(uint32_t)>(&obinarystream::pop24)),
      "push16", &obinarystream::push16, "pop16",
      sol::overload(static_cast<void (obinarystream ::*)()>(&obinarystream::pop16),
                    static_cast<void (obinarystream ::*)(uint16_t)>(&obinarystream::pop16)),
      "push8", &obinarystream::push8, "pop8",
      sol::overload(static_cast<void (obinarystream ::*)()>(&obinarystream::pop8),
                    static_cast<void (obinarystream ::*)(uint8_t)>(&obinarystream::pop8)),
      "write_bool", &obinarystream::write_i<bool>, "write_i8", &obinarystream::write_i<int8_t>,
      "write_i16", &obinarystream::write_i<int16_t>, "write_i24", &obinarystream::write_i24,
      "write_i32", &obinarystream::write_i<int32_t>, "write_i64", &obinarystream::write_i<int64_t>,
      "write_u8", &obinarystream::write_i<uint8_t>, "write_u16", &obinarystream::write_i<uint16_t>,
      "write_u32", &obinarystream::write_i<uint32_t>, "write_u64",
      &obinarystream::write_i<uint64_t>, "write_f", &obinarystream::write_i<float>, "write_lf",
      &obinarystream::write_i<double>,

      "write_string",
      static_cast<size_t (obinarystream::*)(stdport::string_view)>(&obinarystream::write_v),
      "length", &obinarystream::length, "to_string",
      [](obinarystream *obs) { return stdport::string_view(obs->data(), obs->length()); });

  // ##-- lyasio::ibstream
  yasio.new_usertype<lyasio::ibstream>(
      "lyasio::ibstream",
      sol::constructors<lyasio::ibstream(std::vector<char>),
                        lyasio::ibstream(const obinarystream *)>(),
      "read_bool", &lyasio::ibstream::read_ix<bool>, "read_i8", &lyasio::ibstream::read_ix<int8_t>,
      "read_i16", &lyasio::ibstream::read_ix<int16_t>, "read_i24", &lyasio::ibstream::read_i24,
      "read_i32", &lyasio::ibstream::read_ix<int32_t>, "read_i64",
      &lyasio::ibstream::read_ix<int64_t>, "read_u8", &lyasio::ibstream::read_ix<uint8_t>,
      "read_u16", &lyasio::ibstream::read_ix<uint16_t>, "read_u24", &lyasio::ibstream::read_u24,
      "read_u32", &lyasio::ibstream::read_ix<uint32_t>, "read_u64",
      &lyasio::ibstream::read_ix<uint64_t>, "read_f", &lyasio::ibstream::read_ix<float>, "read_lf",
      &lyasio::ibstream::read_ix<double>, "read_string",
      static_cast<stdport::string_view (lyasio::ibstream::*)()>(&lyasio::ibstream::read_v),
      "to_string",
      [](lyasio::ibstream *ibs) { return stdport::string_view(ibs->data(), ibs->size()); });

  // ##-- yasio enums
  yasio["YCM_TCP_CLIENT"]               = YCM_TCP_CLIENT;
  yasio["YCM_TCP_SERVER"]               = YCM_TCP_SERVER;
  yasio["YCM_UDP_CLIENT"]               = YCM_UDP_CLIENT;
  yasio["YCM_UDP_SERVER"]               = YCM_UDP_SERVER;
  yasio["YEK_CONNECT_RESPONSE"]         = YEK_CONNECT_RESPONSE;
  yasio["YEK_CONNECTION_LOST"]          = YEK_CONNECTION_LOST;
  yasio["YEK_PACKET"]                   = YEK_PACKET;
  yasio["YOPT_CONNECT_TIMEOUT"]         = YOPT_CONNECT_TIMEOUT;
  yasio["YOPT_SEND_TIMEOUT"]            = YOPT_CONNECT_TIMEOUT;
  yasio["YOPT_RECONNECT_TIMEOUT"]       = YOPT_RECONNECT_TIMEOUT;
  yasio["YOPT_DNS_CACHE_TIMEOUT"]       = YOPT_DNS_CACHE_TIMEOUT;
  yasio["YOPT_DEFER_EVENT"]             = YOPT_DEFER_EVENT;
  yasio["YOPT_TCP_KEEPALIVE"]           = YOPT_TCP_KEEPALIVE;
  yasio["YOPT_RESOLV_FUNCTION"]         = YOPT_RESOLV_FUNCTION;
  yasio["YOPT_LOG_FILE"]                = YOPT_LOG_FILE;
  yasio["YOPT_LFBFD_PARAMS"]            = YOPT_LFBFD_PARAMS;
  yasio["YOPT_IO_EVENT_CALLBACK"]       = YOPT_IO_EVENT_CALLBACK;
  yasio["YOPT_CHANNEL_LOCAL_PORT"]      = YOPT_CHANNEL_LOCAL_PORT;
  yasio["YOPT_CHANNEL_REMOTE_HOST"]     = YOPT_CHANNEL_REMOTE_HOST;
  yasio["YOPT_CHANNEL_REMOTE_PORT"]     = YOPT_CHANNEL_REMOTE_PORT;
  yasio["YOPT_CHANNEL_REMOTE_ENDPOINT"] = YOPT_CHANNEL_REMOTE_ENDPOINT;

  return yasio.push(); /* return 'yasio' table */
}

} /* extern "C" */

#else

#  include "yasio/detail/kaguya.hpp"
/// customize the type conversion from/to lua
namespace kaguya
{
// stdport::string_view
template <> struct lua_type_traits<stdport::string_view>
{
  typedef stdport::string_view get_type;
  typedef stdport::string_view push_type;

  static bool strictCheckType(lua_State *l, int index) { return lua_type(l, index) == LUA_TSTRING; }
  static bool checkType(lua_State *l, int index) { return lua_isstring(l, index) != 0; }
  static get_type get(lua_State *l, int index)
  {
    size_t size        = 0;
    const char *buffer = lua_tolstring(l, index, &size);
    return stdport::string_view(buffer, size);
  }
  static int push(lua_State *l, push_type s)
  {
    lua_pushlstring(l, s.data(), s.size());
    return 1;
  }
};
// std::vector<char>
template <> struct lua_type_traits<std::vector<char>>
{
  typedef std::vector<char> get_type;
  typedef const std::vector<char> &push_type;

  static bool strictCheckType(lua_State *l, int index) { return lua_type(l, index) == LUA_TSTRING; }
  static bool checkType(lua_State *l, int index) { return lua_isstring(l, index) != 0; }
  static get_type get(lua_State *l, int index)
  {
    size_t size        = 0;
    const char *buffer = lua_tolstring(l, index, &size);
    return std::vector<char>(buffer, buffer + size);
  }
  static int push(lua_State *l, push_type s)
  {
    lua_pushlstring(l, s.data(), s.size());
    return 1;
  }
};
}; // namespace kaguya

extern "C" {

YASIO_API int luaopen_yasio(lua_State *L)
{
  using namespace purelib::inet;
  kaguya::State state(L);

  auto yasio     = state.newTable();
  state["yasio"] = yasio;
  // No any interface need export, only for holder
  yasio["io_transport"].setClass(kaguya::UserdataMetatable<io_transport>());

  yasio["io_hostent"].setClass(
      kaguya::UserdataMetatable<io_hostent>()
          .setConstructors<io_hostent(), io_hostent(const std::string &, u_short)>()
          .addProperty("host", &io_hostent::get_ip, &io_hostent::set_ip)
          .addProperty("port", &io_hostent::get_port, &io_hostent::set_port));

  yasio["io_event"].setClass(kaguya::UserdataMetatable<io_event>()
                                 .addFunction("channel_index", &io_event::channel_index)
                                 .addFunction("kind", &io_event::kind)
                                 .addFunction("status", &io_event::status)
                                 .addFunction("transport", &io_event::transport)
                                 .addStaticFunction("take_packet", [](io_event *ev) {
                                   return std::shared_ptr<lyasio::ibstream>(
                                       new lyasio::ibstream(ev->take_packet()));
                                 }));

  yasio["io_service"].setClass(
      kaguya::UserdataMetatable<io_service>()
          .setConstructors<io_service()>()
          .addOverloadedFunctions(
              "start_service",
              static_cast<void (io_service::*)(std::vector<io_hostent>, io_event_callback_t)>(
                  &io_service::start_service),
              static_cast<void (io_service::*)(const io_hostent *channel_eps,
                                               io_event_callback_t cb)>(&io_service::start_service))
          .addFunction("stop_service", &io_service::stop_service)
          .addFunction("dispatch_events", &io_service::dispatch_events)
          .addFunction("open", &io_service::open)
          .addOverloadedFunctions(
              "write",
              static_cast<void (io_service::*)(transport_ptr transport, std::vector<char> data)>(
                  &io_service::write),
              [](io_service *service, transport_ptr transport, obinarystream *obs) {
                service->write(transport, obs->take_buffer());
              })
          .addStaticFunction("set_option", [](io_service *service, int opt,
                                              kaguya::VariadicArgType args) {
            switch (opt)
            {
              case YOPT_CHANNEL_REMOTE_HOST:
                service->set_option(opt, static_cast<int>(args[0]),
                                    static_cast<const char *>(args[1]));
                break;
              case YOPT_CHANNEL_REMOTE_PORT:
              case YOPT_CHANNEL_LOCAL_PORT:
                service->set_option(opt, static_cast<int>(args[0]), static_cast<int>(args[1]));
                break;
              case YOPT_CHANNEL_REMOTE_ENDPOINT:
                service->set_option(opt, static_cast<int>(args[0]),
                                    static_cast<const char *>(args[1]), static_cast<int>(args[2]));
                break;
              case YOPT_TCP_KEEPALIVE:
                service->set_option(opt, static_cast<int>(args[0]), static_cast<int>(args[1]),
                                    static_cast<int>(args[2]));
                break;
              case YOPT_LFBFD_PARAMS:
                service->set_option(opt, static_cast<int>(args[0]), static_cast<int>(args[1]),
                                    static_cast<int>(args[2]), static_cast<int>(args[3]));
                break;
              case YOPT_RESOLV_FUNCTION: // lua does not support set custom
                                         // resolv function
                break;
              case YOPT_IO_EVENT_CALLBACK:
                (void)0;
                {
                  kaguya::LuaFunction fn     = args[0];
                  io_event_callback_t fnwrap = [=](event_ptr e) mutable -> void { fn(e.get()); };
                  service->set_option(opt, std::addressof(fnwrap));
                }
                break;
              default:
                service->set_option(opt, static_cast<int>(args[0]));
            }
          }));

  // ##-- obinarystream
  yasio["obstream"].setClass(
      kaguya::UserdataMetatable<obinarystream>()
          .setConstructors<obinarystream(), obinarystream(int)>()
          .addFunction("push32", &obinarystream::push32)
          .addOverloadedFunctions(
              "pop32", static_cast<void (obinarystream ::*)()>(&obinarystream::pop32),
              static_cast<void (obinarystream ::*)(uint32_t)>(&obinarystream::pop32))
          .addFunction("push24", &obinarystream::push24)
          .addOverloadedFunctions(
              "pop24", static_cast<void (obinarystream ::*)()>(&obinarystream::pop24),
              static_cast<void (obinarystream ::*)(uint32_t)>(&obinarystream::pop24))
          .addFunction("push16", &obinarystream::push16)
          .addOverloadedFunctions(
              "pop16", static_cast<void (obinarystream ::*)()>(&obinarystream::pop16),
              static_cast<void (obinarystream ::*)(uint16_t)>(&obinarystream::pop16))
          .addFunction("push8", &obinarystream::push8)
          .addOverloadedFunctions(
              "pop8", static_cast<void (obinarystream ::*)()>(&obinarystream::pop8),
              static_cast<void (obinarystream ::*)(uint8_t)>(&obinarystream::pop8))
          .addFunction("write_bool", &obinarystream::write_i<bool>)
          .addFunction("write_i8", &obinarystream::write_i<int8_t>)
          .addFunction("write_i16", &obinarystream::write_i<int16_t>)
          .addFunction("write_i24", &obinarystream::write_i24)
          .addFunction("write_i32", &obinarystream::write_i<int32_t>)
          .addFunction("write_i64", &obinarystream::write_i<int64_t>)
          .addFunction("write_u8", &obinarystream::write_i<uint8_t>)
          .addFunction("write_u16", &obinarystream::write_i<uint16_t>)
          .addFunction("write_u32", &obinarystream::write_i<uint32_t>)
          .addFunction("write_u64", &obinarystream::write_i<uint64_t>)
          .addFunction("write_f", &obinarystream::write_i<float>)
          .addFunction("write_lf", &obinarystream::write_i<double>)
          .addFunction("write_string", static_cast<size_t (obinarystream::*)(stdport::string_view)>(
                                           &obinarystream::write_v))
          .addFunction("length", &obinarystream::length)
          .addStaticFunction("to_string", [](obinarystream *obs) {
            return stdport::string_view(obs->data(), obs->length());
          }));

  // ##-- ibinarystream
  yasio["ibinarystream"].setClass(
      kaguya::UserdataMetatable<ibinarystream>()
          .setConstructors<ibinarystream(), ibinarystream(const void *, int),
                           ibinarystream(const obinarystream *)>()
          .addFunction("read_bool", &ibinarystream::read_ix<bool>)
          .addFunction("read_i8", &ibinarystream::read_ix<int8_t>)
          .addFunction("read_i16", &ibinarystream::read_ix<int16_t>)
          .addFunction("read_i24", &ibinarystream::read_i24)
          .addFunction("read_i32", &ibinarystream::read_ix<int32_t>)
          .addFunction("read_i64", &ibinarystream::read_ix<int64_t>)
          .addFunction("read_u8", &ibinarystream::read_ix<uint8_t>)
          .addFunction("read_u16", &ibinarystream::read_ix<uint16_t>)
          .addFunction("read_u24", &ibinarystream::read_u24)
          .addFunction("read_u32", &ibinarystream::read_ix<uint32_t>)
          .addFunction("read_u64", &ibinarystream::read_ix<uint64_t>)
          .addFunction("read_f", &ibinarystream::read_ix<float>)
          .addFunction("read_lf", &ibinarystream::read_ix<double>)
          .addFunction("read_string", static_cast<stdport::string_view (ibinarystream::*)()>(
                                          &ibinarystream::read_v))
          .addStaticFunction("to_string", [](ibinarystream *ibs) {
            return stdport::string_view(ibs->data(), ibs->size());
          }));

  // ##-- ibstream
  state["ibstream"].setClass(kaguya::UserdataMetatable<lyasio::ibstream, ibinarystream>()
                                 .setConstructors<lyasio::ibstream(std::vector<char>),
                                                  lyasio::ibstream(const obinarystream *)>());

  // ##-- yasio enums
  yasio["YCM_TCP_CLIENT"]               = YCM_TCP_CLIENT;
  yasio["YCM_TCP_SERVER"]               = YCM_TCP_SERVER;
  yasio["YCM_UDP_CLIENT"]               = YCM_UDP_CLIENT;
  yasio["YCM_UDP_SERVER"]               = YCM_UDP_SERVER;
  yasio["YEK_CONNECT_RESPONSE"]         = YEK_CONNECT_RESPONSE;
  yasio["YEK_CONNECTION_LOST"]          = YEK_CONNECTION_LOST;
  yasio["YEK_PACKET"]                   = YEK_PACKET;
  yasio["YOPT_CONNECT_TIMEOUT"]         = YOPT_CONNECT_TIMEOUT;
  yasio["YOPT_SEND_TIMEOUT"]            = YOPT_CONNECT_TIMEOUT;
  yasio["YOPT_RECONNECT_TIMEOUT"]       = YOPT_RECONNECT_TIMEOUT;
  yasio["YOPT_DNS_CACHE_TIMEOUT"]       = YOPT_DNS_CACHE_TIMEOUT;
  yasio["YOPT_DEFER_EVENT"]             = YOPT_DEFER_EVENT;
  yasio["YOPT_TCP_KEEPALIVE"]           = YOPT_TCP_KEEPALIVE;
  yasio["YOPT_RESOLV_FUNCTION"]         = YOPT_RESOLV_FUNCTION;
  yasio["YOPT_LOG_FILE"]                = YOPT_LOG_FILE;
  yasio["YOPT_LFBFD_PARAMS"]            = YOPT_LFBFD_PARAMS;
  yasio["YOPT_IO_EVENT_CALLBACK"]       = YOPT_IO_EVENT_CALLBACK;
  yasio["YOPT_CHANNEL_LOCAL_PORT"]      = YOPT_CHANNEL_LOCAL_PORT;
  yasio["YOPT_CHANNEL_REMOTE_HOST"]     = YOPT_CHANNEL_REMOTE_HOST;
  yasio["YOPT_CHANNEL_REMOTE_PORT"]     = YOPT_CHANNEL_REMOTE_PORT;
  yasio["YOPT_CHANNEL_REMOTE_ENDPOINT"] = YOPT_CHANNEL_REMOTE_ENDPOINT;

  return yasio.push(); /* return 'yasio' table */
}

} /* extern "C" */

#endif /* _HAS_CXX17_FULL_FEATURES */
