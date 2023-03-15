@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION

SET DRIVER_INF_FOLDER=%~1
IF NOT EXIST "%DRIVER_INF_FOLDER%" (
	ECHO Usage: TestInstall.bat ^<path-to-inf-folder^>
	GOTO :EOF
)

:: Find the Windows SDK tools folder
SET WIN_SDK_ROOT_DIR=%ProgramFiles(x86)%\Windows Kits\10\bin
SET WIN_SDK_X64_DIR=
FOR /F "usebackq" %%A IN (`dir /B /O-N "%WIN_SDK_ROOT_DIR%\10.*"`) DO (
	IF EXIST "!WIN_SDK_ROOT_DIR!\%%A\x64\makecert.exe" (
		SET WIN_SDK_X64_DIR=!WIN_SDK_ROOT_DIR!\%%A\x64
		ECHO Using WIN_SDK: !WIN_SDK_X64_DIR!
		GOTO WIN_SDK_X64_DIR_EXISTS
	)
)
ECHO Could not find Windows SDK: "%WIN_SDK_ROOT_DIR%"
EXIT /B 1
:WIN_SDK_X64_DIR_EXISTS

:: Import test sign certificate to local store
"%WIN_SDK_X64_DIR%\certmgr.exe" -add "%DRIVER_INF_FOLDER%\P4VFS.Driver.WDKTest.pfx" -all -s -r LocalMachine ROOT
IF NOT "%ERRORLEVEL%"=="0" (
	ECHO Failed to add test certificate to LocalMachine Root
	EXIT /B 1
)
"%WIN_SDK_X64_DIR%\certmgr.exe" -add "%DRIVER_INF_FOLDER%\P4VFS.Driver.WDKTest.pfx" -all -s -r LocalMachine TRUSTEDPUBLISHER
IF NOT "%ERRORLEVEL%"=="0" (
	ECHO Failed to add test certificate to LocalMachine Trusted Publisher
	EXIT /B 1
)

:: Use SetupAPI to install the driver
rundll32.exe SETUPAPI.dll,InstallHinfSection DefaultInstall 128 %DRIVER_INF_FOLDER%\p4vfsflt.inf
IF NOT "%ERRORLEVEL%"=="0" (
	ECHO Failed to run driver DefaultInstall
	EXIT /B 1
)

:: Have filter manager load the driver
fltmc.exe load p4vfsflt
IF NOT "%ERRORLEVEL%"=="0" (
	ECHO Failed to load driver
	EXIT /B 1
)

ECHO Successfully installed and loaded p4vfsflt
EXIT /B 0
