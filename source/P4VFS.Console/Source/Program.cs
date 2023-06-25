// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;
using Microsoft.P4VFS.Extensions;
using Microsoft.P4VFS.Extensions.Linq;
using Microsoft.P4VFS.Extensions.Utilities;
using Microsoft.P4VFS.CoreInterop;

namespace Microsoft.P4VFS.Console
{
	public class Program
	{
		private static HelpList _Help = new HelpList {

{"help", @"
Perforce Virtual File System command line tool.
Microsoft Xbox Studios

Usage:

  p4vfs [options] command [arg ...]

Help:

  help commands    List all available commands
  help options     Show global options shared by all commands
  help all         Show all help for all commands
  help <command>   Help on a specific command
"},

{"options", @"
Global options:

  -p <port>   Specifies the perforce server port address, overriding $P4PORT
  -c <client> Specifies the perforce client name, overriding $P4CLIENT
  -u <user>   Specifies the perforce user name, overriding $P4USER
  -d <dir>    Specifies the current working directory, overriding $PWD
  -P <passwd> Specifies the perforce user password, overriding $P4PASSWD
  -H <host>   Specifies the perforce host name, overriding $P4HOST
  -x <file>   Instructs p4vfs to read arguments, one per line, from the
              specified file. If you specify '-', standard input is read.
  -b          Launch and attach a debugger before command execution
  -r          Enable remote logging to publish telemetry for this execution
  -v          Enable verbose logging for this execution
"},

{"commands", @"
Available commands:

  help        Show help for options and commands.
  sync        Synchronize the client with its view of the depot.
  info        Print out client/server information
  set         Modify current service settings temporarily for this login session. 
  resident    Modify current resident status of local files.
  populate    Perform sync as fast as possible using quiet, single flush
  reconfig    Modify the perforce configuration of local placeholder files.
  monitor     Launch and control the P4VFS monitor application.
  install     Install virtual file system driver and service.
  uninstall   Uninstall virtual file system driver and service.
  login       Login to the perforce server and update the current ticket.
  test        Run one or more unit tests.
  ctrl        Perform administrative and debugging operations on P4VFS driver.
  restart     Restart the P4VFS driver and/or service. 
"},

{"sync,populate", @"
  sync        Synchronize the client with its view of the depot.
  populate    Synonym for 'sync -q -m Single'

              p4vfs sync [-v -r -f -q -l -n -k -t -w -s -x <csx> -p <reg>] [file[rev] ...]

   -v         Uses a virtual sync method where any file updates are quickly 
              marked in the file system to be downloaded on-demand when file
              is accessed. (default behaviour)
   -r         Uses the regular perforce sync method where all file updates 
              are performed immediatly and data can be accessed offline. 
              Cannot be used with -v
   -f         Forces resynchronization even if the client already has the 
              file, and overwriting any writable files. This flag doesn't 
              affect open files.
   -q         Minimal progress output will be displayed, except for errors.
   -l         Performs regular logging during sync option. This is the opposite
              as quiet [-q] (default behaviour)
   -n         Flag previews the operation without updating the workspace.
   -k         Flag updates server metadata without effecting files on client. 
              This operation is also known as a flush
   -w         Force clobber of files that are already writeable. Ignores opened files.
   -t         Force the sync to be performed in p4vfs.exe process instead
              of through the service. This method is not guaranteed to leave
              client files consistent if p4vfs.exe is forcefully terminted.
   -s         Allow a guaranteed safe-to-terminate sync to be performed through
              the service using a TCP Socket connection. (default behaviour)
   -x <csx>   Specify a string of comma separated file extensions that should
              be synced and made resident. The <csx> string is of the example 
              format ""cpp,h"" to make resident updated *.cpp and *.h files.
   -p <reg>   Specify a regular expression string to match files that should
              be physically synced and made resident similar to [-x] switch.
   -m <type>  Sets the flush mechanism to use when syncing have file information 
              to the server.
              Single : Performs a fast and quiet sync operation that is not guaranteed to
                       be safe-to-terminate. If p4vfs is terminated, the sync operation
                       may continue in the service untill completed. Faster but can leave 
                       an inconsistent state if cancelled.
              Atomic : Performs a guaranteed safe-to-terminate sync to be performed where
                       a Flush is sent to the server per file synced. This operation
                       is cancellation safe.
"},

{"info", @"
  info        Display the current perforce client/server information
              Lists information about current connection depending on current
              environment, command line options, current directory, p4 set
              and P4CONFIG settings.

              p4vfs info [-x]

   -x         Show extended information including service and driver settings
"},

{"set", @"
  set         Modify current service settings temporarily for this login session. 
              These temporarly setting changes will not persist after service is
              restarted. This command can also be used to display current values of 
              the service settings by name or substring.

              p4vfs set <SettingName>=[SettingValue] 
              p4vfs set [SettingName|SubString]
"},

{"resident,hydrate", @"
  resident    Modify current resident status of local files. This can be used
              to change existing files back to virtual state (zero downloaded size),
              or change files to a resident state (full downloaded size).
  hydrate     Synonym for 'resident -r'

              p4vfs resident [-r -v -x <csx> -p <reg>] [file ...]

   -n         Flag previews the operation without updating the workspace.
   -r         Change file status to resident state (full downloaded size). Default.
   -v         Change file status to virtual state (zero downloaded size).
   -t         Force the sync to be performed in p4vfs.exe process instead
              of through the service. This method is not guaranteed to leave
              client files consistent if p4vfs.exe is forcefully terminted.
   -s         Allow a guaranteed safe-to-terminate sync to be performed through
              the service using a TCP Socket connection. (default behaviour)
   -x <csx>   Specify a string of comma separated file extensions to operate on.
   -p <reg>   Specify a regular expression string to match files to operate on.
"},

{"reconfig", @"
  reconfig    Modify the perforce configuration of local placeholder files. 
              This can be used to reconfigure existing virtual synced (placeholder) 
              files to reference a different P4PORT or P4CLIENT. This is commonly
              used to change files in an existing workspace to use a different 
              perforce server endpoint (ie, a different broker or proxy server)
              without requiring a force sync #have.

              p4vfs reconfig [-n -p -c -u] [file ...]

   -n         Flag previews the operation without updating the workspace.
   -p         Set current P4PORT for existing placeholder files. 
              Default enabled unless -c or -u is specified.
   -c         Set current P4CLIENT for existing placeholder files.
   -u         Set current P4USER for existing placeholder files.
"},

{"monitor", @"
  monitor     Launch and control the P4VFS monitor application to gather and
              display status of current P4VFS activity on this machine.

              p4vfs monitor [-s|show] [-h|hide]

   -s         Show the P4VFS monitor application in the taskbar
   -h         Hide the P4VFS monitor application
"},

{"install", @"
  install     Install virtual file system driver and service.
              If no options are specified, all types will be installed.

              p4vfs install [-s -d]

   -s         Install the virtual file system service only
   -d         Install the virtual file system driver only
"},

{"uninstall", @"
  uninstall   Uninstall virtual file system driver and service.
              If no options are specified, all types will be uninstalled.

              p4vfs uninstall [-s -d]

   -s         Uninstall the virtual file system service only
   -d         Uninstall the virtual file system driver only
   -p         Preserve the service as stopped and undeleted. This is suitable
              for quick reinstalls while preserving opened service handles.
"},

{"login", @"
  login       Login to the perforce server and update the current ticket.

              p4vfs login [-i -w] [password]

   -i         Display a modal dialog for password entry
   -w         Write the password to stdout after entering
"},

{"test", @"
  test        Run one or more unit tests from Microsoft.P4VFS.UnitTest.dll
              If no options are specified, all tests will be run.

              p4vfs test [name ...]
"},

{"ctrl", @"
  ctrl        Perform administrative and debugging operations on P4VFS driver

              p4vfs ctrl [-dc] [-dv] [-sv] [-f <name>=<value>]

   -dc        Print the current connectivity state of the driver
   -dv        Print the current driver version
   -sv        Print the current service version
   -f <v>     Set experimental driver flag by <name>=<value>. Examples include:

              p4vfs ctrl -f SanitizeAttributes=1
              p4vfs ctrl -f ShareModeDuringHydration=0
"},

{"restart", @"
  restart     Restart the driver and/or service. Usefull for testing or 
              reinitializing service settings.

              p4vfs restart [-s -d]

   -s         Restart the virtual file system service only
   -d         Restart the virtual file system driver only
"},
};

		private static string _P4Client;
		private static string _P4Port;
		private static string _P4User;
		private static string _P4Directory;
		private static string _P4Passwd;
		private static string _P4Host;
		private static string _P4ArgFile;

		public static int Main(string[] args)
		{
			bool status = true;
			try
			{
				VirtualFileSystemLog.Intitialize();
				SettingManager.RemoteLogging = SettingManager.ConsoleRemoteLogging;
				SettingManager.ImmediateLogging = SettingManager.ConsoleImmediateLogging;
				_P4Directory = Environment.CurrentDirectory;

				int argIndex = 0;
				for (; argIndex < args.Length; ++argIndex)
				{
					if (String.Compare(args[argIndex], "-c") == 0 && argIndex+1 < args.Length)
					{
						_P4Client = args[++argIndex];
					}
					else if (String.Compare(args[argIndex], "-p") == 0 && argIndex+1 < args.Length)
					{
						_P4Port = args[++argIndex];
					}
					else if (String.Compare(args[argIndex], "-u") == 0 && argIndex+1 < args.Length)
					{
						_P4User = args[++argIndex];
					}
					else if (String.Compare(args[argIndex], "-d") == 0 && argIndex+1 < args.Length)
					{
						_P4Directory = args[++argIndex];
					}
					else if (String.Compare(args[argIndex], "-P") == 0 && argIndex+1 < args.Length)
					{
						_P4Passwd = args[++argIndex];
					}
					else if (String.Compare(args[argIndex], "-H") == 0 && argIndex + 1 < args.Length)
					{
						_P4Host = args[++argIndex];
					}
					else if (String.Compare(args[argIndex], "-x") == 0 && argIndex+1 < args.Length)
					{
						_P4ArgFile = args[++argIndex];
					}
					else if (String.Compare(args[argIndex], "-b") == 0)
					{
						System.Diagnostics.Debugger.Launch();
					}
					else if (String.Compare(args[argIndex], "-r") == 0)
					{
						SettingManager.RemoteLogging = true;
					}
					else if (String.Compare(args[argIndex], "-v") == 0)
					{
						SettingManagerExtensions.Verbosity = LogChannel.Verbose;
					}
					else
					{
						break;
					}
				}

				VirtualFileSystemLog.Info("P4VFS version {0}", VirtualFileSystem.CurrentVersion);
				if (argIndex >= args.Length)
				{
					argIndex = 0;
					args = new string[]{"help"};
				}

				string[] cmdArgs = args.Skip(argIndex+1).ToArray();
				switch (args[argIndex])
				{
					case "help":
					case "/?":
						status = CommandHelp(cmdArgs);
						break;
					case "sync":
						status = CommandSync(cmdArgs);
						break;
					case "info":
						status = CommandInfo(cmdArgs);
						break;
					case "resident":
						status = CommandResident(cmdArgs);
						break;
					case "hydrate":
						status = CommandHydrate(cmdArgs);
						break;
					case "populate":
						status = CommandPopulate(cmdArgs);
						break;
					case "reconfig":
						status = CommandReconfig(cmdArgs);
						break;
					case "monitor":
						status = CommandMonitor(cmdArgs);
						break;
					case "install":
						status = CommandInstall(cmdArgs);
						break;
					case "uninstall":
						status = CommandUninstall(cmdArgs);
						break;
					case "login":
						status = CommandLogin(cmdArgs);
						break;
					case "set":
						status = CommandSet(cmdArgs);
						break;
					case "test":
						status = CommandTest(cmdArgs);
						break;
					case "ctrl":
						status = CommandControl(cmdArgs);
						break;
					case "restart":
						status = CommandRestart(cmdArgs);
						break;
					default:
						VirtualFileSystemLog.Error("P4VFS Unknown Command {0}", args[argIndex]);
						status = false;
						break;
				}
			}
			catch (Exception e)
			{
				VirtualFileSystemLog.Error("P4VFS Error: {0}", e.Message);
				status = false;
			}

			VirtualFileSystemLog.Shutdown();
			return status ? 0 : 1;
		}

		private static bool CommandHelp(string[] args)
		{
			string helpText = null;
			string helpCmd = args?.FirstOrDefault();
			
			if (helpCmd == "all")
			{
				helpText = String.Join("\n", _Help.Select(h => h.Text));
			}
			else
			{
				helpText = _Help.FirstOrDefault(h => h.Names.Contains(helpCmd))?.Text ?? _Help[0].Text;
			}

			VirtualFileSystemLog.Info(helpText);
			return true;
		}

		private static bool CommandSync(string[] args)
		{
			DepotSyncMethod syncMethod = DepotSyncMethod.Virtual;
			SyncProtocol syncProtocol = SyncProtocol.Service | SyncProtocol.Local;
			string syncResident = SettingManager.SyncResidentPattern?.Trim();
			
			DepotSyncType syncType = SettingManager.SyncDefaultQuiet ? DepotSyncType.Quiet : DepotSyncType.Normal;
			DepotFlushType flushType = SettingManagerExtensions.DefaultFlushType;
			DepotFlushType? explicitFlush = null;

			int argIndex = 0;
			for (; argIndex < args.Length; ++argIndex)
			{
				if (String.Compare(args[argIndex], "-v") == 0)
				{
					syncMethod = DepotSyncMethod.Virtual;
					syncType &= ~DepotSyncType.IgnoreOutput;
				}
				else if (String.Compare(args[argIndex], "-r") == 0)
				{
					syncMethod = DepotSyncMethod.Regular;
					syncType |= DepotSyncType.IgnoreOutput;
				}
				else if (String.Compare(args[argIndex], "-f") == 0)
				{
					syncType |= DepotSyncType.Force;
				}
				else if (String.Compare(args[argIndex], "-q") == 0)
				{
					syncType |= DepotSyncType.Quiet;
				}
				else if (String.Compare(args[argIndex], "-l") == 0)
				{
					syncType &= ~DepotSyncType.Quiet;
				}
				else if (String.Compare(args[argIndex], "-n") == 0)
				{
					syncType |= DepotSyncType.Preview;
				}
				else if (String.Compare(args[argIndex], "-k") == 0)
				{
					syncType |= DepotSyncType.Flush;
				}
				else if (String.Compare(args[argIndex], "-w") == 0)
				{
					syncType |= DepotSyncType.Writeable;
				}
				else if (String.Compare(args[argIndex], "-t") == 0)
				{
					syncProtocol = SyncProtocol.Local | (syncProtocol & ~SyncProtocol.Service);
				}
				else if (String.Compare(args[argIndex], "-s") == 0)
				{
					syncProtocol = SyncProtocol.Service | (syncProtocol & ~SyncProtocol.Local);
				}
				else if (String.Compare(args[argIndex], "-x") == 0 && argIndex+1 < args.Length)
				{
					syncResident = String.Join("|", args[++argIndex].Split(',',';').Select(x => String.Format("({0}$)", Regex.Escape(x))));
				}
				else if (String.Compare(args[argIndex], "-p") == 0 && argIndex+1 < args.Length)
				{
					syncResident = args[++argIndex];
				}
				else if (String.Compare(args[argIndex], "-m") == 0 && argIndex+1 < args.Length)
				{
					string flushTypeString = args[++argIndex];
					if (!Enum.TryParse(flushTypeString, true, out flushType))
					{
						VirtualFileSystemLog.Error($"{flushTypeString} is not a valid Flush Type.");
						return false;
					}
					explicitFlush = flushType;
				}
				else
				{
					break;
				}
			}

			if (syncType.HasFlag(DepotSyncType.Quiet) && explicitFlush == null)
			{
				flushType = DepotFlushType.Atomic;
			}

			List<string> files = new List<string>(args.Skip(argIndex));
			files.AddRange(ReadInputFileArgs());

			DepotSyncOptions syncOptions = new DepotSyncOptions();
			syncOptions.Files = files.ToArray();
			syncOptions.SyncType = syncType;
			syncOptions.SyncMethod = syncMethod;
			syncOptions.SyncResident = syncResident;
			syncOptions.FlushType = flushType;
			syncOptions.Context = new CoreInterop.UserContext { ProcessId = Process.GetCurrentProcess().Id };

			if (syncType.HasFlag(DepotSyncType.Quiet))
			{
				SettingManagerExtensions.Verbosity = LogChannel.Warning;
			}

			using (DepotClient depotClient = new DepotClient())
			{
				if (depotClient.Connect(depotServer: _P4Port, depotClient: _P4Client, depotUser: _P4User, directoryPath: _P4Directory, depotPasswd: _P4Passwd, host: _P4Host) == false)
				{
					VirtualFileSystemLog.Error("Failed to connect to perforce");
					return false;
				}
				
				if (syncProtocol.HasFlag(SyncProtocol.Service))
				{
					VirtualFileSystemLog.Verbose("Performing service sync ...");
					Extensions.SocketModel.SocketModelClient serviceClient = new Extensions.SocketModel.SocketModelClient();
					DepotSyncStatus status = serviceClient.Sync(depotClient.Config(), syncOptions);
					return status == DepotSyncStatus.Success;
				}

				if (syncProtocol.HasFlag(SyncProtocol.Local))
				{
					VirtualFileSystemLog.Verbose("Performing local sync ...");
					DepotSyncStatus status = depotClient.Sync(syncOptions)?.Status ?? DepotSyncStatus.Success;
					return status == DepotSyncStatus.Success;
				}
			}

			VirtualFileSystemLog.Error("Failed to complete sync");
			return false;
		}

		private static bool CommandPopulate(string[] args)
		{
			return CommandSync(new[]{"-q", "-m", nameof(DepotFlushType.Single)}.Concat(args).ToArray());
		}

		private static bool CommandReconfig(string[] args)
		{
			DepotReconfigFlags reconfigFlags = DepotReconfigFlags.None;

			int argIndex = 0;
			for (; argIndex < args.Length; ++argIndex)
			{
				if (String.Compare(args[argIndex], "-n") == 0)
				{
					reconfigFlags |= DepotReconfigFlags.Preview;
				}
				else if (String.Compare(args[argIndex], "-q") == 0)
				{
					reconfigFlags |= DepotReconfigFlags.Quiet;
				}
				else if (String.Compare(args[argIndex], "-p") == 0)
				{
					reconfigFlags |= DepotReconfigFlags.P4Port;
				}
				else if (String.Compare(args[argIndex], "-c") == 0)
				{
					reconfigFlags |= DepotReconfigFlags.P4Client;
				}
				else if (String.Compare(args[argIndex], "-u") == 0)
				{
					reconfigFlags |= DepotReconfigFlags.P4User;
				}
				else
				{
					break;
				}
			}

			if ((reconfigFlags & (DepotReconfigFlags.P4Port|DepotReconfigFlags.P4Client|DepotReconfigFlags.P4User)) == 0)
			{
				reconfigFlags |= DepotReconfigFlags.P4Port;
			}

			if (reconfigFlags.HasFlag(DepotReconfigFlags.Quiet))
			{
				SettingManagerExtensions.Verbosity = LogChannel.Warning;
			}

			List<string> files = new List<string>(args.Skip(argIndex));
			files.AddRange(ReadInputFileArgs());

			DepotReconfigOptions reconfigOptions = new DepotReconfigOptions();
			reconfigOptions.Files = files.ToArray();
			reconfigOptions.Flags = reconfigFlags;

			using (DepotClient depotClient = new DepotClient())
			{
				if (depotClient.Connect(depotServer: _P4Port, depotClient: _P4Client, depotUser: _P4User, directoryPath: _P4Directory, depotPasswd: _P4Passwd, host: _P4Host) == false)
				{
					VirtualFileSystemLog.Error("Failed to connect to perforce");
					return false;
				}

				VirtualFileSystemLog.Verbose("Performing local reconfig ...");
				return DepotOperations.Reconfig(depotClient, reconfigOptions);
			}
		}

		private static bool CommandInfo(string[] args)
		{
			bool showExtended = false;

			int argIndex = 0;
			for (; argIndex < args.Length; ++argIndex)
			{
				if (String.Compare(args[argIndex], "-x") == 0)
				{
					showExtended = true;
				}
				else
				{
					break;
				}
			}

			List<KeyValuePair<string, string>> lines = new List<KeyValuePair<string, string>>();
			Action<string, object> addLine = (string key, object value) => 
			{
				lines.Add(new KeyValuePair<string, string>(key, value != null ? value.ToString() : null));
			};
			Action<string, Func<object>> addFunc = (string key, Func<object> value) => 
			{
				addLine(key, (new Func<object>(() => { try { return value(); } catch {} return null; }).Invoke()));
			};

			using (DepotClient depotClient = new DepotClient())
			{
				if (depotClient.Connect(depotServer: _P4Port, depotClient: _P4Client, depotUser: _P4User, directoryPath: _P4Directory, depotPasswd: _P4Passwd, host: _P4Host))
				{
					DepotResultInfo.Node info = depotClient.Info();
					DepotResultClient.Node connection = depotClient.ConnectionClient();
					addFunc("P4 UserName", () => info.UserName);
					addFunc("P4 ClientName", () => connection.Client);
					addFunc("P4 ClientHost", () => connection.Host);
					addFunc("P4 ClientRoot", () => connection.Root);
					addFunc("P4 ClientStream", () => connection.Stream);
					addFunc("P4 ServerAddress", () => info.ServerAddress);
					addFunc("P4 ServerVersion", () => info.ServerVersion);
					addFunc("P4 CaseHandling", () => info.CaseHandling);
				}
			}

			addLine("P4VFS CurrentVersion", VirtualFileSystem.CurrentVersion);
			addLine("P4VFS ServiceVersion", VirtualFileSystem.GetServiceVersion());
			addLine("P4VFS DriverVersion", VirtualFileSystem.GetDriverVersion());
			addLine("P4VFS DriverReady", PredicateLogRetry(() => VirtualFileSystem.IsDriverReady(), null));
			addLine("P4VFS ServiceReady", PredicateLogRetry(() => VirtualFileSystem.IsServiceReady(), null));
			addLine("P4VFS SystemReady", PredicateLogRetry(() => VirtualFileSystem.IsVirtualFileSystemAvailable(), null));

			if (showExtended)
			{
				Extensions.SocketModel.SocketModelClient serviceClient = new Extensions.SocketModel.SocketModelClient();
				SettingNodeMap nodeMap = serviceClient.GetServiceSettings();
				foreach (KeyValuePair<string, SettingNode> nodeProperty in nodeMap)
				{
					addLine(String.Format("P4VFS {0}", nodeProperty.Key), ServiceSettings.SettingNodeToText(nodeProperty.Value));
				}
			}

			foreach (KeyValuePair<string, string> line in lines)
			{
				VirtualFileSystemLog.Info("{0}: {1}", line.Key, line.Value ?? "");
			}
			return true;
		}

		private static bool CommandSet(string[] args)
		{
			if (args.Length > 1)
			{
				VirtualFileSystemLog.Error("Invalid number of arguments {0}. Expecting: <SettingName>=[SettingValue]", args.Length);
				return false;
			}

			string arg = args.Length > 0 ? args[0] : "";
			Match m = Regex.Match(arg, @"(?<name>.+?)=(?<value>.*)");
			if (m.Success)
			{
				Extensions.SocketModel.SocketModelClient serviceClient = new Extensions.SocketModel.SocketModelClient();
				if (serviceClient.SetServiceSetting(m.Groups["name"].Value, SettingNode.FromString(m.Groups["value"].Value)) == false)
				{
					VirtualFileSystemLog.Error("Failed to set service setting");
					return false;
				}
			}
			else
			{
				Extensions.SocketModel.SocketModelClient serviceClient = new Extensions.SocketModel.SocketModelClient();
				SettingNodeMap nodeMap = serviceClient.GetServiceSettings();
				foreach (KeyValuePair<string, SettingNode> nodeProperty in nodeMap)
				{
					if (nodeProperty.Key.ToLower().Contains(arg.ToLower()))
					{
						VirtualFileSystemLog.Info("{0}={1}", nodeProperty.Key, ServiceSettings.SettingNodeToText(nodeProperty.Value));
					}
				}
			}
			return true;
		}

		private static bool CommandResident(string[] args)
		{
			SyncProtocol syncProtocol = SyncProtocol.Service | SyncProtocol.Local;
			DepotSyncMethod syncMethod = DepotSyncMethod.Regular;
			DepotSyncType syncType = SettingManager.SyncDefaultQuiet ? DepotSyncType.Quiet : DepotSyncType.Normal;
			string syncResident = null;

			int argIndex = 0;
			for (; argIndex < args.Length; ++argIndex)
			{
				if (String.Compare(args[argIndex], "-v") == 0)
				{
					syncMethod = DepotSyncMethod.Virtual;
				}
				else if (String.Compare(args[argIndex], "-r") == 0)
				{
					syncMethod = DepotSyncMethod.Regular;
				}
				else if (String.Compare(args[argIndex], "-n") == 0)
				{
					syncType |= DepotSyncType.Preview;
				}
				else if (String.Compare(args[argIndex], "-t") == 0)
				{
					syncProtocol = SyncProtocol.Local | (syncProtocol & ~SyncProtocol.Service);
				}
				else if (String.Compare(args[argIndex], "-s") == 0)
				{
					syncProtocol = SyncProtocol.Service | (syncProtocol & ~SyncProtocol.Local);
				}
				else if (String.Compare(args[argIndex], "-x") == 0 && argIndex+1 < args.Length)
				{
					syncResident = String.Join("|", args[++argIndex].Split(',',';').Select(x => String.Format("({0}$)", Regex.Escape(x))));
				}
				else if (String.Compare(args[argIndex], "-p") == 0 && argIndex+1 < args.Length)
				{
					syncResident = args[++argIndex];
				}
				else if (String.Compare(args[argIndex], "-q") == 0)
				{
					syncType |= DepotSyncType.Quiet;
				}
				else if (String.Compare(args[argIndex], "-l") == 0)
				{
					syncType &= ~DepotSyncType.Quiet;
				}
				else
				{
					break;
				}
			}

			List<string> fileArguments = new List<string>(args.Skip(argIndex));
			fileArguments.AddRange(ReadInputFileArgs());

			if (syncType.HasFlag(DepotSyncType.Quiet))
			{
				SettingManagerExtensions.Verbosity = LogChannel.Warning;
			}

			using (DepotClient depotClient = new DepotClient())
			{
				if (depotClient.Connect(depotServer: _P4Port, depotClient: _P4Client, depotUser: _P4User, directoryPath: _P4Directory, depotPasswd: _P4Passwd, host: _P4Host) == false)
				{
					VirtualFileSystemLog.Error("Failed to connect to perforce");
					return false;
				}

				if (syncMethod == DepotSyncMethod.Virtual)
				{
					DepotSyncOptions syncOptions = new DepotSyncOptions();
					syncOptions.Files = fileArguments.ToArray();
					syncOptions.SyncType = DepotSyncType.Force | syncType;
					syncOptions.SyncMethod = DepotSyncMethod.Virtual;
					syncOptions.SyncResident = String.IsNullOrEmpty(syncResident) ? null : String.Format("^(?!.*({0})).*$", syncResident);
					syncOptions.Revision = new DepotRevisionHave().ToString();

					if (syncProtocol.HasFlag(SyncProtocol.Service))
					{
						VirtualFileSystemLog.Verbose("Performing remote socket model sync ...");
						Extensions.SocketModel.SocketModelClient serviceClient = new Extensions.SocketModel.SocketModelClient();
						DepotSyncStatus status = serviceClient.Sync(depotClient.Config(), syncOptions);
						return status == DepotSyncStatus.Success;
					}

					if (syncProtocol.HasFlag(SyncProtocol.Local))
					{
						VirtualFileSystemLog.Verbose("Performing local sync ...");
						DepotSyncStatus status = depotClient.Sync(syncOptions)?.Status ?? DepotSyncStatus.Success;
						return status == DepotSyncStatus.Success;
					}

				}
				else if (syncMethod == DepotSyncMethod.Regular)
				{
					DepotSyncOptions syncOptions = new DepotSyncOptions();
					syncOptions.Files = fileArguments.ToArray();
					syncOptions.SyncType = syncType;
					syncOptions.SyncResident = syncResident;

					VirtualFileSystemLog.Verbose("Hydrating files ...");
					DepotSyncStatus status = DepotOperations.Hydrate(depotClient, syncOptions)?.Status ?? DepotSyncStatus.Success;
					return status == DepotSyncStatus.Success;
				}
			}

			return true;
		}

		private static bool CommandHydrate(string[] args)
		{
			return CommandResident(new[]{"-r"}.Concat(args).ToArray());
		}

		private static bool CommandMonitor(string[] args)
		{
			bool show = args.Any(arg => Regex.Match("^(-s)|(-?show)$", arg).Success);
			bool hide = args.Any(arg => Regex.Match("^(-h)|(-?hide)$", arg).Success);
			bool success = false;

			if (show)
			{
				VirtualFileSystemLog.Info("Showing {0}", VirtualFileSystem.MonitorTitle);
				success = VirtualFileSystem.ShowMonitor();
			}
			else if (hide)
			{
				VirtualFileSystemLog.Info("Hiding {0}", VirtualFileSystem.MonitorTitle);
				success = VirtualFileSystem.HideMonitor();
			}
			return success;
		}

		private static bool CommandInstall(string[] args)
		{
			bool settings = args.Contains("-t");
			bool service = args.Contains("-s");
			bool driver = args.Contains("-d");
			if (settings == false && service == false && driver == false)
			{
				settings = true;
				service = true;
				driver = true;
			}

			bool result = true;
			if (service)
			{
				VirtualFileSystemLog.Info("Installing {0}", VirtualFileSystem.ServiceTitle);
				bool status = VirtualFileSystem.InstallService();
				VirtualFileSystemLog.Info("Service install {0}", status.ToStatusString());
				result &= status;
			}

			if (driver)
			{
				VirtualFileSystemLog.Info("Installing {0}", VirtualFileSystem.DriverTitle);
				bool status = VirtualFileSystem.InstallDriver();
				VirtualFileSystemLog.Info("Driver install {0}", status.ToStatusString());
				result &= status;

				VirtualFileSystemLog.Info("Loading {0}", VirtualFileSystem.DriverTitle);
				status = VirtualFileSystem.LoadDriver();
				VirtualFileSystemLog.Info("Driver load {0}", status.ToStatusString());
				result &= status;
			}

			if (settings)
			{
				string settingFilePath = VirtualFileSystem.InstalledSettingsFilePath;
				string templateFile = String.Format("{0}\\{1}.Template.xml", Path.GetDirectoryName(settingFilePath), Path.GetFileNameWithoutExtension(settingFilePath));
				VirtualFileSystemLog.Info("Installing {0}", templateFile);
				bool status = ServiceSettings.SaveToFile(templateFile);
				VirtualFileSystemLog.Info("Template install {0}", status.ToStatusString());
				result &= status;
			}

			result &= PredicateLogRetry(() => VirtualFileSystem.IsDriverLoaded(), "IsDriverLoaded", retryWait:1000, limitWait:10000);
			result &= PredicateLogRetry(() => VirtualFileSystem.IsDriverReady(), "IsDriverReady");
			result &= PredicateLogRetry(() => VirtualFileSystem.IsVirtualFileSystemAvailable(), "IsVirtualFileSystemAvailable");

			ushort major = 0;
			ushort minor = 0;
			ushort build = 0;
			ushort revision = 0;
			result &= PredicateLog(() => CoreInterop.NativeMethods.GetDriverVersion(ref major, ref minor, ref build, ref revision), "GetDriverVersion");
			result &= PredicateLog(() => CoreInterop.NativeConstants.VersionMajor == major, String.Format("Driver major version {0} expected {1}", major, CoreInterop.NativeConstants.VersionMajor));
			return result;
		}

		private static bool CommandRestart(string[] args)
		{
			bool service = args.Contains("-s");
			bool driver = args.Contains("-d");
			if (service == false && driver == false)
			{
				service = true;
				driver = true;
			}

			bool result = true;
			if (driver)
			{
				VirtualFileSystemLog.Info("Unloading {0}", VirtualFileSystem.DriverTitle);
				bool status = VirtualFileSystem.UnloadDriver();
				VirtualFileSystemLog.Info("Driver unload {0}", status.ToStatusString());
				result &= status;

				VirtualFileSystemLog.Info("Loading {0}", VirtualFileSystem.DriverTitle);
				status = VirtualFileSystem.LoadDriver();
				VirtualFileSystemLog.Info("Driver load {0}", status.ToStatusString());
				result &= status;
			}

			if (service)
			{
				VirtualFileSystemLog.Info("Stopping {0}", VirtualFileSystem.ServiceTitle);
				bool status = CoreInterop.ServiceOperations.StopLocalService(VirtualFileSystem.ServiceTitle) == 0;
				VirtualFileSystemLog.Info("Service stop {0}", status.ToStatusString());
				result &= status;

				VirtualFileSystemLog.Info("Starting {0}", VirtualFileSystem.ServiceTitle);
				status = CoreInterop.ServiceOperations.StartLocalService(VirtualFileSystem.ServiceTitle) == 0;
				VirtualFileSystemLog.Info("Service start {0}", status.ToStatusString());
				result &= status;
			}
			return result;
		}

		private static bool CommandUninstall(string[] args)
		{
			bool service = args.Contains("-s");
			bool driver = args.Contains("-d");
			bool preserve = args.Contains("-p");

			if (service == false && driver == false)
			{
				service = true;
				driver = true;
			}

			bool result = true;
			if (service)
			{
				VirtualFileSystemLog.Info("Uninstalling {0}{1}", preserve ? "preserved " : "", VirtualFileSystem.ServiceTitle);
				bool status = VirtualFileSystem.UninstallService(preserve ? CoreInterop.ServiceOperations.UninstallFlags.NoDelete : CoreInterop.ServiceOperations.UninstallFlags.None);
				VirtualFileSystemLog.Info("Service uninstall {0}", status.ToStatusString());
				result &= status;
			}

			if (driver)
			{
				VirtualFileSystemLog.Info("Unloading {0}", VirtualFileSystem.DriverTitle);
				bool status = VirtualFileSystem.UnloadDriver();
				VirtualFileSystemLog.Info("Driver unload {0}", status.ToStatusString());
				result &= status;

				VirtualFileSystemLog.Info("Uninstalling {0}", VirtualFileSystem.DriverTitle);
				status = VirtualFileSystem.UninstallDriver();
				VirtualFileSystemLog.Info("Driver uninstall {0}", status.ToStatusString());
				result &= status;
			}

			result &= PredicateLogRetry(() => VirtualFileSystem.IsDriverLoaded() == false, "IsDriverLoaded");
			result &= PredicateLogRetry(() => VirtualFileSystem.IsDriverReady() == false, "IsDriverReady");
			result &= PredicateLogRetry(() => VirtualFileSystem.IsVirtualFileSystemAvailable() == false, "IsVirtualFileSystemAvailable");
			return result;
		}

		private static bool CommandLogin(string[] args)
		{
			bool interactive = false;
			bool writePasswd = false;
			int argIndex = 0;
			for (; argIndex < args.Length; ++argIndex)
			{
				if (String.Compare(args[argIndex], "-i") == 0)
				{
					interactive = true;
				}
				else if (String.Compare(args[argIndex], "-w") == 0)
				{
					writePasswd = true;
				}
				else
				{
					break;
				}
			}

			DepotConfig p4Config = DepotInfo.DepotConfigFromPath(_P4Directory);

			if (String.IsNullOrEmpty(_P4Port))
			{
				_P4Port = p4Config.Port;
			}
			if (String.IsNullOrEmpty(_P4User))
			{
				_P4User = p4Config.User;
			}
			if (String.IsNullOrEmpty(_P4Client))
			{
				_P4Client = p4Config.Client;
			}
			if (String.IsNullOrEmpty(_P4Passwd))
			{
				_P4Passwd = p4Config.Passwd;
			}

			if (argIndex < args.Length)
			{
				_P4Passwd = args[argIndex];
			}

			Func<bool> depotClientLogin = () =>
			{
				using (DepotClient depotClient = new DepotClient())
				{
					depotClient.Unattended = true;
					return depotClient.Connect(_P4Port, _P4Client, _P4User, _P4Directory, _P4Passwd, _P4Host) && depotClient.Login(_P4Passwd);
				}
			};

			// Try an unattended login to find if our current config or SSO is already valid
			bool status = depotClientLogin();
			bool disabled = false;

			// Try an unattended login using the internal P4VFS unit testing P4PASSWD (undocumented)
			if (status == false)
			{
				string passwd = null;
				RegistryInfo.GetTypedValue(Win32.Registry.LocalMachine, VirtualFileSystem.AppRegistryKey, DepotConstants.P4PASSWD, ref passwd);
				if (String.IsNullOrEmpty(passwd) == false && passwd != _P4Passwd)
				{
					_P4Passwd = passwd;
					status = depotClientLogin();
				}
			}
			
			// Request a new password interactively from a dialog or console
			if (status == false)
			{
				if (interactive)
				{
					var thread = new System.Threading.Thread(new System.Threading.ThreadStart(() => 
					{
						var dlg = new Microsoft.P4VFS.Extensions.Controls.LoginWindow();
						dlg.Port = _P4Port;
						dlg.Client = _P4Client;
						dlg.User = _P4User;
						dlg.Passwd = _P4Passwd;
						if (dlg.ShowDialog() == true)
						{
							_P4Passwd = dlg.Passwd;
						}
						else
						{
							disabled = true;
						}
					}));

					thread.SetApartmentState(System.Threading.ApartmentState.STA);
					thread.Start();
					thread.Join();
				}
				else if (String.IsNullOrEmpty(_P4Passwd))
				{
					System.Console.Write("Enter Password: ");
					_P4Passwd = ConsoleReader.ReadLine();
				}
			}
		
			// Try an unattended login using the new password entered from the prompt
			if (status == false && disabled == false)
			{
				status = depotClientLogin();
			}

			if (status && writePasswd)
			{
				System.Console.WriteLine(String.Format("P4PASSWD={0}", _P4Passwd));
			}

			if (disabled)
			{
				VirtualFileSystemLog.Info("Interactive login canceled, switching to Unattended service mode");
				Extensions.SocketModel.SocketModelClient serviceClient = new Extensions.SocketModel.SocketModelClient();
				if (serviceClient.SetServiceSetting("Unattended", SettingNode.FromBool(true)) == false)
				{
					VirtualFileSystemLog.Error("Failed to set service setting Unattended=true");
				}
			}

			VirtualFileSystemLog.Info("Login {0}.", status ? "successful" : "failed");
			return status;
		}

		private static bool CommandTest(string[] args)
		{
			DepotConfig config = new DepotConfig
			{
				Port = _P4Port,
				Client = _P4Client,
				User = _P4User,
				Passwd = _P4Passwd,
			};

			return UnitTest.UnitTestBase.Execute(config, args);
		}

		private static bool CommandControl(string[] args)
		{
			try
			{
				for (int argIndex = 0; argIndex < args.Length; ++argIndex)
				{
					if (String.Compare(args[argIndex], "-dc") == 0)
					{
						bool connected = false;
						if (CoreInterop.NativeMethods.GetDriverIsConnected(ref connected) == false)
						{
							VirtualFileSystemLog.Error("Failed to GetDriverIsConnected");
							return false;
						}
						VirtualFileSystemLog.Info("DriverConnected: {0}", connected);
						VirtualFileSystemLog.Info("DriverTraceChannel: {0}", CoreInterop.NativeConstants.WppGuid);
					}
					else if (String.Compare(args[argIndex], "-dv") == 0)
					{
						VirtualFileSystemLog.Info("{0} Version: {1}", VirtualFileSystem.DriverTitle, VirtualFileSystem.GetDriverVersion());
					}
					else if (String.Compare(args[argIndex], "-sv") == 0)
					{
						VirtualFileSystemLog.Info("{0} Version: {1}", VirtualFileSystem.ServiceTitle, VirtualFileSystem.GetServiceVersion());
					}
					else if (String.Compare(args[argIndex], "-f") == 0 && argIndex+1 < args.Length)
					{
						Match m = Regex.Match(args[++argIndex], @"(?<name>\S+)\s*=\s*(?<value>\d+)");
						if (m.Success == false)
						{
							VirtualFileSystemLog.Error("CommandControl invalid flag name and value: {0}", args[argIndex]);	
							return false;
						}
						if (NativeMethods.SetDriverFlag(m.Groups["name"].Value, UInt32.Parse(m.Groups["value"].Value)) == false)
						{
							VirtualFileSystemLog.Error("CommandControl failed to set flag {0}", args[argIndex]);
							return false;
						}
						VirtualFileSystemLog.Info("CommandControl successfully set flag {0}", args[argIndex]);
					}
					else
					{
						VirtualFileSystemLog.Error("CommandControl unrecognized switch: {0}", args[argIndex]);
						return false;
					}
				}
			}
			catch (Exception e)
			{
				VirtualFileSystemLog.Error("CommandControl exception: {0}", e.Message);
				return false;
			}
			return true;
		}

		private static IEnumerable<string> ReadInputFileArgs()
		{
			if (String.IsNullOrEmpty(_P4ArgFile) == false)
			{
				System.IO.Stream stream = null;
				if (_P4ArgFile == "-")
				{
					stream = System.Console.OpenStandardInput();
				}
				else
				{
					if (File.Exists(_P4ArgFile) == false)
					{
						throw new Exception(String.Format("Argument file does not exist: {0}", _P4ArgFile));
					}

					stream = File.OpenRead(_P4ArgFile);
				}

				if (stream == null)
				{
					throw new Exception(String.Format("Failed to open input stream: {0}", _P4ArgFile));
				}

				using (System.IO.StreamReader sr = new System.IO.StreamReader(stream))
				{
					for (string line = sr.ReadLine(); line != null; line = sr.ReadLine())
					{
						yield return line;
					}
				}
			}
		}

		private static bool PredicateLog(Func<bool> expression, string message)
		{
			if (expression())
			{
				return true;
			}

			if (String.IsNullOrEmpty(message) == false)
			{
				VirtualFileSystemLog.Error("Predicate failed: {0}", message);
			}

			return false;
		}

		private static bool PredicateLogRetry(Func<bool> expression, string message, int retryWait = 500, int limitWait = 5000)
		{
			DateTime endTime = DateTime.Now.AddMilliseconds((double)limitWait);
			do
			{
				if (expression())
				{
					return true;
				}

				System.Threading.Thread.Sleep(retryWait);
			}
			while (DateTime.Now < endTime);

			if (String.IsNullOrEmpty(message) == false)
			{
				VirtualFileSystemLog.Error("Predicate retry failed: {0}", message);
			}

			return false;
		}

		[Flags]
		private enum SyncProtocol
		{
			Local			= 1<<0,
			Service			= 1<<1,
		}

		private class HelpItem
		{
			public string[] Names;
			public string Text;
		}

		private class HelpList : List<HelpItem>
		{
			public void Add(string name, string text)
			{
				Add(new HelpItem{ Names=name.Split(new[]{','}, StringSplitOptions.RemoveEmptyEntries), Text=text });
			}
		}

	}
}

