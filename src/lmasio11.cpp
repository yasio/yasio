//////////////////////////////////////////////////////////////////////////////////////////
// A cross platform socket APIs, support ios & android & wp8 & window store
// universal app version: 3.9.3
//////////////////////////////////////////////////////////////////////////////////////////
/*
The MIT License (MIT)

Copyright (c) 2012-2018 halx99

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
#include "ibinarystream.h"
#include "kaguya.hpp"
#include "lmasio.h"
#include "masio.h"
#include "obinarystream.h"

/// customize the type conversion from/to lua
namespace kaguya
{
// std::string_view
template <> struct lua_type_traits<std::string_view>
{
  typedef std::string_view get_type;
  typedef std::string_view push_type;

  static bool strictCheckType(lua_State *l, int index) { return lua_type(l, index) == LUA_TSTRING; }
  static bool checkType(lua_State *l, int index) { return lua_isstring(l, index) != 0; }
  static get_type get(lua_State *l, int index)
  {
    size_t size        = 0;
    const char *buffer = lua_tolstring(l, index, &size);
    return std::string_view(buffer, size);
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

void lua_open_masio(lua_State *L)
{
  using namespace purelib::inet;
  kaguya::State state(L);

  auto masio     = state.newTable();
  state["masio"] = masio;
  // No any interface need export, only for holder
  masio["io_transport"].setClass(kaguya::UserdataMetatable<io_transport>());

  masio["io_hostent"].setClass(
      kaguya::UserdataMetatable<io_hostent>()
          .setConstructors<io_hostent(), io_hostent(const std::string &, u_short)>()
          .addProperty("address", &io_hostent::get_address, &io_hostent::set_address)
          .addProperty("port", &io_hostent::get_port, &io_hostent::set_port));

  masio["io_event"].setClass(kaguya::UserdataMetatable<io_event>()
                                 .addFunction("channel_index", &io_event::channel_index)
                                 .addFunction("kind", &io_event::type)
                                 .addFunction("error_code", &io_event::error_code)
                                 .addFunction("transport", &io_event::transport)
                                 .addStaticFunction("packet", [](io_event *ev) {
                                   return std::shared_ptr<ibinarystream>(
                                       new ibinarystream(ev->packet().data(), ev->packet().size()));
                                 }));

  masio["io_service"].setClass(
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
              "write", &io_service::write,
              [](io_service *service, transport_ptr transport, obinarystream *obs) {
                service->write(transport, obs->take_buffer());
              })
          .addStaticFunction("set_option", [](io_service *service, int opt,
                                              kaguya::VariadicArgType args) {
            switch (opt)
            {
              case MASIO_OPT_TCP_KEEPALIVE:
                service->set_option(opt, static_cast<int>(args[0]), static_cast<int>(args[1]),
                                    static_cast<int>(args[2]));
                break;
              case MASIO_OPT_LFIB_PARAMS:
                service->set_option(opt, static_cast<int>(args[0]), static_cast<int>(args[1]),
                                    static_cast<int>(args[2]), static_cast<int>(args[3]));
                break;
              case MASIO_OPT_RESOLV_FUNCTION: // lua does not support set custom
                                              // resolv function
                break;
              case MASIO_OPT_IO_EVENT_CALLBACK:
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
  masio["obstream"].setClass(
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
          .addFunction("write_f",
                       static_cast<size_t (obinarystream::*)(float)>(&obinarystream::write_i))
          .addFunction("write_lf",
                       static_cast<size_t (obinarystream::*)(double)>(&obinarystream::write_i))
          .addFunction("write_string", static_cast<size_t (obinarystream::*)(std::string_view)>(
                                           &obinarystream::write_v))
          .addFunction("length", &obinarystream::length)
          .addStaticFunction("to_string", [](obinarystream *obs) {
            return std::string_view(obs->data(), obs->length());
          }));

  // ##-- ibinarystream
  masio["ibstream"].setClass(
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
          .addFunction("read_u32", &ibinarystream::read_ix<uint32_t>)
          .addFunction("read_u64", &ibinarystream::read_ix<uint64_t>)
          .addFunction("read_f", &ibinarystream::read_ix<float>)
          .addFunction("read_lf", &ibinarystream::read_ix<double>)
          .addFunction("read_string",
                       static_cast<std::string_view (ibinarystream::*)()>(&ibinarystream::read_v))
          .addStaticFunction("to_string", [](ibinarystream *ibs) {
            return std::string_view(ibs->data(), ibs->size());
          }));

  // ##-- masio enums
  masio["CHANNEL_TCP_CLIENT"]           = channel_type::CHANNEL_TCP_CLIENT;
  masio["CHANNEL_TCP_SERVER"]           = channel_type::CHANNEL_TCP_SERVER;
  masio["MASIO_OPT_CONNECT_TIMEOUT"]    = MASIO_OPT_CONNECT_TIMEOUT;
  masio["MASIO_OPT_SEND_TIMEOUT"]       = MASIO_OPT_CONNECT_TIMEOUT;
  masio["MASIO_OPT_RECONNECT_TIMEOUT"]  = MASIO_OPT_RECONNECT_TIMEOUT;
  masio["MASIO_OPT_DNS_CACHE_TIMEOUT"]  = MASIO_OPT_DNS_CACHE_TIMEOUT;
  masio["MASIO_OPT_DEFER_EVENT"]        = MASIO_OPT_DEFER_EVENT;
  masio["MASIO_OPT_TCP_KEEPALIVE"]      = MASIO_OPT_TCP_KEEPALIVE;
  masio["MASIO_OPT_RESOLV_FUNCTION"]    = MASIO_OPT_RESOLV_FUNCTION;
  masio["MASIO_OPT_LOG_FILE"]           = MASIO_OPT_LOG_FILE;
  masio["MASIO_OPT_LFIB_PARAMS"]        = MASIO_OPT_LFIB_PARAMS;
  masio["MASIO_OPT_IO_EVENT_CALLBACK"]  = MASIO_OPT_IO_EVENT_CALLBACK;
  masio["MASIO_EVENT_CONNECT_RESPONSE"] = MASIO_EVENT_CONNECT_RESPONSE;
  masio["MASIO_EVENT_CONNECTION_LOST"]  = MASIO_EVENT_CONNECTION_LOST;
  masio["MASIO_EVENT_RECV_PACKET"]      = MASIO_EVENT_RECV_PACKET;
}
