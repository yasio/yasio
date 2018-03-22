@echo off

rmdir /q /s ipch
rmdir /q /s *ipch
del /q /f /s *.iobj
del /q /f /s *.ipdb
del /q /f /s *.sdf
del /q /f /s *.ncb
del /q /f /s *.sbr
del /q /f /s *.bsc
del /q /f /s *.ilk
del /q /f /s *.pdb
del /q /f /s *.user
del /q /f /s *.log
del /q /f /s *.tlog
del /q /f /s *.obj
del /q /f /s *.idb
del /q /f /s *.lastbuildstate
del /q /f /s *.ib_pdb_index
del /q /f /s *.ib_tag
del /q /f /s *.res
del /q /f /s *.pch
del /q /f /s /a *.suo
del /q /f /s *.db
rem del /q /f /s /a *.dll
rem del /q /f /s /a *.lib
rem del /q /f /s /a *.exe
del /q /f /s /a *.exp

ping /n 3 127.0.1 >nul

goto :eof

