// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Linq;
using System.IO;
using System.Diagnostics;
using System.Reflection;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text.RegularExpressions;
using System.Security.Principal;
using System.Management;
using Microsoft.Win32;

namespace Microsoft.P4VFS.Setup
{
	public class Program
	{
		public static SetupWindow _SetupWindow;
		public static SetupConfiguration _Configuration;
		public static bool _Admin;
		public static bool _Console;
		public static bool _P4vfsDebug;
		public const string _HelpText =
@"
Perforce Virtual File System Setup tool.
The Coalition Microsoft Studio

Usage:
  P4VFS.Setup [options] command

Options:
  -b          Launch and attach a debugger before command execution
  -a          Assume application is running as administrator
  -c          Run as a console application
  -i <file>   Include new file or substitute in place of embedded resource.
  -d          Launch and attach a debugger for p4vfs commands

Commands:

  help        Print this help

  reinstall   Default command. Performs a complete uninstall and install
              of the virtual file system driver, service, and tools

  install     Performs a complete install

  uninstall   Performs a complete uninstall

  extract     Extract this installer to a specified folder, or current directory
              if argument not specified. Does not perform an installation.
";

		[STAThread]
		public static int Main(string[] args)
		{
			bool status = true;
			_Admin = false;
			_Console = false;
			_P4vfsDebug = false;
			_Configuration = new SetupConfiguration();

			_Configuration.Import(LoadConfiguration());

			int argIndex = 0;
			for (; argIndex < args.Length; ++argIndex)
			{
				if (String.Compare(args[argIndex], "-b") == 0)
					System.Diagnostics.Debugger.Launch();
				else if (String.Compare(args[argIndex], "-a") == 0)
					_Admin = true;
				else if (String.Compare(args[argIndex], "-c") == 0)
					_Console = true;
				else if (String.Compare(args[argIndex], "-i") == 0 && argIndex+1 < args.Length)
					_Configuration.IncludeFiles.Add(args[++argIndex]);
				else if (String.Compare(args[argIndex], "-d") == 0)
					_P4vfsDebug = true;
				else
					break;
			} 

			if (_Admin == false && IsProcessElevated() == false)
			{
				List<string> shellArgs = new List<string>();
				shellArgs.Add("-a");
				//shellArgs.Add("-b");
				shellArgs.AddRange(args);
				return ExecuteWait(Assembly.GetExecutingAssembly().Location, String.Format("\"{0}\"", String.Join("\" \"", shellArgs)), shell:true, hidden:false);
			}

			if (argIndex >= args.Length)
			{
				argIndex = 0;
				args = new string[]{"reinstall"};
			}

			string command = args[argIndex];
			string[] commandArgs = args.Skip(argIndex+1).ToArray();

			if (_Console == false)
			{
				_SetupWindow = new SetupWindow();
				_SetupWindow.DoWork = () => { status = RunCommand(command, commandArgs); return status; };
				_SetupWindow.ShowDialog();
			}
			else
			{
				AttachConsole(UInt32.MaxValue);
				status = RunCommand(command, commandArgs);
			}

			return status ? 0 : 1;
		}

		private static bool RunCommand(string command, string[] commandArgs)
		{
			bool status = false;
			try
			{
				WriteLine(String.Format("P4VFS.Setup Version: {0}", CoreInterop.NativeConstants.Version));
				switch (command)
				{
					case "help":
					case "/?":
						status = CommandHelp();
						break;
					case "reinstall":
						status = CommandReinstall(commandArgs);
						break;
					case "install":
						status = CommandInstall(commandArgs);
						break;
					case "uninstall":
						status = CommandUninstall(commandArgs);
						break;
					case "extract":
						status = CommandExtract(commandArgs);
						break;
					default:
						WriteLine(String.Format("P4VFS.Setup Unknown Command {0}", command));
						status = false;
						break;
				}
			}
			catch (Exception e)
			{
				WriteLine(String.Format("P4VFS.Setup Error: {0}", e.Message));
				status = false;
			}

			if (status == false)
			{
				WriteStatus("Setup encountered errors!");
			}

			WriteLine(String.Format("P4VFS.Setup {0}", status ? "SUCCEEDED" : "FAILED"));
			return status;
		}

