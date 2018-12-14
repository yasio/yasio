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
#include "obinarystream.h"
#include "masio.h"
#include "lmasio.h"
#include "kaguya.hpp"

void lua_open_masio(lua_State *L) {
  using namespace purelib::inet;
  kaguya::State state(L);

  state["io_hostent"].setClass(
      kaguya::UserdataMetatable<io_hostent>()
          .setConstructors<io_hostent(),
                           io_hostent(const std::string &, u_short)>()
          /*.addPropertyAny("address",
                          [](io_hostent *hst, const std::string &value) {})*/
  );
      /*sol::constructors<io_hostent(),
                        io_hostent(const std::string &, u_short)>(),
      "address", &io_hostent::address_, "port", &io_hostent::port_);*/

#if 0
  sol2.new_usertype<io_event>(
      "io_event", "channel_index", &io_event::channel_index, "kind",
      &io_event::type, "error_code", &io_event::error_code, "transport",
      &io_event::transport, "packet", [](io_event *event) {
        return std::unique_ptr<ibinarystream>(
            new ibinarystream(event->packet().data(), event->packet().size()));
      });

  sol2.new_usertype<io_service>(
      "io_service", "start_service",
#if _HAS_CXX17
      sol::overload(static_cast<void (io_service::*)(std::vector<io_hostent>,
                                                     io_event_callback_t)>(
                        &io_service::start_service),
                    static_cast<void (io_service::*)(
                        const io_hostent *channel_eps, io_event_callback_t cb)>(
                        &io_service::start_service)),
#else
      [](io_service *service, sol::object obj, io_event_callback_t cb) {
        if (obj.get_type() == sol::type::table) {
          std::vector<io_hostent> hostents;
          for (auto &v : static_cast<sol::table>(obj)) {
            auto ioh = v.second.as<io_hostent *>();
            hostents.push_back(*ioh);
          }
          service->start_service(std::move(hostents), cb);
        } else {
          service->start_service(obj.as<io_hostent *>(), 1, cb);
        }
      },
#endif
      "stop_service", &io_service::stop_service, "set_option",
      [](io_service *service, int opt, sol::variadic_args va) {
        switch (opt) {
        case MASIO_OPT_TCP_KEEPALIVE:
          service->set_option(opt, static_cast<int>(va[0]),
                              static_cast<int>(va[1]), static_cast<int>(va[2]));
          break;
        case MASIO_OPT_LFIB_PARAMS:
          service->set_option(opt, static_cast<int>(va[0]),
                              static_cast<int>(va[1]), static_cast<int>(va[2]),
                              static_cast<int>(va[3]));
          break;
        case MASIO_OPT_RESOLV_FUNCTION: // lua does not support set custom
                                        // resolv function
          break;
        case MASIO_OPT_IO_EVENT_CALLBACK:
          (void)0;
          {
            sol::function fn = va[0];
            io_event_callback_t fnwrap = [=](event_ptr e) mutable -> void {
              fn(e);
            };
            service->set_option(opt, std::addressof(fnwrap));
          }
          break;
        default:
          service->set_option(opt, static_cast<int>(va[0]));
        }
      },
      "dispatch_events", &io_service::dispatch_events, "open",
      &io_service::open, "write",
      sol::overload(
          [](io_service *aio, transport_ptr transport,
#if _HAS_CXX17
             std::string_view s
#else
             const std::string &s
#endif
          ) {
            aio->write(transport,
                       std::vector<char>(s.data(), s.data() + s.length()));
          },
          [](io_service *aio, transport_ptr transport, obinarystream *obs) {
            aio->write(transport, obs->take_buffer());
          }));

  // ##-- obinarystream
  sol2.new_usertype<obinarystream>(
      "obstream", "push32", &obinarystream::push32, "pop32",
      sol::overload(
          static_cast<void (obinarystream ::*)()>(&obinarystream::pop32),
          static_cast<void (obinarystream ::*)(uint32_t)>(
              &obinarystream::pop32)),
      "push24", &obinarystream::push24, "pop24",
      sol::overload(
          static_cast<void (obinarystream ::*)()>(&obinarystream::pop24),
          static_cast<void (obinarystream ::*)(uint32_t)>(
              &obinarystream::pop24)),
      "push16", &obinarystream::push16, "pop16",
      sol::overload(
          static_cast<void (obinarystream ::*)()>(&obinarystream::pop16),
          static_cast<void (obinarystream ::*)(uint16_t)>(
              &obinarystream::pop16)),
      "push8", &obinarystream::push8, "pop8",
      sol::overload(
          static_cast<void (obinarystream ::*)()>(&obinarystream::pop8),
          static_cast<void (obinarystream ::*)(uint8_t)>(&obinarystream::pop8)),
      "write_bool", &obinarystream::write_i<bool>, "write_i8",
      &obinarystream::write_i<int8_t>, "write_i16",
      &obinarystream::write_i<int16_t>, "write_i24", &obinarystream::write_i24,
      "write_i32", &obinarystream::write_i<int32_t>, "write_i64",
      &obinarystream::write_i<int64_t>, "write_u8",
      &obinarystream::write_i<uint8_t>, "write_u16",
      &obinarystream::write_i<uint16_t>, "write_u32",
      &obinarystream::write_i<uint32_t>, "write_u64",
      &obinarystream::write_i<uint64_t>, "write_f",
      static_cast<size_t (obinarystream::*)(float)>(&obinarystream::write_i),
      "write_lf",
      static_cast<size_t (obinarystream::*)(double)>(&obinarystream::write_i),

      "write_string",
#if _HAS_CXX17
      static_cast<size_t (obinarystream::*)(std::string_view)>(
          &obinarystream::write_v),
#else
        [](obinarystream *obs, const std::string &value) { obs->write_v(value); },
#endif
      "length", &obinarystream::length, "to_string", [](obinarystream *obs) {
#if _HAS_CXX17
        return std::string_view(obs->data(), obs->length());
#else
          return std::string(obs->data(), obs->length());
#endif
      });

  // ##-- ibinarystream
  sol2.new_usertype<ibinarystream>(
      "ibstream",
      sol::constructors<ibinarystream(), ibinarystream(const void *, int),
                        ibinarystream(const obinarystream *)>(),
      "assign", &ibinarystream::assign, "read_bool",
      &ibinarystream::read_ix<bool>, "read_i8", &ibinarystream::read_ix<int8_t>,
      "read_i16", &ibinarystream::read_ix<int16_t>, "read_i16",
      &ibinarystream::read_i24, "read_i32", &ibinarystream::read_ix<int32_t>,
      "read_i64", &ibinarystream::read_ix<int64_t>, "read_u8",
      &ibinarystream::read_ix<uint8_t>, "read_u16",
      &ibinarystream::read_ix<uint16_t>, "read_u32",
      &ibinarystream::read_ix<uint32_t>, "read_u64",
      &ibinarystream::read_ix<uint64_t>, "read_f",
      &ibinarystream::read_ix<float>, "read_lf",
      &ibinarystream::read_ix<double>, "read_string",
#if _HAS_CXX17
      static_cast<std::string_view (ibinarystream::*)()>(
          &ibinarystream::read_v),
#else
        [](ibinarystream* ibs) {
          auto sv = ibs->read_v();
          return std::string(sv.data(), sv.length());
        },
#endif
      "to_string", [](ibinarystream *ibs) {
#if _HAS_CXX17
        return std::string_view(ibs->data(), ibs->size());
#else
          return std::string(ibs->data(), ibs->size());
#endif
      });
#endif
}
