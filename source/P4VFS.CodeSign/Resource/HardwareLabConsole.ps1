param(
    [Parameter(Mandatory=$true)][String] $Command,
    [Parameter(ValueFromRemainingArguments=$true)][String[]] $CommandArgs
)

function P4vfsGetCommandArg
{
    param(
        [Parameter(Mandatory=$true)][String] $Name, 
        [Parameter(Mandatory=$false)][String] $DefaultValue
    )
    for ($CommandArgsIndex = 0; $CommandArgsIndex -lt $CommandArgs.Length; ++$CommandArgsIndex)
    {
        if ($CommandArgs[$CommandArgsIndex].ToLower().TrimStart('-') -eq $Name -and $CommandArgsIndex+1 -lt $CommandArgs.Length)
        {
            return $CommandArgs[++$CommandArgsIndex]
        }
    }
    return $DefaultValue
}

function P4vfsGetWinSdkToolsFolder
{
    $WinSdkRoot = [System.Environment]::ExpandEnvironmentVariables("%ProgramFiles(x86)%\\Windows Kits\\10\\bin")
    $WinSdkToolsFolder = $null
    if ([System.IO.Directory]::Exists($WinSdkRoot))
    {
        foreach ($VersionFolder in [System.IO.Directory]::GetDirectories($WinSdkRoot, "10.*"))
        {
            if ([System.IO.File]::Exists("${VersionFolder}\\x64\\makecert.exe"))
            {
                $WinSdkToolsFolder = "${VersionFolder}\\x64"
            }
        }
    }
    if ([String]::IsNullOrEmpty($WinSdkToolsFolder))
    {
        throw "Could not find Windows SDK: ${WinSdkRoot}"
    }
    return $WinSdkToolsFolder
}

function P4vfsSetupClientAgent()
{
    Write-Host "P4vfsSetupClientAgent"
    
    $DriverFolder = P4vfsGetCommandArg -Name "DriverFolder" -DefaultValue $PSScriptRoot
    if ([System.IO.Directory]::Exists($DriverFolder) -eq $false)
    {
        throw "Missing or invalid DriverFolder parameter: ${DriverFolder}"
    }

    # Find the Windows SDK tools folder
    $WinSdkToolsFolder = P4vfsGetWinSdkToolsFolder
    Write-Host "Using WinSdkToolsFolder ${WinSdkToolsFolder}"

    # Import test sign certificate to local store
    Write-Host "Importing certificate ""${DriverFolder}\\p4vfsflt.cer"" to LocalMachine ROOT"
    $P = Start-Process -FilePath "${WinSdkToolsFolder}\certmgr.exe" -ArgumentList ("-add","${DriverFolder}\\p4vfsflt.cer","-all","-s","-r","LocalMachine","ROOT") -PassThru -Wait -NoNewWindow
    if ($P.ExitCode -ne 0)
    {
        throw "Failed to add test certificate to LocalMachine Root"
    }

    Write-Host "Importing certificate ""${DriverFolder}\\p4vfsflt.cer"" to LocalMachine TRUSTEDPUBLISHER"
    $P = Start-Process -FilePath "${WinSdkToolsFolder}\certmgr.exe" -ArgumentList ("-add","${DriverFolder}\\p4vfsflt.cer","-all","-s","-r","LocalMachine","TRUSTEDPUBLISHER") -PassThru -Wait -NoNewWindow
    if ($P.ExitCode -ne 0)
    {
        throw "Failed to add test certificate to LocalMachine Trusted Publisher"
    }

    # Stop and Uninstall the driver if needed
    Write-Host "Checking p4vfsflt driver status"
    if (& sc.exe query p4vfsflt | Where-Object {$_ -match "SERVICE_NAME"})
    {
        Write-Host "Uninstalling previous p4vfsflt driver"
        Start-Process -FilePath "fltmc.exe" -ArgumentList ("unload","p4vfsflt") -PassThru -Wait -NoNewWindow
        Start-Process -FilePath "rundll32.exe" -ArgumentList ("SETUPAPI.dll,InstallHinfSection","DefaultUninstall","128","${DriverFolder}\\p4vfsflt.inf") -PassThru -Wait -NoNewWindow
    }
    
    # Use SetupAPI to install the driver
    Write-Host "Installing driver ""${DriverFolder}\\p4vfsflt.inf"""
    $P = Start-Process -FilePath "rundll32.exe" -ArgumentList ("SETUPAPI.dll,InstallHinfSection","DefaultInstall","128","${DriverFolder}\\p4vfsflt.inf") -PassThru -Wait -NoNewWindow
    if ($P.ExitCode -ne 0)
    {
        throw "Failed to run driver DefaultInstall"
    }

    # Have filter manager load the driver
    Write-Host "Loading driver p4vfsflt"
    $P = Start-Process -FilePath "fltmc.exe" -ArgumentList ("load","p4vfsflt") -PassThru -Wait -NoNewWindow
    if ($P.ExitCode -ne 0)
    {
        throw "Failed to load driver"
    }

    # Set the filter driver to automatically load at system startup, instead of on-demand. This is a special case for HLK testing without P4VFS.Service installed
    Write-Host "Configuring p4vfsflt driver to load at system startup"
    $P = Start-Process -FilePath "sc.exe" -ArgumentList ("config","p4vfsflt","start=system") -PassThru -Wait -NoNewWindow
    if ($P.ExitCode -ne 0)
    {
        throw "Failed to configure driver to load at system startup"
    }
}

function P4vfsCreateControllerPackage
{
    Write-Host "P4vfsCreateControllerPackage"
}

switch ($Command)
{
    "SetupClientAgent"
    {
        P4vfsSetupClientAgent
        break
    }
    "CreateControllerPackage"
    {
        P4vfsCreateControllerPackage
        break
    }
    default
    {
        throw "Unsupported command ${Command}"
    }
}

return 0

$ObjectModel  = [Reflection.Assembly]::LoadFrom($env:WTTSTDIO + "microsoft.windows.Kits.Hardware.objectmodel.dll")
$DbConnection = [Reflection.Assembly]::LoadFrom($env:WTTSTDIO + "microsoft.windows.Kits.Hardware.objectmodel.dbconnection.dll")
$Submission   = [Reflection.Assembly]::LoadFrom($env:WTTSTDIO + "microsoft.windows.Kits.Hardware.objectmodel.submission.dll")
$Submission   = [Reflection.Assembly]::LoadFrom($env:WTTSTDIO + "microsoft.windows.Kits.Hardware.objectmodel.submission.package.dll")

$ControllerName = $args[0]
$projectName = $args[1];
$driverPath = $args[2];
$saveFileName = $args[3];
    
# we need to connect to the Server

if ($ControllerName -eq $null -OR $ControllerName -eq "")
{
    write-host "Need to supply the controller Name as a parameter to this script"
    return
}

if ($projectName -eq $null -OR $projectName -eq "")
{
    write-host "Need to supply the Project Name as a parameter to this script"
    return
}
