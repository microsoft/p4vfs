@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION

:: Enable test-signing boot configuration
bcdedit.exe -set TESTSIGNING ON
IF NOT "%ERRORLEVEL%"=="0" (
	ECHO Failed to enable boot option TESTSIGNING
	EXIT /B 1
)
bcdedit.exe -set NOINTEGRITYCHECKS ON 
IF NOT "%ERRORLEVEL%"=="0" (
	ECHO Failed to enable boot option NOINTEGRITYCHECKS
	EXIT /B 1
)

:: Enable Security Audit Policy  
auditpol.exe /set /Category:System /failure:enable
IF NOT "%ERRORLEVEL%"=="0" (
	ECHO Failed to enable security audit policy
	EXIT /B 1
)

:: disable UAC remote user restrictions
reg.exe add HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\system /v LocalAccountTokenFilterPolicy /t REG_DWORD /d 1 /f
IF NOT "%ERRORLEVEL%"=="0" (
	ECHO Failed to disable UAC remote user restrictions
	EXIT /B 1
)

ECHO Successfully setup machine for test drivers. Reboot is probably required.
EXIT /B 0
