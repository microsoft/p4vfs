@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION

SET DRIVER_INF_FOLDER=%~1
IF NOT EXIST "%DRIVER_INF_FOLDER%" (
	ECHO Usage: TestUninstall.bat ^<path-to-inf-folder^>
	GOTO :EOF
)

fltmc.exe unload p4vfsflt
IF NOT "%ERRORLEVEL%"=="0" (
	ECHO Failed to unload driver
	EXIT /B 1
)

rundll32.exe SETUPAPI.dll,InstallHinfSection DefaultUninstall 128 %DRIVER_INF_FOLDER%\p4vfsflt.inf
IF NOT "%ERRORLEVEL%"=="0" (
	ECHO Failed to run driver DefaultUninstall
	EXIT /B 1
)

ECHO Successfully unloaded and uninstalled p4vfsflt
EXIT /B 0
