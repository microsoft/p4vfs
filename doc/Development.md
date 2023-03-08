# Microsoft P4VFS Development 

Test driven development is an integral part of creating and debugging P4VFS. The P4VFS.UnitTest project, and `p4vfs.exe test` command are essential for ensuring unexpected side-effects and bugs are not introduced. 

This document is intended to describe a procedure and best practices for common P4VFS development goals. 

---

## How to build and run P4VFS
- See the [README.md](../README.md) for basic instructions

---

## How to find the logs for P4VFS
- The P4VFS writes logs multiple targets:

  - Writes to STDOUT/STDERR if applicable

  - Writes to attached debugger trace output

  - Writes to setting file location `FileLoggerLocalDirectory`. By default this is "%PUBLIC%\\Public Logs\\P4VFS". This file location must be accessible by the local system accounts and typically all machine users.

  - Writes to setting file location `FileLoggerRemoteDirectory`. This default this is unset. This file location will be accessed by impersonating the current operation's user permissions (usually the current Windows session user). You could typically override this to be a network share.

---

## How to debug a p4vfs sync 
- Build the solution "Debug|x64" or "Release|x64"

- By default, the `p4vfs sync` operation is executed in the running `P4VFS.Service.exe`. Attach the Visual Studio debugger to `P4VFS.Service.exe` with "Native" engine and break in `DepotOperations::Sync`

- Using `p4vfs sync -t` you can force the sync to be performed in p4vfs.exe process instead of through the service. This will allow you set `P4VFS.Console` project as the Startup Project in Visual Studio, and then F5 to stepwise debug the entire operation executed within `p4vfs.exe`

---

## How to run a specific P4VFS test

- Each test has function, both Managed in P4VFS.UnitTest or Native in P4VFS.Core have a Priority number. 

- Tests can be run from `p4vfs test` by specifying a test name, Priority number or range of Priority numbers.

- In `P4VFS.UnitTest` the Priority number for a TestMethod is derived by the method's PriorityAttribute plus class BasePriorityAttribute. Two most commonly used tests are:

  ```
    Test 0:     UnitTestInstall.StagingUninstallTest   // BasePriority(0) + Priority(0)
    Test 1:     UnitTestInstall.StagingInstallTest     // BasePriority(0) + Priority(1)
    Test 1020:  UnitTestCommon.DepotServerConfigTest   // BasePriority(1000) + Priority(20)
  ```

  You could run all three tests as:
  > p4vfs test 0 1 1020

  You could run just the uninstall=0, install=1, and all basic tests from UnitTestCommon as:
  > p4vfs test 0 1 1000:1999

  You could run just the test to start up the local perforce server `UnitTestServer.StartupLocalPerforceServerTest`
  > p4vfs test 500

- To run all tests starting zero to all b

---

## How to make a code change and debug P4VFS
- Code changes to common libraries (such as P4VFS.Core) will need to be reinstalled with the service. Once the new code is running in the `P4VFS.Service`, you can attach the debugger.

- Build the solution "Debug|x64" or "Release|x64"

- Run this `p4vfs test` command to uninstall and install the new binaries.
  > p4vfs test 0 1
  
- Attach Native + Managed (.NET 4.x) debugger to `P4VFS.Service`

