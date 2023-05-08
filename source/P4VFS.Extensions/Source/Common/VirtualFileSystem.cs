// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using Microsoft.P4VFS.Extensions.Linq;
using Microsoft.P4VFS.Extensions.Utilities;
using Microsoft.P4VFS.CoreInterop;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;

namespace Microsoft.P4VFS.Extensions
{
	public static class VirtualFileSystem
	{
		public static readonly string DriverTitle = NativeConstants.DriverTitle;
		public static readonly string ServiceTitle = NativeConstants.ServiceTitle;
		public static readonly string MonitorTitle = NativeConstants.MonitorTitle;
		public static readonly string AppRegistryKey = "SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\P4VFS";
		public static readonly string DriverRegistryKey = String.Format("SYSTEM\\CurrentControlSet\\Services\\{0}", DriverTitle);
		public static readonly string ServiceRegistryKey = String.Format("SYSTEM\\CurrentControlSet\\Services\\{0}", ServiceTitle);
		public static readonly string SettingsFile = "P4VFS.Settings.xml";
		private static SocketModel.SocketModelServer _SocketModelServer;
		private static VersionDescriptor _CurrentVersion;
		private static ServiceHost _ServiceHost;


		static VirtualFileSystem()
		{
			ServiceSettings.Reset();
			_CurrentVersion = new VersionDescriptor(System.Reflection.Assembly.GetExecutingAssembly().GetName().Version.ToString());
		}

		public static bool LoadDriver()
		{
			return DriverOperations.LoadFilter(DriverTitle) == 0;
		}

		public static bool UnloadDriver()
		{
			return DriverOperations.UnloadFilter(DriverTitle) == 0;
		}

		public static bool IsDriverLoaded()
		{
			return DriverOperations.IsFilterLoaded(DriverTitle);
		}

		public static bool IsDriverReady()
		{
			bool isConnected = false;
			if (NativeMethods.GetDriverIsConnected(ref isConnected) == false || isConnected == false)
			{
				return false;
			}
			return true;
		}

		public static bool IsVirtualFileSystemAvailable()
		{
			return GetVirtualFileSystemStatus() == VirtualFileSystemStatus.Available;
		}

		public static VirtualFileSystemStatus GetVirtualFileSystemStatus()
		{
			// Check the driver is installed
			string displayName = null;
			if (Utilities.RegistryInfo.GetTypedValue<string>(Microsoft.Win32.Registry.LocalMachine, DriverRegistryKey, "DisplayName", ref displayName) == false)
			{
				return VirtualFileSystemStatus.DriverNotInstalled;
			}

			// Check the Virtual File System driver service is installed
			string driverService = null;
			if (Utilities.RegistryInfo.GetTypedValue<string>(Microsoft.Win32.Registry.LocalMachine, ServiceRegistryKey, "DisplayName", ref driverService) == false)
			{
				return VirtualFileSystemStatus.ServiceNotInstalled;
			}

			// Check the Virtual File System Driver Service is running
			if (Utilities.ServiceInfo.IsServiceRunning(ServiceTitle) == false)
			{
				return VirtualFileSystemStatus.ServiceNotRunning;
			}

			// Check if the driver is connected and ready
			if (IsDriverReady() == false)
			{
				return VirtualFileSystemStatus.DriverNotReady;
			}

			return VirtualFileSystemStatus.Available;
		}

		public static bool InstallDriver()
		{
			return NativeMethods.SetupInstallHinfSection("DefaultInstall", GetDriverSetupInfomationFile());
		}

		public static bool UninstallDriver()
		{
			return NativeMethods.SetupInstallHinfSection("DefaultUninstall", GetDriverSetupInfomationFile());
		}

		public static ServiceHost ServiceHost
		{
			get { return _ServiceHost; }
		}

		public static bool InitializeServiceHost(ServiceHost srvHost)
		{
			bool status = true;
			ShutdownServiceHost();

			_SocketModelServer = new SocketModel.SocketModelServer();
			status &= _SocketModelServer.Initialize();

			_ServiceHost = srvHost;
			return status;
		}

		public static bool ShutdownServiceHost()
		{
			if (_SocketModelServer != null)
			{
				_SocketModelServer.Shutdown();
				_SocketModelServer = null;
			}
			
			_ServiceHost = null;
			return true;
		}

