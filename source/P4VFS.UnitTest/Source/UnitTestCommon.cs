// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Diagnostics;
using System.Linq;
using System.IO;
using System.Xml;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.P4VFS.Extensions;
using Microsoft.P4VFS.Extensions.Utilities;
using Microsoft.P4VFS.CoreInterop;
using Newtonsoft.Json;

namespace Microsoft.P4VFS.UnitTest
{
	[TestClass, BasePriority(1000)]
	public class UnitTestCommon : UnitTestBase
	{
		[TestMethod, Priority(0), TestRemote]
		public void BasicWorkspaceTest()
		{
			foreach (ServiceSettingsScope settings in EnumerateCommonServiceSyncSettings())
			{
				using (settings) {
				using (DepotClient depotClient = new DepotClient()) {
				DepotSyncFlags syncFlags = settings.SyncFlags.Value;

				WorkspaceReset();
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));
				string clientRoot = GetClientRoot(depotClient);

				DepotSyncResult syncResult = depotClient.Sync("//depot/gears1/Development/Src/Core/...", null, syncFlags, DepotSyncMethod.Virtual);
				Assert(syncResult?.Status == DepotSyncStatus.Success);
				Assert(syncFlags.HasFlag(DepotSyncFlags.IgnoreOutput) ? syncResult.Modifications == null : syncResult.Modifications.Count() > 0);
				long rootLocalSize = FileUtilities.GetDirectorySize(clientRoot);
				Assert(rootLocalSize >= 0);
				long rootDepotSize = depotClient.GetDepotSize(new[]{ "//depot/gears1/Development/Src/Core/..." });
				Assert(rootDepotSize >= 0);
				Assert(rootDepotSize == rootLocalSize);
				long rootFileCount = FileUtilities.GetDirectoryFileCount(clientRoot);
				Assert(rootFileCount == 136);

				string coreFile = String.Format(@"{0}\depot\gears1\Development\Src\Core\Src\Core.cpp", clientRoot);
				Assert(IsPlaceholderFile(coreFile));
				Assert(IsPlaceholderFile(coreFile));
				string[] coreFileLines = File.ReadAllLines(coreFile);
				Assert(coreFileLines.Length == 280);
				Assert(coreFileLines.First() == "9.\tEXPORT RESTRICTIONS. The software is subject to United States export laws and regulations. ");
				Assert(coreFileLines.Last() == "syst");
				Assert(IsPlaceholderFile(coreFile) == false);

				string canvasFile = String.Format(@"{0}\depot\gears1\Development\Src\Engine\Src\UnCanvas.cpp", clientRoot);
				syncResult = depotClient.Sync(canvasFile, null, syncFlags, DepotSyncMethod.Virtual);
				Assert(syncResult?.Status == DepotSyncStatus.Success);
				Assert(syncFlags.HasFlag(DepotSyncFlags.IgnoreOutput) ? syncResult.Modifications == null : syncResult.Modifications.Count() > 0);
				Assert(IsPlaceholderFile(canvasFile));

				Assert(depotClient.Run("edit", new[]{ canvasFile }).HasError == false);
				Assert(IsPlaceholderFile(canvasFile));
				Assert(File.ReadAllLines(canvasFile).Length == 521);
				Assert(IsPlaceholderFile(canvasFile) == false);

				Assert(depotClient.Run("revert", new[]{ canvasFile }).HasError == false);
				syncResult = depotClient.Sync(canvasFile, new DepotRevisionNumber(1), syncFlags, DepotSyncMethod.Virtual);
				Assert(syncResult?.Status == DepotSyncStatus.Success);
				Assert(syncFlags.HasFlag(DepotSyncFlags.IgnoreOutput) ? syncResult.Modifications == null : syncResult.Modifications.Count() > 0);
				Assert(IsPlaceholderFile(canvasFile));
				Assert(File.ReadAllLines(canvasFile).Length == 515);
				Assert(IsPlaceholderFile(canvasFile) == false);

				syncResult = depotClient.Sync(canvasFile + "#2", null, syncFlags, DepotSyncMethod.Virtual);
				Assert(syncResult?.Status == DepotSyncStatus.Success);
				Assert(syncFlags.HasFlag(DepotSyncFlags.IgnoreOutput) ? syncResult.Modifications == null : syncResult.Modifications.Count() > 0);
				Assert(IsPlaceholderFile(canvasFile));
				Assert(File.ReadAllLines(canvasFile).Length == 521);
				Assert(IsPlaceholderFile(canvasFile) == false);

				syncResult = depotClient.Sync(canvasFile, new DepotRevisionNone(), syncFlags, DepotSyncMethod.Virtual);
				Assert(syncResult?.Status == DepotSyncStatus.Success);
				Assert(syncFlags.HasFlag(DepotSyncFlags.IgnoreOutput) ? syncResult.Modifications == null : syncResult.Modifications.Count() > 0);
				Assert(Directory.Exists(String.Format(@"{0}\depot\gears1\Development\Src\Engine", clientRoot)) == false);
				Assert(Directory.Exists(String.Format(@"{0}\depot\gears1\Development\Src", clientRoot)) == true);
			}}}
		}

		[TestMethod, Priority(1), TestRemote]
		public void MakeFileResidentTest()
		{
			foreach (ServiceSettingsScope settings in EnumerateCommonServiceSyncSettings())
			{
				using (settings) {
				using (DepotClient depotClient = new DepotClient()) {
				DepotSyncFlags syncFlags = settings.SyncFlags.Value;

				WorkspaceReset();
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));

				string clientRoot = GetClientRoot(depotClient);
				string fileLocalPath = String.Format(@"{0}\depot\gears1\Development\Src\Core\Src\Core.cpp", clientRoot);

				DepotSyncResult syncResult = depotClient.Sync(fileLocalPath, new DepotRevisionHead(), syncFlags, DepotSyncMethod.Regular);
				Assert(syncResult?.Status == DepotSyncStatus.Success);
				Assert(syncFlags.HasFlag(DepotSyncFlags.IgnoreOutput) ? syncResult.Modifications == null : syncResult.Modifications.Count() == 1);
				Assert(File.Exists(fileLocalPath), "File Path should not exist.");
				Assert(IsPlaceholderFile(fileLocalPath) == false, "File Path should not be placeholder.");

				// Delete the file that we're interested in
				syncResult = depotClient.Sync(fileLocalPath, new DepotRevisionNone(), syncFlags, DepotSyncMethod.Virtual);
				Assert(syncResult?.Status == DepotSyncStatus.Success);
				Assert(syncFlags.HasFlag(DepotSyncFlags.IgnoreOutput) ? syncResult.Modifications == null : syncResult.Modifications.Count() == 1);
				Assert(File.Exists(fileLocalPath) == false, "File Path should not exist.");

				// Now virtual sync to the head revision
				syncResult = depotClient.Sync(fileLocalPath, new DepotRevisionHead(), syncFlags, DepotSyncMethod.Virtual);
				Assert(syncResult?.Status == DepotSyncStatus.Success);
				Assert(syncFlags.HasFlag(DepotSyncFlags.IgnoreOutput) ? syncResult.Modifications == null : syncResult.Modifications.Count() == 1);
				Assert(IsPlaceholderFile(fileLocalPath), "File Path should exist.");

				// Now make the file resident and check that the file has changed
				long fileLengthBefore = FileUtilities.GetFileLength(fileLocalPath);
				DateTime fileCreationTimeBefore = File.GetCreationTime(fileLocalPath);
				DateTime fileWriteTimeBefore = File.GetLastWriteTime(fileLocalPath);
				Assert(fileLengthBefore == 17303);

				using (File.OpenRead(fileLocalPath)) {}
				Assert(File.Exists(fileLocalPath), "File Path should exist.");
				Assert(IsPlaceholderFile(fileLocalPath) == false);
				Assert(NativeMethods.GetFileDiskSize(fileLocalPath) >= fileLengthBefore);

				long fileLengthAfter = FileUtilities.GetFileLength(fileLocalPath);
				DateTime fileCreationTimeAfter = File.GetCreationTime(fileLocalPath);
				DateTime fileWriteTimeAfter = File.GetLastWriteTime(fileLocalPath);

				Assert(fileLengthAfter >= 17582, "Now that it's resident, the file should be a correct size.");
				Assert(fileCreationTimeBefore == fileCreationTimeAfter, "File create times should not be different.");
				Assert(fileWriteTimeBefore == fileWriteTimeAfter, "File write times should not be different.");
				Assert(NativeMethods.GetFileDiskSize(fileLocalPath) >= fileLengthAfter, "Disk file size is unexpected");
				Assert(ReconcilePreview(Path.GetDirectoryName(fileLocalPath)).Any() == false);
			}}}
		}

		[TestMethod, Priority(2), TestRemote]
		public void MakeDirectoryResidentTest()
		{
			foreach (ServiceSettingsScope settings in EnumerateCommonServiceSyncSettings())
			{
				using (settings) {
				using (DepotClient depotClient = new DepotClient()) {
				DepotSyncFlags syncFlags = settings.SyncFlags.Value;

				WorkspaceReset();
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));

				string clientRoot = GetClientRoot(depotClient);
				string clientFolder = String.Format(@"{0}\depot\gears1\Binaries\Xenon", clientRoot);

				// Now sync to the head revision
				DepotSyncResult syncResult = depotClient.Sync(String.Format("{0}\\...", clientFolder), new DepotRevisionHead(), syncFlags, DepotSyncMethod.Virtual);
				Assert(syncResult?.Status == DepotSyncStatus.Success);
				Assert(syncFlags.HasFlag(DepotSyncFlags.IgnoreOutput) ? syncResult.Modifications == null : syncResult.Modifications.Count() > 0);
				Assert(Directory.Exists(clientFolder), "Directory Path should exist.");
					
				foreach (string file in Directory.GetFiles(clientFolder, "*", SearchOption.AllDirectories).ToArray())
				{
					// Now make the file resident and check that the file has changed
					DateTime fileCreationTimeBefore = File.GetCreationTime(file);
					DateTime fileWriteTimeBefore = File.GetLastWriteTime(file);

					VirtualFileSystemLog.Info("Downloading: {0}", file);
					using (File.OpenRead(file)) {}	
					Assert(File.Exists(file), "File Path should exist.");

					DateTime fileCreationTimeAfter = File.GetCreationTime(file);
					DateTime fileWriteTimeAfter = File.GetLastWriteTime(file);

					Assert(fileCreationTimeBefore == fileCreationTimeAfter, "File create times should not be different.");
					Assert(fileWriteTimeBefore == fileWriteTimeAfter, "File write times should not be different.");
				}

				// One last reconcile to ensure contents are consistent
				Assert(ReconcilePreview(clientFolder).Any() == false);
			}}}
		}

		[TestMethod, Priority(3), TestRemote]
		public void WritableClientAndFileTest()
		{
			foreach (ServiceSettingsScope settings in EnumerateCommonServiceSyncLineEndSettings())
			{
				using (settings) {
				using (DepotClient depotClient = new DepotClient()) {
				DepotSyncFlags syncFlags = settings.SyncFlags.Value;
				
				WorkspaceReset();
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));
				string clientRoot = GetClientRoot(depotClient);

				DepotResultClient.Node client = depotClient.Client();
				client.Fields["LineEnd"] = settings.LineEnd;
				Assert(client.LineEnd == settings.LineEnd);
				Assert(depotClient.UpdateClient(client).HasError == false);
				client = depotClient.Client();
				Assert(client.LineEnd == settings.LineEnd);

				string writableFile = String.Format(@"{0}\depot\tools\dev\bin\studio\maya\scripts\python\Utilities\shaderTools.py", clientRoot);
				string rootFolder = Path.GetDirectoryName(writableFile);
				{
					string folderSpec = String.Format(@"{0}\...@5", rootFolder);
					DepotSyncResult syncResult = depotClient.Sync(folderSpec, null, syncFlags, DepotSyncMethod.Virtual);
					Assert(syncResult?.Status == DepotSyncStatus.Success);
					Assert(syncFlags.HasFlag(DepotSyncFlags.IgnoreOutput) ? syncResult.Modifications == null : syncResult.Modifications.Count() == 10);
					Assert(FileUtilities.GetDirectorySize(rootFolder) > 0);
					Assert(FileUtilities.GetDirectoryFileCount(clientRoot) == 10);
					Assert(File.Exists(writableFile));
					Assert(FileUtilities.IsReadOnly(writableFile) == true);
					DepotResultFStat writableFileStat = depotClient.FStat(new[]{ writableFile });
					Assert(writableFileStat.Count == 1 && writableFileStat[0].HaveRev == 1);
					Assert(ReconcilePreview(rootFolder).Any() == false);
					Assert(FileUtilities.GetDirectorySize(rootFolder) >= depotClient.GetDepotSize(new[]{ folderSpec }));
				}
				{
					string folderSpec = String.Format(@"{0}\...@6", rootFolder);
					DepotSyncResult syncResult = depotClient.Sync(folderSpec, null, syncFlags, DepotSyncMethod.Virtual);
					Assert(syncResult?.Status == DepotSyncStatus.Success);
					Assert(syncFlags.HasFlag(DepotSyncFlags.IgnoreOutput) ? syncResult.Modifications == null : syncResult.Modifications.Count() > 0);
					Assert(FileUtilities.GetDirectorySize(rootFolder) > 0);
					Assert(File.Exists(writableFile));
					Assert(FileUtilities.IsReadOnly(writableFile) == false);
					DepotResultFStat writableFileStat = depotClient.FStat(new[]{ writableFile });
					Assert(writableFileStat.Count == 1 && writableFileStat[0].HaveRev == 2);
					Assert(ReconcilePreview(rootFolder).Any() == false);
					Assert(FileUtilities.GetDirectorySize(rootFolder) >= depotClient.GetDepotSize(new[]{ folderSpec }));
				}
				{
					string folderSpec = String.Format(@"{0}\...@7", rootFolder);
					DepotSyncResult syncResult = depotClient.Sync(folderSpec, null, syncFlags, DepotSyncMethod.Virtual);
					Assert(syncResult?.Status == DepotSyncStatus.Success);
					Assert(syncFlags.HasFlag(DepotSyncFlags.IgnoreOutput) ? syncResult.Modifications == null : syncResult.Modifications.Count() > 0);
					Assert(FileUtilities.GetDirectorySize(rootFolder) > 0);
					Assert(File.Exists(writableFile));
					Assert(FileUtilities.IsReadOnly(writableFile) == false);
					DepotResultFStat writableFileStat = depotClient.FStat(new[]{ writableFile });
					Assert(writableFileStat.Count == 1 && writableFileStat[0].HaveRev == 3);
					Assert(ReconcilePreview(rootFolder).Any() == false);
					Assert(FileUtilities.GetDirectorySize(rootFolder) >= depotClient.GetDepotSize(new[]{ folderSpec }));
				}
				{
					string folderSpec = String.Format(@"{0}\...@5", rootFolder);
					DepotSyncResult syncResult = depotClient.Sync(folderSpec, null, syncFlags, DepotSyncMethod.Virtual);
					Assert(syncResult?.Status == DepotSyncStatus.Success);
					Assert(syncFlags.HasFlag(DepotSyncFlags.IgnoreOutput) ? syncResult.Modifications == null : syncResult.Modifications.Count() > 0);
					Assert(FileUtilities.GetDirectorySize(rootFolder) > 0);
					Assert(File.Exists(writableFile));
					Assert(FileUtilities.IsReadOnly(writableFile) == true);
					DepotResultFStat writableFileStat = depotClient.FStat(new[]{ writableFile });
					Assert(writableFileStat.Count == 1 && writableFileStat[0].HaveRev == 1);
					Assert(ReconcilePreview(rootFolder).Any() == false);
					Assert(FileUtilities.GetDirectorySize(rootFolder) >= depotClient.GetDepotSize(new[]{ folderSpec }));
				}
			}}}
		}

		[TestMethod, Priority(4), TestRemote]
		public void OpenedFileSyncTest()
		{
			foreach (ServiceSettingsScope settings in EnumerateCommonServiceSyncSettings())
			{
				using (settings) {
				using (DepotClient depotClient = new DepotClient()) {
				DepotSyncFlags syncFlags = settings.SyncFlags.Value;
				
				WorkspaceReset();
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));

				string clientRoot = GetClientRoot(depotClient);
				string writableFile = String.Format(@"{0}\depot\tools\dev\bin\studio\maya\scripts\python\Utilities\shaderTools.py", clientRoot);
				{
					string folderSpec = String.Format("{0}@8", writableFile);
					depotClient.Sync(folderSpec, null, syncFlags, DepotSyncMethod.Virtual);
					Assert(IsPlaceholderFile(writableFile));
					DepotResultFStat writableFileStat = depotClient.FStat(new[]{ writableFile });
					Assert(writableFileStat.Count == 1 && writableFileStat[0].HaveRev == 4);
					Assert(DiffAgainstWorkspace(String.Format("{0}#4", writableFile)).LinesTotal == 0);
					Assert(DiffAgainstWorkspace(String.Format("{0}#3", writableFile)).LinesTotal != 0);
				}

				Assert(depotClient.Run("edit", new[]{ writableFile }).HasError == false);

				{
					string folderSpec = String.Format("{0}@6", writableFile);
					depotClient.Sync(folderSpec, null, syncFlags, DepotSyncMethod.Virtual);
					DepotResultFStat writableFileStat = depotClient.FStat(new[]{ writableFile });
					Assert(writableFileStat.Count == 1 && writableFileStat[0].HaveRev == 2);
					Assert(DiffAgainstWorkspace(String.Format("{0}#4", writableFile)).LinesTotal == 0);
				}
				{
					string folderSpec = String.Format("{0}@9", writableFile);
					depotClient.Sync(folderSpec, null, syncFlags, DepotSyncMethod.Virtual);
					DepotResultFStat writableFileStat = depotClient.FStat(new[]{ writableFile });
					Assert(writableFileStat.Count == 1 && writableFileStat[0].HaveRev == 5);
					Assert(DiffAgainstWorkspace(String.Format("{0}#4", writableFile)).LinesTotal == 0);
				}

				Assert(depotClient.Run("revert", new[]{ writableFile }).HasError == false);
			}}} 
		}

		[TestMethod, Priority(5), TestRemote]
		public void VirtualFileSystemLogTest()
		{
			bool lastImmediateLogging = SettingManager.ImmediateLogging;
			SettingManager.ImmediateLogging = false;
			VirtualFileSystemLog.Flush();
			VirtualFileSystemLog.Suspend();

			int total = 0;
			for (int i = 0; i < 4; ++i)
				VirtualFileSystemLog.Info("LogTest Info Async {0}", total++);

			Assert(VirtualFileSystemLog.IsPending == true);
			VirtualFileSystemLog.Resume();
			VirtualFileSystemLog.Flush();
			Assert(VirtualFileSystemLog.IsPending == false);
			
			SettingManager.ImmediateLogging = true;
			VirtualFileSystemLog.Flush();
			for (int i = 0; i < 4; ++i)
			{
				VirtualFileSystemLog.Info("LogTest Info Sync {0}", total++);
				Assert(VirtualFileSystemLog.IsPending == false);
			}
			
			SettingManager.ImmediateLogging = false;
			VirtualFileSystemLog.Flush();
			VirtualFileSystemLog.Suspend();

			for (int i = 0; i < 4; ++i)
				VirtualFileSystemLog.Info("LogTest Info Async {0}", total++);

			Assert(VirtualFileSystemLog.IsPending == true);
			VirtualFileSystemLog.Resume();
			VirtualFileSystemLog.Flush();
			Assert(VirtualFileSystemLog.IsPending == false);

			SettingManager.ImmediateLogging = lastImmediateLogging;
			VirtualFileSystemLog.Flush();
		}

		[TestMethod, Priority(6)]
		public void DepotConnectionTest()
		{
			WorkspaceReset();
			string depotPasswd = UnitTestServer.GetUserP4Passwd(_P4User);
			string depotSyncPath = "//depot/gears1/Development/Src/Core/...";
			string ssoLoginFile = String.Format("{0}\\{1}_ClientLoginSSO.bat", UnitTestServer.GetServerRootFolder(), nameof(DepotConnectionTest));
			FileUtilities.DeleteFile(ssoLoginFile);

			Func<bool> p4logout = () => 
			{ 
				System.Text.StringBuilder logoutOutput = new System.Text.StringBuilder();
				return ProcessInfo.ExecuteWait(P4Exe, String.Format("{0} logout", ClientConfig), echo:true, stdout:logoutOutput) == 0 || logoutOutput.ToString().Contains("Perforce password (P4PASSWD) invalid or unset.");
			};

			// Logout and fail to reconnect without password, unattended
			Assert(p4logout());
			using (DepotClient depotClient = new DepotClient())
			{
				depotClient.Unattended = true;
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User) == false);
			}

			// Logout and successfully reconnect without password, but interactively with correct InteractiveOverridePasswd to prevent dialog
			Assert(p4logout());
			using (DepotClient depotClient = new DepotClient())
			{
				InteractiveOverridePasswd = depotPasswd;
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));
				InteractiveOverridePasswd = null;
			}

			// Logout and successfully reconnect with incorrect password, but interactively with correct InteractiveOverridePasswd to prevent dialog
			Assert(p4logout());
			using (DepotClient depotClient = new DepotClient())
			{
				InteractiveOverridePasswd = depotPasswd;
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User, depotPasswd:"_incorrect_password_"));
				InteractiveOverridePasswd = null;
			}

			// Logout and fail to reconnect without password, unattended, but valid P4SSOLOGIN with incorrect password
			Assert(p4logout());
			using (DepotClient depotClient = new DepotClient())
			{
				using (new DepotTempFile(ssoLoginFile)) {
				using (new SetEnvironmentVariableScope(DepotConstants.P4LOGINSSO, ssoLoginFile)) {
				File.WriteAllText(ssoLoginFile, String.Format("@ECHO OFF\nECHO {0}", "_incorrect_sso_password_"), System.Text.Encoding.ASCII);
				depotClient.Unattended = true;
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User) == false);
			}}}

			// Logout and successfully reconnect without password, unattended, but valid P4SSOLOGIN with correct password
			Assert(p4logout());
			using (DepotClient depotClient = new DepotClient())
			{
				using (new DepotTempFile(ssoLoginFile)) {
				using (new SetEnvironmentVariableScope(DepotConstants.P4LOGINSSO, ssoLoginFile)) {
				File.WriteAllText(ssoLoginFile, String.Format("@ECHO OFF\nECHO {0}", depotPasswd), System.Text.Encoding.ASCII);
				depotClient.Unattended = true;
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));
			}}}

			// Logout and successfully reconnect with correct password
			Assert(p4logout());
			using (DepotClient depotClient = new DepotClient())
			{
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User, depotPasswd:depotPasswd));
				Assert(depotClient.Login((_) => depotPasswd));
			}

			// Logout and interactive login to sync with the correct password
			Assert(p4logout());
			InteractiveOverridePasswd = depotPasswd;
			Assert(ProcessInfo.ExecuteWait(P4vfsExe, String.Format("{0} sync {1}", ClientConfig, depotSyncPath), echo:true, log:true) == 0);
			InteractiveOverridePasswd = null;
			
			// Logout and interactive login from service with the correct password
			Assert(p4logout());
			InteractiveOverridePasswd = depotPasswd;
			Extensions.SocketModel.SocketModelClient service = new Extensions.SocketModel.SocketModelClient(); 
			Assert(ServiceGarbageCollect());
			Assert(GetServiceIdleConnectionCount() == 0);
			Assert(ReconcilePreview(depotSyncPath).Any() == false);
			InteractiveOverridePasswd = null;
		}

		[TestMethod, Priority(7), TestRemote]
		public void CreateProcessImpersonatedTest()
		{
			{
				System.Text.StringBuilder output = new System.Text.StringBuilder();
				string cmd = String.Format("\"{0}\\p4vfs.exe\" {1} login -w _incorrect_password_", System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location), ClientConfig);
				Assert(NativeMethods.CreateProcessImpersonated(cmd, null, ProcessExecuteFlags.WaitForExit, output, null));
				Assert(output.ToString().Split(new char[]{'\n','\r'}, StringSplitOptions.RemoveEmptyEntries).Contains("Login failed."));
			}
			{
				System.Text.StringBuilder output = new System.Text.StringBuilder();
				string cmd = String.Format("cmd.exe /s /c echo foobar");
				Assert(NativeMethods.CreateProcessImpersonated(cmd, null, ProcessExecuteFlags.WaitForExit, output, null));
				Assert(output.ToString().Split(new char[]{'\n','\r'}, StringSplitOptions.RemoveEmptyEntries).Contains("foobar"));
			}
			{
				System.Text.StringBuilder output = new System.Text.StringBuilder();
				string cmd = String.Format("cmd.exe /s /c");
				Assert(NativeMethods.CreateProcessImpersonated(cmd, null, ProcessExecuteFlags.WaitForExit, output, null));
				Assert(output.ToString().Length == 0);
			}
		}

		[TestMethod, Priority(8), TestRemote]
		public void DepotFolderClientFileMappingTest()
		{
			WorkspaceReset();
			using (DepotClient depotClient = new DepotClient())
			{
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));
				string clientRoot = GetClientRoot(depotClient);

				DepotResultWhere.Node f0 = depotClient.WhereFolder("//depot/gears1/Development");
				Assert(f0 != null);
				Assert(f0.DepotPath == "//depot/gears1/Development");
				Assert(f0.WorkspacePath == String.Format("//{0}/depot/gears1/Development", _P4Client));
				Assert(f0.LocalPath == String.Format(@"{0}\depot\gears1\Development", clientRoot));

				DepotResultWhere.Node f1 = depotClient.WhereFolder(String.Format(@"{0}\depot\\gears1\Development\", clientRoot));
				Assert(f1 != null);
				Assert(f1.DepotPath == "//depot/gears1/Development");
				Assert(f1.WorkspacePath == String.Format("//{0}/depot/gears1/Development", _P4Client));
				Assert(f1.LocalPath == String.Format(@"{0}\depot\gears1\Development", clientRoot));

				DepotResultWhere.Node f2 = depotClient.WhereFolder("//depot");
				Assert(f2 != null);
				Assert(f2.DepotPath == "//depot");
				Assert(f2.WorkspacePath == String.Format("//{0}/depot", _P4Client));
				Assert(f2.LocalPath == String.Format(@"{0}\depot", clientRoot));

				DepotResultWhere.Node f3 = depotClient.WhereFolder("//does/not/exist");
				Assert(f3 == null);
			}
		}

		[TestMethod, Priority(9), TestRemote]
		public void DepotReconcilePreviewTest()
		{
			WorkspaceReset();
			using (DepotClient depotClient = new DepotClient())
			{
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));
				string clientRoot = GetClientRoot(depotClient);

				depotClient.Sync("//depot/gears1/Development/Src/Core/...", null, DepotSyncFlags.Normal, DepotSyncMethod.Virtual);
				Assert(ReconcilePreview("//depot/gears1/Development/Src/Core").Any() == false);

				string newFile = String.Format(@"{0}\depot\gears1\Development\Src\Core\Src\NewFile.cpp", clientRoot);
				Assert(File.Exists(newFile) == false);
				using (var s0 = File.Create(newFile)) {}
				Assert(File.Exists(newFile) == true);

				string missingFile = String.Format(@"{0}\depot\gears1\Development\Src\Core\Src\BitArray.cpp", clientRoot);
				Assert(File.Exists(missingFile) == true);
				FileUtilities.DeleteFile(missingFile);
				Assert(File.Exists(missingFile) == false);

				string changedFile = String.Format(@"{0}\depot\gears1\Development\Src\Core\Src\Core.cpp", clientRoot);
				Assert(File.Exists(changedFile) == true);
				FileUtilities.MakeWritable(changedFile);
				using (var s0 = File.Create(changedFile)) {}
				Assert(File.Exists(changedFile) == true);
				Assert(FileUtilities.GetFileLength(changedFile) == 0);

				ReconcileDifference[] differences = ReconcilePreview("//depot/gears1/Development/Src/Core").ToArray();
				Assert(differences.Length == 4);
				Assert(differences[0].Action == ReconcileAction.DifferentOnClient);
				Assert(differences[0].ClientFile.ToLower() == changedFile.ToLower());
				Assert(differences[1].Action == ReconcileAction.MissingOnClient);
				Assert(differences[1].ClientFile.ToLower() == missingFile.ToLower());
				Assert(differences[2].Action == ReconcileAction.DifferentType);
				Assert(differences[2].ClientFile.ToLower() == changedFile.ToLower());
				Assert(differences[3].Action == ReconcileAction.OnClientOnly);
				Assert(differences[3].ClientFile.ToLower() == newFile.ToLower());
			}
		}

		[TestMethod, Priority(10), TestRemote]
		public void MakeResidentRaceTest()
		{
			foreach (ServiceSettingsScope settings in EnumerateCommonServiceSyncSettings())
			{
				using (settings) {
				using (DepotClient depotClient = new DepotClient()) {
				
				WorkspaceReset();
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));
				string clientRoot = GetClientRoot(depotClient);
				
				for (int racePass = 0; racePass < 4; ++racePass)
				{
					string raceFile = String.Format(@"{0}\depot\gears1\Development\Src\Core\Src\Core.cpp", clientRoot);
					depotClient.Sync(raceFile, null, DepotSyncFlags.Force, DepotSyncMethod.Virtual);
					Assert(IsPlaceholderFile(raceFile) == true);

					Random random = new Random();
					List<System.Threading.Thread> workers = new List<System.Threading.Thread>();
					List<Dictionary<string, object>> workerArgs = new List<Dictionary<string, object>>();
					for (int workerIndex = 0; workerIndex < 20; ++workerIndex)
					{
						System.Threading.Thread worker = new System.Threading.Thread(new System.Threading.ParameterizedThreadStart(p => 
						{
							try
							{
								Dictionary<string, object> args = p as Dictionary<string, object>;
								VirtualFileSystemLog.Verbose("MakeResidentRaceTest Process [{0}.{1}]", Process.GetCurrentProcess().Id, System.AppDomain.GetCurrentThreadId());
								System.Threading.Thread.Sleep((int)args["SleepTime"]);
								using (FileStream stream = File.Open(raceFile, FileMode.Open, FileAccess.Read, FileShare.Read))
								{
									System.Threading.Thread.Sleep(500);
									MemoryStream memStream = new MemoryStream();
									stream.CopyTo(memStream);
									memStream.Seek(0, SeekOrigin.Begin);
									int lineCount = 0;
									for (StreamReader reader = new StreamReader(memStream); reader.ReadLine() != null;)
										lineCount++;
									Assert(lineCount == 280, lineCount.ToString());
								}
								args["Result"] = true;
							}
							catch (Exception e)
							{
								VirtualFileSystemLog.Error("MakeResidentRaceTest Process [{0}.{1}] failed race to read file: {2}\n{3}", Process.GetCurrentProcess().Id, System.AppDomain.GetCurrentThreadId(), e.Message, e.StackTrace); 
							}
						}));

						var workerArg = new Dictionary<string, object>();
						workerArg["SleepTime"] = random.Next(200)+500;
						workerArg["Result"] = false;
						workerArgs.Add(workerArg);
						workers.Add(worker);
						worker.Start(workerArg);
					}

					for (int workerIndex = 0; workerIndex < workers.Count; ++workerIndex)
					{
						workers[workerIndex].Join();
						Assert((workerArgs[workerIndex]["Result"] as bool?) == true);
					}
				}
			}}}
		}

		[TestMethod, Priority(11), TestRemote]
		public void ForceSyncTest()
		{
			WorkspaceReset();
			using (DepotClient depotClient = new DepotClient())
			{
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));
				string depotPath = "//depot/gears1/Development/External/nvDXT/...";
				{
					int syncRevision = 10;
					DepotSyncResult syncResults = depotClient.Sync(depotPath, new DepotRevisionChangelist(syncRevision), DepotSyncFlags.Force, DepotSyncMethod.Virtual);
					Assert(syncResults?.Status == DepotSyncStatus.Success);
					Assert(syncResults.Modifications?.Count() == 12);
					Assert(syncResults.Modifications.Any(a => a.SyncActionType != DepotSyncActionType.Added) == false);
					Assert(ProcessInfo.ExecuteWait(P4Exe, String.Format("{0} flush -f \"{1}@{2}\"", ClientConfig, depotPath, syncRevision), echo:true) == 0);
					Assert(ReconcilePreview(depotPath).Any() == false);
				}
				{
					int syncRevision = 11;
					DepotSyncResult syncResults = depotClient.Sync(depotPath, new DepotRevisionChangelist(syncRevision), DepotSyncFlags.Force, DepotSyncMethod.Virtual);
					Assert(syncResults?.Status == DepotSyncStatus.Success);
					Assert(syncResults.Modifications?.Count() == 18);
					Assert(syncResults.Modifications.Count(a => a.SyncActionType == DepotSyncActionType.Deleted) == 6);
					Assert(syncResults.Modifications.Count(a => a.SyncActionType == DepotSyncActionType.Updated) == 6);
					Assert(syncResults.Modifications.Count(a => a.SyncActionType == DepotSyncActionType.Added) == 6);
					Assert(ProcessInfo.ExecuteWait(P4Exe, String.Format("{0} flush -f \"{1}@{2}\"", ClientConfig, depotPath, syncRevision), echo:true) == 0);
					Assert(ReconcilePreview(depotPath).Any() == false);
				}
			}
		}

		[TestMethod, Priority(12), TestRemote]
		public void FilePopulateMethodTest()
		{
			WorkspaceReset();
			using (DepotClient depotClient = new DepotClient())
			{
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));
				string clientRoot = GetClientRoot(depotClient);

				bool expectFolderTimeEqual = SettingManagerExtensions.PopulateMethod != FilePopulateMethod.Move;
				string srcFile = String.Format(@"{0}\depot\gears1\Development\External\nvDXT\Lib\nvDXTlib.vc7.lib", clientRoot);
				string srcFileFolder = System.IO.Path.GetDirectoryName(srcFile);

				depotClient.Sync(String.Format("{0}\\...", srcFileFolder));
				Assert(IsPlaceholderFile(srcFile));
				Assert(FileUtilities.GetDirectorySize(srcFileFolder) == depotClient.GetDepotSize(new[]{ String.Format("{0}\\...", srcFileFolder) }));
				
				DateTime srcFileCreationTimeBefore = File.GetCreationTime(srcFile);
				DateTime srcFileWriteTimeBefore = File.GetLastWriteTime(srcFile);
				DateTime srcFileFolderCreationTimeBefore = File.GetCreationTime(srcFileFolder);
				DateTime srcFileFolderWriteTimeBefore = File.GetLastWriteTime(srcFileFolder);
				Assert(IsPlaceholderFile(srcFile));

				foreach (string filePath in Directory.EnumerateFiles(srcFileFolder))
				{
					using (var fs = File.OpenRead(filePath)) {}	
					Assert(IsPlaceholderFile(filePath) == false);
				}

				Assert(IsPlaceholderFile(srcFile) == false);
				Assert(srcFileCreationTimeBefore == File.GetCreationTime(srcFile), "File creation time changed");
				Assert(srcFileWriteTimeBefore == File.GetLastWriteTime(srcFile), "File write time changed");
				Assert(srcFileFolderCreationTimeBefore == File.GetCreationTime(srcFileFolder), "Folder creation time changed");
				Assert((srcFileFolderWriteTimeBefore == File.GetLastWriteTime(srcFileFolder)) == expectFolderTimeEqual, "Unexpected Folder write time");
			}
		}

		[TestMethod, Priority(13), TestRemote]
		public void DepotCreateFileSpecTest()
		{
			WorkspaceReset();
			using (DepotClient depotClient = new DepotClient())
			{
				foreach (string path in new[]{ "", "//depot/gears1", "//depot/gears1/...", "D:\\depot\\gears1", "D:\\depot\\gears1\\..." })
				{
					foreach (string rev in new[]{ "", "@1234", "#have", "@my_label", "#", "@" })
					{
						string validRev = rev.Length == 1 ? "" : rev;
						Assert(DepotOperations.CreateFileSpec(path, null, DepotOperations.CreateFileSpecFlags.None) == path);
						Assert(DepotOperations.CreateFileSpec(path+rev, null, DepotOperations.CreateFileSpecFlags.None) == path+validRev);
						Assert(DepotOperations.CreateFileSpec(path, null, DepotOperations.CreateFileSpecFlags.OverrideRevison) == path);
						Assert(DepotOperations.CreateFileSpec(path+rev, null, DepotOperations.CreateFileSpecFlags.OverrideRevison) == path);

						validRev = rev;
						if (rev.Length <= 1)
							validRev = "#head";
						if (rev.Length > 1 && path.Length == 0)
							validRev = rev;

						Assert(DepotOperations.CreateFileSpec(path, new DepotRevisionHead().ToString(), DepotOperations.CreateFileSpecFlags.None) == path+"#head");
						Assert(DepotOperations.CreateFileSpec(path+rev, new DepotRevisionHead().ToString(), DepotOperations.CreateFileSpecFlags.None) == path+validRev);
						Assert(DepotOperations.CreateFileSpec(path, new DepotRevisionHead().ToString(), DepotOperations.CreateFileSpecFlags.OverrideRevison) == path+"#head");
						Assert(DepotOperations.CreateFileSpec(path+rev, new DepotRevisionHead().ToString(), DepotOperations.CreateFileSpecFlags.OverrideRevison) == path+"#head");
					}
				}
			}
		}

		[TestMethod, Priority(14), TestRemote]
		public void SyncInvalidFileSpecTest()
		{
			WorkspaceReset();
			using (DepotClient depotClient = new DepotClient())
			{
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));
				DepotSyncResult syncResult;
				
				syncResult = depotClient.Sync("//fenix/main/tools/...", null, DepotSyncFlags.Normal, DepotSyncMethod.Virtual);
				Assert(syncResult != null);
				Assert(syncResult.Status == DepotSyncStatus.Error);
				Assert(syncResult.Modifications.Count() == 1);
				Assert(syncResult.Modifications[0].SyncActionType == DepotSyncActionType.NotInClientView);

				syncResult = depotClient.Sync("//depot/gears1/Binaries/...@vfs_bad_label", null, DepotSyncFlags.Normal, DepotSyncMethod.Virtual);
				Assert(syncResult != null);
				Assert(syncResult.Status == DepotSyncStatus.Error);
				Assert(syncResult.Modifications.Count() == 1);
				Assert(syncResult.Modifications[0].SyncActionType == DepotSyncActionType.GenericError);
				Assert(syncResult.Modifications[0].Message == "Invalid changelist/client/label/date '@vfs_bad_label'.");

				syncResult = depotClient.Sync("//depot/gears1/Binaries/...", new DepotRevisionLabel("vfs_bad_label"), DepotSyncFlags.Normal, DepotSyncMethod.Virtual);
				Assert(syncResult != null);
				Assert(syncResult.Status == DepotSyncStatus.Error);
				Assert(syncResult.Modifications.Count() == 1);
				Assert(syncResult.Modifications.First().SyncActionType == DepotSyncActionType.GenericError);
				Assert(syncResult.Modifications[0].Message == "Invalid changelist/client/label/date '@vfs_bad_label'.");
			}
		}

		[TestMethod, Priority(15), TestRemote]
		public void SyncAlwaysResidentFileTest()
		{
			WorkspaceReset();
			
			string clientRoot = GetClientRoot();
			string srcFolder = String.Format(@"{0}\depot\gears1\Development\External\nvTriStrip", clientRoot);
			string srcRevision = "12";
			string srcResidentPattern = "dsp|dsw|lib";
			string srcResidentExtensions = "dsp,dsw;lib";

			var verifyResidentFiles = new Action<bool>(alwaysResident =>
			{
				Assert(Directory.Exists(srcFolder));
				string[] srcFiles = Directory.GetFiles(srcFolder, "*", SearchOption.AllDirectories);
				Assert(srcFiles.Length == 10);
				foreach (string file in srcFiles)
				{
					Assert(File.Exists(file));
					bool expectingPlaceholderFile = !alwaysResident && !Regex.IsMatch(Path.GetExtension(file), srcResidentPattern, RegexOptions.IgnoreCase);
					Assert(IsPlaceholderFile(file) == expectingPlaceholderFile);
				}
				Assert(ProcessInfo.ExecuteWait(P4Exe, String.Format("{0} flush -f \"{1}\\...@{2}\"", ClientConfig, srcFolder, srcRevision), echo:true) == 0);
				Assert(ReconcilePreview(srcFolder).Any() == false);
			});

			using (DepotClient depotClient = new DepotClient())
			{
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));
				Assert(depotClient.Sync(String.Format("{0}\\...", srcFolder), new DepotRevisionChangelist(Int32.Parse(srcRevision)), DepotSyncFlags.Normal, DepotSyncMethod.Virtual, DepotFlushType.Atomic, srcResidentPattern)?.Status == DepotSyncStatus.Success);
				verifyResidentFiles(false);
				
				WorkspaceReset();
				Assert(depotClient.Sync(String.Format("{0}\\...", srcFolder), new DepotRevisionChangelist(Int32.Parse(srcRevision)), DepotSyncFlags.Normal, DepotSyncMethod.Virtual, DepotFlushType.Atomic)?.Status == DepotSyncStatus.Success);
				Assert(DepotOperations.Hydrate(depotClient, new DepotSyncOptions{ Files=new[]{ String.Format("{0}\\...", srcFolder) }, SyncResident=srcResidentPattern })?.Status == DepotSyncStatus.Success);
				verifyResidentFiles(false);

				WorkspaceReset();
				Assert(depotClient.Sync(String.Format("{0}\\...", srcFolder), new DepotRevisionChangelist(Int32.Parse(srcRevision)), DepotSyncFlags.Normal, DepotSyncMethod.Virtual, DepotFlushType.Atomic)?.Status == DepotSyncStatus.Success);
				Assert(DepotOperations.Hydrate(depotClient, new DepotSyncOptions{ Files=new[]{ String.Format("{0}\\...", srcFolder) } })?.Status == DepotSyncStatus.Success);
				verifyResidentFiles(true);
			}

			foreach (string syncOption in EnumerateCommonConsoleSyncOptions())
			{
				bool alwaysResident = syncOption.Split(' ').Contains("-r");
				WorkspaceReset();
				Assert(ProcessInfo.ExecuteWait(P4vfsExe, String.Format("{0} sync {1} -p \"{2}\" \"{3}\\...@{4}\"", ClientConfig, syncOption, srcResidentPattern, srcFolder, srcRevision), echo:true, log:true) == 0);
				verifyResidentFiles(alwaysResident);

				WorkspaceReset();
				Assert(ProcessInfo.ExecuteWait(P4vfsExe, String.Format("{0} sync {1} -x \"{2}\" \"{3}\\...@{4}\"", ClientConfig, syncOption, srcResidentExtensions, srcFolder, srcRevision), echo:true, log:true) == 0);
				verifyResidentFiles(alwaysResident);
			}
		}

		[TestMethod, Priority(16), TestRemote]
		public void DepotClientOwnerChangeTest()
		{
			WorkspaceReset();
			using (DepotClient depotClient = new DepotClient())
			{
				Assert(DepotOperations.GetClientOwnerUserName(depotClient, _P4Client, _P4Port) == _P4User);
				Assert(DepotOperations.GetClientOwnerUserName(depotClient, "gatineau-pc-depot", _P4Port) == "northamerica\\gatineau");
				Assert(DepotOperations.GetClientOwnerUserName(depotClient, "quebec-pc-depot", _P4Port) == "northamerica\\quebec");
				Assert(DepotOperations.GetClientOwnerUserName(depotClient, "montreal-pc-main", _P4Port) == "northamerica\\montreal");

				Assert(depotClient.Connect(_P4Port, _P4Client, "northamerica\\quebec"));
				Assert(new[]{ _P4User, "northamerica\\quebec" }.Contains(depotClient.Info().UserName));
				Assert(depotClient.ConnectionClient().Owner == _P4User);
			}

			int srcRevision = 10;
			string clientRoot = GetClientRoot();
			string srcFolder = String.Format(@"{0}\depot\gears1\Development\External\nvDXT", clientRoot);
			string srcMissingUserName = "northamerica\\vfs_missing_user";

			Assert(ProcessInfo.ExecuteWait(P4vfsExe, String.Format("{0} sync \"{1}\\...@{2}\"", ClientConfig, srcFolder, srcRevision), echo:true, log:true) == 0);
			string[] srcFiles = Directory.GetFiles(srcFolder, "*", SearchOption.AllDirectories).ToArray();
			Assert(srcFiles.Length == 12);
			foreach (string srcFile in srcFiles)
			{
				Assert(IsPlaceholderFile(srcFile));
				FilePopulateInfo popInfoBefore = NativeMethods.GetFilePopulateInfo(srcFile);
				Assert(popInfoBefore != null);
				Assert(popInfoBefore.DepotServer == _P4Port);
				Assert(popInfoBefore.DepotClient == _P4Client);
				Assert(popInfoBefore.DepotUser == _P4User);
				Assert(IsPlaceholderFile(srcFile));

				Assert(NativeMethods.InstallReparsePointOnFile(
					VirtualFileSystem.CurrentVersion.Major,
					VirtualFileSystem.CurrentVersion.Minor,
					VirtualFileSystem.CurrentVersion.Build,
					srcFile,
					(byte)FileResidencyPolicy.Resident,
					popInfoBefore.FileRevision,
					FileUtilities.GetFileLength(srcFile),
					FileUtilities.IsReadOnly(srcFile) ? WindowsInterop.FILE_ATTRIBUTE_READONLY : 0,
					popInfoBefore.DepotPath,
					popInfoBefore.DepotServer,
					popInfoBefore.DepotClient,
					srcMissingUserName) == 0);

				FilePopulateInfo popInfoAfter = NativeMethods.GetFilePopulateInfo(srcFile);
				Assert(popInfoAfter != null);
				Assert(popInfoAfter.DepotServer == _P4Port);
				Assert(popInfoAfter.DepotClient == _P4Client);
				Assert(popInfoAfter.DepotUser == srcMissingUserName);
				Assert(IsPlaceholderFile(srcFile));
			}

			Assert(ProcessInfo.ExecuteWait(P4Exe, String.Format("{0} flush -f \"{1}\\...@{2}\"", ClientConfig, srcFolder, srcRevision), echo:true) == 0);
			Assert(ReconcilePreview(srcFolder).Any() == false);
			foreach (string srcFile in srcFiles)
				Assert(IsPlaceholderFile(srcFile) == false);
		}

		[TestMethod, Priority(17)]
		public void DepotClientPathTests()
		{
			WorkspaceReset();

			Assert(DepotOperations.NormalizePath(null) == "");
			Assert(DepotOperations.NormalizePath("") == "");
			Assert(DepotOperations.NormalizePath("Y:\\depot\\gears1\\Development\\Src\\Core\\Src\\Core.cpp") == "Y:\\depot\\gears1\\Development\\Src\\Core\\Src\\Core.cpp");
			Assert(DepotOperations.NormalizePath("//depot/gears1/Development\\Src\\Core\\Src\\Core.cpp") == "//depot/gears1/Development/Src/Core/Src/Core.cpp");
			Assert(DepotOperations.NormalizePath("Development\\Src\\Core\\Src\\Core.cpp") == "Development\\Src\\Core\\Src\\Core.cpp");
			Assert(DepotOperations.NormalizePath("Development/Src\\\\Core\\Src\\Core.cpp") == "Development\\Src\\Core\\Src\\Core.cpp");
			Assert(DepotOperations.NormalizePath("//foo/bar/../star.txt") == "//foo/star.txt");
			Assert(DepotOperations.NormalizePath("//foo/bar/data\\../..\\star.txt") == "//foo/star.txt");
			Assert(DepotOperations.NormalizePath("//foo/bar/../star.txt") == "//foo/star.txt");
			Assert(DepotOperations.NormalizePath("d:\\foo\\bar\\..\\star.txt") == "D:\\foo\\star.txt");
			Assert(DepotOperations.NormalizePath("x:\\\\foo/bar/data\\../..\\star.txt") == "X:\\foo\\star.txt");
			Assert(DepotOperations.NormalizePath("//foo/bar/data\\car\\../..\\stuff\\..\\star.txt") == "//foo/bar/star.txt");
		}

		[TestMethod, Priority(18), TestRemote]
		public void DepotClientSymlinkSyncTest()
		{
			foreach (ServiceSettingsScope settings in EnumerateCommonServiceSyncLineEndSettings().Where(s => s.SyncFlags == DepotSyncFlags.Normal))
			{
				using (settings) {
				using (DepotClient depotClient = new DepotClient()) {
				
				WorkspaceReset();
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));
				string clientRoot = GetClientRoot(depotClient);

				DepotResultClient.Node client = depotClient.Client();
				client.Fields[DepotResultClient.FieldName.LineEnd] = settings.LineEnd;
				Assert(depotClient.UpdateClient(client) != null);
				client = depotClient.Client();
				Assert(client.LineEnd == settings.LineEnd);

				string srcRevision = "@13";
				string[] srcDepotSymlinks = new[]{ 
					"//depot/mono/2.0/Accessibility.dll",
					"//depot/mono/compat-2.0/System.Web.Mvc.dll",
				};
				foreach (string srcDepotSymlink in srcDepotSymlinks)
				{
					string srcDepotTarget = DepotOperations.GetSymlinkTargetDepotFile(depotClient, srcDepotSymlink, srcRevision);
					Assert(String.IsNullOrEmpty(srcDepotTarget) == false);
					Assert(srcDepotTarget.StartsWith("//"));
					Assert(srcDepotTarget.Split('/').Last() == Path.GetFileName(srcDepotSymlink));
					
					DepotResultWhere srcSymlinkWhere = depotClient.Where(srcDepotSymlink);
					Assert(srcSymlinkWhere.Count == 1);
					string srcClientSymlink = srcSymlinkWhere[0].LocalPath;
					Assert(String.IsNullOrEmpty(srcClientSymlink) == false);

					DepotResultWhere srcTargetWhere = depotClient.Where(srcDepotTarget);
					Assert(srcTargetWhere.Count == 1);
					string srcClientTarget = srcTargetWhere[0].LocalPath;
					Assert(String.IsNullOrEmpty(srcClientTarget) == false);

					Action<string, string>[] syncActions = new Action<string, string>[] {
							(path, rev) => Assert(ProcessInfo.ExecuteWait(P4Exe, String.Format("{0} sync \"{1}{2}\"", ClientConfig, path, rev), echo:true) == 0),
							(path, rev) => Assert(depotClient.Sync(path, DepotRevision.FromString(rev), DepotSyncFlags.Normal, DepotSyncMethod.Virtual)?.Modifications?.Count() == 1),
							(path, rev) => Assert(depotClient.Sync(path, DepotRevision.FromString(rev), DepotSyncFlags.Normal, DepotSyncMethod.Regular)?.Modifications?.Count() == 1),
							(path, rev) => Assert(ProcessInfo.ExecuteWait(P4vfsExe, String.Format("{0} sync -t \"{1}{2}\"", ClientConfig, path, rev), echo:true) == 0),
							(path, rev) => Assert(ProcessInfo.ExecuteWait(P4vfsExe, String.Format("{0} sync -s \"{1}{2}\"", ClientConfig, path, rev), echo:true) == 0),
					};

					for (int syncActionIndex = 0; syncActionIndex < syncActions.Length; ++syncActionIndex)
					{
						var syncAction = syncActions[syncActionIndex];
						depotClient.Sync("//...", new DepotRevisionNone(), DepotSyncFlags.Force, DepotSyncMethod.Regular);
						Assert(File.Exists(srcClientSymlink) == false && File.Exists(srcClientTarget) == false);
						Assert(Directory.Exists(clientRoot) == false || FileUtilities.GetDirectoryFileCount(clientRoot) == 0);

						syncAction(srcDepotSymlink, srcRevision);
						Assert(NativeMethods.IsFileSymlink(srcClientSymlink));
						Assert(File.Exists(srcClientSymlink) && NativeMethods.GetFileUncompressedSize(srcClientSymlink) == -1);
						Assert(ProcessInfo.ExecuteWait(P4Exe, String.Format("{0} sync \"{1}{2}\"", ClientConfig, srcDepotTarget, srcRevision), echo:true) == 0);
						Assert(NativeMethods.IsFileSymlink(srcClientTarget) == false);
						Assert(File.Exists(srcClientTarget) && FileUtilities.GetFileLength(srcClientTarget) > 0);
						Assert(FileUtilities.GetFileLength(srcClientSymlink) == 0);
						Assert(FileUtilities.GetFileLength(srcClientTarget) == NativeMethods.GetFileUncompressedSize(srcClientTarget));
						Assert(NativeMethods.GetFileUncompressedSize(srcClientSymlink) == NativeMethods.GetFileUncompressedSize(srcClientTarget));

						syncAction(srcDepotSymlink, "#none");
						Assert(File.Exists(srcClientSymlink) == false);
						
						syncAction(srcDepotSymlink, srcRevision);
						Assert(NativeMethods.IsFileSymlink(srcClientSymlink));
						Assert(FileUtilities.GetFileLength(srcClientSymlink) == 0);
						Assert(FileUtilities.GetFileLength(srcClientTarget) == NativeMethods.GetFileUncompressedSize(srcClientTarget));
						Assert(NativeMethods.GetFileUncompressedSize(srcClientSymlink) == NativeMethods.GetFileUncompressedSize(srcClientTarget));
					}
				}
			}}}
		}

		[TestMethod, Priority(19), TestRemote]
		public void ParallelServiceTaskTest()
		{
			WorkspaceReset();
			using (DepotClient depotClient = new DepotClient())
			{
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));
				string clientRoot = GetClientRoot(depotClient);

				for (int racePass = 0; racePass < 2; ++racePass)
				{
					string clientFolder = String.Format(@"{0}\depot\gears3\GearGame\Movies", clientRoot);
					depotClient.Sync(String.Format("{0}\\...@14", clientFolder), null, DepotSyncFlags.Force, DepotSyncMethod.Virtual);
					
					string[] clientFiles = Directory.EnumerateFiles(clientFolder).ToArray();
					Assert(clientFiles.Length == 21);
					foreach (string clientFile in clientFiles)
						Assert(IsPlaceholderFile(clientFile) == true);
					
					Random random = new Random();
					List<System.Threading.Thread> workers = new List<System.Threading.Thread>();
					List<Dictionary<string, object>> workerArgs = new List<Dictionary<string, object>>();
					for (int workerIndex = 0; workerIndex < 32; ++workerIndex)
					{
						System.Threading.Thread worker = new System.Threading.Thread(new System.Threading.ParameterizedThreadStart(p => 
						{
							try
							{
								Dictionary<string, object> args = p as Dictionary<string, object>;
								VirtualFileSystemLog.Verbose("ParallelServiceTaskTest Begin Process [{0}.{1}] -> {2}", Process.GetCurrentProcess().Id, System.AppDomain.GetCurrentThreadId(), args["ClientFile"]);
								System.Threading.Thread.Sleep((int)args["SleepTime"]);
								using (FileStream stream = File.Open((string)args["ClientFile"], FileMode.Open, FileAccess.Read, FileShare.Read)) {}
								VirtualFileSystemLog.Verbose("ParallelServiceTaskTest End Process [{0}.{1}] -> {2}", Process.GetCurrentProcess().Id, System.AppDomain.GetCurrentThreadId(), args["ClientFile"]);
								Assert(IsPlaceholderFile((string)args["ClientFile"]) == false);
								args["Result"] = true;
							}
							catch (Exception e)
							{
								VirtualFileSystemLog.Error("ParallelServiceTaskTest Process [{0}.{1}] failed to read file: {2}\n{3}", Process.GetCurrentProcess().Id, System.AppDomain.GetCurrentThreadId(), e.Message, e.StackTrace); 
							}
						}));

						var workerArg = new Dictionary<string, object>();
						workerArg["ClientFile"] = clientFiles[random.Next(clientFiles.Length-1)];
						workerArg["SleepTime"] = random.Next(200)+500;
						workerArg["Result"] = false;
						workerArgs.Add(workerArg);
						workers.Add(worker);
						worker.Start(workerArg);
					}

					for (int workerIndex = 0; workerIndex < workers.Count; ++workerIndex)
					{
						workers[workerIndex].Join();
						Assert((workerArgs[workerIndex]["Result"] as bool?) == true);
					}
				}
			}
		}

		[TestMethod, Priority(20), TestRemote]
		public void DepotServerConfigTest()
		{
			ServiceRestart();

			Action AssertCurrentServiceSettings = () =>
			{
				// Test DepotServerConfig get/set
				DepotServerConfig c0 = SettingManagerExtensions.DepotServerConfig;
				Assert(c0 != null);
				SettingManagerExtensions.DepotServerConfig = c0;
				DepotServerConfig c1 = SettingManagerExtensions.DepotServerConfig;
				Assert(c1 != null);
				Assert(JsonConvert.SerializeObject(c0) == JsonConvert.SerializeObject(c1));

				// Test DepotServerConfig GetJson/SetJson
				Newtonsoft.Json.Linq.JToken t0 = ServiceSettings.GetPropertyJson("DepotServerConfig");
				ServiceSettings.SetPropertyJson(t0, "DepotServerConfig");
				Newtonsoft.Json.Linq.JToken t1 = ServiceSettings.GetPropertyJson("DepotServerConfig");
				Assert(JsonConvert.SerializeObject(t0) == JsonConvert.SerializeObject(t1));
				Assert(JsonConvert.SerializeObject(t0) == JsonConvert.SerializeObject(ServiceSettings.SettingNodeToJson(ServiceSettings.SettingNodeFromJson(t1))));

				// Test the SocketModelProtocol Serialize/Deserialize
				string s0 = Extensions.SocketModel.SocketModelProtocol.Serialize(t0);
				t1 = Extensions.SocketModel.SocketModelProtocol.Deserialize<Newtonsoft.Json.Linq.JToken>(s0);
				Assert(JsonConvert.SerializeObject(t0) == JsonConvert.SerializeObject(t1));
				Assert(JsonConvert.SerializeObject(c0) == JsonConvert.SerializeObject(DepotServerConfig.FromNode(ServiceSettings.SettingNodeFromJson(t1))));
			};

			SettingManager.Reset();
			AssertCurrentServiceSettings();

			ServiceSettings.Reset();
			AssertCurrentServiceSettings();

			// Test DepotServerConfig serialization
			DepotServerConfig config0 = new DepotServerConfig{ Servers = new[]{ new DepotServerConfigEntry{ Pattern = "MyHost*", Address = "127.0.0.1:1666" }}};
			SettingNode configNode = config0.ToNode();
			DepotServerConfig config1 = DepotServerConfig.FromNode(configNode);
			Assert(JsonConvert.SerializeObject(config0) == JsonConvert.SerializeObject(config1));
			AssertCurrentServiceSettings();

			// Add remapping entries to a DepotServerConfig for testing
			Extensions.SocketModel.SocketModelClient client = new Extensions.SocketModel.SocketModelClient();
			DepotServerConfig depotServerConfig = DepotServerConfig.FromNode(client.GetServiceSetting(nameof(SettingManagerExtensions.DepotServerConfig)));
			List<DepotServerConfigEntry> servers = new List<DepotServerConfigEntry>(depotServerConfig.Servers ?? new DepotServerConfigEntry[0]);
			servers.Add(new DepotServerConfigEntry { Pattern = @"^p4-localhost4([\.:].+)?$", Address = _P4Port });
			servers.Add(new DepotServerConfigEntry { Pattern = @"^p4-contoso4([\.:].+)?$", Address = "p4-contoso:1666" });
			depotServerConfig.Servers = servers.ToArray();
			DepotServerConfig previousDepotServerConfig = SettingManagerExtensions.DepotServerConfig;
			SettingManagerExtensions.DepotServerConfig = depotServerConfig;
			Assert(JsonConvert.SerializeObject(SettingManagerExtensions.DepotServerConfig) == JsonConvert.SerializeObject(depotServerConfig));
			AssertCurrentServiceSettings();

			// Test assigning the new DepotServerConfig on the Service
			Assert(client.SetServiceSetting(nameof(SettingManagerExtensions.DepotServerConfig), depotServerConfig.ToNode()));
			Assert(JsonConvert.SerializeObject(depotServerConfig) == JsonConvert.SerializeObject(DepotServerConfig.FromNode(client.GetServiceSetting(nameof(SettingManagerExtensions.DepotServerConfig)))));

			// Test the local DepotServerConfig pattern matching of the host name resolving
			Assert(DepotOperations.ResolveDepotServerName(_P4Port) == _P4Port);
			Assert(DepotOperations.ResolveDepotServerName("p4-contoso") == "p4-contoso");
			Assert(DepotOperations.ResolveDepotServerName("p4-ConToso:1666") == "p4-ConToso:1666");
			Assert(DepotOperations.ResolveDepotServerName("p4-contoso.microsoft.com:1666") == "p4-contoso.microsoft.com:1666");
			Assert(DepotOperations.ResolveDepotServerName("p4-contoso4:1666") == "p4-contoso:1666");
			Assert(DepotOperations.ResolveDepotServerName("p4-contoso4.zipline.local:1666") == "p4-contoso:1666");
			Assert(DepotOperations.ResolveDepotServerName("p4-contoso4.microsoft.com:1667") == "p4-contoso:1666");
			Assert(DepotOperations.ResolveDepotServerName("p4-lolah:1667") == "p4-lolah:1667");
			Assert(DepotOperations.ResolveDepotServerName("CTO-P4MAIN.local:1666") == "CTO-P4MAIN.local:1666");
			Assert(DepotOperations.ResolveDepotServerName("CTO-P4MAIN:1666") == "CTO-P4MAIN:1666");
			Assert(DepotOperations.ResolveDepotServerName("CTO-P4MAIN") == "CTO-P4MAIN");

			// Test the service DepotServerConfig pattern matching of the host name resolving
			foreach (ServiceSettingsScope settings in EnumerateCommonServicePopulateSettings())
			{
				using (settings) {
				using (DepotClient depotClient = new DepotClient()) {

				WorkspaceReset();
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));
				string clientRoot = GetClientRoot(depotClient);
				
				settings.ApplyGlobal();
				SettingManagerExtensions.DepotServerConfig = depotServerConfig;
				Assert(JsonConvert.SerializeObject(SettingManagerExtensions.DepotServerConfig) == JsonConvert.SerializeObject(depotServerConfig));
				AssertCurrentServiceSettings();

				string srcFile = String.Format(@"{0}\depot\gears1\Development\Src\Core\Src\Core.cpp", clientRoot);				
				string invalidServer = "p4-localhost4.invalid.domain:1666";
				string validServer = _P4Port;

				depotClient.Sync(srcFile, new DepotRevisionHead(), DepotSyncFlags.Normal, DepotSyncMethod.Virtual);
				Assert(IsPlaceholderFile(srcFile));

				FilePopulateInfo popInfoBefore = NativeMethods.GetFilePopulateInfo(srcFile);
				Assert(popInfoBefore != null);
				Assert(popInfoBefore.DepotServer == _P4Port);
				Assert(popInfoBefore.DepotClient == _P4Client);
				Assert(popInfoBefore.DepotUser == _P4User);
				Assert(IsPlaceholderFile(srcFile));

				// Write an invalid server name to the placeholder which will require redirection
				Assert(NativeMethods.InstallReparsePointOnFile(
					VirtualFileSystem.CurrentVersion.Major,
					VirtualFileSystem.CurrentVersion.Minor,
					VirtualFileSystem.CurrentVersion.Build,
					srcFile,
					(byte)FileResidencyPolicy.Resident,
					popInfoBefore.FileRevision,
					FileUtilities.GetFileLength(srcFile),
					FileUtilities.IsReadOnly(srcFile) ? WindowsInterop.FILE_ATTRIBUTE_READONLY : 0,
					popInfoBefore.DepotPath,
					invalidServer,
					popInfoBefore.DepotClient,
					popInfoBefore.DepotUser) == 0);

				// Confirm that the placeholder and been written with invalid server
				FilePopulateInfo popInfoAfter = NativeMethods.GetFilePopulateInfo(srcFile);
				Assert(popInfoAfter != null);
				Assert(popInfoAfter.DepotServer == invalidServer);
				Assert(popInfoAfter.DepotClient == _P4Client);
				Assert(popInfoAfter.DepotUser == _P4User);
				Assert(IsPlaceholderFile(srcFile));

				// Confirm that the invalid server has been resolved to a valid server
				FilePopulateInfo popInfoQuery = VirtualFileSystem.QueryFilePopulateInfo(srcFile);
				Assert(popInfoQuery != null);
				Assert(popInfoQuery.DepotServer == validServer);
				Assert(popInfoQuery.DepotClient == _P4Client);
				Assert(popInfoQuery.DepotUser == _P4User);
				Assert(IsPlaceholderFile(srcFile));

				Assert(ReconcilePreview(Path.GetDirectoryName(srcFile)).Any() == false);
				Assert(IsPlaceholderFile(srcFile) == false);
			}

			using (DepotTempFile tempFile = new DepotTempFile(Path.GetTempFileName()))
			{
				Assert(File.Exists(tempFile.LocalFilePath));
				Assert(FileUtilities.GetFileLength(tempFile.LocalFilePath) == 0);
				
				// Testing save XML with modified settings
				SettingNodeMap settings0 = SettingManager.GetProperties();
				SettingManager.ImmediateLogging = !SettingManager.ImmediateLogging;
				SettingManagerExtensions.Verbosity = SettingManagerExtensions.Verbosity + 1;
				SettingManagerExtensions.DepotServerConfig = new DepotServerConfig{ Servers = new[]{ new DepotServerConfigEntry{ Pattern = "MyHost*", Address = "127.0.0.1:1666" }}};
				Assert(ServiceSettings.SaveToFile(tempFile.LocalFilePath));
				Assert(FileUtilities.GetFileLength(tempFile.LocalFilePath) > 0);
				
				// Restore settings to original values and verify match
				SettingNodeMap settings1 = SettingManager.GetProperties();
				Assert(SettingManager.SetProperties(settings0));
				SettingNodeMap settings2 = SettingManager.GetProperties();
				string jsonSettings0 = JsonConvert.SerializeObject(settings0);
				string jsonSettings1 = JsonConvert.SerializeObject(settings1);
				string jsonSettings2 = JsonConvert.SerializeObject(settings2);
				Assert(String.IsNullOrEmpty(jsonSettings0) == false);
				Assert(jsonSettings0 != jsonSettings1);
				Assert(jsonSettings0 == jsonSettings2);

				// Testing load XML with modified settings and verify match
				Assert(ServiceSettings.LoadFromFile(tempFile.LocalFilePath));
				SettingNodeMap settings3 = SettingManager.GetProperties();
				string jsonSettings3 = JsonConvert.SerializeObject(settings3);
				Assert(String.IsNullOrEmpty(jsonSettings3) == false);
				Assert(jsonSettings1 == jsonSettings3);

				// Testing removal of default values in file
				XmlDocument settingsDoc = new XmlDocument();
				settingsDoc.Load(tempFile.LocalFilePath);
				var settingsDocRemove = new Func<string, int>(s => settingsDoc.SelectNodes(".//"+s).OfType<XmlElement>().ToList().Count(n => { n.ParentNode.RemoveChild(n); return true; }));
				Assert(settingsDocRemove(nameof(SettingManager.FileLoggerRemoteDirectory)) == 1);
				Assert(settingsDocRemove(nameof(SettingManager.ReportUsageExternally)) == 1);
				Assert(settingsDocRemove(nameof(SettingManager.RemoteLogging)) == 1);
				Assert(settingsDocRemove(nameof(SettingManager.MaxSyncConnections)) == 1);
				settingsDoc.Save(tempFile.LocalFilePath);

				// Testing load of partial overrides to confirm defaults and overrides
				Assert(SettingManager.SetProperties(settings0));
				Assert(jsonSettings0 == JsonConvert.SerializeObject(SettingManager.GetProperties()));
				Assert(ServiceSettings.LoadFromFile(tempFile.LocalFilePath));
				SettingNodeMap settings4 = SettingManager.GetProperties();
				string jsonSettings4 = JsonConvert.SerializeObject(settings4);
				Assert(jsonSettings1 == jsonSettings4);

				// Restore settings to original values and verify match
				Assert(SettingManager.SetProperties(settings0));
				SettingNodeMap settings5 = SettingManager.GetProperties();
				string jsonSettings5 = JsonConvert.SerializeObject(settings5);
				Assert(String.IsNullOrEmpty(jsonSettings5) == false);
				Assert(jsonSettings0 == jsonSettings5);
			}}}

			SettingManagerExtensions.DepotServerConfig = previousDepotServerConfig;
			AssertCurrentServiceSettings();

			ServiceRestart();
		}

		[TestMethod, Priority(21), TestRemote]
		public void FileCopyTest()
		{
			foreach (ServiceSettingsScope settings in EnumerateCommonServicePopulateSettings())
			{
				using (settings) {
				using (DepotClient depotClient = new DepotClient()) {

				WorkspaceReset();
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));
				string clientRoot = GetClientRoot(depotClient);

				string depotPath = "//depot/gears1/Development/External/nvDXT";
				string clientPath = depotClient.WhereFolder(depotPath).LocalPath;
				int syncChangelist = 10;
				DepotSyncResult syncResults = depotClient.Sync(String.Format("{0}/...", depotPath), new DepotRevisionChangelist(syncChangelist));
				Assert(syncResults?.Modifications?.Count() == 12);
				Assert(syncResults.Modifications.Any(a => a.SyncActionType != DepotSyncActionType.Added) == false);
				string[] srcFiles = Directory.GetFiles(clientPath, "*", SearchOption.AllDirectories);
				Assert(srcFiles.Length == 12);
				Assert(srcFiles.All(f => IsPlaceholderFile(f)));
				Assert(FileUtilities.GetDirectorySize(clientPath) == depotClient.GetDepotSize(new[]{ String.Format("{0}/...@{1}", depotPath, syncChangelist) }));
				Assert(ProcessInfo.ExecuteWait("xcopy.exe", String.Format("/s /f /y \"{0}\" \"{1}\\Temp\\\"", clientPath, clientRoot), echo:true, log:true) == 0);
				foreach (string srcFile in srcFiles)
				{
					string dstFile = srcFile.Replace(clientPath, String.Format("{0}\\Temp", clientRoot));
					Assert(File.Exists(dstFile));
					Assert(IsPlaceholderFile(srcFile) == false);
					string dstFileHash = FileUtilities.GetFileHash(dstFile);
					Assert(String.IsNullOrEmpty(dstFileHash) == false);
					Assert(dstFileHash == FileUtilities.GetFileHash(srcFile));
				}
				Assert(ProcessInfo.ExecuteWait(P4Exe, String.Format("{0} flush -f \"{1}@{2}\"", ClientConfig, depotPath, syncChangelist), echo:true) == 0);
				Assert(ReconcilePreview(depotPath).Any() == false);
			}}}
		}

		[TestMethod, Priority(22), TestRemote]
		public void ServiceSettingNodeTest()
		{
			// Testing SettingNode bool
			{
				SettingNode settingBool = new SettingNode();
				Assert(settingBool.ToString(null) == null);
				Assert(settingBool.ToBool() == false);
				Assert(settingBool.ToBool(true) == true && settingBool.ToBool(false) == false);
				Assert(String.Format("{0}", settingBool.ToString("")) == "");
				
				settingBool = SettingNode.FromBool(true);
				Assert(settingBool.ToString() == "True");
				Assert(settingBool.ToBool(true) == true && settingBool.ToBool(false) == true);
				Assert(String.Format("{0}", settingBool) == "True");
				
				settingBool = SettingNode.FromBool(false);
				Assert(settingBool.ToString() == "False");
				Assert(settingBool.ToBool(true) == false && settingBool.ToBool(false) == false);
				Assert(String.Format("{0}", settingBool) == "False");

				settingBool = SettingNode.FromString("xyz");
				Assert(settingBool.ToString() == "xyz");
				Assert(settingBool.ToBool() == false);
				Assert(settingBool.ToBool(true) == true && settingBool.ToBool(false) == false);
				Assert(String.Format("{0}", settingBool) == "xyz");
			}
			// Testing SettingNode Int32
			{
				SettingNode settingInt32 = new SettingNode();
				Assert(settingInt32.ToInt32(16) == 16);
				Assert(settingInt32.ToInt32() == 0);
				Assert(settingInt32.ToInt32(1) == 1 && settingInt32.ToInt32(2) == 2);
				Assert(String.Format("{0}", settingInt32.ToString("")) == "");
				
				settingInt32 = SettingNode.FromInt32(16);
				Assert(settingInt32.ToString() == "16");
				Assert(settingInt32.ToInt32(8) == 16 && settingInt32.ToInt32(0) == 16);
				Assert(String.Format("{0}", settingInt32) == "16");
			}
			// Testing SettingNode string
			{
				SettingNode settingString = new SettingNode();
				Assert(settingString.ToString() == null);
				Assert(settingString.ToString("") == "");
				Assert(settingString.m_Data == null);
				Assert(String.Format("{0}", settingString.ToString("")) == "");
				
				settingString = SettingNode.FromString("bar");
				Assert(settingString.ToString() == "bar");
				Assert(settingString.ToString("") == "bar");
				Assert(String.Format("{0}", settingString) == "bar");
				
				settingString = SettingNode.FromString("");
				Assert(settingString.ToString() == "");
				Assert(settingString.ToString(null) == "");
				Assert(String.Format("{0}", settingString) == "");
			}
		}

		[TestMethod, Priority(23), TestRemote]
		public void SparseUnderflowCopyTest()
		{
			foreach (ServiceSettingsScope settings in EnumerateCommonServicePopulateSettings())
			{
				using (settings) {
				using (DepotClient depotClient = new DepotClient()) {
				
				WorkspaceReset();
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));
				string clientRoot = GetClientRoot(depotClient);

				string largeFile = String.Format(@"{0}\depot\gears1\Development\Src\Core\Src\Core.cpp", clientRoot);
				depotClient.Sync(largeFile, new DepotRevisionNumber(1), DepotSyncFlags.Normal, DepotSyncMethod.Virtual);
				Int64 largeFileSize = FileUtilities.GetFileLength(largeFile);

				string smallFile = String.Format(@"{0}\depot\gears1\Development\Src\Core\Src\BitArray.cpp", clientRoot);
				depotClient.Sync(smallFile, new DepotRevisionNumber(1), DepotSyncFlags.Normal, DepotSyncMethod.Virtual);
				Int64 smallFileSize = FileUtilities.GetFileLength(smallFile);

				Assert(largeFileSize > smallFileSize);
				Assert(IsPlaceholderFile(largeFile));
				Assert(IsPlaceholderFile(smallFile));

				FilePopulateInfo largePopInfoBefore = NativeMethods.GetFilePopulateInfo(largeFile);
				Assert(largePopInfoBefore.DepotPath == depotClient.Where(largeFile)[0].DepotPath);
				Assert(IsPlaceholderFile(largeFile));

				FilePopulateInfo smallPopInfoBefore = NativeMethods.GetFilePopulateInfo(smallFile);
				Assert(smallPopInfoBefore.DepotPath == depotClient.Where(smallFile)[0].DepotPath);
				Assert(IsPlaceholderFile(smallFile));

				// Write populate info to the large file with large size, as a reference to data from small file
				Assert(NativeMethods.InstallReparsePointOnFile(
					VirtualFileSystem.CurrentVersion.Major,
					VirtualFileSystem.CurrentVersion.Minor,
					VirtualFileSystem.CurrentVersion.Build,
					largeFile,
					(byte)FileResidencyPolicy.Resident,
					smallPopInfoBefore.FileRevision,
					largeFileSize,
					WindowsInterop.FILE_ATTRIBUTE_READONLY,
					smallPopInfoBefore.DepotPath,
					largePopInfoBefore.DepotServer,
					largePopInfoBefore.DepotClient,
					largePopInfoBefore.DepotUser) == 0);

				Assert(FileUtilities.GetFileLength(largeFile) == largeFileSize);
				Assert(IsPlaceholderFile(largeFile));

				byte[] largeFileData = File.ReadAllBytes(largeFile);
				byte[] smallFileData = File.ReadAllBytes(smallFile);

				Assert(smallFileData.Length >= smallFileSize);
				Assert(smallFileData.Length == largeFileData.Length);
				Assert(Enumerable.SequenceEqual(smallFileData, largeFileData));
			}}}
		}

		[TestMethod, Priority(24), TestRemote]
		public void SocketModelCommunicationTest()
		{
			ServiceRestart();
			SettingManager.Reset();
			Assert(ServiceSettings.LoadFromFile(VirtualFileSystem.InstalledSettingsFilePath));
			
			Extensions.SocketModel.SocketModelClient client = new Extensions.SocketModel.SocketModelClient();
			Extensions.SocketModel.SocketModelReplyServiceStatus status = client.GetServiceStatus();
			Assert(status != null);
			Assert(status.IsDriverConnected == true);
			Assert(status.LastModifiedTime > DateTime.MinValue);
			Assert(status.LastRequestTime > DateTime.MinValue);

			SettingNodeMap defaultSettings = SettingManager.GetProperties();
			SettingNodeMap currentSettings = client.GetServiceSettings();
			Assert(currentSettings != null);
			Assert(JsonConvert.SerializeObject(currentSettings) != null);
			Assert(JsonConvert.SerializeObject(currentSettings) == JsonConvert.SerializeObject(defaultSettings));

			Assert(client.SetServiceSetting("SyncDefaultQuiet", SettingNode.FromBool(false)));
			Assert(client.GetServiceSetting("SyncDefaultQuiet").ToString().ToLower() == "false");
			Assert(client.GetServiceSettings()["SyncDefaultQuiet"].ToBool(true) == false);
			Assert(client.SetServiceSetting("SyncDefaultQuiet", SettingNode.FromBool(true)));
			Assert(client.GetServiceSetting("SyncDefaultQuiet").ToString().ToLower() == "true");
			Assert(client.GetServiceSettings()["SyncDefaultQuiet"].ToBool(false) == true);

			Assert(ProcessInfo.ExecuteWait(P4vfsExe, "set SyncDefaultQuiet=false", echo:true) == 0);
			Assert(ProcessInfo.ExecuteWaitOutput(P4vfsExe, "set SyncDefaultQuiet").Lines.Contains("SyncDefaultQuiet=false"));
			Assert(ProcessInfo.ExecuteWaitOutput(P4vfsExe, "set SyncDefaultQuiet").Lines.Count(s => s.Contains("=")) == 1);
			Assert(client.GetServiceSettings()["SyncDefaultQuiet"].ToBool(true) == false);
			Assert(ProcessInfo.ExecuteWait(P4vfsExe, "set SyncDefaultQuiet=true", echo:true) == 0);
			Assert(ProcessInfo.ExecuteWaitOutput(P4vfsExe, "set SyncDefaultQuiet").Lines.Contains("SyncDefaultQuiet=true"));
			Assert(client.GetServiceSettings()["SyncDefaultQuiet"].ToBool(false) == true);

			Assert(client.SetServiceSetting("MaxSyncConnections", SettingNode.FromInt32(10)));
			Assert(client.GetServiceSetting("MaxSyncConnections").ToInt32() == 10);
			Assert(client.GetServiceSettings()["MaxSyncConnections"].ToInt32() == 10);
			Assert(client.SetServiceSetting("MaxSyncConnections", SettingNode.FromString(" 6  ")));
			Assert(client.GetServiceSetting("MaxSyncConnections").ToInt32() == 6);
			Assert(client.GetServiceSettings()["MaxSyncConnections"].ToInt32() == 6);

			Assert(ProcessInfo.ExecuteWait(P4vfsExe, "set MaxSyncConnections=7", echo:true) == 0);
			Assert(ProcessInfo.ExecuteWaitOutput(P4vfsExe, "set MaxSyncConnections").Lines.Contains("MaxSyncConnections=7"));
			Assert(client.GetServiceSetting("MaxSyncConnections").ToString() == "7");
			Assert(ProcessInfo.ExecuteWait(P4vfsExe, "set MaxSyncConnections=  32 ", echo:true) != 0);
			Assert(ProcessInfo.ExecuteWait(P4vfsExe, "set MaxSyncConnections=", echo:true) == 0);
			Assert(client.GetServiceSetting("MaxSyncConnections").ToString() == "");
			Assert(ProcessInfo.ExecuteWait(P4vfsExe, "set MaxSyncConnections=8", echo:true) == 0);
			Assert(ProcessInfo.ExecuteWaitOutput(P4vfsExe, "set MaxSyncConnections").Lines.Contains("MaxSyncConnections=8"));
			Assert(ProcessInfo.ExecuteWait(P4vfsExe, "set MaxSyncConnections=0", echo:true) == 0);
			Assert(ProcessInfo.ExecuteWaitOutput(P4vfsExe, "set MaxSyncConnections").Lines.Contains("MaxSyncConnections=0"));
			Assert(client.GetServiceSettings()["MaxSyncConnections"].ToInt32(-1) == 0);

			Assert(client.SetServiceSetting("SyncResidentPattern", SettingNode.FromString(@"\.(xml|txt)$")));
			Assert(client.GetServiceSettings()["SyncResidentPattern"].ToString() == @"\.(xml|txt)$");
			Assert(client.GetServiceSetting("SyncResidentPattern").ToString() == @"\.(xml|txt)$");
			Assert(client.SetServiceSetting("SyncResidentPattern", SettingNode.FromString("")));
			Assert(client.GetServiceSetting("SyncResidentPattern").ToString() == "");
			Assert(client.GetServiceSettings()["SyncResidentPattern"].ToString(null) == "");

			Assert(ProcessInfo.ExecuteWait(P4vfsExe, "set \"SyncResidentPattern=\\.(png|xml)$\"", echo:true) == 0);
			Assert(ProcessInfo.ExecuteWaitOutput(P4vfsExe, "set SyncResidentPattern").Lines.Contains("SyncResidentPattern=\\.(png|xml)$"));
			Assert(client.GetServiceSettings()["SyncResidentPattern"].ToString("") == @"\.(png|xml)$");
			Assert(ProcessInfo.ExecuteWait(P4vfsExe, "set SyncResidentPattern=", echo:true) == 0);
			Assert(ProcessInfo.ExecuteWaitOutput(P4vfsExe, "set SyncResidentPattern").Lines.Contains("SyncResidentPattern="));
			Assert(client.GetServiceSetting("SyncResidentPattern").ToString() == "");

			Assert(client.SetServiceSetting("NonExistingSetting", SettingNode.FromBool(false)) == false);
			Assert(client.GetServiceSetting("NonExistingSetting") == null);

			RandomFast random = new RandomFast(291481);
			const int maxPackageSize = 1<<25; // 32*1024*1024 (32 MiB)
			for (int packageSize = 1; packageSize <= maxPackageSize; packageSize <<= 5)
			{
				VirtualFileSystemLog.Info("SocketModelCommunicationTest ReflectPackage size {0}", packageSize);
				byte[] packageBytes = random.NextBytes(packageSize);
				byte[] receiveBytes = client.ReflectPackage(packageBytes);
				Assert(receiveBytes?.Length == packageBytes.Length);
				Assert(receiveBytes.SequenceEqual(packageBytes));
			}

			ServiceRestart();
		}

		[TestMethod, Priority(25), TestRemote]
		public void EnvironmentConfigTest()
		{
			WorkspaceReset();

			string clientRoot = GetClientRoot();
			string configFileName = ".p4vfsconfig";
			string environmentUser = "northamerica\\quebec";
			string environmentClient = "quebec-pc-depot";
			string environmentPasswd = UnitTestServer.GetUserP4Passwd(environmentUser);
			string rootFolder = clientRoot;
			string subFolder = Path.Combine(rootFolder, "depot");
			AssertLambda(() => FileUtilities.CreateDirectory(subFolder));

			Action emptyConfigTest = () => {
				using (new SetEnvironmentVariableScope("P4CONFIG", "__non_existing_file__")) {
				using (DepotClient depotClient = new DepotClient())
				{
					depotClient.Unattended = true;
					// Connect from environment without P4CONFIG
					Assert(depotClient.Connect(directoryPath:rootFolder));
					Assert(depotClient.ConnectionClient().Client == environmentClient);
					Assert(depotClient.Info().UserName == environmentUser);
					Assert(ProcessInfo.ExecuteWaitOutput(P4vfsExe, "info", directory:rootFolder, echo:true, log:true).Lines.Contains("P4 UserName: northamerica\\quebec"));
				}}
			};

			Action fullConfigTest = () => {
				using (new SetEnvironmentVariableScope("P4CONFIG", configFileName)) {
				using (DepotClient depotClient = new DepotClient())	
				{
					depotClient.Unattended = true;
					string[] configFileLines = new[]{ "P4CLIENT=p4vfstest-depot", "P4USER=p4vfstest", "P4PORT="+_P4Port };
					AssertLambda(() => File.WriteAllLines(Path.Combine(rootFolder, configFileName), configFileLines));
					// Connect from P4CONFIG in that folder
					Assert(depotClient.Connect(directoryPath:rootFolder));
					Assert(depotClient.ConnectionClient().Client == "p4vfstest-depot");
					Assert(depotClient.Info().UserName == "p4vfstest");
					Assert(ProcessInfo.ExecuteWaitOutput(P4vfsExe, "info", directory:rootFolder, echo:true, log:true).Lines.Contains("P4 UserName: p4vfstest"));
					// Connect from P4CONFIG in a subfolder
					Assert(depotClient.Connect(directoryPath:subFolder));
					Assert(depotClient.ConnectionClient().Client == "p4vfstest-depot");
					Assert(depotClient.Info().UserName == "p4vfstest");
					Assert(ProcessInfo.ExecuteWaitOutput(P4vfsExe, "info", directory:subFolder, echo:true, log:true).Lines.Contains("P4 UserName: p4vfstest"));
					// Override P4TICKETS from P4CONFIG
					string configTicketsFile = Path.Combine(subFolder, "tix.txt");
					AssertLambda(() => FileUtilities.DeleteFile(configTicketsFile));
					AssertLambda(() => File.WriteAllLines(Path.Combine(rootFolder, configFileName), configFileLines.Append($"P4TICKETS={configTicketsFile}").ToArray()));
					Assert(depotClient.Connect(directoryPath:rootFolder) == false);
					Assert(File.Exists(configTicketsFile) && FileUtilities.GetFileLength(configTicketsFile) == 0);
					ProcessInfo.ExecuteResultOutput xr = ProcessInfo.ExecuteWaitOutput(P4Exe, "depots", directory:rootFolder, echo:true);
					Assert(xr.ExitCode != 0 && xr.Data.Any(s => s.Text.Contains("Perforce password (P4PASSWD) invalid or unset.")));
					Assert(depotClient.Connect(directoryPath:rootFolder, depotPasswd:UnitTestServer.GetUserP4Passwd(_P4User)));
					Assert(depotClient.Depots().HasError == false);
					Assert(File.Exists(configTicketsFile) && FileUtilities.GetFileLength(configTicketsFile) > 0);
				}}
			};

			using (new SetEnvironmentVariableScope("P4USER", environmentUser)) {
			using (new SetEnvironmentVariableScope("P4CLIENT", environmentClient)) {
			using (new SetEnvironmentVariableScope("P4PASSWD", environmentPasswd)) {
			using (new SetEnvironmentVariableScope("P4PORT", _P4Port)) 
			{
				emptyConfigTest();
				fullConfigTest();
			}}}}

			fullConfigTest();
		}

		[TestMethod, Priority(26), TestRemote]
		public void SyncEmptyFileSpecTest()
		{
			using (DepotClient depotClient = new DepotClient()) 
			{
				WorkspaceReset();
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));
				
				string clientRoot = GetClientRoot(depotClient);
				string clientFolder = String.Format(@"{0}\depot\gears1\Development\Src\Core\Src", clientRoot);
				int clientFileCount = 46;

				var AssertSyncEmpty = new Action<Func<DepotSyncResult>>(doSync =>
				{
					DepotSyncResult syncResult = depotClient.Sync("//...", null, DepotSyncFlags.Flush|DepotSyncFlags.Quiet);
					Assert(syncResult != null && syncResult.Modifications != null);
					Assert((syncResult.Status == DepotSyncStatus.Success && syncResult.Modifications.Length > clientFileCount) || 
						   (syncResult.Status == DepotSyncStatus.Success && syncResult.Modifications.Length == 1 && syncResult.Modifications[0].SyncActionType == DepotSyncActionType.UpToDate));

					syncResult = depotClient.Sync("//...");
					Assert(syncResult?.Status == DepotSyncStatus.Success);
					Assert(syncResult.Modifications?.Count() == 1);
					Assert(syncResult.Modifications.First().SyncActionType == DepotSyncActionType.UpToDate);

					syncResult = depotClient.Sync(String.Format("{0}\\...#none", clientFolder));
					Assert(syncResult?.Status == DepotSyncStatus.Success);
					Assert(syncResult.Modifications?.Count() == clientFileCount);
					Assert(Directory.Exists(clientFolder) == false);
					Assert(ReconcilePreview(clientFolder).Any() == false);

					syncResult = doSync();
					Assert(syncResult == null || syncResult?.Status == DepotSyncStatus.Success);
					Assert(syncResult == null || syncResult.Modifications.Count() == clientFileCount);
					Assert(FileUtilities.GetDirectoryFileCount(clientFolder) == clientFileCount);
					Assert(ReconcilePreview(clientFolder).Any() == false);
				});

				foreach (DepotSyncMethod syncMethod in new[]{ DepotSyncMethod.Regular, DepotSyncMethod.Virtual })
				{
					AssertSyncEmpty(() => depotClient.Sync("", null, DepotSyncFlags.Normal, syncMethod));
					AssertSyncEmpty(() => depotClient.Sync(new DepotSyncOptions{ SyncFlags=DepotSyncFlags.Normal, SyncMethod=syncMethod }));
				}

				foreach (string syncOption in EnumerateCommonConsoleSyncOptions())
				{
					AssertSyncEmpty(() => 
					{
						Assert(ProcessInfo.ExecuteWait(P4vfsExe, String.Format("{0} sync {1}", ClientConfig, syncOption), echo:true, log:true) == 0);
						return null;
					});
				}
			}
		}

		[TestMethod, Priority(27), TestRemote]
		public void EnvironmentDiffTest()
		{
			foreach (string diff in new[]{"", "_non_existing_p4vfs_app_.exe", "p4merge.exe"})
			{
				using (new SetEnvironmentVariableScope("P4DIFF", diff)) {
				using (DepotClient depotClient = new DepotClient()) 
				{
					WorkspaceReset();
					Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));
					string clientRoot = GetClientRoot(depotClient);

					Assert(depotClient.GetEnv("P4DIFF") == diff || diff == "");
					Assert(depotClient.GetEnv("_NON_EXISTING_P4VFS_EVAR_") == "");
					Assert(depotClient.SetEnv("P4DIFF", null));
					Assert(depotClient.GetEnv("P4DIFF") == "");
					Assert(depotClient.SetEnv("P4DIFF", diff));
					Assert(depotClient.GetEnv("P4DIFF") == diff);

					string writableFile = String.Format(@"{0}\depot\tools\dev\bin\studio\maya\scripts\python\Utilities\shaderTools.py", clientRoot);
					depotClient.Sync(writableFile, new DepotRevisionNumber(4));
					Assert(IsPlaceholderFile(writableFile));
					DepotResultFStat writableFileStat = depotClient.FStat(new[]{ writableFile });
					Assert(writableFileStat.Count == 1 && writableFileStat[0].HaveRev == 4);
					Assert(DiffAgainstWorkspace(String.Format("{0}#4", writableFile)).LinesTotal == 0);
					FileUtilities.MakeWritable(writableFile);
					File.AppendAllLines(writableFile, new[]{"\nThis is one line\nThis is another"});
					Assert(DiffAgainstWorkspace(String.Format("{0}#4", writableFile)).LinesTotal == 3);
				}
			}}
		}

		[TestMethod, Priority(28), TestRemote]
		public void ConnectInvalidConfigTest()
		{
			WorkspaceReset();
			using (DepotClient depotClient = new DepotClient()) 
			{
				DepotConfig config = new DepotConfig{ Port=_P4Port, Client="__invalid_p4vfs_client__", User=_P4User };
				Assert(depotClient.Connect(config));
				DepotResultInfo.Node info = depotClient.Info();
				Assert(info.UserName == config.User);
				Assert(info.ClientName == "*unknown*");
				DepotResultClient.Node connection = depotClient.ConnectionClient();
				Assert(connection.Access == null);
				Assert(connection.Owner == config.User);
				Assert(connection.Client == config.Client);
			}
			using (DepotClient depotClient = new DepotClient()) 
			{
				DepotConfig config = new DepotConfig{ Port=_P4Port, Client="__invalid_p4vfs_client__", User="__invalid_user__" };
				Assert(depotClient.Connect(config));
				DepotResultInfo.Node info = depotClient.Info();
				Assert(String.IsNullOrEmpty(info.UserName) == false || info.UserName != "*unknown*");
				Assert(info.ClientName == "*unknown*");
				DepotResultClient.Node connection = depotClient.ConnectionClient();
				Assert(connection.Access == null);
				Assert(connection.Owner == info.UserName);
				Assert(connection.Client == config.Client);
			}
			using (DepotClient depotClient = new DepotClient()) 
			{
				DepotConfig config = new DepotConfig{ Port=_P4Port, Client="__invalid_p4vfs_client__" };
				Assert(depotClient.Connect(config));
				DepotResultInfo.Node info = depotClient.Info();
				Assert(String.IsNullOrEmpty(info.UserName) == false || info.UserName != "*unknown*");
				Assert(info.ClientName == "*unknown*");
				DepotResultClient.Node connection = depotClient.ConnectionClient();
				Assert(connection.Access == null);
				Assert(connection.Owner == info.UserName);
				Assert(connection.Client == config.Client);
			}
			using (DepotClient depotClient = new DepotClient()) 
			{
				DepotConfig config = new DepotConfig{ Port=_P4Port, User="__invalid_user__" };
				Assert(depotClient.Connect(config) == false);
			}
			using (DepotClient depotClient = new DepotClient()) 
			{
				DepotConfig config = new DepotConfig{ Port=_P4Port, Client=_P4Client };
				Assert(depotClient.Connect(config));
				DepotResultInfo.Node info = depotClient.Info();
				Assert(String.IsNullOrEmpty(info.UserName) == false || info.UserName != "*unknown*");
				Assert(info.ClientName == config.Client);
				DepotResultClient.Node connection = depotClient.ConnectionClient();
				Assert(String.IsNullOrEmpty(connection.Access) == false);
				Assert(connection.Owner == info.UserName);
				Assert(connection.Client == config.Client);
			}
			using (DepotClient depotClient = new DepotClient()) 
			{
				DepotConfig config = new DepotConfig{ Port=_P4Port, User=_P4User };
				Assert(depotClient.Connect(config));
				DepotResultInfo.Node info = depotClient.Info();
				Assert(info.UserName == config.User);
				Assert(info.ClientName == "*unknown*");
				DepotResultClient.Node connection = depotClient.ConnectionClient();
				Assert(connection.Access == null);
				Assert(connection.Owner == config.User);
				Assert(String.IsNullOrEmpty(connection.Client) == false);
			}
		}

		[TestMethod, Priority(29), TestRemote]
		public void LongPathSyncTest()
		{
			foreach (ServiceSettingsScope settings in EnumerateCommonServiceSyncSettings())
			{
				using (settings) {
				using (DepotClient depotClient = new DepotClient()) {
				DepotSyncFlags syncFlags = settings.SyncFlags.Value;

				WorkspaceReset();
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));
				string clientRoot = GetClientRoot(depotClient);

				DepotSyncResult syncResult = depotClient.Sync("//depot/tools/dev/external/packages/thirdparty/microsoft/...", null, syncFlags, DepotSyncMethod.Virtual);
				Assert(syncResult?.Status == DepotSyncStatus.Success);
				Assert(syncFlags.HasFlag(DepotSyncFlags.IgnoreOutput) ? syncResult.Modifications == null : syncResult.Modifications.Count() == 11);
				string[] srcFiles = Directory.GetFiles(clientRoot, "*", SearchOption.AllDirectories);
				Assert(srcFiles.Length == 11);
				foreach (string srcFile in srcFiles)
				{
					Assert(File.Exists(srcFile));
					Assert(IsPlaceholderFile(srcFile));
				}
				Assert(ReconcilePreview(clientRoot).Any() == false);
				foreach (string srcFile in srcFiles)
				{
					Assert(File.Exists(srcFile));
					Assert(IsPlaceholderFile(srcFile) == false);
				}
			}}}
		}

		[TestMethod, Priority(30), TestRemote]
		public void InstallPlaceholderConflictTest()
		{
			var AssertSyncConflict = new Action<DepotClient, string, int, int, DepotSyncMethod>((depotClient, depotFile, startRev, endRev, syncMethod) =>
			{
				WorkspaceReset();

				DepotResultFStat.Node startFStat = depotClient.FStat(String.Format("{0}#{1}", depotFile, startRev)).First;
				Assert(startFStat.DepotFile == depotFile && startFStat.HeadRev == startRev);
				DepotResultFStat.Node endFStat = depotClient.FStat(String.Format("{0}#{1}", depotFile, endRev)).First;
				Assert(endFStat.DepotFile == depotFile && endFStat.HeadRev == endRev);

				// Sync the file to the start revision and verify consistent state
				string clientFile = startFStat.ClientFile;
				Assert(String.IsNullOrEmpty(clientFile) == false);
				DepotSyncResult syncResult = depotClient.Sync(depotFile, new DepotRevisionNumber(startRev), DepotSyncFlags.Normal, syncMethod);
				Assert(syncResult.Status == DepotSyncStatus.Success);
				Assert(syncResult.Modifications?.Count() == 1);
				Assert(IsPlaceholderFile(clientFile) == (syncMethod == DepotSyncMethod.Virtual));
				Assert(FileUtilities.IsReadOnly(clientFile) == !DepotInfo.IsWritableFileType(startFStat.HeadType));

				// Open the file for exclusive read
				using (FileStream stream = File.OpenRead(clientFile))
				{
					// Sync the file to the end revision, which will fail, and verify consistent state still at start revision
					syncResult = depotClient.Sync(depotFile, new DepotRevisionNumber(endRev), DepotSyncFlags.Normal, syncMethod);
					Assert(syncResult.Status == DepotSyncStatus.Error);
					Assert(syncResult.Modifications?.Count() > 0);
					Assert(IsPlaceholderFile(clientFile) == false);
					Assert(ReconcilePreview(clientFile).Any() == false);
					Assert(depotClient.FStat(clientFile).First.HaveRev == startRev);
					Assert(FileUtilities.IsReadOnly(clientFile) == !DepotInfo.IsWritableFileType(startFStat.HeadType));
				}

				// Sync the file to the end revision, which will succeed, and verify consistent state
				syncResult = depotClient.Sync(depotFile, new DepotRevisionNumber(endRev), DepotSyncFlags.Normal, syncMethod);
				Assert(syncResult.Status == DepotSyncStatus.Success);
				Assert(syncResult.Modifications?.Count() == 1);
				Assert(IsPlaceholderFile(clientFile) == (syncMethod == DepotSyncMethod.Virtual));
				Assert(ReconcilePreview(clientFile).Any() == false);
				Assert(depotClient.FStat(clientFile).First.HaveRev == endRev);
				Assert(FileUtilities.IsReadOnly(clientFile) == !DepotInfo.IsWritableFileType(endFStat.HeadType));
			});

			using (DepotTunableScope p4apiSysRenameMax = new DepotTunableScope("sys.rename.max", 10)) {
			using (DepotTunableScope p4apiSysRenameWait = new DepotTunableScope("sys.rename.wait", 50)) {
			using (DepotClient depotClient = new DepotClient()) 
			{
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));
				foreach (DepotSyncMethod syncMethod in Enum.GetValues(typeof(DepotSyncMethod)).Cast<DepotSyncMethod>())
				{
					// Testing sync filetype <text> to <text>
					AssertSyncConflict(depotClient, "//depot/gears1/Development/Src/Engine/Src/UnCanvas.cpp", 1, 2, syncMethod);
					// Testing sync filetype <text> to <text+w>
					AssertSyncConflict(depotClient, "//depot/tools/dev/bin/studio/maya/scripts/python/Utilities/profiling.py", 1, 2, syncMethod);
					// Testing sync filetype <text+w> to <text>
					AssertSyncConflict(depotClient, "//depot/tools/dev/bin/studio/maya/scripts/python/Utilities/profiling.py", 2, 1, syncMethod);
				}
			}}}
		}

		[TestMethod, Priority(31)]
		public void DepotTrustConnectionTest()
		{
			string depotPasswd = UnitTestServer.GetUserP4Passwd(_P4User);	
			string sslPort = String.Format("ssl:localhost:2666");
			DepotConfig sslConfig = new DepotConfig(){ Port = sslPort, Client = _P4Client, User = _P4User };
			
			string trustFile = String.Format("{0}\\p4trust.txt", UnitTestServer.GetServerRootFolder(sslPort));
			FileUtilities.DeleteFile(trustFile);

			// Create a duplicate SSL server
			UnitTestServer.CreateDuplicatePerforceServer(sslPort);
			WorkspaceReset(sslPort, _P4Client, _P4User);

			using (new SetEnvironmentVariableScope(DepotConstants.P4TRUST, trustFile)) 
			{
				FileUtilities.DeleteFile(trustFile);
				ProcessInfo.ExecuteResultOutput xr;

				// With our new P4TRUST file, verify that p4 requires a trust
				xr = ProcessInfo.ExecuteWaitOutput(P4Exe, String.Format("{0} logout", sslConfig), echo:true);
				Assert(xr.ExitCode != 0 && xr.Text.Contains("may be your first attempt to connect to this P4PORT") && xr.Text.Contains("use the 'p4 trust' command"));

				// Verify that DepotClient will automatically trust and connect
				using (DepotClient depotClient = new DepotClient())
				{
					Assert(depotClient.Connect(sslConfig.Port, sslConfig.Client, sslConfig.User));
					Assert(depotClient.Depots().HasError == false);
				}

				// Logout and verify password is required
				xr = ProcessInfo.ExecuteWaitOutput(P4Exe, String.Format("{0} logout", sslConfig), echo:true);
				Assert(xr.ExitCode == 0);
				xr = ProcessInfo.ExecuteWaitOutput(P4Exe, String.Format("{0} depots", sslConfig), echo:true);
				Assert(xr.ExitCode != 0 && xr.Data.Any(s => s.Text.Contains("Perforce password (P4PASSWD) invalid or unset.")));

				// Verify that DepotClient with trust and login given the password
				using (DepotClient depotClient = new DepotClient())
				{
					Assert(depotClient.Connect(sslConfig.Port, sslConfig.Client, sslConfig.User, depotPasswd:depotPasswd));
					Assert(depotClient.Depots().HasError == false);
				}

				// Verify that p4 also trusts this connection now
				xr = ProcessInfo.ExecuteWaitOutput(P4Exe, String.Format("{0} depots", sslConfig), echo:true);
				Assert(xr.ExitCode == 0);

				// Alter the SSL fingerprint
				string[] oldTrustLines = File.ReadAllLines(trustFile);
				string[] newTrustLines = oldTrustLines.Select(
					line => Regex.Replace(line, @"[\dA-F]{2}\s*$", new MatchEvaluator(
					m => { byte b = byte.Parse(m.Value, System.Globalization.NumberStyles.HexNumber); return (b^255).ToString("X"); }), RegexOptions.IgnoreCase)).ToArray();
				FileUtilities.MakeWritable(trustFile);
				File.WriteAllLines(trustFile, newTrustLines);
				FileUtilities.MakeReadOnly(trustFile);

				// After changing the fingerprint, verify that p4 requires a trust
				xr = ProcessInfo.ExecuteWaitOutput(P4Exe, String.Format("{0} logout", sslConfig), echo:true);
				Assert(xr.ExitCode != 0 && xr.Text.Contains("fingerprint for the mismatched key sent") && xr.Text.Contains("use the 'p4 trust' command"));

				// Verify that DepotClient will automatically trust and connect with a different fingerprint
				using (DepotClient depotClient = new DepotClient())
				{
					Assert(depotClient.Connect(sslConfig.Port, sslConfig.Client, sslConfig.User));
					Assert(depotClient.Depots().HasError == false);
				}

				// Verify that p4 also trusts this connection now
				xr = ProcessInfo.ExecuteWaitOutput(P4Exe, String.Format("{0} depots", sslConfig), echo:true);
				Assert(xr.ExitCode == 0);
			}
		}

		[TestMethod, Priority(32), TestRemote]
		public void DepotClientCacheIdleTimeoutTest()
		{
			WorkspaceReset();

			Action<Extensions.SocketModel.SocketModelClient, DepotConfig> assertSyncAndReconcile = (service, config) =>
			{
				string depotFolder = "//depot/gears1/Development/Src/Core";

				// Perform a sync through the service. We expect connections to be closed when done (not cached)
				Assert(service.Sync(config, new DepotSyncOptions{ Files=new[]{ depotFolder+"/..." }, SyncFlags=DepotSyncFlags.Force, SyncMethod=DepotSyncMethod.Virtual }) == DepotSyncStatus.Success);

				// Reconcile the folder using p4.exe. There will be one or more cached service connections
				Assert(ReconcilePreview(depotFolder).Any() == false);
				Assert(GetServiceIdleConnectionCount() > 1);
			};

			// There should be no idle connections from any process, including this process
			Assert(GetServiceIdleConnectionCount() == 0);
			using (DepotClient depotClient = new DepotClient())
			{
				Extensions.SocketModel.SocketModelClient service = new Extensions.SocketModel.SocketModelClient(); 

				// Open a single idle connection
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));
				Assert(GetServiceIdleConnectionCount() == 1);
				
				Assert(ServiceGarbageCollect());
				Assert(GetServiceIdleConnectionCount() == 1);

				// Sync and reconcile to open some cached service connections
				assertSyncAndReconcile(service, depotClient.Config());
				Assert(GetServiceIdleConnectionCount() > 1);

				// Close the idle service connections with a GC
				Assert(ServiceGarbageCollect());
				Assert(GetServiceIdleConnectionCount() == 1);

				// Write a temporary PublicSettingsFilePath settings file with very short GC timeout
				using (new LocalSettingScope(nameof(SettingManager.GarbageCollectPeriodMs), "100")) {
				using (new LocalSettingScope(nameof(SettingManager.DepotClientCacheIdleTimeoutMs), "5000")) {
				using (new DepotTempFile(VirtualFileSystem.PublicSettingsFilePath))
				{
					Assert(ServiceSettings.SaveToFile(VirtualFileSystem.PublicSettingsFilePath));
					Assert(File.Exists(VirtualFileSystem.PublicSettingsFilePath));

					// Restart the service to load our new settings
					ServiceRestart();
					Assert(GetServiceIdleConnectionCount() == 1);

					Assert(service.GetServiceSetting(nameof(SettingManager.GarbageCollectPeriodMs)).ToInt32() == SettingManager.GarbageCollectPeriodMs);
					Assert(service.GetServiceSetting(nameof(SettingManager.DepotClientCacheIdleTimeoutMs)).ToInt32() == SettingManager.DepotClientCacheIdleTimeoutMs);

					// Sync and reconcile to open some cached service connections
					assertSyncAndReconcile(service, depotClient.Config());
					Assert(GetServiceIdleConnectionCount() > 1);

					// Wait at least DepotClientCacheIdleTimeoutMs for the service connections to be closed
					AssertRetry(() => GetServiceIdleConnectionCount() == 1, "Waiting for GC", retryWait:500, limitWait:8000);
				}}}
			
				// Restart the service to load base settings again
				Assert(File.Exists(VirtualFileSystem.PublicSettingsFilePath) == false);
				ServiceRestart();
				Assert(GetServiceIdleConnectionCount() == 1);

				Assert(service.GetServiceSetting(nameof(SettingManager.GarbageCollectPeriodMs)).ToInt32() == SettingManager.GarbageCollectPeriodMs);
				Assert(service.GetServiceSetting(nameof(SettingManager.DepotClientCacheIdleTimeoutMs)).ToInt32() == SettingManager.DepotClientCacheIdleTimeoutMs);
			}
		}

		[TestMethod, Priority(33)]
		public void DepotOperationsReconfigTest()
		{
			WorkspaceReset();
			using (DepotClient sourceDepotClient = new DepotClient())
			{
				Assert(sourceDepotClient.Connect(_P4Port, _P4Client, _P4User));
				DepotConfig sourceConfig = sourceDepotClient.Config();

				string clientRoot = GetClientRoot(sourceDepotClient);
				string clientFolder = Path.Combine(clientRoot, "depot\\gears1\\Development\\Src");
				string clientSubFolder = Path.Combine(clientFolder, "Core\\Inc");

				DepotSyncResult syncResult = sourceDepotClient.Sync(String.Format("{0}\\...", clientFolder), null, DepotSyncFlags.Normal, DepotSyncMethod.Virtual);
				Assert(syncResult?.Status == DepotSyncStatus.Success);
				Assert(ReconcilePreview(clientSubFolder).Any() == false);

				var files = new List<Tuple<string, FileAttributes, long>>();
				foreach (string filePath in Directory.EnumerateFiles(clientRoot, "*", SearchOption.AllDirectories))
				{
					files.Add(new Tuple<string, FileAttributes, long>(filePath, FileUtilities.GetAttributes(filePath), FileUtilities.GetFileLength(filePath)));
				}

				Action<DepotConfig> assertPlaceholderFileConfig = (DepotConfig fileConfig) =>
				{
					System.Threading.Tasks.Parallel.ForEach(files, file =>
					{
						string filePath = file.Item1;
						FileAttributes attributeMask = ~FileAttributes.Archive;
						Assert((file.Item2 & attributeMask) == (FileUtilities.GetAttributes(filePath) & attributeMask));
						Assert(file.Item3 == FileUtilities.GetFileLength(filePath));
						bool isPlaceholder = IsPlaceholderFile(filePath);
						Assert(isPlaceholder != filePath.StartsWith(String.Format("{0}\\", clientSubFolder), StringComparison.InvariantCultureIgnoreCase));
						if (isPlaceholder)
						{
							FilePopulateInfo info = NativeMethods.GetFilePopulateInfo(filePath);
							Assert(info != null);
							Assert(info.DepotServer == fileConfig.Port);
							Assert(info.DepotClient == fileConfig.Client);
							Assert(info.DepotUser == fileConfig.User);
							Assert(IsPlaceholderFile(filePath));
						}
					});
				};

				assertPlaceholderFileConfig(sourceConfig);

				string portName = sourceConfig.PortName();
				string portNumber = sourceConfig.PortNumber();
				Assert(System.Net.IPAddress.TryParse(portName, out System.Net.IPAddress _) == false);
				string portIP = UnitTestServer.GetServerPortIPAddress(portName)?.ToString();
				Assert(System.Net.IPAddress.TryParse(portIP, out System.Net.IPAddress _) == true);

				DepotConfig targetConfig = new DepotConfig();
				targetConfig.Port = String.Format("{0}:{1}", portIP, portNumber);
				targetConfig.Client = sourceConfig.Client;
				targetConfig.User = sourceConfig.User;

				InteractiveOverridePasswd = UnitTestServer.GetUserP4Passwd(_P4User);
				using (DepotClient targetDepotClient = new DepotClient())
				{
					Assert(targetDepotClient.Connect(targetConfig));
					Assert(targetDepotClient.Config().Port == targetConfig.Port);

					Assert(DepotOperations.Reconfig(targetDepotClient, new DepotReconfigOptions{ Files=new[]{ String.Format("{0}\\...", clientFolder) }, Flags=DepotReconfigFlags.P4Port|DepotReconfigFlags.Preview }));
					assertPlaceholderFileConfig(sourceConfig);
					Assert(DepotOperations.Reconfig(targetDepotClient, new DepotReconfigOptions{ Files=new[]{ String.Format("{0}\\...", clientFolder) }, Flags=DepotReconfigFlags.P4Port }));
					assertPlaceholderFileConfig(targetConfig);

					Assert(ProcessInfo.ExecuteWait(P4vfsExe, String.Format("{0} reconfig -p -n \"{1}\\...\"", targetConfig, clientFolder), echo:true, log:true) == 0);
					assertPlaceholderFileConfig(targetConfig);
					Assert(ProcessInfo.ExecuteWait(P4vfsExe, String.Format("{0} reconfig -p \"{1}\\...\"", sourceConfig, clientFolder), echo:true, log:true) == 0);
					assertPlaceholderFileConfig(sourceConfig);
					Assert(ProcessInfo.ExecuteWait(P4vfsExe, String.Format("{0} reconfig \"{1}\\...\"", targetConfig, clientFolder), echo:true, log:true) == 0);
					assertPlaceholderFileConfig(targetConfig);
				}

				Assert(ReconcilePreview(clientRoot).Any() == false);
				InteractiveOverridePasswd = null;
			}
		}

		[TestMethod, Priority(34), TestRemote]
		public void NonAsciiFilePathTest()
		{
			HashSet<string> expectedFileSet = new HashSet<string>()
			{
				"\\français\\LaCédille.txt",
				"\\français\\Médecin.txt",
				"\\français\\deuxième.txt",
				"\\français\\forêt.txt",
				"\\français\\coïncidence.txt",
				"\\anglaise\\microsoft©.txt",
				"\\anglaise\\xbox—one.txt",
			};

			foreach (ServiceSettingsScope settings in EnumerateCommonServiceSyncSettings())
			{
				using (settings) {
				using (DepotClient depotClient = new DepotClient()) {
				DepotSyncFlags syncFlags = settings.SyncFlags.Value;

				WorkspaceReset();
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));
				string clientRoot = GetClientRoot(depotClient);
				string clientFolder = String.Format("{0}\\depot\\tools\\dev\\documentation", clientRoot);

				DepotSyncResult syncResult = depotClient.Sync(String.Format("{0}\\...", clientFolder), null, syncFlags, DepotSyncMethod.Virtual);
				Assert(syncResult?.Status == DepotSyncStatus.Success);
				Assert(syncFlags.HasFlag(DepotSyncFlags.IgnoreOutput) ? syncResult.Modifications == null : syncResult.Modifications.Count() == 7);
				string[] srcFiles = Directory.GetFiles(clientRoot, "*", SearchOption.AllDirectories);
				Assert(srcFiles.Length == 7);
				HashSet<string> srcFileSet = srcFiles.Select(path => Regex.Replace(path, String.Format(@"^{0}", Regex.Escape(clientFolder)), "", RegexOptions.IgnoreCase)).ToHashSet();
				Assert(expectedFileSet.Except(srcFileSet).Any() == false);

				Assert(ReconcilePreview(clientRoot).Any() == false);
			}}}
		}

		[TestMethod, Priority(35), TestRemote]
		public void SyncClientSizeTest()
		{
			foreach (ServiceSettingsScope settings in EnumerateCommonServiceSyncLineEndSettings(false))
			{
				settings.SyncFlags = settings.SyncFlags.Value | DepotSyncFlags.ClientSize;
				VirtualFileSystemLog.Info("EnumerateCommonServiceSyncLineEndSettings: {0}", settings);

				using (settings) {
				using (DepotClient depotClient = new DepotClient()) {
				
				WorkspaceReset();
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));
				string clientRoot = GetClientRoot(depotClient);
				string clientFolder = String.Format("{0}\\depot\\gears1\\Development\\External", clientRoot);

				DepotResultClient.Node client = depotClient.Client();
				client.Fields["LineEnd"] = settings.LineEnd;
				Assert(client.LineEnd == settings.LineEnd);
				Assert(depotClient.UpdateClient(client).HasError == false);
				client = depotClient.Client();
				Assert(client.LineEnd == settings.LineEnd);

				DepotSyncResult syncResult = depotClient.Sync(String.Format("{0}\\...", clientFolder), null, settings.SyncFlags.Value, DepotSyncMethod.Virtual);
				Assert(syncResult?.Status == DepotSyncStatus.Success);

				Dictionary<string, long> placeholderSizeMap = new Dictionary<string, long>();
				foreach (string filePath in Directory.GetFiles(clientRoot, "*", SearchOption.AllDirectories))
				{
					placeholderSizeMap[filePath] = FileUtilities.GetFileLength(filePath);
					Assert(IsPlaceholderFile(filePath));
				}

				Assert(placeholderSizeMap.Count == 22);
				Assert(ReconcilePreview(clientRoot).Any() == false);

				foreach (string filePath in placeholderSizeMap.Keys)
				{
					long hydrateSize = FileUtilities.GetFileLength(filePath);
					Assert(placeholderSizeMap[filePath] == hydrateSize, $"ClientSize mismatch {filePath}");
				}
			}}}
		}

		[TestMethod, Priority(36)]
		public void ShellLoginTimeoutTest()
		{
			WorkspaceReset();
			Assert(ShellUtilities.IsProcessElevated());

			string workingFolder = String.Format("{0}\\{1}", UnitTestServer.GetServerRootFolder(), nameof(ShellLoginTimeoutTest));
			AssertLambda(() => FileUtilities.DeleteDirectoryAndFiles(workingFolder));
			AssertLambda(() => Directory.CreateDirectory(workingFolder));

			string shellCommandFile = String.Format("{0}\\ShellCommand.bat", workingFolder);
			string shellCommandUri = String.Format("file:///{0}", shellCommandFile.Replace('\\','/'));
			Assert(Uri.IsWellFormedUriString(shellCommandUri, UriKind.Absolute));

			var assertShellLogin = new Action<int,int,bool>((int cmdTimeout, int shellTimeout, bool native) =>
			{
				VirtualFileSystemLog.Info($"assertShellLogin cmdTimeout={cmdTimeout} shellTimeout={shellTimeout} native={native}");
				FileUtilities.DeleteFile(shellCommandFile);
				
				string shellOutputFile = String.Format("{0}\\{1}-{2}.txt", workingFolder, Path.GetFileNameWithoutExtension(shellCommandFile), DateTime.Now.Ticks);
				FileUtilities.DeleteFile(shellOutputFile);

				File.WriteAllLines(shellCommandFile, new string[]{
					$"fltmc.exe > {shellOutputFile}",
					$"timeout.exe /nobreak {cmdTimeout} > nul 2>&1",
					$"echo done >> {shellOutputFile}",
				});

				bool expectTimeout = cmdTimeout > shellTimeout;
				System.Text.StringBuilder loginOutput = new System.Text.StringBuilder();
				string loginExe = P4vfsExe;
				string loginArgs = String.Format("{0} login -t {1} -u {2}", ClientConfig, shellTimeout, shellCommandUri);

				if (native)
				{
					ProcessExecuteFlags flags = ProcessExecuteFlags.HideWindow | ProcessExecuteFlags.WaitForExit;
					bool loginSuccess = NativeMethods.CreateProcessImpersonated($"{loginExe} {loginArgs}", null, flags, loginOutput, null);
					Assert(loginSuccess);
				}
				else
				{
					int loginExitCode = ProcessInfo.ExecuteWait(loginExe, loginArgs, echo:true, stdout:loginOutput);
					Assert(loginExitCode == 0);
				}

				Assert(Regex.IsMatch(loginOutput.ToString(), "timeout waiting", RegexOptions.IgnoreCase) == expectTimeout);
				Assert(File.Exists(shellOutputFile));
				Assert(Regex.IsMatch(File.ReadAllText(shellOutputFile), @"p4vfsflt\s+\d+\s+\d+"));
			});

			foreach (bool native in new[]{ true, false })
			{
				assertShellLogin(10, 30, native);
				assertShellLogin(30, 10, native);
			}
		}
	}
}
