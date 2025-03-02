[![Build](https://github.com/microsoft/p4vfs/actions/workflows/p4vfs-verify.yml/badge.svg)](https://github.com/microsoft/p4vfs/actions/workflows/p4vfs-verify.yml)

# Introduction 
Microsoft Virtual File System for Perforce (**P4VFS**)

The P4VFS is a Windows service that allows you to sync files from Perforce quickly using almost no disk space. The contents of a file will be download automatically on-demand when first needed. You can seamlessly mix any use of P4VFS virtual sync with regular Perforce sync's using P4V and p4.exe.

For example, you can use the P4VFS to perform a "virtual sync" to a Perforce filespec and it will complete relatively quickly, and the files will exist on machine as usual. However, the actual size of the files on disk will be zero, and the file contents will be downloaded on-demand during the first read operation.

# Installation
You can install the latest signed release of P4VFS from here:
> [P4VFS.Setup](https://github.com/microsoft/p4vfs/releases/download/v1.28.3.0/P4VFS.Setup-1.28.3.0.exe)

The entire history of release notes is included with the installer.
> [Release Notes](https://github.com/microsoft/p4vfs/blob/main/source/P4VFS.Console/P4VFS.Notes.txt)

### Supported operating systems
* Windows 11
* Windows 10
* Windows Server 2022 
* Windows Server 2019
* Windows Server 2016
* Windows 8.1 (P4VFS 1.24.0.0 and earlier)

# Technical Overview
P4VFS is a Windows service, driver, and console application that allows us to sync files from Perforce immediately, and then actually download the file contents on-demand. It introduces the concept of a "virtual" sync, where a file revision can be sync'ed from Perforce and will exist locally on disk with a correct size, but zero disk-size, until accessed. When first opened, the Windows NTFS file system will automatically download file's contents, and the file will be read as expected.

The P4VFS is intended to work perfectly seamless with regular p4 and p4v usage. There is no need for special perforce server configuration, special workspaces, or any other client settings. You can feel free to sync files immediately using **p4.exe**, or use **p4vfs.exe** to virtual sync zero sized "offline" files with contents downloaded on-demand.

# Basic Usage
The main program that you'll use to do a virtual sync, and possibly other P4VFS operations, is **p4vfs.exe**. 

    C:\Program Files\P4VFS\p4vfs.exe

 The tool has a very similar interface to **p4.exe**. If you ever need to terminate **p4vfs.exe** while in-progress, simply use Ctrl+C or Ctrl+Break from the terminal, or just terminate the process with the Windows Task Manager. A p4vfs sync will always terminate gracefully and always leave your client consistent with Perforce. Take a look at the command help:

    p4vfs.exe help
    p4vfs.exe help sync
 
Try syncing as usual: 

    p4vfs.exe sync //depot/tools/dev/...
    p4vfs.exe sync //depot/tools/dev/...@600938
    p4vfs.exe sync //depot/tools/dev/...#head  //depot/tools/release/...@599820

The tool respects P4CONFIG file usage, as well as supports typical configuration settings (just like **p4.exe**)
 
    p4vfs.exe -c joe-pc-depot -u contonso\joe -p p4-contoso:1666 sync //depot/tools/dev/...

# Build and Test
### Build Requirments:

1. Visual Studio 2022 version 17.5.0 or later
1. Windows SDK version 10.0.26100.1742
1. Windows WDK version 10.0.26100.2454

Details for installing Visual Studio 2022, the Windows Software Development Kit (SDK), and the Windows Driver Kit (WDK) can be found here: 
> [Download the Windows Driver Kit](https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk)

### Instructions:
1. Using Visual Studio 2022, open solution file P4VFS.sln
1. Build solution configurations Release/Debug for regular user mode development including the current production signed driver binaries. Build solution configuration Release-Dev/Debug-Dev for local built, test signed, driver binaries.
1. Run P4VFS.Setup project to install local build.

### Testing:
1. Full suite of unit tests can be run locally by first opening Visual Studio as Administrator, and building the P4VFS.sln solution
1. In the P4VFS.Console project, set the debug command arguments to:

       p4vfs.exe test

1. See the [Development.md](doc/Development.md) for details.


