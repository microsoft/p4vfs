// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Linq;
using System.IO;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using System.Reflection;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.P4VFS.Extensions;
using Microsoft.P4VFS.Extensions.Utilities;
using Microsoft.P4VFS.CoreInterop;
using Microsoft.P4VFS.Extensions.Linq;

namespace Microsoft.P4VFS.UnitTest
{
	public class UnitTestBase
	{
		public static string _P4User = "p4vfstest";
		public static string _P4Port = "localhost:1666";
		public static string _P4Client = "p4vfstest-depot";

		public static bool Execute(DepotConfig config, string[] args)
		{
			try
			{
				int argIndex = 0;
				for (; argIndex < args.Length; ++argIndex)
				{
					if (String.Compare(args[argIndex], "-r") == 0)
					{
						IsTestRemote = true;
					}
					else if (String.Compare(args[argIndex], "-e") == 0)
					{
						IsTestAllowUnelevated = true;
					}
					else
					{
						break;
					}
				}

				args = args.Skip(argIndex).ToArray();
				
				if (IsTestAllowUnelevated == false && ShellUtilities.IsProcessElevated() == false)
				{
					VirtualFileSystemLog.Error("Unable to run tests as non-elevated process");
					return false;
				}

				if (config != null)
				{
					if (String.IsNullOrWhiteSpace(config.Port) == false)
					{
						_P4Port = config.Port;
					}

					if (String.IsNullOrWhiteSpace(config.Client) == false)
					{
						_P4Client = config.Client;
					}

					if (String.IsNullOrWhiteSpace(config.User) == false)
					{
						_P4User = config.User;
					}

					if (String.IsNullOrWhiteSpace(config.Passwd) == false)
					{
						Environment.SetEnvironmentVariable("P4PASSWD", config.Passwd);
					}
				}

				Assembly unitTestAssembly = Assembly.GetExecutingAssembly();
				List<TestMethodInfo> testMethods = new List<TestMethodInfo>();
				foreach (Type unitTestClassType in unitTestAssembly.GetTypes().Where(t => t.IsClass && t.GetCustomAttribute<TestClassAttribute>(true) != null))
				{
					foreach (MethodInfo methodInfo in unitTestClassType.GetMethods(BindingFlags.Public|BindingFlags.Instance).Where(m => m.GetCustomAttribute<TestMethodAttribute>() != null))
					{
						testMethods.Add(new TestMethodInfo{ Info = methodInfo, Priority = UnitTestBase.GetPriority(methodInfo) });
					}
				}

				// Sort the tests by priority... lowest value first
				testMethods.Sort(new Comparison<TestMethodInfo>((a,b) => a.Priority - b.Priority));
				
				foreach (TestMethodInfo method in testMethods)
				{
					string methodName = String.Format("[{0}] {1}.{2}", method.Priority, method.Info.ReflectedType.Name, method.Info.Name);
					if (IsRequestingToRunUnitTest(args, method) == false)
					{
						VirtualFileSystemLog.Info("Skipping unit test: {0}", methodName);
						continue;
					}
					using (new SetConsoleTitleScope(String.Format("P4VFS UnitTest {0}", methodName)))
					{
						VirtualFileSystemLog.Info("Running unit test: {0}", methodName);
						UnitTestBase unitTestInstance = unitTestAssembly.CreateInstance(method.Info.ReflectedType.FullName) as UnitTestBase;
						unitTestInstance.CommandLineArgs = args;
						method.Info.Invoke(unitTestInstance, new object[]{});
						VirtualFileSystemLog.Info("Successfull unit test: {0}", methodName);
					}
				}
			}
			catch (Exception e)
			{
				VirtualFileSystemLog.Error("Failed running unit tests: {0}", e.Message);
				if (e.InnerException != null)
				{
					VirtualFileSystemLog.Error("Exception: {0}\n{1}", e.InnerException.Message, e.InnerException.StackTrace);
				}
				return false;
			}
			return true;
		}

