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
#if !defined(LUA_LIB)
#  define LUA_LIB
#endif
#include "yasio/ibstream.h"
#include "yasio/obstream.h"
#include "yasio/yasio.h"
#include "yasio/lyasio.h"

namespace lyasio
{
static auto obstream_write_v = [](yasio::obstream *obs, yasio::string_view val,
                                  int length_field_bits) {
  switch (length_field_bits)
  {
    case 16:
      return obs->write_v16(val);
    case 8:
      return obs->write_v8(val);
    default: // default is: 32bits length field
      return obs->write_v(val);
  }
};
static auto ibstream_read_v = [](yasio::ibstream *ibs, int length_field_bits,
                                 bool /*raw*/ = false) {
  switch (length_field_bits)
  {
    case 16:
      return ibs->read_v16();
    case 8:
      return ibs->read_v8();
    default: // default is: 32bits length field
      return ibs->read_v();
  }
};
} // namespace lyasio

#if _HAS_CXX17_FULL_FEATURES

#  include "yasio/detail/sol.hpp"

extern "C" {

YASIO_LUA_API int luaopen_yasio(lua_State *L)
{
  using namespace yasio::inet;
  sol::state_view sol2(L);

  auto lyasio = sol2.create_named_table("yasio");

  lyasio.new_usertype<io_event>(
      "io_event", "kind", &io_event::kind, "status", &io_event::status, "packet",
      [](io_event *ev, sol::variadic_args args) {
        bool copy = false;
        if (args.size() >= 2)
          copy = args[1];
        return std::unique_ptr<yasio::ibstream>(!copy ? new yasio::ibstream(std::move(ev->packet()))
                                                      : new yasio::ibstream(ev->packet()));
      },
      "cindex", &io_event::cindex, "transport", &io_event::transport, "timestamp",
      &io_event::timestamp);

  lyasio.new_usertype<io_service>(
      "io_service", "start_service",
      [](io_service *service, sol::table channel_eps, io_event_cb_t cb) {
        std::vector<io_hostent> hosts;
        auto host = channel_eps["host"];
        if (host != sol::nil)
          hosts.push_back(io_hostent(host, channel_eps["port"]));
        else
        {
          for (auto item : channel_eps)
          {
            auto ep = item.second.as<sol::table>();
            hosts.push_back(io_hostent(ep["host"], ep["port"]));
          }
        }
        service->start_service(hosts, std::move(cb));
      },
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
              sol::function fn     = va[0];
              io_event_cb_t fnwrap = [=](event_ptr e) mutable -> void { fn(std::move(e)); };
              service->set_option(opt, std::addressof(fnwrap));
            }
            break;
          default:
            service->set_option(opt, static_cast<int>(va[0]));
        }
      },
      "dispatch_events", &io_service::dispatch_events, "open", &io_service::open, "is_open",
      sol::overload(static_cast<bool (io_service::*)(size_t) const>(&io_service::is_open),
                    static_cast<bool (io_service::*)(transport_ptr) const>(&io_service::is_open)),
      "close",
      sol::overload(static_cast<void (io_service::*)(transport_ptr)>(&io_service::close),
                    static_cast<void (io_service::*)(size_t)>(&io_service::close)),
      "write",
      sol::overload(
          [](io_service *service, transport_ptr transport, yasio::string_view s) {
            return service->write(transport, std::vector<char>(s.data(), s.data() + s.length()));
          },
          [](io_service *service, transport_ptr transport, yasio::obstream *obs) {
            return service->write(transport, obs->take_buffer());
          }));

  // ##-- obstream
  lyasio.new_usertype<yasio::obstream>(
      "obstream", "push32", &yasio::obstream::push32, "pop32",
      sol::overload(static_cast<void (yasio::obstream ::*)()>(&yasio::obstream::pop32),
                    static_cast<void (yasio::obstream ::*)(uint32_t)>(&yasio::obstream::pop32)),
      "push24", &yasio::obstream::push24, "pop24",
      sol::overload(static_cast<void (yasio::obstream ::*)()>(&yasio::obstream::pop24),
                    static_cast<void (yasio::obstream ::*)(uint32_t)>(&yasio::obstream::pop24)),
      "push16", &yasio::obstream::push16, "pop16",
      sol::overload(static_cast<void (yasio::obstream ::*)()>(&yasio::obstream::pop16),
                    static_cast<void (yasio::obstream ::*)(uint16_t)>(&yasio::obstream::pop16)),
      "push8", &yasio::obstream::push8, "pop8",
      sol::overload(static_cast<void (yasio::obstream ::*)()>(&yasio::obstream::pop8),
                    static_cast<void (yasio::obstream ::*)(uint8_t)>(&yasio::obstream::pop8)),
      "write_bool", &yasio::obstream::write_i<bool>, "write_i8", &yasio::obstream::write_i<int8_t>,
      "write_i16", &yasio::obstream::write_i<int16_t>, "write_i24", &yasio::obstream::write_i24,
      "write_i32", &yasio::obstream::write_i<int32_t>, "write_i64",
      &yasio::obstream::write_i<int64_t>, "write_u8", &yasio::obstream::write_i<uint8_t>,
      "write_u16", &yasio::obstream::write_i<uint16_t>, "write_u32",
      &yasio::obstream::write_i<uint32_t>, "write_u64", &yasio::obstream::write_i<uint64_t>,
      "write_f", &yasio::obstream::write_i<float>, "write_lf", &yasio::obstream::write_i<double>,
      "write_s", &yasio::obstream::write_s, "write_string",
      static_cast<void (yasio::obstream::*)(yasio::string_view)>(&yasio::obstream::write_v),
      "write_v",
      [](yasio::obstream *obs, yasio::string_view sv, sol::variadic_args args) {
        int lfl = 32;
        if (args.size() > 0)
          lfl = static_cast<int>(args[0]);
        return lyasio::obstream_write_v(obs, sv, lfl);
      },
      "write_bytes",
      static_cast<void (yasio::obstream::*)(yasio::string_view)>(&yasio::obstream::write_bytes),
      "length", &yasio::obstream::length, "to_string",
      [](yasio::obstream *obs) { return yasio::string_view(obs->data(), obs->length()); });

  // ##-- yasio::ibstream
  lyasio.new_usertype<yasio::ibstream>(
      "ibstream",
      sol::constructors<yasio::ibstream(std::vector<char>),
                        yasio::ibstream(const yasio::obstream *)>(),
      "read_bool", &yasio::ibstream::read_i<bool>, "read_i8", &yasio::ibstream::read_i<int8_t>,
      "read_i16", &yasio::ibstream::read_i<int16_t>, "read_i24", &yasio::ibstream::read_i24,
      "read_i32", &yasio::ibstream::read_i<int32_t>, "read_i64", &yasio::ibstream::read_i<int64_t>,
      "read_u8", &yasio::ibstream::read_i<uint8_t>, "read_u16", &yasio::ibstream::read_i<uint16_t>,
      "read_u24", &yasio::ibstream::read_u24, "read_u32", &yasio::ibstream::read_i<uint32_t>,
      "read_u64", &yasio::ibstream::read_i<uint64_t>, "read_f", &yasio::ibstream::read_i<float>,
      "read_lf", &yasio::ibstream::read_i<double>, "read_s", &yasio::ibstream::read_s,
      "read_string",
      static_cast<yasio::string_view (yasio::ibstream::*)()>(&yasio::ibstream::read_v), "read_v",
      [](yasio::ibstream *ibs, sol::variadic_args args) {
        int lfl = 32;
        if (args.size() > 0)
          lfl = static_cast<int>(args[0]);
        return lyasio::ibstream_read_v(ibs, lfl);
      },
      "read_bytes",
      static_cast<yasio::string_view (yasio::ibstream::*)(int)>(&yasio::ibstream::read_bytes),
      "seek", &yasio::ibstream_view::seek, "length", &yasio::ibstream_view::length, "to_string",
      [](yasio::ibstream *ibs) { return yasio::string_view(ibs->data(), ibs->length()); });

  lyasio["highp_clock"] = &highp_clock<highp_clock_t>;
  lyasio["highp_time"]  = &highp_clock<system_clock_t>;

  // ##-- yasio enums
