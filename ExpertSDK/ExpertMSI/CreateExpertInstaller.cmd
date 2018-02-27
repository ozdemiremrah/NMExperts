@echo off
for %%p in (%0) do set ROOTDIR="%%~dp%%~pp"
SETLOCAL

REM *** set default settings
set WixToolPath=
set PlatformID=
set BinaryPath=
set UPGRADECODE=
set ExpertName=
set Manufacturer=
set ExpertVersion=
set ExpertProductVersion=
set TargetNetmonVersion=any
set DotNetRequired=
set GuidGenerator=
set ExpertCommand=
set ExpertCommandLineArgs=
set ExpertEscapeFilterArgs=1
set ExpertWorkingDirectory=[INSTALLDIR]
set ExpertHelpFile=

set PRODUCTID=????????-????-????-????-????????????
set PACKAGEID=????????-????-????-????-????????????
set UpdatePath=0
set SetupSourcePath=%ROOTDIR%
set TargetFile=

REM *** internal flags
set ABORT=
set USING_SETTINGS_FILE=
set ComponentPending=
set CurrentDir=
set NumSubFolders=0
set NumFiles=0
set FolderLevel=0

:paramloop
if "%~1"=="" (
	if defined USING_SETTINGS_FILE goto :eof
	goto :main
)
if /i "%~1"=="-?" (
	type %ROOTDIR%\usage.txt | more
	goto :eof
)
if /i "%~1"=="/?" (
	type %ROOTDIR%\usage.txt | more
	goto :eof
)
if /i "%~1"=="-TargetFile" (
	set TargetFile=%~2
	shift
	shift
	goto :paramloop
)
if /i "%~1"=="-WixToolPath" (
	set WixToolPath=%~2
	shift
	shift
	goto :paramloop
)
if /i "%~1"=="-GuidGenerator" (
	set GuidGenerator=%~2
	shift
	shift
	goto :paramloop
)
if /i "%~1"=="-ExpertName" (
	set ExpertName=%~2
	shift
	shift
	goto :paramloop
)
if /i "%~1"=="-Manufacturer" (
	set Manufacturer=%~2
	shift
	shift
	goto :paramloop
)
if /i "%~1"=="-ExpertVersion" (
	set ExpertVersion=%~2
	shift
	shift
	goto :paramloop
)
if /i "%~1"=="-TargetNetmonVersion" (
	set TargetNetmonVersion=%~2
	shift
	shift
	goto :paramloop
)
if /i "%~1"=="-PlatformID" (
	set PlatformID=%~2
	shift
	shift
	goto :paramloop
)
if /i "%~1"=="-BinaryPath" (
	set BinaryPath=%~2
	shift
	shift
	goto :paramloop
)
if /i "%~1"=="-PRODUCTID" (
	set PRODUCTID=%~2
	shift
	shift
	goto :paramloop
)
if /i "%~1"=="-PACKAGEID" (
	set PACKAGEID=%~2
	shift
	shift
	goto :paramloop
)
if /i "%~1"=="-UPGRADECODE" (
	set UPGRADECODE=%~2
	shift
	shift
	goto :paramloop
)
if /i "%~1"=="-UpdatePath" (
	set UpdatePath=1
	shift
	goto :paramloop
)
if /i "%~1"=="-SetupSourcePath" (
	set SetupSourcePath=%~2
	shift
	shift
	goto :paramloop
)
if /i "%~1"=="-ExpertCommand" (
	set ExpertCommand=%~2
	shift
	shift
	goto :paramloop
)
if /i "%~1"=="-ExpertCommandLineArgs" (
	set ExpertCommandLineArgs=%~2
	shift
	shift
	goto :paramloop
)
if /i "%~1"=="-ExpertDontEscapeFilterArgs" (
	set ExpertEscapeFilterArgs=0
	shift
	goto :paramloop
)
if /i "%~1"=="-ExpertWorkingDirectory" (
	set ExpertWorkingDirectory=%~2
	shift
	shift
	goto :paramloop
)
if /i "%~1"=="-ExpertHelpFile" (
	set ExpertHelpFile=[INSTALLDIR]\%~2
	shift
	shift
	goto :paramloop
)