		private static bool IsRequestingToRunUnitTest(string[] args, TestMethodInfo method)
		{
			if (args.Length == 0 && method.Info.GetCustomAttribute<TestExplicitAttribute>() == null)
			{
				return true;
			}

			if (method.Info.GetCustomAttribute<TestAlwaysAttribute>() != null)
			{
				return true;
			}

			string methodName = String.Format("{0}.{1}", method.Info.ReflectedType.Name, method.Info.Name);
			if (args.Contains(methodName))
			{
				return true;
			}

			if (args.Contains(method.Info.Name))
			{
				return true;
			}

			foreach (string arg in args)
			{
				if (arg == method.Priority.ToString())
				{
					return true;
				}

				if (method.Info.GetCustomAttribute<TestExplicitAttribute>() == null)
				{
					Match range = Regex.Match(arg, @"^\s*\[?(?<begin>\d*):(?<end>\d*)\]?\s*$");
					if (range.Success)
					{
						int begin = range.Groups["begin"].Value.ToInt32(Int32.MinValue);
						int end = range.Groups["end"].Value.ToInt32(Int32.MaxValue);
						if (method.Priority >= begin && method.Priority <= end)
						{
							return true;
						}
					}
				}
			}
			return false;
		}

		public static void Assert(bool value, string message = "")
		{
			if (value == false)
			{
				if (System.Diagnostics.Debugger.IsAttached)
					System.Diagnostics.Debugger.Break();

				System.Diagnostics.StackTrace stack = new System.Diagnostics.StackTrace(true);
				System.Diagnostics.StackFrame[] frames = stack.GetFrames();
				int frameIndex = Array.FindIndex(frames, f => f.GetMethod().Name == "Assert");
				string frameText = frameIndex >= 0 && frameIndex+1 < frames.Length ? frames[frameIndex+1].ToString().Trim() : "<unknown>";

				Microsoft.VisualStudio.TestTools.UnitTesting.Assert.IsTrue(value, String.Format("{0} [{1}]", frameText, message));
			}
		}

		public static void AssertRetry(Func<bool> expression, string message = "", int retryWait = 500, int limitWait = 5000)
		{
			DateTime endTime = DateTime.Now.AddMilliseconds((double)limitWait);
			do
			{
				if (expression())
					return;
				VirtualFileSystemLog.Info("UnitTest.AssertRetry [{0} ms] {1}", (endTime-DateTime.Now).TotalMilliseconds, message);
				System.Threading.Thread.Sleep(retryWait);
			}
			while (DateTime.Now < endTime);
			Assert(false, message);
		}

		public static void AssertLambda(Func<bool> expression, string message = "")
		{
			try { Assert(expression(), message); } 
			catch { Assert(false, message); }
		}

		public static void AssertLambda(Action expression, string message = "")
		{
			try { expression(); } 
			catch { Assert(false, message); }
		}

		public static string GetCachedFilePath(ref string filePath, Func<string> getFilePath)
		{
			if (String.IsNullOrEmpty(filePath))
			{
				filePath = getFilePath();
			}
			Assert(File.Exists(filePath));
			return filePath;
		}

		public static void WaitForAttachedDebugger()
		{
			VirtualFileSystemLog.Info("Waiting for attached debugger ...");
			while (System.Diagnostics.Debugger.IsAttached == false)
				System.Threading.Thread.Sleep(100);
		}

		public string ExcludedProcessNames
		{
			get 
			{ 
				return String.Join(";", new[]
				{
					"SearchProtocolHost.exe",
					"MsSense.exe",
					"MsMpEng.exe",
					"SenseCE.exe",
					"SenseIR.exe",
				});
			}
		}

		public void WorkspaceReset(string port, string client, string user)
		{
			WorkspaceReset(new DepotConfig{ Port=port??_P4Port, Client=client??_P4Client, User=user??_P4User });
		}