		private static string InstallFolder
		{
			get { return Environment.ExpandEnvironmentVariables("%ProgramFiles%\\P4VFS"); }
		}

		private static string LocalMachineSoftwareKey
		{
			get { return "SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"; }
		}

		private static String ApplicationDisplayName
		{
			get { return "P4VFS"; }
		}

		private static String ApplicationSetupExe
		{
			get { return "P4VFS.Setup.exe"; }
		}

		private static bool CommandHelp()
		{
			WriteLine(_HelpText.TrimStart());
			return true;
		}

		private static bool CommandReinstall(string[] args)
		{
			using (LogProgress progress = new LogProgress(2))
			{
				progress.WriteLine(0, null);
				if (CommandUninstall(args.Concat(new[]{"-p"}).ToArray()) == false)
					return false;

				progress.WriteLine(1, null);
				if (CommandInstall(args) == false)
					return false;
			}
			return true;
		}

		private static bool CommandInstall(string[] args)
		{
			WriteStatus("Installing ...");
			using (LogProgress progress = new LogProgress(8))
			{
				progress.WriteLine(1, "Performing Install ...");

				string stagingFolder = CreateStagingFolder();
				if (Directory.Exists(stagingFolder) == false)
				{
					return false;
				}

				if (Directory.Exists(InstallFolder))
				{
					WriteLine(String.Format("Failed to install over existing installation: {0}", InstallFolder));
					return false; 
				}

				progress.WriteLine(2, String.Format("Creating: {0}", InstallFolder));
				if (ExecuteWait("xcopy.exe", String.Format("/s /y /r /f \"{0}\" \"{1}\\\"", stagingFolder, InstallFolder)) != 0)
				{
					WriteLine(String.Format("Failed copy: {0} -> {1}", stagingFolder, InstallFolder));
					return false;
				}

				progress.WriteLine(3, String.Format("Installing Dependencies ..."));
				if (InstallDependencies(stagingFolder) == false)
				{
					WriteLine("Failed installing dependencies");
					return false;
				}

				progress.WriteLine(4, String.Format("Installing application: {0}", InstallFolder));
				if (ExecuteWait(String.Format("{0}\\p4vfs.exe", InstallFolder), AppendP4vfsArgs(new[]{"install"})) != 0)
				{
					WriteLine(String.Format("Failed install using: {0}\nBacking out ...", String.Format("{0}\\p4vfs.exe", InstallFolder)));
					if (ExecuteWait(String.Format("{0}\\p4vfs.exe", InstallFolder), AppendP4vfsArgs(new[]{"uninstall"})) != 0)
						WriteLine("Failed backout installation!");
					return false;
				}

				progress.WriteLine(5, String.Format("Installing environment ..."));
				if (InstallEnvironment() == false)
				{
					WriteLine(String.Format("Failed installing system environment"));
					return false;
				}

				progress.WriteLine(6, String.Format("Installing application setup ..."));
				if (InstallApplicationSetup() == false)
				{
					WriteLine(String.Format("Failed installing requirements for application setup"));
					return false;
				}

				progress.WriteLine(7, String.Format("Removing: {0}", stagingFolder));
				if (RemoveDirectoryRecursive(stagingFolder) == false)
				{
					WriteLine(String.Format("Failed delete staging folder: {0}", stagingFolder));
					return false;
				}
			}
			WriteStatus("Install complete!");
			return true;
		}

