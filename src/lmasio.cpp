#include "ibinarystream.h"
#include "obinarystream.h"
#include "masio.h"
#include "lmasio.h"

void lua_open_masio(lua_State *L) {
    sol::state_view sol2(L);
#if 0
    auto t = sol2.create_named_table("simple_timer");
    // the simple timer implementation is here: https://github.com/halx99/x-studio365/blob/master/cocos2d-x-patch/cocos/editor-support/cocostudio/ext/SimpleTimer.h
    t.set_function("delay", simple_timer::delay);
    t.set_function("loop", simple_timer::loop);
    t.set_function("kill", simple_timer::kill);
#endif

    sol2.new_usertype<purelib::inet::io_hostent>(
        "io_hostent",
        "address_", &purelib::inet::io_hostent::address_,
        "port_", &purelib::inet::io_hostent::port_);

    sol2.new_usertype<purelib::inet::io_event>(
        "io_event", "channel_index", &purelib::inet::io_event::channel_index,
        "type", &purelib::inet::io_event::type,
        "error_code", &purelib::inet::io_event::error_code,
        "transport", &purelib::inet::io_event::transport,
        "packet", [](purelib::inet::io_event *event) {
        return std::unique_ptr<ibinarystream>(new ibinarystream(event->packet().data(), event->packet().size()));
    });

    sol2.new_usertype<purelib::inet::io_service>(
        "io_service", 
        "start_service", &purelib::inet::io_service::start_service,
        "stop_service", &purelib::inet::io_service::stop_service, 
        "set_option", [](purelib::inet::io_service* service, int opt, sol::variadic_args va) {
            switch(opt) { 
            case purelib::inet::MASIO_OPT_TCP_KEEPALIVE:
            case purelib::inet::MASIO_OPT_LFIB_PARAMS:
                service->set_option(opt, static_cast<int>(va[0]), static_cast<int>(va[1]), static_cast<int>(va[2]));
                break;
            case purelib::inet::MASIO_OPT_RESOLV_FUNCTION: // lua does not support set custom resolv function
                break;
            default:
                service->set_option(opt, static_cast<int>(va[0]));
            }
        },
        "set_event_callback", &purelib::inet::io_service::set_event_callback,
        "dispatch_events", &purelib::inet::io_service::dispatch_events,
        "open", &purelib::inet::io_service::open,
        "write", sol::overload([](purelib::inet::io_service *aio, purelib::inet::transport_ptr transport, const char *s, size_t n) {
            aio->write(transport, std::vector<char>(s, s + n));
        }, [](purelib::inet::io_service *aio, purelib::inet::transport_ptr transport, obinarystream* obs) {
            aio->write(transport, obs->take_buffer());
        })
    );

    // ##-- obinarystream
    sol2.new_usertype<obinarystream>(
        "obstream",
        "push32", &obinarystream::push32,
        "pop32", sol::overload(static_cast<void (obinarystream ::*)()>(&obinarystream::pop32),
            static_cast<void (obinarystream ::*)(uint32_t)>(&obinarystream::pop32)),
        "write_b", &obinarystream::write_i<bool>,
        "write_i8", &obinarystream::write_i<int8_t>,
        "write_i16", &obinarystream::write_i<int16_t>,
        "write_i32", &obinarystream::write_i<int32_t>,
        "write_i64", &obinarystream::write_i<int64_t>,
        "write_u8", &obinarystream::write_i<uint8_t>,
        "write_u16", &obinarystream::write_i<uint16_t>,
        "write_u32", &obinarystream::write_i<uint32_t>,
        "write_u64", &obinarystream::write_i<uint64_t>,
        "write_f", static_cast<size_t(obinarystream::*)(float)>(&obinarystream::write_i),
        "write_lf", static_cast<size_t(obinarystream::*)(double)>(&obinarystream::write_i),
        "write_string", static_cast<size_t(obinarystream::*)(std::string_view)>(&obinarystream::write_v),
        "length", &obinarystream::length
        );

    // ##-- ibinarystream
    sol2.new_usertype<ibinarystream>(
        "ibstream",
        sol::constructors<ibinarystream(), ibinarystream(const void*, int), ibinarystream(const obinarystream*)>(),
        "assign", &ibinarystream::assign,
        "read_b", &ibinarystream::read_i0<bool>,
        "read_i8", &ibinarystream::read_i0<int8_t>,
        "read_i16", &ibinarystream::read_i0<int16_t>,
        "read_i32", &ibinarystream::read_i0<int32_t>,
        "read_i64", &ibinarystream::read_i0<int64_t>,
        "read_u8", &ibinarystream::read_i0<uint8_t>,
        "read_u16", &ibinarystream::read_i0<uint16_t>,
        "read_u32", &ibinarystream::read_i0<uint32_t>,
        "read_u64", &ibinarystream::read_i0<uint64_t>,
        "read_f", &ibinarystream::read_i0<float>,
        "read_lf", &ibinarystream::read_i0<double>,
        "read_string", static_cast<std::string_view(ibinarystream::*)()>(&ibinarystream::read_v),
        "to_string", [](ibinarystream* ibs) {
              return std::string_view(ibs->data(), ibs->size());
        });
}
