@echo off
rem Builds the standalone NCM driver (.\ncm.exe).
rem
rem After the May 2026 consolidation, the canonical NCM source-of-
rem truth is symta/ncm/src/ncm.h and the stb_ds.h dependency is
rem shared with the rest of symta at ../runtime/stb_ds.h.
rem
rem If your toolchain isn't on PATH, edit the line below.
if "%MAKE_PATH%"=="" set MAKE_PATH=C:\soft\w64devkit\bin
set PATH=%MAKE_PATH%;%PATH%
rem -D_GNU_SOURCE: ncu_opts.h uses strdup/strndup unconditionally;
rem under -std=c99 those POSIX/GNU extensions are hidden by <string.h>,
rem so the build fails to link without this flag.
set cflags= -O2 -fno-stack-protector -fomit-frame-pointer -fno-exceptions -fno-unwind-tables -fno-ident -Wno-return-type -Wno-pointer-sign -D WINDOWS -D_GNU_SOURCE
gcc %cflags% -s -O2 -I..\runtime src/main.c -o .\ncm.exe
