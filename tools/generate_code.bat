@echo off

pcode_autog.exe

move protocol_*.lua ..\test\test\
del /f /s /q *.h
del /f /s /q *.cpp
del /f /s /q *.lua

ping /n 2 127.0.1>nul && goto :eof
