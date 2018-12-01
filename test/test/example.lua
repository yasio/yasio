local masio = require('masio') -- constants
local stopFlag = 0

local obs = obstream.new()

obs:push32()
obs:write_u8(247)
obs:write_b(false)
obs:write_i16(2)
obs:write_i32(3)
obs:write_i64(4)
obs:write_f(5.2)
obs:write_lf(6.3)
obs:write_string("hello world!")
obs:pop32()

local hostent = io_hostent.new()
hostent.address_ = "0.0.0.0";
hostent.port_ = 8081;
local server = io_service.new()
server:start_service(hostent, 1)
server:set_event_callback(function(event)
        local t = event:type()
        if t == masio.MASIO_EVENT_RECV_PACKET then
            local packet = event:packet()
            -- print(packet)
        elseif(t == masio.MASIO_EVENT_CONNECT_RESPONSE) then -- connect responseType
            if(event:error_code() == 0) then
                local transport = event:transport()
                -- local requestData = "GET /index.htm HTTP/1.1\r\nHost: www.ip138.com\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.106 Safari/537.36\r\nAccept: */*;q=0.8\r\nConnection: Close\r\n\r\n"
                server:write(transport, obs)
            else
                print("connect server failed!")
            end
        elseif(t == masio.MASIO_EVENT_CONNECTION_LOST) then -- connection lost event
            print("The connection is lost!")
        end
    end)

server:open(0, masio.CHANNEL_TCP_SERVER)

local client = io_service.new()
hostent.address_ = "127.0.0.1"
client:set_option(masio.MASIO_OPT_LFIB_PARAMS, 0, 0, 16384)
client:start_service(hostent, 1)
client:set_event_callback(function(event)
        local t = event:type()
        if t == masio.MASIO_EVENT_RECV_PACKET then
            local ibs = event:packet()
            local len = ibs:read_i32()
            local i8 = ibs:read_u8()
            local boolval = ibs:read_b()
            local i16 = ibs:read_i16()
            local i32 = ibs:read_i32()
            local i64 = ibs:read_i64()
            local f = ibs:read_f()
            local lf = ibs:read_lf()
            local v = ibs:read_string()
            print(string.format('receve data from server: %s', v))
            stopFlag = stopFlag + 1
            -- test buffer overflow exception handler
            local succeed, result = pcall(ibs.read_i8, ibs)
            -- print(packet)
        elseif(t == masio.MASIO_EVENT_CONNECT_RESPONSE) then -- connect responseType
            if(event:error_code() == 0) then
                local transport = event:transport()
                -- local requestData = "GET /index.htm HTTP/1.1\r\nHost: www.ip138.com\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.106 Safari/537.36\r\nAccept: */*;q=0.8\r\nConnection: Close\r\n\r\n"
                -- client:write(transport, obs)
            else
                print("connect server failed!")
            end
        elseif(t == masio.MASIO_EVENT_CONNECTION_LOST) then -- connection lost event
            print("The connection is lost!")
        end
    end)
client:open(0, masio.CHANNEL_TCP_CLIENT)


-- httpclient
local httpclient = io_service.new()
hostent.address_ = "ip138.com"
hostent.port_ = 80
httpclient:set_option(masio.MASIO_OPT_LFIB_PARAMS, -1, 0, 65535)
httpclient:start_service(hostent, 1)
httpclient:set_event_callback(function(event)
        local t = event:type()
        if t == masio.MASIO_EVENT_RECV_PACKET then
            local ibs = event:packet()
            print(string.format('receve data from server: %s', ibs:to_string()))
            -- print(packet)
        elseif(t == masio.MASIO_EVENT_CONNECT_RESPONSE) then -- connect responseType
            if(event:error_code() == 0) then
                local transport = event:transport()
                local requestData = "GET /index.htm HTTP/1.1\r\nHost: www.ip138.com\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.106 Safari/537.36\r\nAccept: */*;q=0.8\r\nConnection: Close\r\n\r\n"
                httpclient:write(transport, requestData, #requestData)
            else
                print("connect server failed!")
            end
        elseif(t == masio.MASIO_EVENT_CONNECTION_LOST) then -- connection lost event
            print("The http connection is lost!")
            stopFlag = stopFlag + 1
        end
    end)
httpclient:open(0, masio.CHANNEL_TCP_CLIENT)    

function global_update(dt)
    server:dispatch_events(128)
    client:dispatch_events(128)
    httpclient:dispatch_events(128)
    return stopFlag >= 2
end

print('done')