set USING_SETTINGS_FILE=1
type %1 | perl -p -e s#\"#@@#g" > %TEMP%\_UserSettings.txt
for /f "eol=# tokens=1,2 delims==" %%a in (%TEMP%\_UserSettings.txt) do (
	if "%%b"=="" (
		echo ERROR:  Value for '%%a' in settings file cannot be empty.
		goto :eof
	)
	call :paramloop -%%a "%%b"
)

set USING_SETTINGS_FILE=
SHIFT
goto :paramloop


:checkSettings
if "%PlatformID%"=="" (
	echo ERROR:  Please provide a value for PlatformID.
	set ABORT=1
) else (
	if /i not "%PlatformID%"=="x86" if /i not "%PlatformID%"=="amd64" if /i not "%PlatformID%"=="ia64" (
		echo ERROR:  Invalid value '%PlatformID%' for PlatformID.
		set ABORT=1
	)
)
if "%BinaryPath%"=="" (
	echo ERROR:  Please provide a value for BinaryPath.
	set ABORT=1
)
if "%UPGRADECODE%"=="" (
	echo ERROR:  Please provide a value for UPGRADECODE.
	set ABORT=1
)
if "%ExpertCommand%"=="" (
	echo ERROR:  Please provide a value for ExpertCommand.
	set ABORT=1
)
if "%ExpertName%"=="" (
	echo ERROR:  Please provide a value for ExpertName.
	set ABORT=1
)
if "%Manufacturer%"=="" (
	echo ERROR:  Please provide a value for Manufacturer.
	set ABORT=1
)
if "%ExpertVersion%"=="" (
	echo ERROR:  Please provide a value for ExpertVersion.
	set ABORT=1
)

if "%TargetFile%"=="" set TargetFile=%TEMP%\Expert.msi
for %%p in ("%TargetFile%") do (
	set FQ_TargetFile=%%~fp
	set TargetFileName=%%~np%%~xp
)
if /i not "%TargetFile%"=="%FQ_TargetFile%" set TargetFile=%TEMP%\%TargetFileName%

if defined ABORT (
	type %ROOTDIR%\usage.txt | more
	goto :eof
)

REM expand the paths to any folders provided
call :getFullyQualifiedPath "%BinaryPath%"
set BinaryPath=%FullPath%

call :getFullyQualifiedPath "%WixToolPath%"
set WixToolPath=%FullPath%

call :getFullyQualifiedPath "%GuidGenerator%"
set GuidGenerator=%FullPath%

call :getFullyQualifiedPath %SetupSourcePath%
set SetupSourcePath=%FullPath%

REM compute ExpertProductVersion
for /f "tokens=1,2 delims=." %%a in ("%ExpertVersion%") do set ExpertProductVersion=%%a.%%b

goto :eof


:main
call :checkSettings
if defined ABORT goto :eof

call :generateRegistrySettings
if defined ABORT goto :eof

call :generateFileList
if defined ABORT goto :eof

call :build
goto :eof


:build
set PATH=%PATH%;%WixToolPath%
pushd %SetupSourcePath%

@candle -nologo Expert.wxs -I%TEMP% -out %TEMP%\expert.wixobj
if %ERRORLEVEL%==0 (
	@light -nologo %TEMP%\expert.wixobj -out "%TargetFile%"
	if %ERRORLEVEL%==0 (
		echo.
		echo Successfully created "%TargetFile%"
	)
) ELSE (
	echo.
	echo ERROR:  unable to compile WIX source file
)

popd

goto :eof


:generateRegistrySettings
del /f %TEMP%\RegistrySettings.wxi 2> nul
del /f %TEMP%\RegistrySettings.wxi.tmp 2> nul
echo "<?xml version='1.0' encoding='utf-8'?>" > %TEMP%\RegistrySettings.wxi.tmp
echo "<Include>" >> %TEMP%\RegistrySettings.wxi.tmp
call :getGUID
echo "  <Component Id='ExpertRegistrySettings' Guid='{%GUID%}' Win64='$(var.Win64State)'>" >> %TEMP%\RegistrySettings.wxi.tmp

