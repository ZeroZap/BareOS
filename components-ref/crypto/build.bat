@echo off
echo XY Tiny Crypto Build Script
echo ============================

set CC=gcc
set CFLAGS=-Wall -Wextra -std=c99 -O2 -Iinclude
set LDFLAGS=-ladvapi32

if not exist build mkdir build

echo Compiling source files...
%CC% %CFLAGS% -c src/xy_md5.c -o build/xy_md5.o
%CC% %CFLAGS% -c src/xy_sha256.c -o build/xy_sha256.o
%CC% %CFLAGS% -c src/xy_aes.c -o build/xy_aes.o
%CC% %CFLAGS% -c src/xy_base64.c -o build/xy_base64.o
%CC% %CFLAGS% -c src/xy_hex.c -o build/xy_hex.o
%CC% %CFLAGS% -c src/xy_crc32.c -o build/xy_crc32.o
%CC% %CFLAGS% -c src/xy_random.c -o build/xy_random.o
%CC% %CFLAGS% -c src/xy_hmac.c -o build/xy_hmac.o

echo Creating static library...
ar rcs build/libxy_tiny_crypto.a build/xy_md5.o build/xy_sha256.o build/xy_aes.o build/xy_base64.o build/xy_hex.o build/xy_crc32.o build/xy_random.o build/xy_hmac.o

echo Compiling test program...
%CC% %CFLAGS% -c test/test_crypto.c -o build/test_crypto.o
%CC% build/test_crypto.o build/libxy_tiny_crypto.a -o build/test_crypto.exe %LDFLAGS%

echo Compiling example program...
%CC% %CFLAGS% -c example.c -o build/example.o
%CC% build/example.o build/libxy_tiny_crypto.a -o build/example.exe %LDFLAGS%

echo.
echo Build completed!
echo Generated files:
echo   - build/libxy_tiny_crypto.a  (static library)
echo   - build/test_crypto.exe      (test program)
echo   - build/example.exe          (example program)
echo.
echo Run test: build\test_crypto.exe
echo Run example: build\example.exe
pause