		public void WorkspaceReset(DepotConfig config = null)
		{
			if (config == null)
			{
				config = ClientConfig;
			}

			Assert(String.IsNullOrEmpty(config.Port) == false);
			Assert(String.IsNullOrEmpty(config.Client) == false);
			Assert(String.IsNullOrEmpty(config.User) == false);

			foreach (string settingsPath in new[]{ VirtualFileSystem.UserSettingsFilePath, VirtualFileSystem.PublicSettingsFilePath })
			{
				Assert(File.Exists(settingsPath) == false, String.Format("User settings file not allowed for unit tests: {0}", settingsPath));
			}

			foreach (string name in new[]{DepotConstants.P4PORT, DepotConstants.P4USER, DepotConstants.P4CLIENT, DepotConstants.P4CONFIG, DepotConstants.P4TRUST, DepotConstants.P4TICKETS})
			{
				Environment.SetEnvironmentVariable(name, "");
				Assert(ProcessInfo.ExecuteWait(P4Exe, String.Format("set {0}=", name)) == 0);
			}

			AssertRetry(() => VirtualFileSystem.IsDriverLoaded(), message:"IsDriverLoaded");
			AssertRetry(() => VirtualFileSystem.IsDriverReady(), message:"IsDriverReady");
			AssertRetry(() => VirtualFileSystem.IsVirtualFileSystemAvailable(), message:"IsVirtualFileSystemAvailable");
			ServiceSettings.Reset();
			InteractiveOverridePasswd = null;

			if (String.IsNullOrEmpty(config.Passwd))
			{
				config.Passwd = UnitTestServer.GetUserP4Passwd(config.User);
				Assert(String.IsNullOrEmpty(config.Passwd) == false);
			}
			
			using (DepotClient depotClient = new DepotClient())
			{
				Assert(depotClient.Connect(config));
				DepotResultClient.Node workspace = depotClient.Client();
				
				string root = workspace?.Root;
				Assert(String.IsNullOrEmpty(root) == false);
				Assert(depotClient.Opened().Count == 0 || depotClient.Run("revert", new[]{ "-k", "//..." }).HasError == false);

				depotClient.Sync("//...", new DepotRevisionNone(), DepotSyncFlags.Force|DepotSyncFlags.Quiet, DepotSyncMethod.Regular);
				AssertLambda(() => FileUtilities.DeleteDirectoryAndFiles(root));
				AssertRetry(() => Directory.Exists(root) == false, String.Format("directory exists {0}", root));
				if (depotClient.GetHeadRevisionChangelist() != null)
				{
					DepotSyncResult syncResult = depotClient.Sync("//...#none", null, DepotSyncFlags.Normal);
					Assert(syncResult?.Modifications != null);
					Assert(syncResult.Modifications.Where(a => !FDepotSyncActionType.IsError(a.SyncActionType)).Any() == false);
				}

				Assert(String.IsNullOrEmpty(workspace?.Client) == false);
				Assert(config.Client == workspace.Client);
				Assert(String.IsNullOrEmpty(workspace?.Owner) == false);
				Assert(config.User == workspace.Owner);

				root = UnitTestServer.GetServerClientRootFolder(workspace.Client, config.Port);
				Assert(String.IsNullOrEmpty(root) == false);

				string clientSpec = String.Join("\n", new[]{
					String.Format("Client:\t{0}", config.Client),
					String.Format("Owner:\t{0}", config.User),
					String.Format("Host:\t{0}", ""),
					String.Format("Root:\t{0}", root),
					String.Format("Options:\t{0}", "noallwrite clobber nocompress unlocked nomodtime rmdir"),
					String.Format("LineEnd:\t{0}", DepotResultClient.LineEnd.Local),
					String.Format("Description:\n\t{0}", "Created by " + nameof(WorkspaceReset)),
					String.Format("View:\n\t{0}", String.Join("\n\t", workspace.View))
				});

				Assert(depotClient.Run(new DepotCommand{ Name="client", Args=new[]{"-i"}, Input=clientSpec }).HasError == false);
				workspace = depotClient.Client();

				Assert(workspace != null);
				Assert(workspace.Client == config.Client);
				Assert(workspace.Owner == config.User);
				Assert(workspace.Root == root);
				Assert(workspace.OptionNames?.Contains(DepotResultClient.Options.RmDir) == true);
				Assert(workspace.LineEnd == DepotResultClient.LineEnd.Local);
				Assert(workspace.Client != _P4Client || workspace.View.Count() == 1);
				Assert(workspace.Client != _P4Client || workspace.View.ElementAt(0) == String.Format("//depot/... //{0}/depot/...", workspace.Client));
			}

			Extensions.SocketModel.SocketModelClient service = new Extensions.SocketModel.SocketModelClient(); 
			Assert(service.GarbageCollect());
			Assert(service.SetServiceSetting(nameof(SettingManager.ExcludedProcessNames), SettingNode.FromString(ExcludedProcessNames)));
		}

