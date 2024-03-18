// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Linq;
using System.IO;
using System.Text;
using System.Reflection;
using System.Collections.Generic;
using System.ServiceProcess;
using System.Text.RegularExpressions;
using Newtonsoft.Json;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.P4VFS.CoreInterop;
using Microsoft.P4VFS.Extensions;
using Microsoft.P4VFS.Extensions.Utilities;

namespace Microsoft.P4VFS.UnitTest
{
	[TestClass, BasePriority(0)]
	public class UnitTestInstall : UnitTestBase
	{
		[TestMethod, Priority(0)]
		public void StagingUninstallTest()
		{
			string setupExe = GetSetupExe();
			Assert(File.Exists(setupExe));
			Assert(ShellUtilities.IsProcessElevated(), message:"must run UnitTest as administrator");
			Assert(ProcessInfo.ExecuteWait(setupExe, String.Format("-a -c uninstall"), echo:true, log:true) == 0);

			AssertRetry(() => VirtualFileSystem.IsDriverLoaded() == false, message:"IsDriverLoaded");
			AssertRetry(() => VirtualFileSystem.IsDriverReady() == false, message:"IsDriverReady");
			AssertRetry(() => VirtualFileSystem.IsServiceReady() == false, message:"IsServiceReady");
			AssertRetry(() => VirtualFileSystem.IsVirtualFileSystemAvailable() == false, message:"IsVirtualFileSystemAvailable");
			AssertRetry(() => IsDevDriveAllowed() == false, message:"IsDevDriveAllowed");
		}

		[TestMethod, Priority(1)]
		public void StagingInstallTest()
		{
			string setupExe = GetSetupExe();
			Assert(File.Exists(setupExe));
			Assert(ShellUtilities.IsProcessElevated(), message:"must run UnitTest as administrator");
			Assert(ProcessInfo.ExecuteWait(setupExe, String.Format("-a -c install"), echo:true, log:true) == 0);

			AssertRetry(() => VirtualFileSystem.IsDriverLoaded(), message:"IsDriverLoaded");
			AssertRetry(() => VirtualFileSystem.IsDriverReady(), message:"IsDriverReady");
			AssertRetry(() => VirtualFileSystem.IsServiceReady(), message:"IsServiceReady");
			AssertRetry(() => VirtualFileSystem.IsVirtualFileSystemAvailable(), message:"IsVirtualFileSystemAvailable");
			AssertRetry(() => IsDevDriveAllowed(), message:"IsDevDriveAllowed");

			Assert(CoreInterop.NativeMethods.GetDriverVersion(out ushort major, out ushort minor, out ushort build, out ushort revision));
			Assert(CoreInterop.NativeConstants.VersionMajor == major, "VersionMajor mismatch");
			Assert(CoreInterop.NativeConstants.VersionMinor == minor, "VersionMinor mismatch");
			Assert(CoreInterop.NativeConstants.VersionBuild == build || IsTestingDevConfig() == false, "VersionBuild mismatch");
			Assert(CoreInterop.NativeConstants.VersionRevision == revision || IsTestingDevConfig() == false, "VersionRevision mismatch");

			Assert(IsInstallationSigned() == IsTestingSignConfig());
		}

