@echo off
:: Build VCC 

set VCC_CONFIG=Release
if exist "%VSINSTALLDIR%\MSBuild\Current\Bin\MSBuild.exe" (
  msbuild vcc.sln /m /p:Configuration=%VCC_CONFIG% /p:Platform=x86
  if errorlevel 0 (
    echo Find VCC binaries in %cd%\__bin\Win32\%VCC_CONFIG%
  )
) else (
  echo Build must be run from Developer Command Prompt for VS 2022
)
