@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION

SET SCRIPT_FOLDER=%~dp0
SET SCRIPT_FOLDER=%SCRIPT_FOLDER:~,-1%
SET REPO_FOLDER=%SCRIPT_FOLDER%\..\..\..
SET DEPLOY_FOLDER=\\jessk-lt1-w11\C$\P4VFS

SET ROBOCOPY_COMMON_OPTIONS=/XD .* lib include /MT /S

robocopy.exe %REPO_FOLDER%\source %DEPLOY_FOLDER%\source /XD .* /MT /S
robocopy.exe %REPO_FOLDER%\intermediate\builds\P4VFS.Setup %DEPLOY_FOLDER%\intermediate\builds\P4VFS.Setup %ROBOCOPY_COMMON_OPTIONS%
robocopy.exe %REPO_FOLDER%\intermediate\builds\P4VFS.Driver %DEPLOY_FOLDER%\intermediate\builds\P4VFS.Driver %ROBOCOPY_COMMON_OPTIONS%
robocopy.exe %REPO_FOLDER%\intermediate\builds\P4VFS.Console %DEPLOY_FOLDER%\intermediate\builds\P4VFS.Console %ROBOCOPY_COMMON_OPTIONS%
robocopy.exe %REPO_FOLDER%\external\P4API %DEPLOY_FOLDER%\external\P4API %ROBOCOPY_COMMON_OPTIONS%
