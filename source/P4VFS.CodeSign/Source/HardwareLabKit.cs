// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Linq;
using System.Diagnostics;
using System.IO;
using System.Text.RegularExpressions;
using Microsoft.P4VFS.Extensions;
using Microsoft.P4VFS.Extensions.Utilities;

namespace Microsoft.P4VFS.CodeSign
{
	public class HardwareLabKitPackage : IDisposable
	{
		public HardwareLabKitPackage()
		{
		}

		public void Dispose()
		{
		}

		public bool GeneratePackage(HardwareLabKitJob job, string signFilePath)
		{
			foreach (HardwareLabKitController controller in job.Controllers)
			{
				foreach (string clientName in controller.Clients)
				{
					DeployTargetToAgent(job, clientName);
					SetupClientAgent(job, clientName);
				}

				DeployTargetToAgent(job, controller.Name);
				CreateControllerPackage(job, controller.Name);
			}

			foreach (HardwareLabKitController controller in job.Controllers)
			{
				string stagingFolder = GetAgentStagingFolder(job, controller.Name);
				string packageFileName = GetControllerPackageFileName(controller);
				string packageFilePath = String.Format("{0}\\{1}", stagingFolder, packageFileName);

				VirtualFileSystemLog.Info("Copying file: {0} -> {1}", packageFilePath, job.CodeSignJob.TargetFolder);
				if (FileUtilities.CopyFile(packageFilePath, job.CodeSignJob.TargetFolder, overwrite:true) == false)
				{
					throw new Exception(String.Format("Failed to copy package file to folder: {0} -> {1}", packageFilePath, job.CodeSignJob.TargetFolder));
				}
			}

			if (job.Controllers.Length == 1)
			{
				string packageFileName = GetControllerPackageFileName(job.Controllers[0]);
				string packageFilePath = String.Format("{0}\\{1}", job.CodeSignJob.TargetFolder, packageFileName);

				VirtualFileSystemLog.Info("Copying file: {0} -> {1}", packageFilePath, signFilePath);
				File.Copy(packageFilePath, signFilePath, overwrite:true);
			}
			return true;
		}

		private void DeployTargetToAgent(HardwareLabKitJob job, string agentName)
		{
			string stagingFolder = GetAgentStagingFolder(job, agentName);
			VirtualFileSystemLog.Info("Deploying to Agent: {0}", stagingFolder);

			FileUtilities.DeleteDirectoryAndFiles(stagingFolder);
			FileUtilities.CreateDirectory(stagingFolder);

			foreach (string sourceFilePath in Directory.EnumerateFiles(job.CodeSignJob.TargetFolder, "*", SearchOption.TopDirectoryOnly))
			{
				VirtualFileSystemLog.Info("Copying file: {0} -> {1}", sourceFilePath, stagingFolder);
				if (FileUtilities.CopyFile(sourceFilePath, stagingFolder, overwrite:true) == false)
				{
					throw new Exception(String.Format("Failed to copy file to folder: {0} -> {1}", sourceFilePath, stagingFolder));
				}
			}

			CodeSignUtilities.ExtractResourceToFile(stagingFolder, CodeSignResources.HardwareLabConsolePs1);
			CodeSignUtilities.ExtractResourceToFile(stagingFolder, CodeSignResources.HardwareLabPlaylistXml);
		}

		private string GetAgentStagingFolder(HardwareLabKitJob job, string agentName, bool local = false)
		{
			string rootFolder = local ? "C:" : String.Format("\\\\{0}", agentName);
			string shareFolder = String.Format("{0}\\Users", rootFolder);

			if (local == false)
			{
				string netArgs = "use";
				netArgs += $" {shareFolder}";
				netArgs += $" /user:{agentName}\\{job.AgentUsername}";
				netArgs += $" {job.AgentPassword}";

				VirtualFileSystemLog.Info("Connecting to Agent: {0}", shareFolder);
				if (ProcessInfo.ExecuteWait("net.exe", netArgs, echo: false, log: true) != 0)
				{
					throw new DirectoryNotFoundException(String.Format("Failed to access network share: {0}", shareFolder));
				}
			}

			return String.Format("{0}\\{1}\\AppData\\Local\\Temp\\{2}", shareFolder, job.AgentUsername, nameof(CodeSign));
		}

		private string GetControllerPackageFileName(HardwareLabKitController controller)
		{
			return String.Format("p4vfsflt-{0}.hlkx", controller.Name.ToLower());
		}

		private void SetupClientAgent(HardwareLabKitJob job, string agentName)
		{
			RunHardwareLabConsoleCommand(job, agentName, "SetupClientAgent");
		}

		private void CreateControllerPackage(HardwareLabKitJob job, string agentName)
		{
			RunHardwareLabConsoleCommand(job, agentName, "CreateControllerPackage");
		}

		private void RunHardwareLabConsoleCommand(HardwareLabKitJob job, string agentName, string command, string commandArgs = null)
		{
			string localStagingFolder = GetAgentStagingFolder(job, agentName, local: true);

			string setupArgs = "powershell.exe";
			setupArgs += $" -ExecutionPolicy Unrestricted";
			setupArgs += $" -File \"{localStagingFolder}\\{CodeSignResources.HardwareLabConsolePs1}\"";
			setupArgs += $" -Command {command}";

			if (String.IsNullOrEmpty(commandArgs) == false)
			{
				setupArgs += $" {commandArgs}";
			}

			if (ProcessInfo.RemoteExecuteWait(setupArgs, agentName, $"{agentName}\\{job.AgentUsername}", job.AgentPassword, admin:true, interactive:true) != 0)
			{
				throw new Exception(String.Format("Failed to run HardwareLabConsole command {0} on agent {1}", command, agentName));
			}
		}
	}

	public class HardwareLabKitTest
	{
		public string Id { get; set; }
		public string Name { get; set; }
	}

	public class HardwareLabKitController
	{
		public string Name { get; set; }
		public string[] Clients { get; set; }
	}

	public class HardwareLabKitJob
	{
		public CodeSignJob CodeSignJob { get; set; }
		public HardwareLabKitController[] Controllers { get; set; }
		public string AgentUsername { get; set; }
		public string AgentPassword { get; set; }
		public string AgentSecretName { get; set; }
		public string ProjectName { get; set; }
		public string PoolName { get; set; }
	}
}
