// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.ComponentModel;
using Microsoft.P4VFS.Extensions.Utilities;

namespace Microsoft.P4VFS.External
{
	public class P4vfsModule : Module
	{
		private const string P4VFS_SIGNED_VERSION = "1.25.0.0";
		private const string P4VFS_SIGNED_ARTIFACTS_URL = "https://github.com/microsoft/p4vfs/releases/download";

		public override string Name
		{
			get { return "P4VFS"; }
		}

		public override string Description
		{
			get { return String.Format("{0} {1}", Name, P4VFS_SIGNED_VERSION); }
		}

		public string SignedVersionFolderName
		{
			get { return Version.Parse(P4VFS_SIGNED_VERSION).ToString(2); }
		}

		private string DownloadP4vfsSetupArtifact(string workingFolder)
		{
			try
			{
				// Try to download the signed P4VFS.Setup package for this version for iteration
				string p4vfsPackageFolder = ModuleInfo.NugetInstall(Context, "P4VFS.Setup", P4VFS_SIGNED_VERSION);
				string p4vfsSetupFile = Directory.GetFiles($"{p4vfsPackageFolder}\\tools", "P4VFS.Setup*.exe", SearchOption.TopDirectoryOnly).FirstOrDefault();
				if (File.Exists(p4vfsSetupFile))
				{
					Trace.TraceInformation("Downloaded nuget package for P4VFS {0}: {1}", P4VFS_SIGNED_VERSION, p4vfsSetupFile);
					return p4vfsSetupFile;
				}
			}
			catch (Exception e)
			{
				Trace.TraceWarning("Unable to download nuget package for P4VFS {0}: {1}", P4VFS_SIGNED_VERSION, e.Message);
			}

			try
			{
				// Try to download the signed production release of P4VFS.Setup for iteration
				string p4vfsSetupSourceUrl = String.Format("{0}/v{1}/P4VFS.Setup-{1}.exe", P4VFS_SIGNED_ARTIFACTS_URL, P4VFS_SIGNED_VERSION);
				string p4vfsSetupFile = ModuleInfo.DownloadFileToFolder(p4vfsSetupSourceUrl, workingFolder);
				if (File.Exists(p4vfsSetupFile))
				{
					Trace.TraceInformation("Downloaded production release of P4VFS {0}: {1}", P4VFS_SIGNED_VERSION, p4vfsSetupFile);
					return p4vfsSetupFile;
				}
			}
			catch (Exception e)
			{
				Trace.TraceWarning("Unable to download production release of P4VFS {0}: {1}", P4VFS_SIGNED_VERSION, e.Message);
			}

			throw new Exception(String.Format("Failed all atempts to download P4VFS {0}", P4VFS_SIGNED_VERSION));
		}

		public override void Restore()
		{
			try
			{
				string p4vfsModuleFolder = Context.Properties.Get(ReservedProperty.ModuleDir);
				string p4vfsTargetFolder = String.Format("{0}\\{1}", p4vfsModuleFolder, SignedVersionFolderName);

				ShellUtilities.RemoveDirectoryRecursive(p4vfsTargetFolder);
				Directory.CreateDirectory(p4vfsTargetFolder);
		
				// Download the signed P4VFS.Setup installer for this version for iteration
				string p4vfsSetupFile = DownloadP4vfsSetupArtifact(p4vfsTargetFolder);

				// Extract the P4VFS.Setup installer to p4vfsTargetFolder
				ProcessInfo.ExecuteParams extractParams = new ProcessInfo.ExecuteParams {
					FileName = p4vfsSetupFile,
					Arguments = String.Format("-a -c extract \"{0}\"", p4vfsTargetFolder),
					AsAdmin = true,
					LogCommand = true,
					LogOutput = false
				};

				ProcessInfo.ExecuteResult extractResult = ProcessInfo.ExecuteWait(extractParams);
				if (extractResult.ExitCode != 0)
				{
					throw new Exception(String.Format("Failed to extract installer: {0} [{1}]", p4vfsSetupFile, extractResult.Exception?.Message));
				}
			}
			catch (Exception e)
			{
				throw new WarningException(String.Format("Failed to restore signed binaries for P4VFS. Non-Dev configurations may not build. {0}", e.Message));
			}
		}
	}
}
