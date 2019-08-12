# obstream
二进制写入流，写入数值类型时自动转换为网络字节序

## Encode Number
write_i8,write_i16,write_i24,write_i32,write_i64位有符号整数编码，函数名中的数字代表整数长度位数(bits), write_i8相当于向stream写入一个字节数据；同样的无符号整数write_u8,write_u16等。

write_f: 单精度浮点数
write_lf: 双精度浮点数

## Encode Blob
write_v(v,lfl): 写入通用数据， 写入数据前会先写入数据长度，参数v可以是字符串，js中的ArrayBuffer,Uint8Array, Int8Array; 参数lfl指是数据长度字段使用多少位存储，有效值为-1，8，16，32, 参数可选，默认值是-1表示使用7bits Encoded Int变长长度域。  
注意: API write_string已移除，如果需要与旧代码兼容，请使用write_v(v, 32)代替。

## Encode Fixed Blob
write_bytes(v): 写入固定字节数据，写入实际字节数据前不会写入数据长度，参数v和write_v一样

## Encapsulate Packet
push8,push16,push32/pop8,pop16,pop32: 主要作用是将整包长度写入特定偏移位置，一次封包只需调用一次  
```void pop([n])```: n可选参数, 如果不传则将当前流数据总字节数写入最近一次push的位置，如果传了则将n值写入最近一次push的位置;  
v3.22.0以及之后的版本，不传参时是将"当前数据总字节数-push偏移-长度域字节数"写入最近一次push的位置。

## length
```int length()```, 返回流数据总字节数

## Example
Lua:
```lua
local obs = yasio.obstream.new(128)

-- 内存布局|-8bits-|
obs:write_i8(12) 

-- 内存布局|-8bits-|-32bits-|
obs:push32() 

-- 内存布局|-8bits-|-32bits-|-16bits-|
obs:write_i16(52013) 

-- 内存布局|-8bits-|-32bits-|-16bits-|-8bits(length of the string)-|-88bits(the string)-|
obs:write_v("hello world")

-- 将整包长度字节数写入最近一次push保留的4字节内存，完成封包，内存布局不变
obs:pop32(obs:length())
```
JavaScript:
```javascript
var obs = new yasio.obstream(128);

// 内存布局|-8bits-|
obs.write_i8(12);

// 内存布局|-8bits-|-32bits-|
obs.push32();

// 内存布局|-8bits-|-32bits-|-16bits-|
obs.write_i16(52013);

// 内存布局|-8bits-|-32bits-|-16bits-|-8bits(length of the string)-|-88bits(the string)-|
obs.write_v("hello world");

// 将整包长度字节数写入最近一次push保留的4字节内存，完成封包，内存布局不变
obs.pop32(obs.length());
```