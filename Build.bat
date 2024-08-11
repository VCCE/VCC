@echo off
:: Build VCC 
if exist "%VSINSTALLDIR%\MSBuild\Current\Bin\MSBuild.exe" (
  msbuild vcc.sln /p:Configuration=Release /p:Platform=x86
) else (
  echo Build must be run from Developer Command Prompt for VS 2022
)