		public static bool IsServiceReady()
		{
			SocketModel.SocketModelClient serviceClient = new SocketModel.SocketModelClient();
			SocketModel.SocketModelReplyServiceStatus status = serviceClient.GetServiceStatus();
			return status != null ? status.IsDriverConnected : false;
		}

		private static string GetDriverInstallFolder()
		{
			string driverFolder = null;
			if (Environment.Is64BitOperatingSystem)
			{
				string executingAssemblyFolder = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
				driverFolder = executingAssemblyFolder;
			}
			return driverFolder;
		}

		private static string GetDriverSetupInfomationFile()
		{
			string driverInf = null;
			string driverFolder = GetDriverInstallFolder();
			if (String.IsNullOrEmpty(driverFolder) == false)
			{
				driverInf = Path.GetFullPath(String.Format("{0}\\{1}.inf", driverFolder, DriverTitle));
			}
			return driverInf;
		}

		public static bool InstallService()
		{
			string serviceExe = String.Format("{0}\\{1}.exe", System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location), ServiceTitle);
			string serviceDesc = String.Format("Microsoft P4VFS Service Version [{0}]", CurrentVersion);

			bool result = false;
			if (CoreInterop.ServiceOperations.InstallLocalService(serviceExe, ServiceTitle, ServiceTitle, serviceDesc) == 0)
			{
				if (RegistryInfo.ValueExists(Microsoft.Win32.Registry.LocalMachine, ServiceRegistryKey, "ImagePath"))
				{
					RegistryInfo.SetStringValue(Microsoft.Win32.Registry.LocalMachine, ServiceRegistryKey, "Version", CurrentVersion.ToString());
				}

				result = true;
			}
			return result;
		}

		public static bool UninstallService(CoreInterop.ServiceOperations.UninstallFlags flags = CoreInterop.ServiceOperations.UninstallFlags.None)
		{
			return CoreInterop.ServiceOperations.UninstallLocalService(ServiceTitle, flags) == 0;
		}

