//////////////////////////////////////////////////////////////////////////////////////////
// A cross platform socket APIs, support ios & android & wp8 & window store
// universal app version: 3.9.2
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
#include "lmasio.h"
#include "masio.h"
#include "obinarystream.h"
#include "sol.hpp"

void lua_open_masio(lua_State *L)
{
  using namespace purelib::inet;
  sol::state_view sol2(L);

#if 0
    auto t = sol2.create_named_table("simple_timer");
    // the simple timer implementation is here: https://github.com/halx99/x-studio365/blob/master/cocos2d-x-patch/cocos/editor-support/cocostudio/ext/SimpleTimer.h
    t.set_function("delay", simple_timer::delay);
    t.set_function("loop", simple_timer::loop);
    t.set_function("kill", simple_timer::kill);
#endif
  auto masio = sol2.create_named_table("masio");

  masio.new_usertype<io_hostent>(
      "io_hostent", sol::constructors<io_hostent(), io_hostent(const std::string &, u_short)>(),
      "address", &io_hostent::address_, "port", &io_hostent::port_);

  masio.new_usertype<io_event>("io_event", "channel_index", &io_event::channel_index, "kind",
                               &io_event::type, "error_code", &io_event::error_code, "transport",
                               &io_event::transport, "packet", [](io_event *event) {
                                 return std::unique_ptr<ibinarystream>(new ibinarystream(
                                     event->packet().data(), event->packet().size()));
                               });

  masio.new_usertype<io_service>(
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
          case MASIO_OPT_TCP_KEEPALIVE:
            service->set_option(opt, static_cast<int>(va[0]), static_cast<int>(va[1]),
                                static_cast<int>(va[2]));
            break;
          case MASIO_OPT_LFIB_PARAMS:
            service->set_option(opt, static_cast<int>(va[0]), static_cast<int>(va[1]),
                                static_cast<int>(va[2]), static_cast<int>(va[3]));
            break;
          case MASIO_OPT_RESOLV_FUNCTION: // lua does not support set custom
                                          // resolv function
            break;
          case MASIO_OPT_IO_EVENT_CALLBACK:
            (void)0;
            {
              sol::function fn           = va[0];
              io_event_callback_t fnwrap = [=](event_ptr e) mutable -> void { fn(e); };
              service->set_option(opt, std::addressof(fnwrap));
            }
            break;
          default:
            service->set_option(opt, static_cast<int>(va[0]));
        }
      },
      "dispatch_events", &io_service::dispatch_events, "open", &io_service::open, "write",
      sol::overload(
          [](io_service *service, transport_ptr transport, std::string_view s) {
            service->write(transport, std::vector<char>(s.data(), s.data() + s.length()));
          },
          [](io_service *service, transport_ptr transport, obinarystream *obs) {
            service->write(transport, obs->take_buffer());
          }));

  // ##-- obinarystream
  masio.new_usertype<obinarystream>(
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
      &obinarystream::write_i<uint64_t>, "write_f",
      static_cast<size_t (obinarystream::*)(float)>(&obinarystream::write_i), "write_lf",
      static_cast<size_t (obinarystream::*)(double)>(&obinarystream::write_i),

      "write_string",
      static_cast<size_t (obinarystream::*)(std::string_view)>(&obinarystream::write_v), "length",
      &obinarystream::length, "to_string",
      [](obinarystream *obs) { return std::string_view(obs->data(), obs->length()); });

  // ##-- ibinarystream
  masio.new_usertype<ibinarystream>(
      "ibstream",
      sol::constructors<ibinarystream(), ibinarystream(const void *, int),
                        ibinarystream(const obinarystream *)>(),
      "read_bool", &ibinarystream::read_ix<bool>, "read_i8", &ibinarystream::read_ix<int8_t>,
      "read_i16", &ibinarystream::read_ix<int16_t>, "read_i24", &ibinarystream::read_i24,
      "read_i32", &ibinarystream::read_ix<int32_t>, "read_i64", &ibinarystream::read_ix<int64_t>,
      "read_u8", &ibinarystream::read_ix<uint8_t>, "read_u16", &ibinarystream::read_ix<uint16_t>,
      "read_u32", &ibinarystream::read_ix<uint32_t>, "read_u64", &ibinarystream::read_ix<uint64_t>,
      "read_f", &ibinarystream::read_ix<float>, "read_lf", &ibinarystream::read_ix<double>,
      "read_string", static_cast<std::string_view (ibinarystream::*)()>(&ibinarystream::read_v),
      "to_string", [](ibinarystream *ibs) { return std::string_view(ibs->data(), ibs->size()); });

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