		private static bool CommandUninstall(string[] args)
		{
			WriteStatus("Uninstalling ...");
			using (LogProgress progress = new LogProgress(8))
			{
				progress.WriteLine(1, "Performing Uninstall ...");

				if (Directory.Exists(InstallFolder) == true)
				{
					string stagingFolder = CreateStagingFolder();
					if (Directory.Exists(stagingFolder) == false)
					{
						return false;
					}

					progress.WriteLine(2, String.Format("Shutting down applications ..."));
					if (ExecuteWait(String.Format("{0}\\p4vfs.exe", stagingFolder), AppendP4vfsArgs(new[]{"monitor", "hide"})) != 0)
					{
						WriteLine(String.Format("Failed shutting down applications using: {0}", String.Format("{0}\\p4vfs.exe", stagingFolder)));
						return false;
					}

					progress.WriteLine(3, String.Format("Uninstalling application: {0}", stagingFolder));
					if (ExecuteWait(String.Format("{0}\\p4vfs.exe", stagingFolder), AppendP4vfsArgs(new[]{"uninstall"}.Concat(args))) != 0)
					{
						WriteLine(String.Format("Failed uninstall using: {0}", String.Format("{0}\\p4vfs.exe", stagingFolder)));
						return false;
					}

					progress.WriteLine(4, String.Format("Removing: {0}", stagingFolder));
					if (RemoveDirectoryRecursive(stagingFolder) == false)
					{
						WriteLine(String.Format("Failed delete staging folder: {0}", stagingFolder));
						return false;
					}

					progress.WriteLine(5, String.Format("Removing application setup ..."));
					if (UninstallApplicationSetup() == false)
					{
						WriteLine(String.Format("Failed uninstalling requirements for application setup"));
					}

					progress.WriteLine(6, String.Format("Removing: {0}", InstallFolder));
					if (RemoveDirectoryRecursive(InstallFolder) == false)
					{
						WriteLine(String.Format("Unable to delete install folder: {0}", InstallFolder));
					}
				}

				progress.WriteLine(7, String.Format("Uninstalling legacy applications ..."));
				if (UninstallLegacyApplications() == false)
				{
					WriteLine(String.Format("Failed uninstalling legacy applications"));
					return false;
				}
			}
			WriteStatus("Uninstall complete!");
			return true;
		}

		private static bool CommandExtract(string[] args)
		{
			WriteStatus("Extracting ...");

			string[] resourceNames = Assembly.GetExecutingAssembly().GetManifestResourceNames();
			using (LogProgress progress = new LogProgress(resourceNames.Length))
			{
				string targetFolder = Path.GetFullPath(args.FirstOrDefault() ?? Environment.CurrentDirectory);
				WriteStatus(String.Format("Extracting to folder: {0}", targetFolder));
				Directory.CreateDirectory(targetFolder);

				if (ExtractResourcesToFolder(resourceNames, targetFolder, progress) == false)
					return false;
			}

			WriteStatus("Extraction complete!");
			return true;
		}

		private static bool InstallEnvironment()
		{
			try
			{
				string envKeyName = "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment";
				using (RegistryKey envKey = Registry.LocalMachine.OpenSubKey(envKeyName, true))
				{
					if (envKey == null)
					{
						WriteLine(String.Format("Failed to open registry key: {0}", envKeyName));
						return false;
					}
				
					string srcPath = envKey.GetValue("PATH", "", RegistryValueOptions.DoNotExpandEnvironmentNames) as string;
					if (String.IsNullOrEmpty(srcPath))
					{
						WriteLine(String.Format("Failed to find PATH value in registry key: {0}", envKeyName));
						return false;
					}

					List<string> paths = new List<string>(srcPath.Split(new char[]{';'}, StringSplitOptions.RemoveEmptyEntries));
					if (paths.Any(s => String.Compare(InstallFolder, Regex.Replace(s, @"[\\/]+", "\\").TrimEnd('\\'), StringComparison.InvariantCultureIgnoreCase) == 0) == false)
					{
						paths.Add(InstallFolder);
					}
					
					string dstPath = String.Join(";", paths);
					envKey.SetValue("PATH", dstPath, RegistryValueKind.ExpandString);
					RefreshEnvironmentVariables();
				}
			}
			catch (Exception e)
			{
				WriteLine(String.Format("Exception installing environment: {0}\n{1}", e, e.StackTrace));
				return false;
			}
			try
			{
				Registry.SetValue(Registry.LocalMachine.Name+"\\SYSTEM\\CurrentControlSet\\Control\\FileSystem", "LongPathsEnabled", 1, RegistryValueKind.DWord);
			}
			catch (Exception e)
			{
				WriteLine(String.Format("Exception enabling long path support: {0}\n{1}", e, e.StackTrace));
			}
			return true;
		}

		private static void RefreshEnvironmentVariables()
		{
			// Set and Remove a system environment variable to force explorer.exe to be updated
			Environment.SetEnvironmentVariable("P4VFS_INSTALL", InstallFolder, EnvironmentVariableTarget.Machine);
			Environment.SetEnvironmentVariable("P4VFS_INSTALL", null, EnvironmentVariableTarget.Machine);
		}

