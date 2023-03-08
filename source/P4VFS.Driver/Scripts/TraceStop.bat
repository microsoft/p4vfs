@ECHO OFF
SETLOCAL

CALL %~dp0\TraceEnv.bat
IF NOT "%ERRORLEVEL%"=="0" GOTO :EOF

"%TRACELOG_EXE%" -flush p4vfslog
"%TRACELOG_EXE%" -stop p4vfslog
"%TRACEFMT_EXE%" p4vfslog.etl -tmf "C:\Program Files\P4VFS\p4vfsflt.tmf" -nosummary
IF EXIST "FmtFile.txt" START FmtFile.txt
