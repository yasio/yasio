# ibstream
二进制读取流，读取数值类型时自动转换为主机字节序

## read_i[x]
从流中读取有符号整数，流内部的数据偏移指针会根据读取整数字节大小，做相应增加

## read_u[x]
从流中读取无符号符号整数，流内部的数据偏移指针会根据读取整数字节大小，做相应增加

## read_f
从流中读取单精度浮点数，流内部的数据偏移指针+4字节

## read_lf
从流中读取双精度浮点数，流内部的数据偏移指针+8字节

## read_v
```string/ArrayBuffer read_v(lfl, raw)```  
从流中读取通用数据:  
参数说明:  
   lfl：数据大小字段长度bits(8,16,32bits), 可不传,默认值-1  
   raw: lua忽略此参数, 返回string类型; javascript: false:返回字符串, true返回ArrayBuffer对象, 默认值false  
注意: API read_string已移除， 若需要与旧代码兼容，请使用read_v(32)代替。  

## read_bytes
```string/ArrayBuffer read_bytes(n)```  
从流中读取固定字节数据，流内部指针+n  
lua返回string类型, javascript返回ArrayBuffer类型  

## seek
```int seek(offset, whence)```  
功能: 类似posix API: lseek  
参数whence取值: yasio.SEEK_CUR, yasio.SEEK_SET, yasio.SEEK_END

## length
```int length()```  
功能: 返回流字节数

## Example
Lua:
```lua
local msg = {}
local ibs = event:packet()

-- 读取1字节整数
msg.cmd = ibs:read_i8()

-- 跳过4字节长度域
ibs:seek(4, yasio.SEEK_CUR)

msg.seq = ibs:read_i16() 
msg.content = ibs:read_v()
```
JavaScript:
```javascript
var msg = {};
var ibs = event.packet();

// 读取1字节整数
msg.cmd = ibs.read_i8();

// 跳过4字节长度域
ibs.seek(4, yasio.SEEK_CUR);

msg.seq = ibs.read_i16();
msg.content = ibs.read_v();
```