		public void ServiceRestart()
		{
			VirtualFileSystemLog.Info("Stopping service {0} ...", VirtualFileSystem.ServiceTitle);
			Assert(CoreInterop.ServiceOperations.StopLocalService(VirtualFileSystem.ServiceTitle) == 0);
			VirtualFileSystemLog.Info("Stopped", VirtualFileSystem.ServiceTitle);
			Assert(CoreInterop.ServiceOperations.GetLocalServiceState(VirtualFileSystem.ServiceTitle) == ServiceInfo.SERVICE_STOPPED);

			VirtualFileSystemLog.Info("Starting service {0} ...", VirtualFileSystem.ServiceTitle);
			Assert(CoreInterop.ServiceOperations.StartLocalService(VirtualFileSystem.ServiceTitle) == 0);
			VirtualFileSystemLog.Info("Started", VirtualFileSystem.ServiceTitle);
			Assert(CoreInterop.ServiceOperations.GetLocalServiceState(VirtualFileSystem.ServiceTitle) == ServiceInfo.SERVICE_RUNNING);
		}

		public static string GetP4vfsInstallFolder()
		{
			string installFolder = null;
			if (RegistryInfo.GetTypedValue<string>(Microsoft.Win32.Registry.LocalMachine, VirtualFileSystem.AppRegistryKey, "InstallLocation", ref installFolder))
			{
				Assert(Directory.Exists(installFolder));
				return installFolder;
			}
			return null;
		}

		public static bool IsTestRemote
		{
			get;
			private set;
		}

		public static bool IsTestAllowUnelevated
		{
			get;
			private set;
		}

		private static string _P4dExe;
		public static string P4dExe
		{
			get { return GetCachedFilePath(ref _P4dExe, () => String.Format("{0}\\bin\\p4d.exe", GetExternalModuleFolder("P4API"))); }
		}

		private static string _P4Exe;
		public static string P4Exe
		{
			get { return IsTestRemote ? "p4.exe" : GetCachedFilePath(ref _P4Exe, () => String.Format("{0}\\bin\\p4.exe", GetExternalModuleFolder("P4API"))); }
		}

		private static string _P4vfsExe;
		public static string P4vfsExe
		{
			get { return GetCachedFilePath(ref _P4vfsExe, () => String.Format("{0}\\p4vfs.exe", Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location))); }
		}

		private static string _InstalledP4vfsExe;
		public static string InstalledP4vfsExe
		{
			get { return GetCachedFilePath(ref _InstalledP4vfsExe, () => Path.GetFullPath(String.Format("{0}\\{1}", GetP4vfsInstallFolder(), Path.GetFileName(P4vfsExe)))); }
		}

