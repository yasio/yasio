yasio-3.33.7
  
* Unix domain socket via ```SOCK_STREAM``` support, define ```YASIO_ENABLE_UDS``` at ```config.hpp``` and use ```YCM_UDS``` combine with ```YCK_TCP_CLIENT```, ```YCK_TCP_SERVER```
  

yaiso-3.33.6
  
* Reduce kcp cpu cost.
  
  
yaiso-3.33.5
  
* Add support to set kcp conv id, use option ```YOPT_C_KCP_CONV```.
  
  
yaiso-3.33.4
  
* Add log level support, use option ```YOPT_S_PRINT_FN2```.
  
  
yaiso-3.33.3
  
* Add API ```xxsocket::tcp_rtt```.
  
  
yaiso-3.33.2
  
* Fix c-ares doesn't get system dns for ios.
* Add option ```YOPT_S_DNS_DIRTY``` for user to change system name servers after mobile network changed when c-ares enabled.
* Refine write event register when system kernel write buffer is full.
  
  
yaiso-3.33.1
  
  
* Reduce the size of the Windows header files.
* Improve yasio::inet::ip::endpoint code style.
* Explicit socket_select_interrupter workaround code logic for borken firewalls on Windows.
* Sets and pass user data of transport through event.
* Fix KCP do_read may can't dispatch upper data to user.
* Fix behavior of yasio::wcsfmt.
  
  
yasio-3.33.0
  
* Refactor UDP like transport, UDP client don't establish 4-tuple with peer, and provide  ```YPOT_T_CONNECT``` and ```YPOT_T_DISCONNECT``` to change association.
* Add ```io_service::write_to``` for ```unconnected/connected``` UDP transport.
* Remove unused channel masks ```YCM_MCAST_CLIENT```, ```YCM_MCAST_SERVER```
* Remove unused channel flag ```YCF_MCAST_LOOPBACK```
* Add new options ```YOPT_C_ENABLE_MCAST```, ```YOPT_C_DISABLE_MCAST``` for multicast support
* Change ```timer_cb_t``` prototype to ```[]()->bool { }```, return ```true``` for once, ```false``` for continue.
* Add ```highp_timer::async_wait_once``` to wait timer timeout once.
* Change ```YCM_XXX_[CLIENT/SERVER]``` to ```YCK_XXX_[CLIENT/SERVER]```.
* Add ```yasio::xhighp_clock``` to retrive nanoseconds timestamp.
* Fix xxsocket APIs connect_n, recv_n doesn't handle signal EINTR.
* Tidy obstream/ibstream API, by default ```write_v/read_v``` use ```7bit encoded int``` for length field.
* Rename io_service ```start_service/stop_service``` to ```start/stop```.
* Fix c-ares timeout behavior.
* Improve c-ares cleanup behavior, now destruct io_service more stable with c-ares enabled.
* Make ```cxx17::string_view``` support unordered set/map on compilers which only support c++11 standard.
* Use ```shared_ptr + shared_mutex``` to ensure destruct io_service safe without side affect for concurrency of name resolving.
* Fix dns cache timeout mechanism doesn't work.
* Simplify c-ares dns-server setup on android, when use yasio as static library, you need call ```yasio__jni_onload``` at ```JNI_OnLoad```.
* Fix ```yasio::_strfmt``` may crash on some incorrect use, now it's more stable on all platforms.
* Improve behavior when kernel send buffer is full, don't sleep a fixed time, just drived by ```select```.
* Optimize udp transport close behavior, by default, udp transport will never close except user request, still can use io_service's option YOPT_S_IGNORE_UDP_ERROR to change this behavior.
* Change write completion handler prototype to: ```std::function<void(int ec, size_t bytes_transferred)>```.
* Implement literals for ```cxx17::string_view```.
* Fix ssl handshake failed with certificate verify failed when cacert file provided and flag ```SSL_VERIFY_PEER``` was set.
* Fix doesn't call io_service destructor when use lua binding library ```kaguya``` on compiler without c++14 support.
* Add ```io_service::init_globals(const print_fn_t&)``` to support redirect initialization log to custom file(U3D/UE4 Console).
* Improve compiler support, now support c++14, c++17, c++20.
* Auto choose library ```sol2``` for lua binding when ```cxx_std >= 14```, older require ```cxx_std >= 17```.
* Recreate the socket_select_interrupter's sockets on error. 
* Update kcp to v1.7, the kcp older version may cause SIGBUS on mobile ARM.
* Simplify API, remove unnecessary API ```io_service::reopen```, please use ```io_service::open``` instead.
* Fix crash at yasio::inet::ip::endpoint::ip() when af=0.
* Make io_service::write to a kcp return value same as other channel.
* Fix kcp server doesn't decode packet header.
* Add xxsocket::disconnect to dissolve the 4-tuple association.
* Rename option ```YOPT_I_SOCKOPT``` to ```YOPT_B_SOCKOPT```.
* Other code quality & stable improvements.
  
