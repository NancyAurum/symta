@echo off
set PATH=C:\Program Files\mingw-w64\x86_64-6.3.0-posix-seh-rt_v5-rev2\mingw64\bin;%PATH%
make
copy lib\main.exe ..\..\..\som\lib\ffi\gfx\main