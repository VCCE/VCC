@echo off
:: Clean Build VCC 

set VCC_CONFIG=Release
if exist "%VSINSTALLDIR%\MSBuild\Current\Bin\MSBuild.exe" (
  msbuild vcc.sln /t:Clean /p:Configuration=%VCC_CONFIG% /p:Platform=x86
  set _CL_=
  call Build.bat
  msbuild vcc.sln /t:Clean /p:Configuration=%VCC_CONFIG% /p:Platform=x86
  set _CL_=/DUSE_LOGGING
  call Build.bat
) else (
  echo Build must be run from Developer Command Prompt for VS 2022
)
