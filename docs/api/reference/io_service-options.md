---
Description: The following are the io_service options.
title: io_service options
date: 12/17/2020
---

# io_service options

io_service支持的所有选项。

|Name|Description|
|----------|-----------------|
|*YOPT_S_DEFER_EVENT_CB*|Set defer event callback<br/>params: callback:defer_event_cb_t<br/>remarks:<br/>a. User can do custom packet resolve at network thread, such as decompress and crc check.<br/>b. Return true, io_service will continue enque to event queue.<br/>c. Return false, io_service will drop the event.|
|*YOPT_S_DEFERRED_EVENT*|Set whether deferred dispatch event, default is: 1<br/>params: deferred_event:int(1)|
|*YOPT_S_RESOLV_FN*|Set custom resolve function, native C++ ONLY<br/>params: func:resolv_fn_t*|
|*YOPT_S_PRINT_FN*|Set custom print function native C++ ONLY<br/>parmas: func:print_fn_t<br/>remarks: you must ensure thread safe of it|
|*YOPT_S_PRINT_FN2*|Set custom print function with log level<br/>parmas: func:print_fn2_t<br/>you must ensure thread safe of it|
|*YOPT_S_EVENT_CB*|Set event callback<br/>params: func:event_cb_t*|
|*YOPT_S_TCP_KEEPALIVE*|Set tcp keepalive in seconds, probes is tries.<br/>params: idle:int(7200), interal:int(75), probes:int(10)|
|*YOPT_S_NO_NEW_THREAD*|Don't start a new thread to run event loop.<br/>params: value:int(0)|
|*YOPT_S_SSL_CACERT*|Sets ssl verification cert, if empty, don't verify.<br/>params: path:const char*|
|*YOPT_S_SSL_CERT*|Sets ssl server cert and private key, if empty, the ssl server doesn't work.<br/>params: cert_file:const char*<br/>params: key_file:const char*|
|*YOPT_S_CONNECT_TIMEOUT*|Set connect timeout in seconds.<br/>params: connect_timeout:int(10)|
|*YOPT_S_CONNECT_TIMEOUTMS*|Set connect timeout in milliseconds.<br/>params: connect_timeout:int(10000)|
|*YOPT_S_DNS_CACHE_TIMEOUT*|Set dns cache timeout in seconds.<br/>params: dns_cache_timeout : int(600)|
|*YOPT_S_DNS_CACHE_TIMEOUTMS*|Set dns cache timeout in milliseconds.<br/>params: dns_cache_timeout : int(600000)|
|*YOPT_S_DNS_QUERIES_TIMEOUT*|Set dns queries timeout in seconds, default is: 5.<br/>params: dns_queries_timeout : int(5)<br/>remark: <br/>a. this option must be set before 'io_service::start'<br/>b. only works when have c-ares<br/>c. since v3.33.0 it's milliseconds, previous is seconds.<br/>d. the timeout algorithm of c-ares is complicated, usually, by default, dns queries<br/>will failed with timeout after more than 75 seconds.<br/>e. for more detail, please see:<br/>https://c-ares.haxx.se/ares_init_options.html|
|*YOPT_S_DNS_QUERIES_TIMEOUTMS*|Set dns queries timeout in seconds, see also *YOPT_S_DNS_QUERIES_TIMEOUT*|
|*YOPT_S_DNS_QUERIES_TRIES*|Set dns queries tries when timeout reached, default is: 5.<br/>params: dns_queries_tries : int(5)<br/>remarks:<br/>a. this option must be set before 'io_service::start'<br/>b. relative option: *YOPT_S_DNS_QUERIES_TIMEOUT*|
|*YOPT_S_DNS_DIRTY*|Set dns server dirty.<br/>params: reserved : int(1)<br/>remarks:<br/>a. this option only works with c-ares enabled<br/>b. you should set this option after your mobile network changed|
|*YOPT_S_DNS_LIST*|Set dns server list.<br/>params: servers : const char*("xxx.xxx.xxx.xxx[:port],xxx.xxx.xxx.xxx[:port]")|
|*YOPT_C_UNPACK_FN*|Sets channel length field based frame decode function.<br/>params: index:int, func:decode_len_fn_t*<br/>remark: native C++ ONLY|
|*YOPT_C_UNPACK_PARAMS*|Sets channel length field based frame decode params.<br/>params:<br/>index:int,<br/>max_frame_length:int(10MBytes),<br/>length_field_offset:int(-1),<br/>length_field_length:int(4),<br/>length_adjustment:int(0),|
|*YOPT_C_UNPACK_STRIP*|Sets channel length field based frame decode initial bytes to strip.<br/>params:index:int,initial_bytes_to_strip:int(0)|
|*YOPT_C_REMOTE_HOST*|Sets channel remote host.<br/>params: index:int, ip:const char*|
|*YOPT_C_REMOTE_PORT*|Sets channel remote port.<br/>params: index:int, port:int|
|*YOPT_C_REMOTE_ENDPOINT*|Sets channel remote endpoint.<br/>params: index:int, ip:const char*, port:int|
|*YOPT_C_LOCAL_HOST*|Sets local host for client channel only.<br/>params: index:int, ip:const char*|
|*YOPT_C_LOCAL_PORT*|Sets local port for client channel only.<br/>params: index:int, port:int|
|*YOPT_C_LOCAL_ENDPOINT*|Sets local endpoint for client channel only.<br/>params: index:int, ip:const char*, port:int|
|*YOPT_C_MOD_FLAGS*|Mods channl flags.<br/>params: index:int, flagsToAdd:int, flagsToRemove:int|
|*YOPT_C_ENABLE_MCAST*|Enable channel multicast mode.<br/>params: index:int, multi_addr:const char*, loopback:int|
|*YOPT_C_DISABLE_MCAST*|Disable channel multicast mode.<br/>params: index:int|
|*YOPT_C_KCP_CONV*|The kcp conv id, must equal in two endpoint from the same connection.<br/>params: index:int, conv:int|
|*YOPT_T_CONNECT*|Change 4-tuple association for io_transport_udp.<br/>params: transport:transport_handle_t<br/>remark: only works for udp client transport|
|*YOPT_T_DISCONNECT*|Dissolve 4-tuple association for io_transport_udp.<br/>params: transport:transport_handle_t<br/>remark: only works for udp client transport|
|*YOPT_B_SOCKOPT*|Sets io_base sockopt.<br/>params: io_base*,level:int,optname:int,optval:int,optlen:int|

## 请参阅

[io_service Class](./io_service-class.md)