		private static bool InstallApplicationSetup()
		{
			try
			{
				string appKeyName = String.Format("{0}\\{1}", LocalMachineSoftwareKey, ApplicationDisplayName);
				using (RegistryKey appKey = Registry.LocalMachine.CreateSubKey(appKeyName))
				{
					if (appKey == null)
					{
						WriteLine(String.Format("Failed to open registry key: {0}", appKeyName));
						return false;
					}

					string installedSetupExe = Path.Combine(InstallFolder, ApplicationSetupExe);

					appKey.SetValue("DisplayIcon", installedSetupExe, RegistryValueKind.String);
					appKey.SetValue("DisplayName", ApplicationDisplayName, RegistryValueKind.String);
					appKey.SetValue("DisplayVersion", Microsoft.P4VFS.CoreInterop.NativeConstants.Version, RegistryValueKind.String);
					appKey.SetValue("UninstallString", String.Format("\"{0}\" uninstall", installedSetupExe), RegistryValueKind.String);
					appKey.SetValue("Publisher", "Microsoft Corporation", RegistryValueKind.String);
					appKey.SetValue("InstallLocation", InstallFolder, RegistryValueKind.String);

					foreach (SetupRegistryKey setupKey in _Configuration.RegistryKeys)
					{
						WriteLine(String.Format("Setting additional registry key: {0}={1}", setupKey.Name, setupKey.Value));
						appKey.SetValue(setupKey.Name, setupKey.Value, RegistryValueKind.String);
					}
				}
			}
			catch (Exception e)
			{
				WriteLine(String.Format("Exception installing application setup: {0}\n{1}", e, e.StackTrace));
				return false;
			}
			return true;
		}

		private static bool UninstallApplicationSetup()
		{
			try
			{
				string appKeyName = String.Format("{0}\\{1}", LocalMachineSoftwareKey, ApplicationDisplayName);
				Registry.LocalMachine.DeleteSubKey(appKeyName, false);
			}
			catch (Exception e)
			{
				WriteLine(String.Format("Exception uninstalling application setup: {0}\n{1}", e, e.StackTrace));
				return false;
			}
			return true;
		}

		private static bool UninstallLegacyApplications()
		{
			try
			{
				string uninstallLocation = GetRegistryValue<string>(String.Format("{0}\\{1}\\BlackTusk.VFS", Registry.LocalMachine, LocalMachineSoftwareKey), "InstallLocation");
				if (String.IsNullOrEmpty(uninstallLocation) == false)
				{
					string uninstallExe = "BlackTusk.VFS.Setup.exe";
					string uninstallArgs = "-a -c uninstall";

					WriteLine(String.Format("Legacy Uninstall: \"{0}\" {1}", uninstallExe, uninstallArgs));
					if (ExecuteWait(uninstallExe, uninstallArgs, cwd : uninstallLocation) != 0)
					{
						WriteLine("Error executing BlackTusk.VFS uninstall");
						return false;
					}

					WriteLine(String.Format("Removing Folder: {0}", uninstallLocation));
					if (ExecuteWait("cmd.exe", String.Format("/s /c \"rmdir /s /q \"{0}\"\"", uninstallLocation)) != 0)
					{
						WriteLine(String.Format("Unable to delete folder: {0}", uninstallLocation));
					}
				}

				string driverInstallString = GetRegistryValue<string>(String.Format("{0}\\SYSTEM\\CurrentControlSet\\Services\\BlackTusk.VFS.Driver", Registry.LocalMachine), "DisplayName");
				if (String.IsNullOrEmpty(driverInstallString) == false)
				{
					WriteLine(String.Format("Unexpected installation of BlackTusk.VFS.Driver still exists: {0}", driverInstallString));
					return false;
				}

				string serviceInstallString = GetRegistryValue<string>(String.Format("{0}\\SYSTEM\\CurrentControlSet\\Services\\BlackTusk.VFS.Service", Registry.LocalMachine), "DisplayName");
				if (String.IsNullOrEmpty(serviceInstallString) == false)
				{
					WriteLine(String.Format("Unexpected installation of BlackTusk.VFS.Service still exists: {0}", serviceInstallString));
					return false;
				}
			}
			catch (Exception e)
			{
				WriteLine(String.Format("Exception uninstalling legacy application: {0}", e.Message));
				return false;
			}
			return true;
		}

