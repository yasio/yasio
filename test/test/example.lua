local stopFlag = false

local obs = obstream.new()

obs:push32()
obs:write_i8(1)
obs:write_i16(2)
obs:write_i32(3)
obs:write_i64(4)
obs:write_f(5.2)
obs:write_lf(6.3)
obs:write_v("hello world!")
obs:pop32()

local hostent = io_hostent.new()
hostent.address_ = "0.0.0.0";
hostent.port_ = 8081;
local server = async_socket_io.new()
server:start_service(hostent, 1)
server:set_callbacks(function(v,n)
        return n
    end,
    function(event)
        local t = event:type()
        if t == 2 then
            local packet = event:packet()
            -- print(packet)
        elseif(t == 0) then -- connect responseType
            if(event:error_code() == 0) then
                local transport = event:transport()
                -- local requestData = "GET /index.htm HTTP/1.1\r\nHost: www.ip138.com\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.106 Safari/537.36\r\nAccept: */*;q=0.8\r\nConnection: Close\r\n\r\n"
                server:write(transport, obs)
            else
                print("connect server failed!")
            end
        elseif(t == 1) then -- connection lost event
            print("The connection is lost!")
        end
    end)

server:open(0, 2)

local client = async_socket_io.new()
hostent.address_ = "127.0.0.1"
client:start_service(hostent, 1)
client:set_callbacks(function(v,n)
        return n
    end,
    function(event)
        local t = event:type()
        if t == 2 then
            local ibs = event:packet()
            local len = ibs:read_i32()
            local i8 = ibs:read_i8()
            local i16 = ibs:read_i16()
            local i32 = ibs:read_i32()
            local i64 = ibs:read_i64()
            local f = ibs:read_f()
            local lf = ibs:read_lf()
            local v = ibs:read_v()
            print(string.format('receve data from server: %s', v))
            
            local succeed, result = pcall(ibs.read_i8, ibs)
            
            stopFlag = true
            -- print(packet)
        elseif(t == 0) then -- connect responseType
            if(event:error_code() == 0) then
                local transport = event:transport()
                -- local requestData = "GET /index.htm HTTP/1.1\r\nHost: www.ip138.com\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.106 Safari/537.36\r\nAccept: */*;q=0.8\r\nConnection: Close\r\n\r\n"
                -- client:write(transport, obs)
            else
                print("connect server failed!")
            end
        elseif(t == 1) then -- connection lost event
            print("The connection is lost!")
        end
    end)
client:open(0, 1)

function global_update(dt)
    server:dispatch_events(128)
    client:dispatch_events(128)
    return stopFlag
end

print('done')