		[TestMethod, Priority(2)]
		public void SignedDriverTest()
		{
			Action<string, bool> assertSignedDriverFolder = (string driverFolder, bool production) =>
			{
				VirtualFileSystemLog.Info("Verifying drivers in folder: {0}", driverFolder);
				Assert(Directory.Exists(driverFolder));

				string driverSysFile = String.Format(@"{0}\p4vfsflt.sys", driverFolder);
				string driverCatFile = String.Format(@"{0}\p4vfsflt.cat", driverFolder);
				Assert(File.Exists(driverSysFile));
				Assert(File.Exists(driverCatFile));
				string signToolExe = GetSignToolExe();
				StringBuilder output;

				string[] allowedLocalSignErrors = new string[]
				{
					@"SignTool Error: A certificate chain processed, but terminated in a root\n(.*\n)*\s+certificate which is not trusted by the trust provider.",
				};
				Func<string, bool> isAllowedLocalSignError = (string text) => allowedLocalSignErrors.Any(e => Regex.IsMatch(text, e, RegexOptions.IgnoreCase|RegexOptions.Multiline));
				Func<string, bool> isAllowedSignError = (string text) => !production && isAllowedLocalSignError(text);

				// Verify for valid kernel mode sign from Hardware Development Center or WDK Test
				output = new StringBuilder();
				Assert(ProcessInfo.ExecuteWait(signToolExe, String.Format("verify /q /pa \"{0}\"", driverCatFile), echo:true, stdout:output) == 0 || isAllowedSignError(output.ToString()), "Catalog sign failure");
				output = new StringBuilder();
				Assert(ProcessInfo.ExecuteWait(signToolExe, String.Format("verify /q /kp \"{0}\"", driverSysFile), echo:true, stdout:output) == 0 || isAllowedSignError(output.ToString()), "Driver sign failure");
				output = new StringBuilder();
				Assert(ProcessInfo.ExecuteWait(signToolExe, String.Format("verify /q /pa /c \"{0}\" \"{1}\"", driverCatFile, driverSysFile), echo:true, stdout:output) == 0 || isAllowedSignError(output.ToString()), "Catalog and driver sign failure");
			};

			// Verify the signature on signed external module driver. (production)
			if (IsTestingDevConfig() == false && IsTestingSignConfig() == false)
			{
				string externalRootFolder = GetExternalRootFolder();
				string moduleDriverFolder = String.Format(@"{0}\P4VFS\{1}.{2}", externalRootFolder, NativeConstants.VersionMajor, NativeConstants.VersionMinor);
				assertSignedDriverFolder(moduleDriverFolder, false);
			}

			// Verify the signature on built driver. (production or test)
			if (IsTestingDevConfig())
			{
				string config = GetAssemblyConfiguration();
				string buildRootFolder = GetBuildRootFolder();
				assertSignedDriverFolder(String.Format(@"{0}\P4VFS.Driver\Win10.0.{1}\P4VFS.Driver", buildRootFolder, config), false);
			}

			// Verify the signature on the P4VFS.Setup driver. (production or test)
			string setupStagingFolder = GetSetupStagingFolder();
			assertSignedDriverFolder(setupStagingFolder, false);
		}

		[TestMethod, Priority(3)]
		public void UnsignedDriverInstallFailureTest()
		{
			string signToolExe = GetSignToolExe();
			string setupExe = GetSetupExe();

			Assert(VirtualFileSystem.IsDriverLoaded());
			Assert(VirtualFileSystem.IsDriverReady());
			Assert(VirtualFileSystem.IsServiceReady());
			Assert(VirtualFileSystem.IsVirtualFileSystemAvailable());
			
			List<string> unsignedInstallArgs = new List<string>();
			string unsignedTestFolder = String.Format("{0}\\unsigned", GetUnitTestRootFolder());
			FileUtilities.CreateDirectory(unsignedTestFolder);

			// Copy the driver files to a temp folder and remove sign
			string[] driverFiles = new string[]{ "p4vfsflt.sys", "p4vfsflt.cat" };
			foreach (string driverFile in driverFiles)
			{
				string signedDriverFile = String.Format("{0}\\{1}", GetSetupStagingFolder(), driverFile);
				string unsignedDriverFile = String.Format("{0}\\{1}", unsignedTestFolder, driverFile);
				Assert(File.Exists(signedDriverFile));
				AssertLambda(() => { File.Copy(signedDriverFile, unsignedDriverFile, overwrite:true); return true; });
				FileUtilities.MakeWritable(unsignedDriverFile);

				ProcessInfo.ExecuteResultOutput removeSignResult = ProcessInfo.ExecuteWaitOutput(signToolExe, String.Format("remove /s \"{0}\"", unsignedDriverFile), echo:true, log:true);
				Assert(removeSignResult.ExitCode == 0 || (driverFile.EndsWith(".cat") && removeSignResult.Lines.Any(line => line.Contains("SignTool Error: Unsupported file type:"))));
				unsignedInstallArgs.Add(String.Format("-i {0}", unsignedDriverFile));
			}

			Assert(ProcessInfo.ExecuteWait(setupExe, String.Format("-a -c uninstall"), echo:true, log:true) == 0);
			Assert(VirtualFileSystem.IsVirtualFileSystemAvailable() == false);

			// Toggle off/on the 'Program Compatibility Assistant Service' for an unattended test... driver install should fail!
			ProcessInfo.ExecuteWait("sc.exe", "stop PcaSvc", echo:true);
			int installExitCode = ProcessInfo.ExecuteWait(setupExe, String.Format("-a -c {0} install", String.Join(" ", unsignedInstallArgs)), echo:true, log:true);
			ProcessInfo.ExecuteWait("sc.exe", "start PcaSvc", echo:true);
			if (installExitCode == 0)
			{
				// In Windows 11, and late Windows 10, there is a problem where "signtool remove /s" will remove the cert but doesn't prevent windows from loading
				// the driver if the cert still exists and driver signature has previously been loaded. It's a nice windows optimization ... but I'm not sure how to flush it from the cache.
				Assert(GetWindowsOSBuildVersion() >= 19044);
				Assert(ProcessInfo.ExecuteWait(setupExe, String.Format("-a -c uninstall"), echo:true, log:true) == 0);
			}

			// Ensure that the service install has also been backed out
			Assert(VirtualFileSystem.IsDriverLoaded() == false);
			Assert(VirtualFileSystem.IsServiceReady() == false);
			Assert(ProcessInfo.ExecuteWaitOutput("sc.exe", String.Format("query {0}", VirtualFileSystem.ServiceTitle)).Lines.Any(line => line.Contains("specified service does not exist")));
			Assert(ProcessInfo.ExecuteWait(setupExe, String.Format("-a -c reinstall"), echo:true, log:true) == 0);

			// After a reinstall... it's good to go
			Assert(VirtualFileSystem.IsDriverLoaded());
			Assert(VirtualFileSystem.IsDriverReady());
			Assert(VirtualFileSystem.IsServiceReady());
			Assert(VirtualFileSystem.IsVirtualFileSystemAvailable());
		}

