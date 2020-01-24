yasio-3.31.3
  
1. Optimize API io_service::write, add write raw buf support.
2. Fix issue: https://github.com/simdsoft/yasio/issues/208
3. Fix issue: https://github.com/simdsoft/yasio/issues/209
  
  
yasio-3.31.2
  
1. Optimize singleton implementation, see: https://github.com/simdsoft/yasio/issues/200
2. Fix typo, YASIO_VERBOS_LOG to YASIO_VERBOSE_LOG.
3. Explicit set socktype for ```getaddrinfo```, see: https://github.com/simdsoft/yasio/issues/201
4. Improve xxsocket send_n/recv_n implementation and behavior, see: https://github.com/simdsoft/yasio/issues/202
  
  
yasio-3.31.1
  
1. Make c-ares works well on android 8 or later.
  
  
yasio-3.31.0
  
1. Add Initial Bytes To Strip support for length field based frame decode, use YOPT_C_LFBFD_IBTS.
2. Add SSL client support, use YASIO_HAVE_SSL to enable, YCM_SSL_CLIENT to open a channel with ssl client support.
3. Integrate c-ares support, YASIO_HAVE_CARES to enable, make sure your build system already have c-ares.
4. Refactor timeout options, use YOPT_S_CONNECT_TIMEOUT, YOPT_S_DNS_CACHE_TIMEOUT, YOPT_S_DNS_QUERIES_TIMEOUT to instead YOPT_S_TIMEOUTS.
5. Optimize schedule_timer behavior, always replace timer_cb when timer exist.
6. Remove deprecated functions.
7. Remove tolua support.
  
  
yasio-3.30.8
  
1. Sync optimizations from v3.31.2.
  
  
yasio-3.30.7
  
1. Sync optimizations from v3.31.
  
  
yasio-3.30.6
  
1. Make option enums compatible with v3.31.
2. Simplify io_service state.
  
  
yasio-3.30.5
  
1. Fix missing break at set_option for YOPT_C_REMOTE_ENDPOINT.
  
  
yasio-3.30.4
  
1. Make normal concurrent queue more safe if SPSC queue is disabled.
2. Make sure udp recv buf enough at kcp transport do_read.
3. Fix io_service::write always retur 0.
  
  
yasio-3.30.3
  
1. Optimize jsb, jsb2.0 and lua binding.
  
  
yasio-3.30.2
  
1. Fix jsb2.0 bindings issue, see: [#192](https://github.com/simdsoft/yasio/issues/192)
  
  
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
