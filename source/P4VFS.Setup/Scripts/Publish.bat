:: -------------------------------------------------------------------------------------------
:: This script is publishes a release of the Microsoft P4VFS
:: -------------------------------------------------------------------------------------------
@ECHO OFF

SET SCRIPT_FOLDER=%~dp0
SET SCRIPT_FOLDER=%SCRIPT_FOLDER:~,-1%

SET PUBLISH_FOLDER=\\tc\studio\tools\P4VFS\Preview
SET PUBLISH_CONFIGURATION=ReleaseSign
SET PUBLISH_TARGET_SCRIPT=%SCRIPT_FOLDER%\..\..\..\intermediate\builds\P4VFS.Setup\%PUBLISH_CONFIGURATION%\Resource\P4VFS.Publish.targets

SET MSBUILD_EXE=C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\MSBuild\15.0\Bin\MSBuild.exe
SET MSBUILD_CMD="%MSBUILD_EXE%" /Target:P4VFSDeploySetup "/Property:Configuration=%PUBLISH_CONFIGURATION%" "/Property:P4VFSPublishDeployDir=%PUBLISH_FOLDER%" "%PUBLISH_TARGET_SCRIPT%"
ECHO %MSBUILD_CMD%
%MSBUILD_CMD%

IF "%1"=="" (
	pause
)