		[TestMethod, Priority(4)]
		public void InstallFileVersionTest()
		{
			string p4vfsExe = InstalledP4vfsExe;
			Assert(File.Exists(p4vfsExe));
			string installFolder = Path.GetDirectoryName(p4vfsExe);
			Assert(Directory.Exists(installFolder));
			string[] installFileNames = {
				"p4vfsflt.sys", 
				"P4VFS.Extensions.dll", 
				"P4VFS.ExtensionsInterop.dll", 
				"P4VFS.CoreInterop.dll", 
				"P4VFS.Monitor.exe", 
				"P4VFS.Service.exe", 
				"P4VFS.UnitTest.dll", 
				"p4vfs.exe",
			};	
			
			var AssertGetValue = new Func<IReadOnlyDictionary<string,string>,string,string>((d,k) => { 
				Assert(d.ContainsKey(k)); 
				Assert(String.IsNullOrEmpty(d[k]) == false);
				return d[k];
			});

			string setupStagingFolder = GetSetupStagingFolder();
			string latestDriverSysFile = String.Format(@"{0}\p4vfsflt.sys", setupStagingFolder);
			string latestDriverSysVersion = AssertGetValue(FileUtilities.GetFileProperties(latestDriverSysFile), "Version");
			Assert(String.IsNullOrEmpty(latestDriverSysVersion) == false);
			
			foreach (string fileName in installFileNames)
			{
				string filePath = String.Format("{0}\\{1}", installFolder, fileName);
				Assert(File.Exists(filePath));
				IReadOnlyDictionary<string,string> properties = FileUtilities.GetFileProperties(filePath);
				
				string fileVersion = NativeConstants.Version;
				if (IsTestingDevConfig() == false && String.Compare(Path.GetFileName(fileName), Path.GetFileName(latestDriverSysFile), StringComparison.InvariantCultureIgnoreCase) == 0)
				{
					fileVersion = latestDriverSysVersion;
				}
				Assert(String.Compare(AssertGetValue(properties, "Version"), fileVersion, StringComparison.InvariantCultureIgnoreCase) == 0);
				Assert(String.Compare(AssertGetValue(properties, "Name"), filePath, StringComparison.InvariantCultureIgnoreCase) == 0);
			}
		}

		[TestMethod, Priority(5), TestRemote]
		public void IsVirtualFileSystemAvailable()
		{
			Assert(VirtualFileSystem.IsVirtualFileSystemAvailable());
			Assert(VirtualFileSystem.GetVirtualFileSystemStatus() == VirtualFileSystemStatus.Available);
		}

