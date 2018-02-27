@echo off
REM for %%p in (%0) do set ROOTDIR=%%~dp%%~pp

REM *** set default settings

REM *** Create MSI on x86 or x64 Release Builds
if "%~1"=="Release" (
  if NOT "%~2"=="Any CPU" (
    mkdir msiinput
    xcopy "%~4*.*" msiinput\ /F /EXCLUDE:exlist.txt
    xcopy "%~4Expert.ico" . /Y
    if "%2"=="x64" (
      echo x64
      call CreateExpertInstaller.cmd -PlatformID amd64 -BinaryPath "msiinput\" -ExpertCommand "%~5" -TargetFile "%~6" "%~3"
    ) else (
      echo x86
      call CreateExpertInstaller.cmd -PlatformID x86 -BinaryPath "msiinput\" -ExpertCommand "%~5" -TargetFile "%~6" "%~3"
    )
    rmdir /S /Q msiinput
  )
)