yasio-3.31.3
  
* Optimize API io_service::write, add write raw buf support.
* Fix issue: https://github.com/yasio/yasio/issues/208
* Fix issue: https://github.com/yasio/yasio/issues/209
  
  
yasio-3.31.2
  
* Optimize singleton implementation, see: https://github.com/yasio/yasio/issues/200
* Fix typo, YASIO_VERBOS_LOG to YASIO_VERBOSE_LOG.
* Explicit set socktype for ```getaddrinfo```, see: https://github.com/yasio/yasio/issues/201
* Improve xxsocket send_n/recv_n implementation and behavior, see: https://github.com/yasio/yasio/issues/202
  
  
yasio-3.31.1
  
* Make c-ares works well on android 8 or later.
  
  
yasio-3.31.0
  
* Add Initial Bytes To Strip support for length field based frame decode, use YOPT_C_LFBFD_IBTS.
* Add SSL client support, use YASIO_HAVE_SSL to enable, YCM_SSL_CLIENT to open a channel with ssl client support.
* Integrate c-ares support, YASIO_HAVE_CARES to enable, make sure your build system already have c-ares.
* Refactor timeout options, use YOPT_S_CONNECT_TIMEOUT, YOPT_S_DNS_CACHE_TIMEOUT, YOPT_S_DNS_QUERIES_TIMEOUT to instead YOPT_S_TIMEOUTS.
* Optimize schedule_timer behavior, always replace timer_cb when timer exist.
* Remove deprecated functions.
* Remove tolua support.
  
  
yasio-3.30.8
  
* Sync optimizations from v3.31.2.
  
  
yasio-3.30.7
  
* Sync optimizations from v3.31.
  
  
yasio-3.30.6
  
* Make option enums compatible with v3.31.
* Simplify io_service state.
  
  
yasio-3.30.5
  
* Fix missing break at set_option for YOPT_C_REMOTE_ENDPOINT.
  
  
yasio-3.30.4
  
* Make normal concurrent queue more safe if SPSC queue is disabled.
* Make sure udp recv buf enough at kcp transport do_read.
* Fix io_service::write always retur 0.
  
  
yasio-3.30.3
  
* Optimize jsb, jsb2.0 and lua binding.
  
  
yasio-3.30.2
  
* Fix jsb2.0 bindings issue, see: [#192](https://github.com/yasio/yasio/issues/192)
  
  
yasio-3.30.1
  
* Add construct channel at io_service constructor, now all options could be set before start_service if you doesn't use the start_service with channel hostents, and mark them deprecated
* Use cmake for trvis ci cross-platform build at win32, linux, osx, ios
* Sync script bindings
  
  
yasio-3.30.0
  
* Tidy option macros.
* Add YCF_ enums to control channel to support more features.
* Add multicast support.
* Add a workaround implementation to support win32 udp-server.
* Add io_service::cindex_to_handle.
* Add ftp sever example.
* Remove loop behavior of deadline_timer, user can schedule again when it's expired.
* Add obstream::write_byte.
* Add to_strf_v4 for ip::endpoint.
* Optimizing for file transfer, avoid high cpu occupation when system kernel send buffer is full.
* More safe to check object valid which allocated from pool.
* Add send complete callback.
* Mark io_service::dispatch_events deprecated, use dispatch to instead.
* Add YCF_REUSEPORT to control whether to enable socket can bind same port, default and previous vesion is enabled.
* Implement case insensitive starts_with, ends_with at string_view.hpp.
* **Ignore SIGPIPE for tcp at non-win32 system.**
* **Remove reconnect timeout.**