		[TestMethod, Priority(6), TestRemote]
		public void ServiceSettingsInstallTest()
		{
			ServiceSettings.Reset();
			ServiceRestart();

			ServiceSettingsInstallTestProperty(nameof(SettingManager.FileLoggerRemoteDirectory), "remote");
			ServiceSettingsInstallTestProperty(nameof(SettingManager.FileLoggerLocalDirectory), "local");

			ServiceSettingsInstallTestProperty(nameof(SettingManager.AllowSymlinkResidencyPolicy), true);
			ServiceSettingsInstallTestProperty(nameof(SettingManager.AllowSymlinkResidencyPolicy), false);
			ServiceSettingsInstallTestProperty(nameof(SettingManager.AllowSymlinkResidencyPolicy), true);

			ServiceSettingsInstallTestProperty(nameof(SettingManager.ReportUsageExternally), true);
			ServiceSettingsInstallTestProperty(nameof(SettingManager.ReportUsageExternally), false);
			ServiceSettingsInstallTestProperty(nameof(SettingManager.ReportUsageExternally), true);

			ServiceSettingsInstallTestProperty(nameof(SettingManager.Verbosity), LogChannel.Debug);
			ServiceSettingsInstallTestProperty(nameof(SettingManager.Verbosity), LogChannel.Error);
			ServiceSettingsInstallTestProperty(nameof(SettingManager.Verbosity), LogChannel.Info);

			ServiceSettingsInstallTestProperty(nameof(SettingManager.MaxSyncConnections), 5);
			ServiceSettingsInstallTestProperty(nameof(SettingManager.MaxSyncConnections), 6);

			ServiceSettingsInstallTestProperty(nameof(SettingManager.PopulateMethod), FilePopulateMethod.Copy);
			ServiceSettingsInstallTestProperty(nameof(SettingManager.PopulateMethod), FilePopulateMethod.Move);
			ServiceSettingsInstallTestProperty(nameof(SettingManager.PopulateMethod), FilePopulateMethod.Stream);

			ServiceSettingsInstallTestProperty(nameof(SettingManager.DefaultFlushType), DepotFlushType.Atomic);
			ServiceSettingsInstallTestProperty(nameof(SettingManager.DefaultFlushType), DepotFlushType.Single);

			ServiceSettingsInstallTestProperty(nameof(SettingManager.DepotServerConfig), new DepotServerConfig());
			ServiceSettingsInstallTestProperty(nameof(SettingManager.DepotServerConfig), new DepotServerConfig{Servers=new DepotServerConfigEntry[]{new DepotServerConfigEntry{Pattern="abc", Address="123"}}});

			ServiceSettingsInstallTestProperty(nameof(SettingManager.Unattended), true);
			ServiceSettingsInstallTestProperty(nameof(SettingManager.Unattended), false);
			ServiceSettingsInstallTestProperty(nameof(SettingManager.Unattended), true);

			ServiceSettings.Reset();
			ServiceRestart();
		}

		[TestMethod, Priority(7)]
		public void ServiceReinstallDeletePending()
		{
			string serviceTitle = VirtualFileSystem.ServiceTitle;
			string serviceExe = String.Format("{0}\\{1}.exe", Path.GetDirectoryName(InstalledP4vfsExe), serviceTitle);
			Assert(File.Exists(serviceExe));

			Func<ServiceController> GetSC = () => { try { return new ServiceController(serviceTitle); } catch {} return null; };
			Func<string, string[]> ExecuteSC = (string args) => { return ProcessInfo.ExecuteWaitOutput("sc.exe", args, log:true).Lines; };

			Assert(CoreInterop.ServiceOperations.UninstallLocalService(serviceTitle, CoreInterop.ServiceOperations.UninstallFlags.None) == 0);
			AssertRetry(() => ExecuteSC(String.Format("query {0}", serviceTitle)).Any(line => line.Contains("specified service does not exist")));
			Assert(CoreInterop.ServiceOperations.InstallLocalService(serviceExe, serviceTitle, serviceTitle, "ServiceReinstallDeletePending") == 0);
			AssertRetry(() => ExecuteSC(String.Format("query {0}", VirtualFileSystem.ServiceTitle)).Any(line => Regex.IsMatch(line, @"STATE\s+.+\s+RUNNING")));
			
			using (ServiceController sc = GetSC())
			{
				Assert(sc != null);
				Assert(sc.Status == ServiceControllerStatus.Running);
				using (System.Runtime.InteropServices.SafeHandle sh = sc.ServiceHandle)
				{
					// Keep an open service handle while we manually uninstall and install
					Assert(sh.IsInvalid == false);
					
					Assert(CoreInterop.ServiceOperations.UninstallLocalService(serviceTitle, CoreInterop.ServiceOperations.UninstallFlags.NoDelete) == 0);
					AssertLambda(() => sc.WaitForStatus(ServiceControllerStatus.Stopped, TimeSpan.FromSeconds(5)));
					Assert(sc.Status == ServiceControllerStatus.Stopped);
					Assert(ExecuteSC(String.Format("query {0}", VirtualFileSystem.ServiceTitle)).Any(line => Regex.IsMatch(line, @"STATE\s+.+\s+STOPPED")));

					Assert(CoreInterop.ServiceOperations.InstallLocalService(serviceExe, serviceTitle, serviceTitle, "ServiceReinstallDeletePending") == 0);
					AssertLambda(() => sc.WaitForStatus(ServiceControllerStatus.Running, TimeSpan.FromSeconds(5)));
					Assert(sc.Status == ServiceControllerStatus.Running);
					Assert(ExecuteSC(String.Format("query {0}", VirtualFileSystem.ServiceTitle)).Any(line => Regex.IsMatch(line, @"STATE\s+.+\s+RUNNING")));

					// Do a full reinstall with a service handle open
					string setupExe = GetSetupExe();
					Assert(File.Exists(setupExe));
					Assert(ShellUtilities.IsProcessElevated(), message: "must run UnitTest as administrator");
					Assert(ProcessInfo.ExecuteWait(setupExe, String.Format("-a -c reinstall"), echo: true, log: true) == 0);
					Assert(VirtualFileSystem.IsVirtualFileSystemAvailable());
				}
			}
			Assert(VirtualFileSystem.IsVirtualFileSystemAvailable());
		}

