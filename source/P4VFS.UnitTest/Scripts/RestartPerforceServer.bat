@ECHO OFF
SETLOCAL

SET SCRIPT_FILE=%~dpn0
SET SCRIPT_FOLDER=%~dp0
SET SCRIPT_FOLDER=%SCRIPT_FOLDER:~0,-1%

IF NOT "%1"=="/admin" (
	powershell.exe -Command "&{ Start-Process -FilePath '%SCRIPT_FILE%' -ArgumentList ('/admin') -Verb runas }"
	GOTO :EOF
)

PUSHD "%SCRIPT_FOLDER%\..\..\..\intermediate\1666"
SET SERVER_FOLDER=%CD%
POPD

taskkill.exe /F /IM p4d.exe > nul 2>&1
START cmd.exe /c ""%SERVER_FOLDER%\p4d.exe" -L "%SERVER_FOLDER%\p4d.log" -r "%SERVER_FOLDER%\db" -p 1666 -Id P4VFS_TEST -J off"

:P4D_WAIT
SET P4_ARGS=-p localhost:1666
p4.exe %P4_ARGS% info > nul
IF "%ERRORLEVEL%"=="0" GOTO :P4D_READY
ECHO waiting for p4d ...
sleep 2
GOTO :P4D_WAIT

:P4D_READY
ECHO p4d ready
ECHO Password1| p4.exe %P4_ARGS% -u p4vfstest login
IF NOT "%ERRORLEVEL%"=="0" GOTO :P4D_FAILED
p4.exe %P4_ARGS% -u p4vfstest login northamerica\gatineau
IF NOT "%ERRORLEVEL%"=="0" GOTO :P4D_FAILED
p4.exe %P4_ARGS% -u p4vfstest login northamerica\quebec
IF NOT "%ERRORLEVEL%"=="0" GOTO :P4D_FAILED
p4.exe %P4_ARGS% -u p4vfstest login northamerica\montreal
IF NOT "%ERRORLEVEL%"=="0" GOTO :P4D_FAILED
ECHO p4d startup success
GOTO :EOF

:P4D_FAILED
ECHO p4d startup failed
pause
GOTO :EOF
