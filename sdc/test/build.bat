@echo off
:: Build the CoCo SDC stream test kit into .\out
:: Set LWASM to your lwasm.exe if it is not on the PATH.
setlocal
cd /d "%~dp0"
if "%LWASM%"=="" set LWASM=lwasm
if not exist out mkdir out
"%LWASM%" --decb --list=out\strmtest.lst -o out\STRMTEST.BIN strmtest.asm || exit /b 1
python mktest.py out\STRMTEST.BIN out || exit /b 1
echo.
echo Copy out\STRMTEST.DSK, out\STRM512.DAT and out\STRMODD.DAT to the SDC card folder.