call :addRegistryKey Name "%ExpertName%"
if not "%Manufacturer%"=="" call :addRegistryKey "Manufacturer" "%Manufacturer%"
call :addRegistryKey Command "%ExpertCommand%"
if not "%ExpertCommandLineArgs%"=="" call :addRegistryKey "Command Parameters" "%ExpertCommandLineArgs%"
call :addRegistryKey "Escape Filter Arguments" "%ExpertEscapeFilterArgs%" integer
if not "%ExpertWorkingDirectory%"=="" call :addRegistryKey "Current Working Directory" "%ExpertWorkingDirectory%"
if not "%ExpertHelpFile%"=="" call :addRegistryKey HelpFile "%ExpertHelpFile%"

echo "  </Component>" >> %TEMP%\RegistrySettings.wxi.tmp
echo "</Include>" >> %TEMP%\RegistrySettings.wxi.tmp
type %TEMP%\RegistrySettings.wxi.tmp | perl -p -e s#\"##g" | perl -p -e s#@@#\"#g" > %TEMP%\RegistrySettings.wxi

goto :eof


:addRegistryKey
set value=%~2
set regType=string
if not "%3"=="" set regType=%3
if not "%value%"=="" set value=%value:"=@@%

echo "    <Registry Id='Reg%~1' Action='write' Type='%regType%'" >> %TEMP%\RegistrySettings.wxi.tmp
echo "              Root='HKLM' Key='Software\Microsoft\Netmon3\Experts\$(env.UPGRADECODE)' Name='%~1'" >> %TEMP%\RegistrySettings.wxi.tmp
echo "              Value='%value%' />" >> %TEMP%\RegistrySettings.wxi.tmp
goto :eof


:generateFileList
del /f %TEMP%\ExpertFiles.wxi 2> nul
del /f %TEMP%\ExpertFiles.wxi.tmp 2> nul
del /f %TEMP%\ExpertComponents.wxi 2> nul
del /f %TEMP%\ExpertComponents.wxi.tmp 2> nul

echo "<?xml version='1.0' encoding='utf-8'?>" > %TEMP%\ExpertFiles.wxi.tmp
echo "<Include>" >> %TEMP%\ExpertFiles.wxi.tmp

echo "<?xml version='1.0' encoding='utf-8'?>" > %TEMP%\ExpertComponents.wxi.tmp
echo "<Include>" >> %TEMP%\ExpertComponents.wxi.tmp
echo "<Feature Id='MainProgram' Title='Program' Description='The main executables. Must be installed locally.' " >> %TEMP%\ExpertComponents.wxi.tmp
echo " TypicalDefault='install'" >> %TEMP%\ExpertComponents.wxi.tmp
echo " Level='1'" >> %TEMP%\ExpertComponents.wxi.tmp
echo " Absent='disallow'" >> %TEMP%\ExpertComponents.wxi.tmp
echo " InstallDefault='local'" >> %TEMP%\ExpertComponents.wxi.tmp
echo " AllowAdvertise='no'>" >> %TEMP%\ExpertComponents.wxi.tmp

echo "  <Directory Id='INSTALLDIR' Name='NetmDir' LongName='$(env.ExpertName) $(env.ExpertProductVersion)'>" >> %TEMP%\ExpertFiles.wxi.tmp

for /r "%BinaryPath%" %%f in (*) do call :addFile "%%f" "%%~nf%%~xf"

if defined ComponentPending echo "</Component>" >> %TEMP%\ExpertFiles.wxi.tmp
for /l %%c in (1, 1, %FolderLevel%) do echo "</Directory>" >> %TEMP%\ExpertFiles.wxi.tmp

echo "  </Directory>" >> %TEMP%\ExpertFiles.wxi.tmp
echo "</Include>" >> %TEMP%\ExpertFiles.wxi.tmp

echo "  </Feature>" >> %TEMP%\ExpertComponents.wxi.tmp
echo "</Include>" >> %TEMP%\ExpertComponents.wxi.tmp