		private static T GetRegistryValue<T>(string keyName, string valueName, T defaultValue = default(T))
		{
			try
			{
				return (T)Registry.GetValue(keyName, valueName, defaultValue);
			}
			catch {}
			return defaultValue;
		}

		private static string[] GetRegistrySubKeys(RegistryHive hKey, string keyName)
		{
			try
			{
				using (RegistryKey baseKey = RegistryKey.OpenBaseKey(hKey, RegistryView.Default).OpenSubKey(keyName, false))
					return baseKey.GetSubKeyNames().Select(subKeyName => String.Format("{0}\\{1}", baseKey.Name, subKeyName)).ToArray();
			}
			catch {}
			return new string[0];
		}

		private static bool IsVcRedistInstalled(string vcRedistExe)
		{
			try
			{
				string vcRedistVersion;	
				IReadOnlyDictionary<string,string> properties = GetFileProperties(vcRedistExe);
				if (properties.TryGetValue("Version", out vcRedistVersion) && String.IsNullOrEmpty(vcRedistVersion) == false)
				{
					foreach (string appKeyName in GetRegistrySubKeys(RegistryHive.LocalMachine, LocalMachineSoftwareKey))
					{
						if (Regex.IsMatch(GetRegistryValue<string>(appKeyName, "DisplayName", ""), @"Microsoft Visual C\+\+ \d+ Redistributable") == false)
							continue;
						if (GetRegistryValue<string>(appKeyName, "DisplayVersion", "") != vcRedistVersion)
							continue;
						if (GetRegistryValue<int>(appKeyName, "Installed", 0) != 1 && GetRegistryValue<int>(appKeyName, "Install", 0) != 1)
							continue;
						return true;
					}
				}
			}
			catch (Exception e)
			{
				WriteLine(String.Format("Failed checking if vcredist is installed: {0}. Exception: {1}", vcRedistExe, e));
			}
			return false;
		}

		private static bool InstallDependencies(string stagingFolder)
		{
			foreach (string vcRedistExe in Directory.GetFiles(stagingFolder, "vcredist*.exe", SearchOption.TopDirectoryOnly))
			{
				if (IsVcRedistInstalled(vcRedistExe) == false)
				{
					WriteLine(String.Format("Installing runtime: {0}", vcRedistExe));
					ExecuteWait(vcRedistExe, String.Format("/norestart {0}", _Console ? "/quiet" : "/passive"));
				}
			}
			return true;
		}

		public static bool IsProcessElevated()
		{
			try
			{
				using (WindowsIdentity user = WindowsIdentity.GetCurrent())
				{
					WindowsPrincipal principal = new WindowsPrincipal(user);
					return principal.IsInRole(WindowsBuiltInRole.Administrator);
				}
			}
			catch {}
			return false;
		}

		private static int ExecuteWait(string fileName, string arguments, string cwd = "", bool shell = false, bool hidden = true)
		{
			if (Directory.Exists(cwd) == false)
				cwd = Environment.CurrentDirectory;
			if (cwd.StartsWith("\\\\"))
				cwd = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);

			bool redirectOutput = !shell;
			object receiverLock = new Object();
			DataReceivedEventHandler outputReceiver = new System.Diagnostics.DataReceivedEventHandler((s, e) => 
			{
				lock (receiverLock)
				{
					WriteLine(e.Data);
				}
			});

			try
			{
				using (Process p = new Process())
				{
					p.StartInfo.FileName = fileName;
					p.StartInfo.Arguments = arguments;
					p.StartInfo.WorkingDirectory = cwd;
					p.StartInfo.UseShellExecute = shell;
					p.StartInfo.CreateNoWindow = hidden;
					p.StartInfo.WindowStyle = ProcessWindowStyle.Hidden;
					if (shell)
						p.StartInfo.Verb = "runas";
					
					if (redirectOutput)
					{
						p.StartInfo.RedirectStandardOutput = true;
						p.StartInfo.RedirectStandardError = true;
						p.OutputDataReceived += outputReceiver;
						p.ErrorDataReceived += outputReceiver;
					}

					p.Start();

					if (redirectOutput)
					{
						p.BeginOutputReadLine();
						p.BeginErrorReadLine();
					}
					p.WaitForExit();
					return p.ExitCode;
				}
			}
			catch (Exception e)
			{
				WriteLine(String.Format("Failed to run '{0}' '{1}' in '{2}': {3}", fileName, arguments, cwd, e));
			}
			return -1;
		}

