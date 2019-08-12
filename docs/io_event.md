# io_event
网络事件回调返回此对象

##  kind
int kind()  
功能：获取事件类型  
返回值:  
yasio.YEK_PACKET: 数据包事件  
yasio.YEK_CONNECT_RESPONSE: 连接响应事件  
yasio.YEK_CONNECTION_LOST: 连接丢失事件  
  
## status
int status()  
返回值: 0: 正常， 非0： 出错
说明: 业务只需要处理0和非0的情况，无需关心具体状态码，非0时可选择做简单打印记录

## packet
ibstream/ArrayBuffer packet(raw,copy)  
参数：  
raw: 可选参数，默认值false且, lua忽略此参数，始终返回ibstream;  true: js返回ArrayBuffer  
copy: 可选参数，是否复制数据包，默认值false  
说明: 注意此函数默认会从事件中取走数据包，也就是说同一个事件对象不可重复调用。

## transport
transport_ptr transport()  
功能: 获取事件当前的传输会话句柄  
说明: 用户使用时可以在YEK_CONNECT_RESPONSE事件，且事件状态码为0时调用此函数获取并保存句柄，以便调用io_service的write接口向远端发送数据

## timestamp
number timestamp()  
功能: 返回事件产生的微妙级时间戳  