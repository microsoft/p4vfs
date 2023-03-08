@ECHO OFF
SETLOCAL

CALL %~dp0\TraceEnv.bat
IF NOT "%ERRORLEVEL%"=="0" GOTO :EOF

"%TRACELOG_EXE%" -start p4vfslog -guid #082E7434-2FF0-4DE7-8470-1BBBD2E48237 -level 0xFF -rt -kd -flag 0x7FFFFFFF -f p4vfslog.etl
