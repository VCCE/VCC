@echo off
:: Build VCC 

set _CL_=/DUSE_LOGGING
set VCC_CONFIG=Release

if exist "%VSINSTALLDIR%\MSBuild\Current\Bin\MSBuild.exe" (
  nuget restore
  msbuild vcc.sln /m /p:Configuration=%VCC_CONFIG% /p:Platform=x86
  if errorlevel 0 (
    echo Find Vcc.exe    in %cd%\build\Win32\%VCC_CONFIG%\bin\vcc.exe
    echo Find [name].dll in %cd%\build\Win32\%VCC_CONFIG%\bin\cartridges\[name].dll
  )
) else (
  echo Build must be run from Developer Command Prompt for VS 2022
)
