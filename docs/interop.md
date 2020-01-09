# yasio interop

## yasio NI(Native Interface) API
```c
YASIO_NI_API void yasio_start(int channel_count,
                              void (*event_cb)(uint32_t emask, int cidx, intptr_t sid,
                              intptr_t bytes, int len));
YASIO_NI_API void yasio_set_option(int opt, const char* params);
YASIO_NI_API void yasio_set_resolv_fn(int (*resolv)(const char* host, intptr_t sbuf));
YASIO_NI_API void yasio_set_print_fn(void (*print_fn)(const char*));
YASIO_NI_API void yasio_open(int cindex, int kind);
YASIO_NI_API void yasio_close(int cindex);
YASIO_NI_API int yasio_write(intptr_t thandle, const unsigned char* bytes, int len);
YASIO_NI_API void yasio_dispatch(int count);
YASIO_NI_API void yasio_stop();
YASIO_NI_API long long yasio_highp_time(void);
YASIO_NI_API long long yasio_highp_clock(void);
YASIO_NI_API void yasio_memcpy(void* dst, const void* src, unsigned int len);
```

## dotnet API
```c#

// Integrate yasio to unity3d xlua, uncomment instead "yasio-ni", for integration detail, see https://github.com/simdsoft/xlua
// #if (UNITY_IPHONE || UNITY_WEBGL || UNITY_SWITCH) && !UNITY_EDITOR
//         public const string LIBNAME = "__Internal";
// #else
//         public const string LIBNAME = "xlua";
// #endif
const string LIBNAME = "yasio-ni";

public delegate void YNIEventDelegate(uint emask, int cidx, IntPtr thandle, IntPtr bytes, int len);
public delegate int YNIResolvDelegate(string host, IntPtr sbuf);
public delegate void YNIPrintDelegate(string msg);

[DllImport(LIBNAME, CallingConvention = CallingConvention.Cdecl)]
public static extern int yasio_start(int channel_count, YNIEventDelegate d);

[DllImport(LIBNAME, CallingConvention = CallingConvention.Cdecl)]
public static extern void yasio_open(int cindex, int cmask);

[DllImport(LIBNAME, CallingConvention = CallingConvention.Cdecl)]
public static extern void yasio_close(int cindex);

[DllImport(LIBNAME, CallingConvention = CallingConvention.Cdecl)]
public static extern void yasio_close_handle(IntPtr thandle);

[DllImport(LIBNAME, CallingConvention = CallingConvention.Cdecl)]
public static extern void yasio_write(IntPtr thandle, byte[] bytes, int len);

[DllImport(LIBNAME, CallingConvention = CallingConvention.Cdecl)]
public static extern void yasio_dispatch(int maxEvents);

[DllImport(LIBNAME, CallingConvention = CallingConvention.Cdecl)]
public static extern void yasio_set_option(int opt, string strParam);

[DllImport(LIBNAME, CallingConvention = CallingConvention.Cdecl)]
public static extern void yasio_set_resolv_fn(YNIResolvDelegate fnResolv);

[DllImport(LIBNAME, CallingConvention = CallingConvention.Cdecl)]
public static extern void yasio_set_print_fn(YNIPrintDelegate fnPrint);

[DllImport(LIBNAME, CallingConvention = CallingConvention.Cdecl)]
public static extern void yasio_stop();

[DllImport(LIBNAME, CallingConvention = CallingConvention.Cdecl)]
public static extern long yasio_highp_time();

[DllImport(LIBNAME, CallingConvention = CallingConvention.Cdecl)]
public static extern void yasio_memcpy(IntPtr destination, IntPtr source, uint length);

/// <summary>
/// The yasio constants
/// </summary>
public enum YEnums
{
    #region Channel mask enums, copy from yasio.hpp
    YCM_CLIENT = 1,
    YCM_SERVER = 1 << 1,
    YCM_POSIX = 1 << 2,
    YCM_TCP = 1 << 3,
    YCM_UDP = 1 << 4,
    YCM_MCAST = 1 << 5,
    YCM_KCP = 1 << 6,
    YCM_SSL = 1 << 7,
    YCM_TCP_CLIENT = YCM_TCP | YCM_CLIENT | YCM_POSIX,
    YCM_TCP_SERVER = YCM_TCP | YCM_SERVER | YCM_POSIX,
    YCM_UDP_CLIENT = YCM_UDP | YCM_CLIENT | YCM_POSIX,
    YCM_UDP_SERVER = YCM_UDP | YCM_SERVER | YCM_POSIX,
    #endregion

    #region Event kind enums, copy from yasio.hpp
    YEK_CONNECT_RESPONSE = 1,
    YEK_CONNECTION_LOST,
    YEK_PACKET,
    #endregion

    #region All supported options by native, copy from yasio.hpp
    // Set with deferred dispatch event, default is: 1
    // params: deferred_event:int(1)
    YOPT_S_DEFERRED_EVENT = 1,

    // Set custom resolve function, native C++ ONLY
    // params: func:resolv_fn_t*
    YOPT_S_RESOLV_FN,

    // Set custom print function, native C++ ONLY, you must ensure thread safe of it.
    // parmas: func:print_fn_t,
    YOPT_S_PRINT_FN,

    // Set custom print function
    // params: func:io_event_cb_t*
    YOPT_S_EVENT_CB,

    // Set tcp keepalive in seconds, probes is tries.
    // params: idle:int(7200), interal:int(75), probes:int(10)
    YOPT_S_TCP_KEEPALIVE,

    // Don't start a new thread to run event loop
    // value:int(0)
    YOPT_S_NO_NEW_THREAD,

    // Sets ssl verification cert, if empty, don't verify
    // value:const char*
    YOPT_S_SSL_CACERT,

    // Set connect timeout in seconds
    // params: connect_timeout:int(10)
    YOPT_S_CONNECT_TIMEOUT,

    // Set dns cache timeout in seconds
    // params: dns_cache_timeout : int(600),
    YOPT_S_DNS_CACHE_TIMEOUT,

    // Set dns queries timeout in seconds, only works when have c-ares
    // params: dns_queries_timeout : int(10)
    YOPT_S_DNS_QUERIES_TIMEOUT,

    // Sets channel length field based frame decode function, native C++ ONLY
    // params: index:int, func:decode_len_fn_t*
    YOPT_C_LFBFD_FN = 101,

    // Sets channel length field based frame decode params
    // params:
    //     index:int,
    //     max_frame_length:int(10MBytes),
    //     length_field_offset:int(-1),
    //     length_field_length:int(4),
    //     length_adjustment:int(0),
    YOPT_C_LFBFD_PARAMS,

    // Sets channel length field based frame decode initial bytes to strip, default is 0
    // params:
    //     index:int,
    //     initial_bytes_to_strip:int(0)
    YOPT_C_LFBFD_IBTS,

    // Sets channel local port
    // params: index:int, port:int
    YOPT_C_LOCAL_PORT,

    // Sets channel local host, for server only to bind specified ifaddr
    // params: index:int, ip:const char*
    YOPT_C_LOCAL_HOST,

    // Sets channel local endpoint
    // params: index:int, ip:const char*, port:int
    YOPT_C_LOCAL_ENDPOINT,

    // Sets channel remote host
    // params: index:int, ip:const char*
    YOPT_C_REMOTE_HOST,

    // Sets channel remote port
    // params: index:int, port:int
    YOPT_C_REMOTE_PORT,

    // Sets channel remote endpoint
    // params: index:int, ip:const char*, port:int
    YOPT_C_REMOTE_ENDPOINT,

    // Sets channl flags
    // params: index:int, flagsToAdd:int, flagsToRemove:int
    YOPT_C_MOD_FLAGS,

    // Sets io_base sockopt
    // params: io_base*,level:int,optname:int,optval:int,optlen:int
    YOPT_I_SOCKOPT = 201,
    #endregion
};

```
