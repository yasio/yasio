# demo

## Remote Debug based on TCP
典型的基于TCP的服务端客户端远程调试示例，使用yasio的io_servcie实现，调试后端和调试前端无需自己开线程，直接使用服务线程处理调试协议即可。协议头一般定义为如下方式即可：
```C++
// odbk_proto_header
struct odbk_proto_header {
  int total_size;
  int version;
  uint8_t reserved; // future: compressed/encrypted
  uint8_t cmd; // 
};
// 例如调试后端将打印消息包封装
std::vector<char> odbkp_create_msg(int type, const std::string &msg)
{
  yasio::obstream obs;
  obs.push32();
  obs.write_i(ODBKP_VERSION);
  obs.write_i((uint8_t)proto_code::EV_MESSAGE);
  obs.write_i<uint8_t>(0);
  obs.write_va(msg);
  obs.pop32(obs.length());
  return std::move(obs.buffer());
}
```

```C++
// ****************** server ***************************
io_hostent ep("0.0.0.0", 'odbk' % 65536); // 25195
yasio_shared_service->start_service(&ep, [=](event_ptr e) {
auto k = e->kind();
if (k == YEK_PACKET)
{
    ibstream ibs(std::move(e->packet()));
    ibs.seek(sizeof(int), SEEK_CUR); // skip length field
    uint32_t proto_version = ibs.read_i<uint32_t>();
    // TODO: check version?
    uint8_t cmd = ibs.read_i<uint8_t>(); // cmd
    // call different handler with different cmd
}
else if (k == YEK_CONNECT_RESPONSE)
{
    if (e->status() == 0)
    {
    printf("Accept frontend succeed."); // debug session estiblished with socket.
    // this->session = e->transport(); // 保存通讯session用于发送数据, m_session是transport_handle_t类型
    }
    else
    {
    printf("Accept frontend exception: %d", e->status());
    }
}
else if (k == YEK_CONNECTION_LOST)
{
    printf("The connection with frontend is lost: %d, waiting new to income...", e->status());
}
});
yasio_shared_service->set_option(YOPT_C_LFBFD_PARAMS,
    0, -- channelIndex  
    65535, -- maxFrameLength
    0,  -- lenghtFieldOffset
    4, -- lengthFieldLength
    0 -- lengthAdjustment
);
yasio_shared_service->set_option(YOPT_S_DEFERRED_EVENT, 0); // 禁用事件队列，网络事件直接在服务线程分派
yasio_shared_service->open(0, YCM_TCP_SERVER);

// ********************* client ***********************
// start process, always try to connect local odbk backend
io_hostent ep("127.0.0.1", 'odbk' % 65536); // 25195
yasio_shared_service->start_service(&ep, [=](event_ptr e) {
    auto k = e->kind();
    if (k == YEK_PACKET)
    {
        ibstream ibs(std::move(e->packet()));
        ibs.seek(sizeof(int), SEEK_CUR); // skip length field
        uint32_t proto_version = ibs.read_i<uint32_t>();
        // TODO: check version?
        uint8_t cmd = ibs.read_i<uint8_t>(); // cmd
        // call different handler with different cmd
    }
    else if (k == YEK_CONNECT_RESPONSE)
    {
        if (e->status() == 0)
        {
            printf("connect backend succeed."); // debug session estiblished with socket.
            // 保存通讯session用于发送数据
            // m_session = e->transport(); // m_session是transport_handle_t类型
        }
        else
        {
            printf("Accept frontend exception: %d", e->status());
        }
    }
    else if (k == YEK_CONNECTION_LOST)
    {
        printf("The connection with backend is lost: %d, waiting new to income...", e->status());
    }
    });
yasio_shared_service->set_option(YOPT_C_LFBFD_PARAMS,
    0, -- channelIndex  
    65535, -- maxFrameLength
    0,  -- lenghtFieldOffset
    4, -- lengthFieldLength
    0 -- lengthAdjustment
);
yasio_shared_service->set_option(YOPT_S_DEFERRED_EVENT, 0); // 禁用事件队列，网络事件直接在服务线程分派
yasio_shared_service->open(0, YCM_TCP_CLIENT);
```

## IPV6 ONLY reference
http://blog.csdn.net/baidu_25743639/article/details/51351638  
http://www.cnblogs.com/SUPER-F/p/IPV6.html
