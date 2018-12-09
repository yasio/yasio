require 'protocol_base'
require 'protocol_enc'
require 'protocol_dec'

local masio = require('masio') -- constants
local stopFlag = 0

local hostent = io_hostent.new()
hostent.address = "0.0.0.0";
hostent.port = 8081;
local server = io_service.new()
local hostents = {
    io_hostent.new('0.0.0.0', 8081),
    io_hostent.new('0.0.0.0', 8082)
}

local transport1 = nil
local data_partial2 = nil
server:start_service(hostents, function(event)
        local t = event:kind()
        if t == masio.MASIO_EVENT_RECV_PACKET then
            local packet = event:packet()
            print(packet)
        elseif(t == masio.MASIO_EVENT_CONNECT_RESPONSE) then -- connect responseType
            if(event:error_code() == 0) then
                local transport = event:transport()
                -- local requestData = "GET /index.htm HTTP/1.1\r\nHost: www.ip138.com\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.106 Safari/537.36\r\nAccept: */*;q=0.8\r\nConnection: Close\r\n\r\n"
                local msg = {
                    id = 110,
                    value1 = 3291,
                    value2 = 391040,
                    value3 = true,
                    value4 = 301.32,
                    value6 = 13883.197,
                    uname = "halx99",
                    passwd = "x-studio365 is a powerful IDE!"
                }
                local obs = proto.e101(msg)
                local data = obs:to_string()
                local data_partial1 = data:sub(1, #data - 10)
                server:write(transport, data_partial1)
                
                print('The remain data will be sent after 6 seconds...')
                transport1 = transport
                data_partial2 = data:sub(#data - 10 + 1, #data)
            else
                print("connect server failed!")
            end
        elseif(t == masio.MASIO_EVENT_CONNECTION_LOST) then -- connection lost event
            print("The connection is lost!")
        end
    end)
server:open(0, masio.CHANNEL_TCP_SERVER)

local client = io_service.new()
hostent.address = "127.0.0.1"
client:set_option(masio.MASIO_OPT_LFIB_PARAMS, 65535, 0, 4, 0)
local func = function(event)
    local t = event:kind()
    if t == masio.MASIO_EVENT_RECV_PACKET then
        local ibs = event:packet()
        local msg = proto.d101(ibs)
        print(string.format('receve data from server: %s', msg.passwd))
        stopFlag = stopFlag + 1
        -- test buffer out_of_range exception handler
        local _, result = pcall(ibs.read_i8, ibs)
        print(result)
    elseif(t == masio.MASIO_EVENT_CONNECT_RESPONSE) then -- connect responseType
        if(event:error_code() == 0) then
            print("connect serve succeed.")
            -- local transport = event:transport()
            -- local requestData = "GET /index.htm HTTP/1.1\r\nHost: www.ip138.com\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.106 Safari/537.36\r\nAccept: */*;q=0.8\r\nConnection: Close\r\n\r\n"
            -- client:write(transport, obs)
        else
            print("connect server failed!")
        end
    elseif(t == masio.MASIO_EVENT_CONNECTION_LOST) then -- connection lost event
        print("The connection is lost!")
    end
end
client:start_service(hostent, func)
-- client:set_option(masio.MASIO_OPT_IO_EVENT_CALLBACK, func)
client:open(0, masio.CHANNEL_TCP_CLIENT)


-- httpclient
local httpclient = io_service.new()
hostent.address = "ip138.com"
hostent.port = 80
httpclient:set_option(masio.MASIO_OPT_LFIB_PARAMS, 65535, -1, 0, 0)
httpclient:start_service(hostent, function(event)
        local t = event:kind()
        if t == masio.MASIO_EVENT_RECV_PACKET then
            local ibs = event:packet()
            print(string.format('receve data from server: %s', ibs:to_string()))
            -- print(packet)
        elseif(t == masio.MASIO_EVENT_CONNECT_RESPONSE) then -- connect responseType
            if(event:error_code() == 0) then
                local transport = event:transport()
                local requestData = "GET /index.htm HTTP/1.1\r\nHost: www.ip138.com\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.106 Safari/537.36\r\nAccept: */*;q=0.8\r\nConnection: Close\r\n\r\n"
                httpclient:write(transport, requestData)
            else
                print("connect server failed!")
            end
        elseif(t == masio.MASIO_EVENT_CONNECTION_LOST) then -- connection lost event
            print("The http connection is lost!")
            stopFlag = stopFlag + 1
        end
end)
httpclient:open(0, masio.CHANNEL_TCP_CLIENT)    

local elapsedTime = 0
local partial2Sent = false
function global_update(dt)
    server:dispatch_events(128)
    client:dispatch_events(128)
    httpclient:dispatch_events(128)
    elapsedTime = elapsedTime + dt
    if elapsedTime > 6 and not partial2Sent then
        server:write(transport1, data_partial2)
    end
    return stopFlag >= 2
end

print('done')
