@echo off
msbuild
if errorlevel 1 goto failed
taskkill /im vcc.exe 2> nul >nul
timeout 2 > nul
copy __obj\Win32\Debug\sdc\out\sdc.dll C:\Users\ed\vcctst\
exit /b
:failed
echo build failed
exit /b