		private static string CreateStagingFolder()
		{
			string stagingFolder = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Temp", Path.GetFileNameWithoutExtension(ApplicationSetupExe));
			if (Directory.Exists(stagingFolder) && ExecuteWait("cmd.exe", String.Format("/s /c \"rmdir /s /q \"{0}\"\"", stagingFolder)) != 0)
			{
				WriteLine(String.Format("Failed to delete local staging folder: {0}", stagingFolder));
				return null;
			}

			if (ExecuteWait("cmd.exe", String.Format("/s /c \"mkdir \"{0}\"\"", stagingFolder)) != 0)
			{
				WriteLine(String.Format("Failed to create local staging folder: {0}", stagingFolder));
				return null;
			}

			string[] resourceNames = Assembly.GetExecutingAssembly().GetManifestResourceNames();
			using (LogProgress progress = new LogProgress(resourceNames.Length+_Configuration.IncludeFiles.Count+2))
			{
				WriteLine(String.Format("Staging: {0}", stagingFolder));
				if (ExtractResourcesToFolder(resourceNames, stagingFolder, progress) == false)
					return null;

				foreach (string includeFile in _Configuration.IncludeFiles)
				{
					progress.Increment();
					string includeFilePath = ResolveIncludeFilePath(includeFile);
					WriteLine(String.Format("Including File: {0}", includeFilePath));
					if (CopyFile(includeFilePath, Path.Combine(stagingFolder, Path.GetFileName(includeFilePath))) == false)
						return null;
				}

				progress.Increment();
				if (CopyFile(Assembly.GetExecutingAssembly().Location, Path.Combine(stagingFolder, ApplicationSetupExe)) == false)
					return null;
			}

			return stagingFolder;
		}

		private static bool ExtractResourcesToFolder(string[] resourceNames, string folderPath, LogProgress progress = null)
		{
			foreach (string resourceName in resourceNames)
			{
				progress?.Increment();
				string fileName = Regex.Replace(resourceName, @"^P4VFS\.Setup\.(Resource\.)?", "");
				if (String.IsNullOrEmpty(fileName) || fileName.EndsWith(".resources", StringComparison.InvariantCultureIgnoreCase))
					continue;

				WriteLine(String.Format("Extracting Resource: {0}", fileName));
				string filePath = Path.Combine(folderPath, fileName);
				if (ExtractResourceToFile(resourceName, filePath) == false)
					return false;
			}
			return true;
		}

		private static bool ExtractResourceToFile(string resourceName, string filePath)
		{
			try
			{
				Stream resourceStream = Assembly.GetExecutingAssembly().GetManifestResourceStream(resourceName);
				using (FileStream fileStream = File.Open(filePath, FileMode.Create))
					resourceStream.CopyTo(fileStream);
				return true;
			}
			catch (Exception e)
			{
				WriteLine(String.Format("Failed to extract resource to file: {0} -> {1}. Exception: {2}", resourceName, filePath, e));
			}
			return false;
		}

		private static bool CopyFile(string srcFile, string dstFile)
		{
			try
			{
				File.Copy(srcFile, dstFile, true);
				return true;
			}
			catch (Exception e)
			{
				WriteLine(String.Format("Failed to copy file: {0} -> {1}. Exception: {2}", srcFile, dstFile, e));
			}
			return false; 
		}