		[TestMethod, Priority(8)]
		public void DriverOperationsTest()
		{
			Func<bool> IsDriverLoaded = () =>
			{
				Assert(DriverOperations.GetLoadedFilters(out string[] driverNames) == 0);
				bool isLoaded = driverNames?.Contains(VirtualFileSystem.DriverTitle, StringComparer.InvariantCultureIgnoreCase) == true;
				Assert(DriverOperations.IsFilterLoaded(VirtualFileSystem.DriverTitle) == isLoaded);
				bool fltmcIsLoaded = ProcessInfo.ExecuteWaitOutput("fltmc.exe", "").Lines.Any(line => Regex.IsMatch(line, String.Format(@"^{0}\s", VirtualFileSystem.DriverTitle), RegexOptions.IgnoreCase));
				Assert(isLoaded == fltmcIsLoaded);
				return isLoaded;
			};

			Assert(CoreInterop.ServiceOperations.StopLocalService(VirtualFileSystem.ServiceTitle) == 0);
			Assert(CoreInterop.ServiceOperations.GetLocalServiceState(VirtualFileSystem.ServiceTitle) == ServiceInfo.SERVICE_STOPPED);

			Assert(IsDriverLoaded());
			Assert(DriverOperations.UnloadFilter(VirtualFileSystem.DriverTitle) == 0);
			Assert(IsDriverLoaded() == false);
			Assert(DriverOperations.LoadFilter(VirtualFileSystem.DriverTitle) == 0);
			Assert(IsDriverLoaded());

			Assert(CoreInterop.ServiceOperations.StartLocalService(VirtualFileSystem.ServiceTitle) == 0);
			Assert(CoreInterop.ServiceOperations.GetLocalServiceState(VirtualFileSystem.ServiceTitle) == ServiceInfo.SERVICE_RUNNING);
			Assert(VirtualFileSystem.IsVirtualFileSystemAvailable());
		}

		public bool IsInstallationSigned()
		{
			string p4vfsExe = InstalledP4vfsExe;
			Assert(File.Exists(p4vfsExe));
			string installFolder = Path.GetDirectoryName(p4vfsExe);
			Assert(Directory.Exists(installFolder));
			string signToolExe = GetSignToolExe();

			foreach (string installFilePath in Directory.GetFiles(installFolder, "*", SearchOption.TopDirectoryOnly))
			{
				if (Regex.IsMatch(Path.GetFileName(installFilePath), @"p4vfs.*\.(exe|dll|sys|cat)$", RegexOptions.IgnoreCase))
				{
					bool isDriverFile = Regex.IsMatch(installFilePath, @"\.(sys|cat)$", RegexOptions.IgnoreCase);
					string signPolicy = isDriverFile ? "" : "/pa";
					if (ProcessInfo.ExecuteWait(signToolExe, String.Format("verify {0} \"{1}\"", signPolicy, installFilePath), echo:true) != 0)
					{
						return false;
					}
				}
			}
			return true;
		}

