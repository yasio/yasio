@echo off
cmake -A Win32 ..
cmake --build . --config Debug --target yasio
cmake --build . --config Release --target yasio