		private static string ResolveIncludeFilePath(string filePath)
		{
			if (String.IsNullOrEmpty(filePath) == false)
			{
				string fullFilePath = Path.GetFullPath(filePath);
				if (File.Exists(fullFilePath))
				{
					return fullFilePath;
				}
				if (Path.IsPathRooted(filePath) == false)
				{
					fullFilePath = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), filePath));
					if (File.Exists(fullFilePath))
					{
						return fullFilePath;
					}
				}
			}
			return filePath;
		}

		private static SetupConfiguration LoadConfiguration()
		{
			string[] configFiles = new[]{
				"P4VFS.Setup.xml",
				String.Format("{0}.xml", Path.GetFileName(Assembly.GetExecutingAssembly().Location)),
			};

			foreach (string configFile in configFiles)
			{
				string configFilePath = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), configFile));
				if (File.Exists(configFilePath))
				{
					SetupConfiguration config = SetupConfiguration.LoadFromFile(configFilePath);
					if (config != null)
					{
						WriteLine(String.Format("Loaded Configuration: {0}", configFilePath));
						return config;
					}
				}
			}
			return null;
		}

		public static IReadOnlyDictionary<string, string> GetFileProperties(string filePath)
		{
			Dictionary<string, string> properties = new Dictionary<string, string>(StringComparer.InvariantCultureIgnoreCase);
			if (File.Exists(filePath))
			{
				ManagementObjectSearcher mos = new ManagementObjectSearcher(String.Format("select * from CIM_DataFile where Name=\"{0}\"", Path.GetFullPath(filePath).Replace("\\","\\\\")));
				if (mos != null)
				{
					foreach (ManagementObject mo in mos.Get())
					{
						foreach (PropertyData pd in mo.Properties)
						{
							if (String.IsNullOrEmpty(pd.Name))
								continue;
							object value = mo[pd.Name];
							if (value != null)
								properties[pd.Name] = value.ToString();
						}
					}
				}
			}
			return properties;
		}

		private static bool RemoveDirectoryRecursive(string folderPath, int retryWait = 500, int limitWait = 5000)
		{
			DateTime endTime = DateTime.Now.AddMilliseconds((double)limitWait);
			do
			{
				if (Directory.Exists(folderPath) == false || ExecuteWait("cmd.exe", String.Format("/s /c \"rmdir /s /q \"{0}\"\"", folderPath)) == 0)
					return true;
				WriteLine(String.Format("Retrying directory removal [{0} ms] {1}", (endTime-DateTime.Now).TotalMilliseconds, folderPath));
				System.Threading.Thread.Sleep(retryWait);
			}
			while (DateTime.Now < endTime);
			return Directory.Exists(folderPath) == false;
		}

		public static IEnumerable<string> CommonP4vfsArgs()
		{
			List<string> args = new List<string>();
			args.Add("-r");
			if (_P4vfsDebug)
				args.Add("-b");
			return args;
		}

		public static string AppendP4vfsArgs(IEnumerable<string> args)
		{
			return String.Join(" ", CommonP4vfsArgs().Concat(args).Where(s => !String.IsNullOrEmpty(s)));
		}

		public static void WriteLine(string text)
		{
			if (String.IsNullOrEmpty(text) == false)
			{
				if (_SetupWindow != null)
					_SetupWindow.Dispatcher.Invoke(new Action(() => _SetupWindow.WriteLine(text)));

				Console.WriteLine(text);
			}
		}

		public static void WriteStatus(string text)
		{
			if (_SetupWindow != null)
				_SetupWindow.Dispatcher.Invoke(new Action(() => Program._SetupWindow.StatusText = text ?? ""));
		}

		[DllImport("kernel32")]
		private static extern bool AllocConsole();

		[DllImport("kernel32.dll")]
		public static extern bool AttachConsole(UInt32 dwProcessId);
	}

	public class LogProgress : IDisposable
	{
		public LogProgress(int totalSteps = 0)
		{
			if (Program._SetupWindow != null)
				Program._SetupWindow.Dispatcher.Invoke(new Action(() => Program._SetupWindow.PushProgress(0, totalSteps)));
		}

		public void WriteLine(int step, string text)
		{
			if (Program._SetupWindow != null)
				Program._SetupWindow.Dispatcher.Invoke(new Action(() => Program._SetupWindow.SetProgress(step)));

			Program.WriteLine(text);
		}

		public void Increment()
		{
			if (Program._SetupWindow != null)
				Program._SetupWindow.Dispatcher.Invoke(new Action(() => Program._SetupWindow.IncrementProgress()));
		}

		public void Dispose()
		{
			if (Program._SetupWindow != null)
				Program._SetupWindow.Dispatcher.Invoke(new Action(() => Program._SetupWindow.PopProgress()));
		}
	}
}