		public static string SysInternalsFolder
		{
			get { return String.Format("{0}\\SysinternalsSuite", Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles)); }
		}

		public static string GetRepositoryRootFolder()
		{
			// Special case running tests using the installed p4vfs (possibly remotely) we will use a temporary relative root under %LOCALAPPDATA%
			string executingFilePath = Assembly.GetExecutingAssembly().Location;
			string installFolder = GetP4vfsInstallFolder();
			if (String.IsNullOrEmpty(installFolder) == false && executingFilePath.StartsWith(String.Concat(Path.GetFullPath(installFolder).TrimEnd('\\'), "\\"), StringComparison.InvariantCultureIgnoreCase))
			{
				return String.Format("{0}\\P4VFS.UnitTest", Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData));
			}

			// Typical case running tests under the P5VFS repository folder
			string repoRootFolder = Path.GetFullPath(String.Format(@"{0}\..\..\..\..", Path.GetDirectoryName(executingFilePath)));
			Assert(Directory.Exists(repoRootFolder), string.Format("{0} does not exist", repoRootFolder));
			foreach (string subFolderName in new[]{ "source", "external", "intermediate" })
			{
				string subFolderPath = Path.Combine(repoRootFolder, subFolderName);
				Assert(Directory.Exists(subFolderPath), string.Format("{0} does not exist", subFolderPath));
			}
			return repoRootFolder;
		}

		public static string GetIntermediateRootFolder()
		{
			return Path.GetFullPath(String.Format(@"{0}\intermediate", GetRepositoryRootFolder()));
		}

		public static string GetBuildRootFolder()
		{
			return Path.GetFullPath(String.Format(@"{0}\builds", GetIntermediateRootFolder()));
		}

		public static string GetSourceRootFolder()
		{
			return Path.GetFullPath(String.Format(@"{0}\source", GetRepositoryRootFolder()));
		}

		public static string GetExternalRootFolder()
		{
			return Path.GetFullPath(String.Format(@"{0}\external", GetRepositoryRootFolder()));
		}

		public static string GetExternalModuleFolder(string moduleName, bool required = false)
		{
			string folderPath = Directory.GetDirectories(String.Format(@"{0}\{1}", GetExternalRootFolder(), moduleName))
				.Where(path => Regex.IsMatch(Path.GetFileName(path), @"^\d+\.\d+"))
				.OrderBy(path => Path.GetFileName(path))
				.FirstOrDefault();
			Assert(Directory.Exists(folderPath) || required == false);
			return folderPath;
		}

		public DepotConfig ClientConfig
		{
			get { return new DepotConfig(){ Port = _P4Port, Client = _P4Client, User = _P4User }; }
		}

		public string GetClientRoot(DepotClient depotClient)
		{
			string root = depotClient.ConnectionClient().Root;
			Assert(String.IsNullOrEmpty(root) == false);
			return root;
		}

		public string GetClientRoot()
		{
			using (DepotClient depotClient = new DepotClient())
			{
				Assert(depotClient.Connect(ClientConfig));
				return GetClientRoot(depotClient);
			}
		}

		public string[] CommandLineArgs
		{
			get; set;
		}

		public string InteractiveOverridePasswd
		{
			get
			{
				string passwd = null;
				RegistryInfo.GetTypedValue<string>(Microsoft.Win32.Registry.LocalMachine, VirtualFileSystem.AppRegistryKey, DepotConstants.P4PASSWD, ref passwd);
				return passwd;
			}
			set
			{
				if (value == null)
					RegistryInfo.DeleteValue(Microsoft.Win32.Registry.LocalMachine, VirtualFileSystem.AppRegistryKey, DepotConstants.P4PASSWD);
				else
					RegistryInfo.SetStringValue(Microsoft.Win32.Registry.LocalMachine, VirtualFileSystem.AppRegistryKey, DepotConstants.P4PASSWD, value);
			}
		}

		public IEnumerable<ReconcileDifference> ReconcilePreview(string folderPath)
		{
			List<ReconcileDifference> differences = new List<ReconcileDifference>();
			using (DepotClient depotClient = new DepotClient())
			{
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));

				// If an existing depot file path is specified, we can infer it's folder path
				DepotResultFStat folderPathFStat = depotClient.FStat(new[]{folderPath}, optionArgs: new[]{"-m","2"});
				if (folderPathFStat.HasError == false && folderPathFStat.Count == 1 && String.IsNullOrEmpty(folderPathFStat[0].ClientFile) == false)
					folderPath = Path.GetDirectoryName(folderPathFStat[0].ClientFile);

				// Find the depot and client paths for this folkerPath so that we can use a format consistently
				DepotResultWhere.Node folderPathMapping = depotClient.WhereFolder(folderPath);
				Assert(folderPathMapping != null);
				string depotFolderSpec = DepotOperations.NormalizePath(String.Format("{0}/...#have", folderPathMapping.DepotPath));

				// Find unopened files that differ from the revision in the depot
				DepotResultView resultDifferentOnClient = depotClient.Run("diff", new[]{ "-ds", "-se", depotFolderSpec });
				if (resultDifferentOnClient.HasError == false)
				{
					foreach (DepotResultNode tag in resultDifferentOnClient.Nodes)
						differences.Add(new ReconcileDifference{ ClientFile = tag["clientFile"], DepotFile = tag["depotFile"], Action = ReconcileAction.DifferentOnClient });
				}

				// Find unopened files that are missing on the client
				DepotResultView resultMissingOnClient = depotClient.Run("diff", new[]{ "-ds", "-sd", depotFolderSpec });
				if (resultDifferentOnClient.HasError == false)
				{
					foreach (DepotResultNode tag in resultMissingOnClient.Nodes)
						differences.Add(new ReconcileDifference{ ClientFile = tag["clientFile"], DepotFile = tag["depotFile"], Action = ReconcileAction.MissingOnClient });
				}

				// Find all of the files currently on the client
				SortedSet<string> clientFileSet = new SortedSet<string>(StringComparer.CurrentCultureIgnoreCase);
				if (Directory.Exists(folderPathMapping.LocalPath))
				{
					foreach (string file in Directory.EnumerateFiles(folderPathMapping.LocalPath, "*", SearchOption.AllDirectories))
						clientFileSet.Add(file);
				}

				// Find unopend files on the client that are not in the depot
				DepotResultView resultFStat = depotClient.Run("fstat", new[]{ depotFolderSpec });
				if (resultFStat.HasError == false)
				{
					foreach (DepotResultNode tag in resultFStat.Nodes)
					{
						string clientFile = tag["clientFile"];
						clientFileSet.Remove(clientFile);

						// Find files on the client that have different readonly type
						if (File.Exists(clientFile) && (FileUtilities.IsReadOnly(clientFile) ^ !DepotInfo.IsWritableFileType(tag["headType"])))
							differences.Add(new ReconcileDifference{ ClientFile = clientFile, Action = ReconcileAction.DifferentType });
					}
				}

				// The remaining files in localFileSet will be the files on the client that are not in the depot
				foreach (string clientFile in clientFileSet)
					differences.Add(new ReconcileDifference{ ClientFile = clientFile, Action = ReconcileAction.OnClientOnly });
			}
			return differences;
		}

		public DepotResultWhere Where(string path)
		{
			using (DepotClient depotClient = new DepotClient())
			{
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));
				DepotResultWhere where = depotClient.Where(path);
				Assert(where != null);
				return where;
			}
		}

		public bool IsPlaceholderFile(string filePath)
		{
			if (String.IsNullOrEmpty(filePath))
				return false;
			if (File.Exists(filePath) == false)
				return false;
			if (NativeMethods.GetFilePopulateInfo(filePath) == null)
				return false;
			using (DepotClient depotClient = new DepotClient())
			{
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));
				Int64 localSize = FileUtilities.GetFileLength(filePath);
				Int64 depotSize = depotClient.GetDepotSize(new[]{ String.Format("{0}#have", filePath) });
				Assert(localSize >= 0 && depotSize >= 0 && depotSize == localSize);
				Int64 diskSize = NativeMethods.GetFileDiskSize(filePath);
				Assert(diskSize == 0);
			}
			return true;
		}

		public DateTime GetServiceLastRequestTime()
		{
			Extensions.SocketModel.SocketModelClient serviceClient = new Extensions.SocketModel.SocketModelClient();
			return serviceClient.GetServiceStatus()?.LastRequestTime ?? DateTime.MinValue;
		}

		public int GetServiceIdleConnectionCount()
		{
			return ProcessInfo.ExecuteWaitOutput(P4Exe, String.Format("{0} monitor show -a -l", ClientConfig), echo:true).Lines
				.Count(line => Regex.IsMatch(line, @"^\s*(?<id>\d+)\s+(?<status>\w)\s+(?<user>\S+)\s+(?<duration>\S+)\s+(IDLE)"));
		}

		public bool ServiceGarbageCollect()
		{
			Extensions.SocketModel.SocketModelClient service = new Extensions.SocketModel.SocketModelClient(); 
			return service.GarbageCollect();
		}

		public FileDifferenceSummary DiffAgainstWorkspace(string fileSpec)
		{
			FileDifferenceSummary summary = new FileDifferenceSummary();	
			using (DepotClient depotClient = new DepotClient())
			{
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));
				DepotResultView resultDiff = depotClient.Run(new DepotCommand{ Name="diff", Args=new[]{ "-ds", "-f", "-t", fileSpec }, Flags=DepotCommandFlags.UnTagged }).ToView();
				Assert(resultDiff?.Result != null);
				foreach (DepotResultText text in resultDiff.Result.TextList)
				{
					Match m = Regex.Match(text.Value, @"(?<type>add|deleted|changed)\s.*?(?<count>\d+)\s+lines");
					if (m.Success == false)
						continue;

					uint count = UInt32.Parse(m.Groups["count"].Value);
					switch (m.Groups["type"].Value)
					{
						case "add":		summary.LinesAdded += count;	break;
						case "deleted":	summary.LinesDeleted += count;	break;
						case "changed": summary.LinesChanged += count;	break;
					}
				}
			}
			return summary;
		}

		public IEnumerable<ServiceSettingsScope> EnumerateCommonServicePopulateSettings(bool verbose = true)
		{
			foreach (FilePopulateMethod populateMethod in new[]{ FilePopulateMethod.Copy, FilePopulateMethod.Stream })
			{
				ServiceSettingsScope item = new ServiceSettingsScope();
				item.ServiceSettings.Add(new ServiceSettingScope("PopulateMethod", populateMethod.ToString()));
				if (verbose)
					VirtualFileSystemLog.Info("EnumerateCommonServicePopulateSettings: {0}", item);
				yield return item;
			}
		}

		public IEnumerable<ServiceSettingsScope> EnumerateCommonServiceSyncSettings(bool verbose = true)
		{
			foreach (ServiceSettingsScope item in EnumerateCommonServicePopulateSettings(false))
			{
				foreach (DepotSyncFlags primarySyncFlags in new[]{ DepotSyncFlags.Normal, DepotSyncFlags.Quiet, DepotSyncFlags.IgnoreOutput })
				{
					item.SyncFlags = primarySyncFlags;
					if (verbose)
						VirtualFileSystemLog.Info("EnumerateCommonServiceSyncSettings: {0}", item);
					yield return item;
				}
			}
		}

		public IEnumerable<ServiceSettingsScope> EnumerateCommonServiceSyncLineEndSettings(bool verbose = true)
		{
			foreach (ServiceSettingsScope item in EnumerateCommonServiceSyncSettings(false))
			{
				foreach (string lineEnd in new[]{ DepotResultClient.LineEnd.Local, DepotResultClient.LineEnd.Unix })
				{
					item.LineEnd = lineEnd;
					if (verbose)
						VirtualFileSystemLog.Info("EnumerateCommonServiceSyncLineEndSettings: {0}", item);
					yield return item;
				}
			}
		}

		public IEnumerable<string> EnumerateCommonConsoleSyncOptions(bool verbose = true)
		{
			foreach (string p0 in new[]{ "-s", "-t", "-q", "-l" })
			{
				foreach (string p1 in new[]{ "-m Single", "-m Atomic", "-r" })
				{
					string item = String.Format("{0} {1}", p0, p1);
					if (verbose)
					{
						VirtualFileSystemLog.Info("EnumerateCommonConsoleSyncOptions: {0}", item);
					}
					yield return item;
				}
			}
		}

		public static bool ExtractResourceToFile(string resourceName, string filePath, Assembly resourceAssembly = null)
		{
			try
			{
				Assert(String.IsNullOrEmpty(resourceName) == false);
				Assert(String.IsNullOrEmpty(filePath) == false);
				Assembly assembly = resourceAssembly ?? Assembly.GetExecutingAssembly();
				string fullResourceName = resourceAssembly == null ? String.Format("{0}.Resource.{1}", Path.GetFileNameWithoutExtension(assembly.Location), resourceName) : resourceName;
				FileUtilities.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(filePath)));
				using (Stream resourceStream = assembly.GetManifestResourceStream(fullResourceName))
				{
					Assert(resourceStream != null);
					using (FileStream fileStream = File.Open(filePath, FileMode.Create))
						resourceStream.CopyTo(fileStream);
				}
				return true;
			}
			catch (Exception e) 
			{
				VirtualFileSystemLog.Error("Failed ExtractResourceToFile: {0}", e.Message);
			}
			return false;
		}

		public static int GetPriority(MethodInfo methodInfo)
		{
			PriorityAttribute priorityAttribute = methodInfo.GetCustomAttribute<PriorityAttribute>();
			return (priorityAttribute?.Priority).GetValueOrDefault(0) + GetPriority(methodInfo.DeclaringType);
		}

		public static int GetPriority(Type classType)
		{
			BasePriorityAttribute basePriorityAttribute = classType.GetCustomAttribute<BasePriorityAttribute>();
			return (basePriorityAttribute?.Priority).GetValueOrDefault(0);
		}
	}

	[AttributeUsage(AttributeTargets.Method, AllowMultiple = false)]
	public class TestExplicitAttribute : Attribute
	{
		public TestExplicitAttribute()
		{
		}
	}

	[AttributeUsage(AttributeTargets.Method, AllowMultiple = false)]
	public class TestRemoteAttribute : Attribute
	{
		public TestRemoteAttribute()
		{
		}
	}

	[AttributeUsage(AttributeTargets.Method, AllowMultiple = false)]
	public class TestAlwaysAttribute : Attribute
	{
		public TestAlwaysAttribute()
		{
		}
	}

	[AttributeUsage(AttributeTargets.Class, AllowMultiple = false)]
	public class BasePriorityAttribute : Attribute
	{
		public BasePriorityAttribute(int priority)
		{
			Priority = priority;
		}

		public int Priority
		{
			get; set;
		}
	}

	public class TestMethodInfo
	{
		public MethodInfo Info;
		public int Priority;
	}

	public struct ReconcileDifference
	{
		public string DepotFile;
		public string ClientFile;
		public ReconcileAction Action;
	}

	public enum ReconcileAction
	{
		MissingOnClient,
		DifferentOnClient,
		OnClientOnly,
		DifferentType,
	}

	public struct FileDifferenceSummary
	{
		public uint LinesAdded;
		public uint LinesDeleted;
		public uint LinesChanged;
		public uint LinesTotal { get { return LinesAdded + LinesDeleted + LinesChanged; } }
	}

	public class LocalSettingScope : IDisposable
	{
		public string Name { get; private set; }
		public string Value { get; private set; }
		public SettingNode PreviousValue { get; private set; }

		public LocalSettingScope(string name, string value)
		{
			Name = name;
			UnitTestBase.Assert(String.IsNullOrEmpty(Name) == false);
			Value = value;
			UnitTestBase.Assert(Value != null);

			PreviousValue = ServiceSettings.GetProperty(Name);
			UnitTestBase.Assert(PreviousValue.ToString() != null);
			UnitTestBase.Assert(ServiceSettings.SetProperty(SettingNode.FromString(Value), Name));
			UnitTestBase.Assert(ServiceSettings.GetProperty(Name).ToString() == Value);
		}

		public void Dispose()
		{
			UnitTestBase.Assert(ServiceSettings.SetProperty(PreviousValue, Name));
		}

		public override string ToString()
		{
			return String.Format("{0}={1}", Name, Value);
		}
	}

	public class ServiceSettingScope : IDisposable
	{
		public string Name { get; private set; }
		public string Value { get; private set; }
		public SettingNode PreviousValue { get; private set; }

		public ServiceSettingScope(string name, string value)
		{
			Name = name;
			UnitTestBase.Assert(String.IsNullOrEmpty(Name) == false);
			Value = value;
			UnitTestBase.Assert(Value != null);

			Extensions.SocketModel.SocketModelClient serviceClient = new Extensions.SocketModel.SocketModelClient();
			PreviousValue = serviceClient.GetServiceSetting(Name);
			UnitTestBase.Assert(PreviousValue.ToString() != null);
			UnitTestBase.Assert(serviceClient.SetServiceSetting(Name, SettingNode.FromString(Value)));
			UnitTestBase.Assert(serviceClient.GetServiceSetting(Name).ToString() == Value);
		}

		public void Dispose()
		{
			Extensions.SocketModel.SocketModelClient serviceClient = new Extensions.SocketModel.SocketModelClient();
			UnitTestBase.Assert(serviceClient.SetServiceSetting(Name, PreviousValue));
		}

		public override string ToString()
		{
			return String.Format("{0}={1}", Name, Value);
		}

		public void ApplyGlobal()
		{
			UnitTestBase.Assert(ServiceSettings.SetProperty(new SettingNode(Value), Name));
			UnitTestBase.Assert(ServiceSettings.GetPropertyJson(Name).ToString() == ServiceSettings.SettingNodeToJson(new SettingNode(Value)).ToString());
		}
	}

	public class ServiceSettingsScope : IDisposable
	{
		public List<ServiceSettingScope> ServiceSettings = new List<ServiceSettingScope>();
		public DepotSyncFlags? SyncFlags;
		public string LineEnd;

		public void Dispose()
		{
			foreach (ServiceSettingScope setting in ServiceSettings)
				setting.Dispose();
		}

		public override string ToString()
		{
			string result = String.Format("ServiceSettings=[{0}]", String.Join(",", ServiceSettings));
			if (SyncFlags.HasValue)
				result += String.Format(", SyncFlags=[{0}]", SyncFlags.Value);
			if (LineEnd != null)
				result += String.Format(", LineEnd={0}", LineEnd);
			return result;
		}

		public void ApplyGlobal()
		{
			foreach (ServiceSettingScope setting in ServiceSettings)
				setting.ApplyGlobal();
		}
	}

	public class DepotTunableScope : IDisposable
	{
		public string Name { get; private set; }

		public DepotTunableScope(string name, int value)
		{
			Name = name;

			UnitTestBase.Assert(String.IsNullOrEmpty(Name) == false);	
			UnitTestBase.Assert(DepotTunable.IsKnown(Name));
			UnitTestBase.Assert(DepotTunable.IsSet(Name) == false);
			DepotTunable.Set(Name, value);
			UnitTestBase.Assert(DepotTunable.IsSet(Name));
			UnitTestBase.Assert(DepotTunable.Get(Name) == value);
		}

		public void Dispose()
		{
			UnitTestBase.Assert(DepotTunable.IsSet(Name));
			DepotTunable.Unset(Name);
			UnitTestBase.Assert(DepotTunable.IsSet(Name) == false);
		}
	}
}
