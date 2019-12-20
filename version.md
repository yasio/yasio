yasio-3.30.1
  
1. Add construct channel at io_service constructor, now all options could be set before start_service if you doesn't use the start_service with channel hostents, and mark them deprecated
2. Use cmake for trvis ci cross-platform build at win32, linux, osx, ios
3. Sync script bindings
  
  
yasio-3.30.0
  
1. Tidy option macros.
2. Add YCF_ enums to control channel to support more features.
3. Add multicast support.
4. Add a workaround implementation to support win32 udp-server.
5. Add io_service::cindex_to_handle.
6. Add ftp sever example.
7. Remove loop behavior of deadline_timer, user can schedule again when it's expired.
8. Add obstream::write_byte.
9. Add to_strf_v4 for ip::endpoint.
10. Optimizing for file transfer, avoid high cpu occupation when system kernel send buffer is full.
11. More safe to check object valid which allocated from pool.
12. Add send complete callback.
13. Mark io_service::dispatch_events deprecated, use dispatch to instead.
14. Add YCF_REUSEPORT to control whether to enable socket can bind same port, default and previous vesion is enabled.
15. Implement case insensitive starts_with, ends_with at string_view.hpp.
16. **Ignore SIGPIPE for tcp at non-win32 system.**
17. **Remove reconnect timeout.**
