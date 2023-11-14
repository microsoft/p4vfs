@ECHO OFF
SETLOCAL

SET SCRIPT_FILE=%~dpn0
SET SCRIPT_FOLDER=%~dp0
SET SCRIPT_FOLDER=%SCRIPT_FOLDER:~0,-1%

IF NOT "%1"=="/admin" (
	powershell.exe -Command "&{ Start-Process -FilePath '%SCRIPT_FILE%' -ArgumentList ('/admin') -Verb runas }"
	GOTO :EOF
)

PUSHD "%SCRIPT_FOLDER%\..\..\.."
SET REPO_FOLDER=%CD%
POPD

SET SERVER_FOLDER=%REPO_FOLDER%\intermediate\1666
SET P4D_EXE=%SERVER_FOLDER%\p4d.exe
SET P4_EXE=p4.exe
FOR /F "tokens=*" %%A IN ('dir /s /b "%REPO_FOLDER%\external\P4API\p4.exe"') DO (
	SET P4_EXE=%%A
)

taskkill.exe /F /IM p4d.exe > nul 2>&1
START cmd.exe /s /c ""%P4D_EXE%" -L "%SERVER_FOLDER%\p4d.log" -r "%SERVER_FOLDER%\db" -p 1666 -Id P4VFS_TEST -J off"

:P4D_WAIT
SET P4_ARGS=-p localhost:1666
"%P4_EXE%" %P4_ARGS% info > nul
IF "%ERRORLEVEL%"=="0" GOTO :P4D_READY
ECHO waiting for p4d ...
timeout /nobreak /t 2 > nul
GOTO :P4D_WAIT

:P4D_READY
ECHO p4d ready
ECHO Password1| "%P4_EXE%" %P4_ARGS% -u p4vfstest login
IF NOT "%ERRORLEVEL%"=="0" GOTO :P4D_FAILED
"%P4_EXE%" %P4_ARGS% -u p4vfstest login northamerica\gatineau
IF NOT "%ERRORLEVEL%"=="0" GOTO :P4D_FAILED
"%P4_EXE%" %P4_ARGS% -u p4vfstest login northamerica\quebec
IF NOT "%ERRORLEVEL%"=="0" GOTO :P4D_FAILED
"%P4_EXE%" %P4_ARGS% -u p4vfstest login northamerica\montreal
IF NOT "%ERRORLEVEL%"=="0" GOTO :P4D_FAILED
ECHO p4d startup success
GOTO :EOF

:P4D_FAILED
ECHO p4d startup failed
pause
GOTO :EOF