		public static bool ShowMonitor()
		{
			if (Process.GetProcessesByName(MonitorTitle).Any())
			{
				VirtualFileSystemLog.Info("VirtualFileSystem.ShowMonitor {0}.exe already running", MonitorTitle);
				return true;
			}
			try
			{
				Process.Start(String.Format("{0}\\{1}.exe", Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location), MonitorTitle));
			}
			catch (Exception e)
			{
				VirtualFileSystemLog.Error("VirtualFileSystem.ShowMonitor failed to start {0}.exe : {1}", MonitorTitle, e.Message);
				return false;
			}
			return true;
		}

		public static bool HideMonitor()
		{
			bool success = true;
			foreach (Process process in Process.GetProcessesByName(MonitorTitle))
			{
				try
				{
					process.Kill();
					process.WaitForExit(5000);
				}
				catch (Exception e)
				{
					VirtualFileSystemLog.Info("VirtualFileSystem.HideMonitor failed to terminate all {0}.exe : {1}", MonitorTitle, e.Message);
					success = false;
				}
			}
			return success;
		}

		public static string UserSettingsFilePath
		{
			get	{ return Path.GetFullPath(String.Format("{0}\\{1}", Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), SettingsFile)); }
		}

		public static string AssemblySettingsFilePath
		{
			get	{ return Path.GetFullPath(String.Format("{0}\\{1}", Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location), SettingsFile)); }
		}

		public static string InstalledSettingsFilePath
		{
			get	
			{
				string serviceFilePath = null;
				RegistryInfo.GetTypedValue(Microsoft.Win32.Registry.LocalMachine, ServiceRegistryKey, "ImagePath", ref serviceFilePath);
				return String.IsNullOrEmpty(serviceFilePath) ? String.Empty : Path.GetFullPath(String.Format("{0}\\{1}", Path.GetDirectoryName(serviceFilePath), SettingsFile));
			}
		}

		public static VersionDescriptor CurrentVersion
		{
			get { return _CurrentVersion; }
		}

		public static VersionDescriptor GetServiceVersion()
		{
			string version = null;
			RegistryInfo.GetTypedValue(Microsoft.Win32.Registry.LocalMachine, ServiceRegistryKey, "Version", ref version);
			return new VersionDescriptor(version);
		}

		public static VersionDescriptor GetDriverVersion()
		{
			ushort major = 0;
			ushort minor = 0;
			ushort build = 0;
			ushort revision = 0;
			NativeMethods.GetDriverVersion(ref major, ref minor, ref build, ref revision);
			return new VersionDescriptor(major, minor, build, revision);
		}

		public static DepotSyncResult Sync(
			DepotClient depotClient, 
			string files, 
			DepotRevision revision = null, 
			DepotSyncType syncType = DepotSyncType.Normal, 
			DepotSyncMethod syncMethod = DepotSyncMethod.Virtual, 
			DepotFlushType flushType = DepotFlushType.Atomic,  
			string syncResident = null)
		{
			DepotSyncOptions syncOptions = new DepotSyncOptions();
			syncOptions.Files = new string[] { files };
			syncOptions.Revision = revision?.ToString();
			syncOptions.SyncType = syncType;
			syncOptions.SyncMethod = syncMethod;
			syncOptions.SyncResident = syncResident;
			syncOptions.FlushType = flushType;

			return Sync(depotClient, syncOptions);
		}

		public static DepotSyncResult Sync(DepotClient depotClient, DepotSyncOptions syncOptions)
		{
			return DepotOperations.Sync(depotClient, syncOptions);
		}

		public static bool PopulateExistingFile(DepotClient depotClient, string clientFile, bool preview = false, LogDevice log = null)
		{
			DepotResultFStat.Node fstatNode = depotClient.FStat(new string[] { clientFile }, "", DepotResultFStat.FieldType.DepotFile | DepotResultFStat.FieldType.HaveRev)[0];
			if (fstatNode == null)
			{
				log?.Error("File not in client view {0}", clientFile);
				return false;
			}

			DepotFileReference fileReference = new DepotFileReference()
			{
				DepotFile = fstatNode.DepotFile,
				Rev = new DepotRevisionNumber(fstatNode.HaveRev),
				ClientFile = clientFile,
			};
			return PopulateExistingFile(fileReference, preview, log);
		}

		public static bool PopulateExistingFile(DepotFileReference fileReference, bool preview = false, LogDevice log = null)
		{
			if (File.Exists(fileReference.ClientFile) && FileUtilities.GetAttributes(fileReference.ClientFile).HasFlag(FileAttributes.Offline))
			{
				log?.Info("{0}#{1} - downloaded as {2}", fileReference.DepotFile, fileReference.Rev, fileReference.ClientFile);
				if (preview)
				{
					return true;
				}
				try
				{
					using (File.Open(fileReference.ClientFile, FileMode.Open, FileAccess.Read, FileShare.Read | FileShare.Write | FileShare.Delete))
					{
						return true;
					}
				}
				catch (IOException e)
				{
					log?.Error("Failed to populate {0}#{1} -> {2} ({3})", fileReference.DepotFile, fileReference.Rev, fileReference, e.Message);
					return false;
				}
			}
			return false;
		}

		public static FilePopulateInfo QueryFilePopulateInfo(string physicalFilePath)
		{
			FilePopulateInfo populateInfo = NativeMethods.GetFilePopulateInfo(physicalFilePath);
			if (populateInfo == null)
			{
				VirtualFileSystemLog.Info("QueryFilePopulateInfo Skipped: {0}", physicalFilePath);
				return null;
			}
			if (String.IsNullOrWhiteSpace(populateInfo.DepotPath))
			{
				VirtualFileSystemLog.Error("QueryFilePopulateInfo Failed: {0}", physicalFilePath);
				return null;
			}

			string targetDepotServer = DepotOperations.ResolveDepotServerName(populateInfo.DepotServer);
			if (String.IsNullOrEmpty(targetDepotServer) == false)
			{
				populateInfo.DepotServer = targetDepotServer;
			}

			VirtualFileSystemLog.Info("QueryFilePopulateInfo: {0}", physicalFilePath);
			return populateInfo;
		}

		private static bool IsFileTypeAlwaysResident(string syncResident, string depotFile)
		{
			try	
			{ 
				return String.IsNullOrEmpty(depotFile) == false && 
					   String.IsNullOrEmpty(syncResident) == false && 
					   Regex.IsMatch(depotFile, syncResident, RegexOptions.IgnoreCase); 
			} 
			catch {}
			return false;
		}
	}

	public enum VirtualFileSystemStatus
	{
		Available,
		DriverNotInstalled,
		DriverNotReady,
		ServiceNotInstalled,
		ServiceNotRunning,
	}
}
