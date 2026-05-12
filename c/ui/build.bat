rem @echo off
set PATH=C:\Program Files\mingw-w64\x86_64-6.3.0-posix-seh-rt_v5-rev2\mingw64\bin;%PATH%
rem set cflags2= -fno-stack-protector -fomit-frame-pointer -fno-exceptions -fno-unwind-tables -fno-ident -Wno-return-type -Wno-pointer-sign
rem -fpic
set cflags=%cflags%
set SDL2I=-I./deps/sdl2/include/SDL2 -I./deps/sdl2_mixer/include/SDL2
set SDL2L=-L./deps/sdl2/lib -L./deps/sdl2_mixer/lib -lsdl2 -lsdl2_mixer
set cflags=-D WINDOWS -O3 -I../../runtime/ %SDL2I% -I./include -I../gfx/src/ -DBZ_STRICT_ANSI -DNO_VIZ 
gcc %cflags% -shared -o ui.ffi ./src/main.c %SDL2L%
