yasio-3.30
1. Tidy option macros.
2. Add YCF_ enums to control channel to support more features.
3. Add multicast support.
4. Add a workaround implementation to support win32 udp-server.
5. Add io_service::cindex_to_handle.
6. Add ftp sever example.
7. Remove loop behavior of deadline_timer, user can schedule again when it's expired.
8. Add defer invoke timer callback support.
9. Add obstream::write_byte.
10. Add to_strf_v4 for ip::endpoint.
11. Optimizing for file transfer, avoid high cpu occupation when system kernel send buffer is full.
12. More safe to check object valid which allocated from pool.
13. Add send complete callback.
14. Mark io_service::dispatch_events deprecated, use dispatch to instead.
15. Add YCF_REUSEPORT to control whether to enable socket can bind same port, default and previous vesion is enabled.
16. Implement case insensitive starts_with, ends_with at string_view.hpp.
17. Ignore SIGPIPE for tcp at non-win32 system.
18. Remove reconnect timeout.