		private static void ServiceSettingsInstallTestProperty<ValueType>(string name, ValueType value)
		{
			PropertyInfo property = typeof(SettingManager).GetProperty(name, BindingFlags.GetProperty|BindingFlags.SetProperty|BindingFlags.Public|BindingFlags.Static);
			Assert(property != null);
			if (typeof(ValueType).IsAssignableFrom(property.PropertyType) == false)
			{
				property = typeof(SettingManagerExtensions).GetProperty(name, BindingFlags.GetProperty|BindingFlags.SetProperty|BindingFlags.Public|BindingFlags.Static);
				Assert(property != null);
				Assert(typeof(ValueType).IsAssignableFrom(property.PropertyType));
			}

			Func<ValueType> get = () => (ValueType)property.GetValue(null);
			Action<ValueType> set = (ValueType v) => property.SetValue(null, v);

			ValueType[] values = new ValueType[]{ get(), value };
			set(values[1]);
			Assert(JsonConvert.SerializeObject(get()) == JsonConvert.SerializeObject(values[1]));
			set(values[0]);
			Assert(JsonConvert.SerializeObject(get()) == JsonConvert.SerializeObject(values[0]));

			Extensions.SocketModel.SocketModelClient client = new Extensions.SocketModel.SocketModelClient();
			Assert(client.SetServiceSetting(name, SettingNode.FromString(values[1].ToString())));
			Assert(client.GetServiceSetting(name).ToString() == values[1].ToString());
			Assert(client.SetServiceSetting(name, SettingNode.FromString(values[0].ToString())));
			Assert(client.GetServiceSetting(name).ToString() == values[0].ToString());
		}

		public static bool IsTestingDevConfig()
		{
			return GetSetupAssemblyConfiguration().EndsWith("Dev");
		}

		public static bool IsTestingSignConfig()
		{
			return GetSetupAssemblyConfiguration().EndsWith("Sign");
		}

		public static string GetAssemblyConfiguration()
		{
			return Regex.Replace(GetSetupAssemblyConfiguration(), "(Dev|Sign)$", "");
		}

		public static string GetSetupAssemblyConfiguration()
		{
			AssemblyConfigurationAttribute config = Assembly.GetExecutingAssembly().GetCustomAttributes(typeof(AssemblyConfigurationAttribute)).OfType<AssemblyConfigurationAttribute>().FirstOrDefault();
			Assert(config != null);
			Assert(String.IsNullOrEmpty(config.Configuration) == false);
			Assert((new string[]{"Debug","DebugDev","Release","ReleaseDev","ReleaseSign"}).Contains(config.Configuration));
			return config.Configuration;
		}

		public static string GetSetupExe()
		{
			return String.Format(@"{0}\P4VFS.Setup\{1}\P4VFS.Setup.exe", GetBuildRootFolder(), GetSetupAssemblyConfiguration());
		}

		public static string GetSetupStagingFolder()
		{
			return String.Format(@"{0}\staging", Path.GetDirectoryName(GetSetupExe()));
		}

		public static string GetSignToolExe()
		{
			string signToolExe = null;
			AssertLambda(() => { signToolExe = WdkUtilities.GetSignToolExe(); });
			Assert(File.Exists(signToolExe), "WDK signtool is not installed");
			return signToolExe;
		}

		public static string GetUnitTestRootFolder()
		{
			return String.Format(@"{0}\P4VFS.UnitTest\{1}", GetBuildRootFolder(), GetSetupAssemblyConfiguration());
		}

		public static int GetWindowsOSBuildVersion()
		{
			string osDescription = System.Runtime.InteropServices.RuntimeInformation.OSDescription;
			Match m = Regex.Match(osDescription, @"\.(?<build>\d+)\s*$");
			Assert(m.Success);
			return Int32.Parse(m.Groups["build"].Value);
		}

		public static bool IsDevDriveAllowed()
		{
			return ProcessInfo.ExecuteWaitOutput("fsutil.exe", "devdrv query").Lines.
				Any(line => Regex.IsMatch(line, String.Format(@"(^|\s|,){0}($|\s|,)", VirtualFileSystem.DriverTitle), RegexOptions.IgnoreCase));
		}
	}
}
