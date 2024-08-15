@echo off
:: Build VCC 

set VCC_CONFIG=Release
if exist "%VSINSTALLDIR%\MSBuild\Current\Bin\MSBuild.exe" (
  msbuild vcc.sln /m /p:Configuration=%VCC_CONFIG% /p:Platform=x86
  if errorlevel 0 (
    echo Find Vcc.exe in %cd%\__obj\Win32\%VCC_CONFIG%\vcc\out\vcc.exe
    echo Find [name].dll in %cd%\__obj\Win32\%VCC_CONFIG%\[name]\out\[name].dll
  )
) else (
  echo Build must be run from Developer Command Prompt for VS 2022
)
