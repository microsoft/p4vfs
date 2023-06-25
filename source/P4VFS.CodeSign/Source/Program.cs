// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Collections.Generic;
using System.Linq;
using System.IO;
using System.Security;
using System.Reflection;
using Microsoft.P4VFS.Extensions;
using Microsoft.P4VFS.Extensions.Linq;
using Microsoft.P4VFS.CoreInterop;

namespace Microsoft.P4VFS.CodeSign
{
	public class Program
	{
		private const string _HelpText = 
@"
Perforce Virtual File System CodeSign tool.
Microsoft Xbox Studios

Usage:
  P4VFS.CodeSign [options] command [arg ...]

Options:
   -b           Launch and attach a debugger before command execution
   -v           Enable verbose logging for this execution
   -u <user>    Run this process as username
   -p <passwd>  Run this process with password
   -l           No logo

Commands:

  help          Print this help


  submit        Submit a codesign job to the ESRP service and wait

                P4VFS.CodeSign.exe submit -s <dir> -i <name> [-t <dir>] [-c <dir>] [-n <dir>]

   -s <dir>     Path to the source folder containing binaries to sign. If not 
                specified then binaries in the assembly folder are used.
   -t <dir>     Path to the target folder to to contain signed binaries,
                submission schema files, and output log. The process will 
                wait indefinatly until signed binaries arrive.
   -i <name>    Name of input json file resource for signing
   -c <dir>     Optional path to the folder containing EsrpClient.exe tool
   -n <dir>     Optional path to the folder containing nuget.exe tool if EsrpClient
                folder is not specified. This may be an interactive login.
";

		public static int Main(string[] args)
		{
			bool status = true;
			try
			{
				VirtualFileSystemLog.Intitialize();
				SettingManager.ImmediateLogging = true;
				bool noLogo = false;
				string username = null;
				SecureString password = null;

				int argIndex = 0;
				for (; argIndex < args.Length; ++argIndex)
				{
					if (String.Compare(args[argIndex], "-b") == 0)
						System.Diagnostics.Debugger.Launch();
					else if (String.Compare(args[argIndex], "-v") == 0)
						SettingManagerExtensions.Verbosity = LogChannel.Verbose;
					else if (String.Compare(args[argIndex], "-u") == 0 && argIndex+1 < args.Length)
						username = args[++argIndex];
					else if (String.Compare(args[argIndex], "-p") == 0 && argIndex+1 < args.Length)
						password = CodeSignUtilities.CreateSecureString(args[++argIndex]);
					else if (String.Compare(args[argIndex], "-l") == 0)
						noLogo = true;
					else
						break;
				} 

				if (noLogo == false)
				{
					VirtualFileSystemLog.Info("P4VFS.CodeSign version {0} {1}", VirtualFileSystem.CurrentVersion, args.ToNiceString());
				}

				if (String.IsNullOrEmpty(username) == false)
				{
					if (password == null)
					{
						password = CodeSignUtilities.ReadConsolePassword(String.Format("Enter Password for {0}: ", username));
					}
					List<string> runasArgs = new List<string>();
					if (SettingManagerExtensions.Verbosity == LogChannel.Verbose)
					{
						runasArgs.Add("-v");
					}
					//runasArgs.Add("-b");
					runasArgs.Add("-l");
					runasArgs.AddRange(args.Skip(argIndex));
					return CodeSignUtilities.ExecuteAsUserWait(Assembly.GetExecutingAssembly().Location, String.Format("\"{0}\"", String.Join("\" \"", runasArgs)), username, password);
				}

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
						status = CommandHelp();
						break;
					case "submit":
						status = CommandSubmit(cmdArgs);
						break;
					default:
						VirtualFileSystemLog.Error("P4VFS.CodeSign Unknown Command {0}", args[argIndex]);
						status = false;
						break;
				}
			}
			catch (Exception e)
			{
				VirtualFileSystemLog.Error("P4VFS.CodeSign Unhandled Exception: {0}\n{1}", e.Message, e.StackTrace);
				status = false;
			}

			VirtualFileSystemLog.Flush();
			return status ? 0 : 1;
		}

		private static bool CommandHelp()
		{
			VirtualFileSystemLog.Info(_HelpText.Trim());
			return true;
		}

		private static bool CommandSubmit(string[] args)
		{
			CodeSignJob job = new CodeSignJob();

			int argIndex = 0;
			for (; argIndex < args.Length; ++argIndex)
			{
				if (String.Compare(args[argIndex], "-c") == 0 && argIndex+1 < args.Length)
					job.EsrpClientFolder = args[++argIndex];
				else if (String.Compare(args[argIndex], "-i") == 0 && argIndex+1 < args.Length)
					job.SignInputName = args[++argIndex];
				else if (String.Compare(args[argIndex], "-s") == 0 && argIndex+1 < args.Length)
					job.SourceFolder = args[++argIndex];
				else if (String.Compare(args[argIndex], "-t") == 0 && argIndex+1 < args.Length)
					job.TargetFolder = args[++argIndex];
				else if (String.Compare(args[argIndex], "-n") == 0 && argIndex+1 < args.Length)
					job.NugetFolder = args[++argIndex];
				else
					break;
			}

			if (argIndex < args.Length)
			{
				VirtualFileSystemLog.Error("Unknown options for submit command: {0}", args.Skip(argIndex).ToNiceString());
				return false;	
			}
			
			if (String.IsNullOrEmpty(job.SourceFolder))
			{
				VirtualFileSystemLog.Error("Unspecified source folder");
				return false;
			}

			job.SourceFolder = Path.GetFullPath(job.SourceFolder);
			if (Directory.Exists(job.SourceFolder) == false)
			{
				VirtualFileSystemLog.Error("Source folder does not exist: {0}", job.SourceFolder);
				return false;
			}

			if (String.IsNullOrEmpty(job.TargetFolder))
			{
				job.TargetFolder = job.SourceFolder;
			}

			if (SubmitCodeSignJob<EsrpClient>(job) == false)
			{
				VirtualFileSystemLog.Error("Failed performing ESRP codesign");
				return false;
			}

			if (SubmitCodeSignJob<DevCenterClient>(job) == false)
			{
				VirtualFileSystemLog.Error("Failed performing HDC codesign");
				return false;
			}

			return true;
		}

		private static bool SubmitCodeSignJob<ClientType>(CodeSignJob job) where ClientType : ICodeSignClient, new()
		{
			using (ICodeSignClient client = new ClientType())
			{
				return client.Submit(job);
			}
		}
	}
}

