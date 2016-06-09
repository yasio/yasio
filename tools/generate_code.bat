@echo off

pcode_autog.exe

move *.h ..\test\xxsocket_test\
move *.cpp ..\test\xxsocket_test\

ping /n 2 127.0.1>nul && goto :eof