type %TEMP%\ExpertFiles.wxi.tmp | perl -p -e s#\"##g" > %TEMP%\ExpertFiles.wxi
type %TEMP%\ExpertComponents.wxi.tmp | perl -p -e s#\"##g" > %TEMP%\ExpertComponents.wxi

goto :eof


:addFile
set File=%~1
set FileName=%~2%

echo set LocalFile=%%File:%BinaryPath%\=%%> %TEMP%\miniscript.cmd
call %TEMP%\miniscript.cmd

echo set LocalDir=%%LocalFile:%FileName%=%%> %TEMP%\miniscript.cmd
call %TEMP%\miniscript.cmd

set Trailer=%LocalDir:~-1%
if "%Trailer%"=="\" set LocalDir=%LocalDir:~0,-1%

if "%CurrentDir%"=="" set CurrentDir=%LocalDir%

if not "%LocalDir%"=="%CurrentDir%" call :addDirectory

set /a NumFiles=%NumFiles% + 1
if not defined ComponentPending call :createComponent

echo "  <File Id='expertFile%NumFiles%' Name='File%NumFiles%' LongName='%FileName%' DiskId='1' Vital='yes'" >> %TEMP%\ExpertFiles.wxi.tmp
echo "        src='%File%' />" >> %TEMP%\ExpertFiles.wxi.tmp

goto :eof


:addDirectory
if defined ComponentPending (
	echo "</Component>" >> %TEMP%\ExpertFiles.wxi.tmp
	set ComponentPending=
)

call :closePendingDirectories

echo set SubDir=%%LocalDir:%CurrentDir%=%%> %TEMP%\miniscript.cmd
if "%CurrentDir%"=="" (
	set SubDir=%LocalDir%
) ELSE (
	call %TEMP%\miniscript.cmd
)
if "%SubDir:~0,1%"=="\" (
	set SubDir=%SubDir:~1%
)

for /f "delims=\" %%d in ("%SubDir%") do call :createDirectory
set CurrentDir=%LocalDir%
goto :eof


:createComponent
call :getGUID
echo "<Component Id='ExpDirFiles%NumSubFolders%' Guid='{%GUID%}'>" >> %TEMP%\ExpertFiles.wxi.tmp
echo "    <ComponentRef Id='ExpDirFiles%NumSubFolders%'/>" >> %TEMP%\ExpertComponents.wxi.tmp
set ComponentPending=1

goto :eof


:createDirectory
echo "<Directory Id='ExpDir%NumSubFolders%' Name='ExpDir%NumSubFolders%' LongName='%SubDir%'>" >> %TEMP%\ExpertFiles.wxi.tmp
set /a NumSubFolders=%NumSubFolders% + 1
goto :eof


:closePendingDirectories
set LocalFolderLevel=0
for /f "tokens=1-12 delims=\" %%a in ("%LocalDir%") do call :countFolders "%%a" "%%b" "%%c" "%%d" "%%e" "%%f" "%%g" "%%h" "%%i" "%%j" "%%k" "%%l"
set /a LevelsToClose= %FolderLevel% - %LocalFolderLevel%

for /l %%c in (0, 1, %LevelsToClose%) do echo "</Directory>" >> %TEMP%\ExpertFiles.wxi.tmp

set FolderLevel=%LocalFolderLevel%
goto :eof


:getGUID
set GUID=
if "%GuidGenerator%"=="" (
	echo Please provide a new GUID in the form ????????-????-????-????-????????????
	echo You can use this web page: http://www.guidgenerator.com/online-guid-generator.aspx
	set /p GUID=
) ELSE (
	for /f "usebackq" %%g in (`"%GuidGenerator%"`) do set GUID=%%g
)

goto :eof


:countFolders
if "%~1"=="" goto :eof
set /a LocalFolderLevel=%LocalFolderLevel% + 1
shift
goto :countFolders


:getFullyQualifiedPath
set FullPath=
for %%p in (%1) do set FullPath=%%~fp
goto :eof