#  define YASIO_EXPORT_ENUM(v) lyasio[#  v] = v
  YASIO_EXPORT_ENUM(YCM_TCP_CLIENT);
  YASIO_EXPORT_ENUM(YCM_TCP_SERVER);
  YASIO_EXPORT_ENUM(YCM_UDP_CLIENT);
  YASIO_EXPORT_ENUM(YCM_UDP_SERVER);
  YASIO_EXPORT_ENUM(YOPT_CONNECT_TIMEOUT);
  YASIO_EXPORT_ENUM(YOPT_CONNECT_TIMEOUT);
  YASIO_EXPORT_ENUM(YOPT_RECONNECT_TIMEOUT);
  YASIO_EXPORT_ENUM(YOPT_DNS_CACHE_TIMEOUT);
  YASIO_EXPORT_ENUM(YOPT_DEFER_EVENT);
  YASIO_EXPORT_ENUM(YOPT_TCP_KEEPALIVE);
  YASIO_EXPORT_ENUM(YOPT_RESOLV_FUNCTION);
  YASIO_EXPORT_ENUM(YOPT_LOG_FILE);
  YASIO_EXPORT_ENUM(YOPT_LFBFD_PARAMS);
  YASIO_EXPORT_ENUM(YOPT_IO_EVENT_CALLBACK);
  YASIO_EXPORT_ENUM(YOPT_CHANNEL_LOCAL_PORT);
  YASIO_EXPORT_ENUM(YOPT_CHANNEL_REMOTE_HOST);
  YASIO_EXPORT_ENUM(YOPT_CHANNEL_REMOTE_PORT);
  YASIO_EXPORT_ENUM(YOPT_CHANNEL_REMOTE_ENDPOINT);
  YASIO_EXPORT_ENUM(YEK_CONNECT_RESPONSE);
  YASIO_EXPORT_ENUM(YEK_CONNECTION_LOST);
  YASIO_EXPORT_ENUM(YEK_PACKET);
  YASIO_EXPORT_ENUM(SEEK_CUR);
  YASIO_EXPORT_ENUM(SEEK_SET);
  YASIO_EXPORT_ENUM(SEEK_END);

  return lyasio.push(); /* return 'yasio' table */
}

} /* extern "C" */

#else

#  include "yasio/detail/kaguya.hpp"
/// customize the type conversion from/to lua
namespace kaguya
{
// yasio::string_view
template <> struct lua_type_traits<yasio::string_view>
{
  typedef yasio::string_view get_type;
  typedef yasio::string_view push_type;

  static bool strictCheckType(lua_State *l, int index) { return lua_type(l, index) == LUA_TSTRING; }
  static bool checkType(lua_State *l, int index) { return lua_isstring(l, index) != 0; }
  static get_type get(lua_State *l, int index)
  {
    size_t size        = 0;
    const char *buffer = lua_tolstring(l, index, &size);
    return yasio::string_view(buffer, size);
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

YASIO_LUA_API int luaopen_yasio(lua_State *L)
{
  using namespace yasio::inet;
  kaguya::State state(L);

  auto lyasio    = state.newTable();
  state["yasio"] = lyasio;
  // No any interface need export, only for holder
  lyasio["io_transport"].setClass(kaguya::UserdataMetatable<io_transport>());

  lyasio["io_event"].setClass(
      kaguya::UserdataMetatable<io_event>()

          .addFunction("kind", &io_event::kind)
          .addFunction("status", &io_event::status)
          .addStaticFunction("packet",
                             [](io_event *ev, bool /*raw*/, bool copy) {
                               return std::unique_ptr<yasio::ibstream>(
                                   !copy ? new yasio::ibstream(std::move(ev->packet()))
                                         : new yasio::ibstream(ev->packet()));
                             })
          .addFunction("cindex", &io_event::cindex)
          .addFunction("transport", &io_event::transport)
          .addFunction("timestamp", &io_event::timestamp));

  lyasio["io_service"].setClass(
      kaguya::UserdataMetatable<io_service>()
          .setConstructors<io_service()>()
          .addOverloadedFunctions(
              "start_service",
              [](io_service *service, kaguya::LuaTable channel_eps, io_event_cb_t cb) {
                std::vector<io_hostent> hosts;
                auto host = channel_eps["host"];
                if (host)
                  hosts.push_back(io_hostent(host, channel_eps["port"]));
                else
                {
                  channel_eps.foreach_table<int, kaguya::LuaTable>([&](int, kaguya::LuaTable ep) {
                    hosts.push_back(io_hostent(ep["host"], ep["port"]));
                  });
                }
                service->start_service(hosts, std::move(cb));
              })
          .addFunction("stop_service", &io_service::stop_service)
          .addFunction("dispatch_events", &io_service::dispatch_events)
          .addFunction("open", &io_service::open)
          .addOverloadedFunctions(
              "is_open", static_cast<bool (io_service::*)(size_t) const>(&io_service::is_open),
              static_cast<bool (io_service::*)(transport_ptr) const>(&io_service::is_open))
          .addOverloadedFunctions(
              "close", static_cast<void (io_service::*)(transport_ptr)>(&io_service::close),
              static_cast<void (io_service::*)(size_t)>(&io_service::close))
          .addOverloadedFunctions(
              "write",
              static_cast<int (io_service::*)(transport_ptr transport, std::vector<char> data)>(
                  &io_service::write),
              [](io_service *service, transport_ptr transport, yasio::obstream *obs) {
                return service->write(transport, obs->take_buffer());
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
                  kaguya::LuaFunction fn = args[0];
                  io_event_cb_t fnwrap   = [=](event_ptr e) mutable -> void { fn(e.get()); };
                  service->set_option(opt, std::addressof(fnwrap));
                }
                break;
              default:
                service->set_option(opt, static_cast<int>(args[0]));
            }
          }));

  // ##-- yasio::obstream
  lyasio["obstream"].setClass(
      kaguya::UserdataMetatable<yasio::obstream>()
          .setConstructors<yasio::obstream(), yasio::obstream(int)>()
          .addFunction("push32", &yasio::obstream::push32)
          .addOverloadedFunctions(
              "pop32", static_cast<void (yasio::obstream ::*)()>(&yasio::obstream::pop32),
              static_cast<void (yasio::obstream ::*)(uint32_t)>(&yasio::obstream::pop32))
          .addFunction("push24", &yasio::obstream::push24)
          .addOverloadedFunctions(
              "pop24", static_cast<void (yasio::obstream ::*)()>(&yasio::obstream::pop24),
              static_cast<void (yasio::obstream ::*)(uint32_t)>(&yasio::obstream::pop24))
          .addFunction("push16", &yasio::obstream::push16)
          .addOverloadedFunctions(
              "pop16", static_cast<void (yasio::obstream ::*)()>(&yasio::obstream::pop16),
              static_cast<void (yasio::obstream ::*)(uint16_t)>(&yasio::obstream::pop16))
          .addFunction("push8", &yasio::obstream::push8)
          .addOverloadedFunctions(
              "pop8", static_cast<void (yasio::obstream ::*)()>(&yasio::obstream::pop8),
              static_cast<void (yasio::obstream ::*)(uint8_t)>(&yasio::obstream::pop8))
          .addFunction("write_bool", &yasio::obstream::write_i<bool>)
          .addFunction("write_i8", &yasio::obstream::write_i<int8_t>)
          .addFunction("write_i16", &yasio::obstream::write_i<int16_t>)
          .addFunction("write_i24", &yasio::obstream::write_i24)
          .addFunction("write_i32", &yasio::obstream::write_i<int32_t>)
          .addFunction("write_i64", &yasio::obstream::write_i<int64_t>)
          .addFunction("write_u8", &yasio::obstream::write_i<uint8_t>)
          .addFunction("write_u16", &yasio::obstream::write_i<uint16_t>)
          .addFunction("write_u32", &yasio::obstream::write_i<uint32_t>)
          .addFunction("write_u64", &yasio::obstream::write_i<uint64_t>)
          .addFunction("write_f", &yasio::obstream::write_i<float>)
          .addFunction("write_lf", &yasio::obstream::write_i<double>)
          .addFunction("write_s", &yasio::obstream::write_s)
          .addFunction("write_string", static_cast<void (yasio::obstream::*)(yasio::string_view)>(
                                           &yasio::obstream::write_v))
          .addStaticFunction(
              "write_v",
              [](yasio::obstream *obs, yasio::string_view sv, kaguya::VariadicArgType args) {
                int lfl = 32;
                if (args.size() > 0)
                  lfl = static_cast<int>(args[0]);
                return lyasio::obstream_write_v(obs, sv, lfl);
              })
          .addFunction("write_bytes", static_cast<void (yasio::obstream::*)(yasio::string_view)>(
                                          &yasio::obstream::write_bytes))
          .addFunction("length", &yasio::obstream::length)
          .addStaticFunction("to_string", [](yasio::obstream *obs) {
            return yasio::string_view(obs->data(), obs->length());
          }));

  // ##-- yasio::ibstream_view
  lyasio["ibstream_view"].setClass(
      kaguya::UserdataMetatable<yasio::ibstream_view>()
          .setConstructors<yasio::ibstream_view(), yasio::ibstream_view(const void *, int),
                           yasio::ibstream_view(const yasio::obstream *)>()
          .addFunction("read_bool", &yasio::ibstream_view::read_i<bool>)
          .addFunction("read_i8", &yasio::ibstream_view::read_i<int8_t>)
          .addFunction("read_i16", &yasio::ibstream_view::read_i<int16_t>)
          .addFunction("read_i24", &yasio::ibstream_view::read_i24)
          .addFunction("read_i32", &yasio::ibstream_view::read_i<int32_t>)
          .addFunction("read_i64", &yasio::ibstream_view::read_i<int64_t>)
          .addFunction("read_u8", &yasio::ibstream_view::read_i<uint8_t>)
          .addFunction("read_u16", &yasio::ibstream_view::read_i<uint16_t>)
          .addFunction("read_u24", &yasio::ibstream_view::read_u24)
          .addFunction("read_u32", &yasio::ibstream_view::read_i<uint32_t>)
          .addFunction("read_u64", &yasio::ibstream_view::read_i<uint64_t>)
          .addFunction("read_f", &yasio::ibstream_view::read_i<float>)
          .addFunction("read_lf", &yasio::ibstream_view::read_i<double>)
          .addFunction("read_s", &yasio::ibstream_view::read_s)
          .addFunction("read_string", static_cast<yasio::string_view (yasio::ibstream_view::*)()>(
                                          &yasio::ibstream_view::read_v))
          .addStaticFunction("read_v",
                             [](yasio::ibstream *ibs, kaguya::VariadicArgType args) {
                               int length_field_bits = 32;
                               if (args.size() > 0)
                                 length_field_bits = static_cast<int>(args[0]);
                               return lyasio::ibstream_read_v(ibs, length_field_bits);
                             })
          .addFunction("read_bytes", static_cast<yasio::string_view (yasio::ibstream_view::*)(int)>(
                                         &yasio::ibstream_view::read_bytes))
          .addFunction("seek", &yasio::ibstream_view::seek)
          .addFunction("length", &yasio::ibstream_view::length)
          .addStaticFunction("to_string", [](yasio::ibstream_view *ibs) {
            return yasio::string_view(ibs->data(), ibs->length());
          }));

  // ##-- ibstream
  lyasio["ibstream"].setClass(kaguya::UserdataMetatable<yasio::ibstream, yasio::ibstream_view>()
                                  .setConstructors<yasio::ibstream(std::vector<char>),
                                                   yasio::ibstream(const yasio::obstream *)>());

  lyasio["highp_clock"] = &highp_clock<highp_clock_t>;
  lyasio["highp_time"]  = &highp_clock<system_clock_t>;

  // ##-- yasio enums
#  define YASIO_EXPORT_ENUM(v) lyasio[#  v] = v
  YASIO_EXPORT_ENUM(YCM_TCP_CLIENT);
  YASIO_EXPORT_ENUM(YCM_TCP_SERVER);
  YASIO_EXPORT_ENUM(YCM_UDP_CLIENT);
  YASIO_EXPORT_ENUM(YCM_UDP_SERVER);
  YASIO_EXPORT_ENUM(YOPT_CONNECT_TIMEOUT);
  YASIO_EXPORT_ENUM(YOPT_CONNECT_TIMEOUT);
  YASIO_EXPORT_ENUM(YOPT_RECONNECT_TIMEOUT);
  YASIO_EXPORT_ENUM(YOPT_DNS_CACHE_TIMEOUT);
  YASIO_EXPORT_ENUM(YOPT_DEFER_EVENT);
  YASIO_EXPORT_ENUM(YOPT_TCP_KEEPALIVE);
  YASIO_EXPORT_ENUM(YOPT_RESOLV_FUNCTION);
  YASIO_EXPORT_ENUM(YOPT_LOG_FILE);
  YASIO_EXPORT_ENUM(YOPT_LFBFD_PARAMS);
  YASIO_EXPORT_ENUM(YOPT_IO_EVENT_CALLBACK);
  YASIO_EXPORT_ENUM(YOPT_CHANNEL_LOCAL_PORT);
  YASIO_EXPORT_ENUM(YOPT_CHANNEL_REMOTE_HOST);
  YASIO_EXPORT_ENUM(YOPT_CHANNEL_REMOTE_PORT);
  YASIO_EXPORT_ENUM(YOPT_CHANNEL_REMOTE_ENDPOINT);
  YASIO_EXPORT_ENUM(YEK_CONNECT_RESPONSE);
  YASIO_EXPORT_ENUM(YEK_CONNECTION_LOST);
  YASIO_EXPORT_ENUM(YEK_PACKET);
  YASIO_EXPORT_ENUM(SEEK_CUR);
  YASIO_EXPORT_ENUM(SEEK_SET);
  YASIO_EXPORT_ENUM(SEEK_END);

  return lyasio.push(); /* return 'yasio' table */
}

} /* extern "C" */

#endif /* _HAS_CXX17_FULL_FEATURES */
