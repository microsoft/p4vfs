// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Linq;
using System.IO;
using System.Reflection;
using Microsoft.P4VFS.Extensions;
using Microsoft.P4VFS.Extensions.Utilities;
using Microsoft.P4VFS.CoreInterop;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace Microsoft.P4VFS.CodeSign
{
	public class EsrpClient : ICodeSignClient
	{
		private const string _EsrpClientVersion = "1.2.85";
		private const string _EsrpClientPkgSource = "https://microsoft.pkgs.visualstudio.com/_packaging/ESRP/nuget/v3/index.json";

		public EsrpClient()
		{
		}

		public void Dispose()
		{
		}

		public bool Submit(CodeSignJob job)
		{
			VirtualFileSystemLog.Info("Performing ESRP codesign");

			string esrpClientExe = Path.Combine(job.EsrpClientFolder ?? "", "EsrpClient.exe");
			if (File.Exists(esrpClientExe) == false)
			{
				string executingFolder = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
				string nugetExe = Path.Combine(job.NugetFolder ?? executingFolder, "nuget.exe");
				if (File.Exists(nugetExe) == false)
				{
					VirtualFileSystemLog.Error("Invalid or unspecified nuget.exe folder: '{0}'", job.NugetFolder);
					return false;
				}

				string esrpPackagesFolder = Path.Combine(executingFolder, "packages");
				string[] esrpClientPkgSources = {
					Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "Microsoft SDKs\\NuGetPackages"),
					Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "Microsoft SDKs\\NuGetPackages"),
					Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), ".nuget\\packages"),
					_EsrpClientPkgSource
				};

				bool esrpClientPkgInstalled = esrpClientPkgSources.Any(pkgSource =>
				{
					string nugetArgs = String.Format("install Microsoft.EsrpClient -Version {0} -source \"{1}\" -OutputDirectory \"{2}\"", _EsrpClientVersion, pkgSource, esrpPackagesFolder);
					return ProcessInfo.ExecuteWait(nugetExe, nugetArgs, echo: true, log: true) == 0;
				});

				if (esrpClientPkgInstalled == false)
				{
					VirtualFileSystemLog.Error("NuGet install of EsrpClient package failed");
					return false;
				}

				job.EsrpClientFolder = String.Format("{0}\\Microsoft.EsrpClient.{1}\\tools", esrpPackagesFolder, _EsrpClientVersion);
			}

			esrpClientExe = Path.Combine(job.EsrpClientFolder ?? "", "EsrpClient.exe");
			if (File.Exists(esrpClientExe) == false)
			{
				VirtualFileSystemLog.Error("Invalid or unspecified EsrpClient.exe folder: '{0}'", job.EsrpClientFolder);
				return false;
			}

			string signAuthFile = CreateJobSignAuthFile(job);
			string signInputFile = CreateJobSignInputFile(job);
			string signPolicyFile = CreateJobSignPolicyFile(job);
			if (!File.Exists(signAuthFile) || !File.Exists(signInputFile) || !File.Exists(signPolicyFile))
			{
				VirtualFileSystemLog.Error("Failed to create all submission files");
				return false;
			}

			string signOutputLogFile = Path.GetFullPath(Path.Combine(job.TargetFolder, "SignOutput.json"));
			FileUtilities.CreateDirectory(Path.GetDirectoryName(signOutputLogFile));

			string esrpClientArgs = $"Sign";
			esrpClientArgs += $" -a \"{signAuthFile}\"";
			esrpClientArgs += $" -p \"{signPolicyFile}\"";
			esrpClientArgs += $" -i \"{signInputFile}\"";
			esrpClientArgs += $" -o \"{signOutputLogFile}\"";
			esrpClientArgs += $" -l {(ServiceSettings.Verbosity == LogChannel.Verbose ? "Verbose" : "Information")}";
			int esrpClientExitCode = ProcessInfo.ExecuteWait(esrpClientExe, esrpClientArgs, echo: true, log: true);

			VirtualFileSystemLog.Info("EsrpClient exit code: {0}", esrpClientExitCode);
			VirtualFileSystemLog.Info("Log File: {0}", signOutputLogFile);
			return esrpClientExitCode == 0;
		}

		private string CreateJobSignAuthFile(CodeSignJob job)
		{
			return CodeSignUtilities.ExtractResourceToFile(job.TargetFolder, CodeSignResources.SignAuthJson);
		}

		private string CreateJobSignInputFile(CodeSignJob job)
		{
			string signInputFile = CodeSignUtilities.ExtractResourceToFile(job.TargetFolder, job.SignInputName);

			int numSignFiles = 0;
			JToken tokenSignInput = JObject.Parse(File.ReadAllText(signInputFile));
			foreach (JToken tokenBatch in tokenSignInput.SelectTokens("$.SignBatches[*]"))
			{
				JValue tokenSourceRootDirectory = tokenBatch.SelectToken("$.SourceRootDirectory") as JValue;
				tokenSourceRootDirectory.Value = job.SourceFolder;
				JValue tokenDestinationRootDirectory = tokenBatch.SelectToken("$.DestinationRootDirectory") as JValue;
				tokenDestinationRootDirectory.Value = job.TargetFolder;

				foreach (JToken tokenSignRequestFile in tokenBatch.SelectTokens("$.SignRequestFiles[*]"))
				{
					JValue tokenFileSourceLocation = tokenSignRequestFile.SelectToken("$.SourceLocation") as JValue;
					string signFilePath = Path.Combine(job.SourceFolder, tokenFileSourceLocation.Value<string>());

					if (PreprocessSignRequestFile(job, tokenSignRequestFile, signFilePath) == false)
					{
						VirtualFileSystemLog.Error("Failed to preprocess input file: {0}", signFilePath);
						return null;
					}

					if (File.Exists(signFilePath) == false)
					{
						VirtualFileSystemLog.Error("Failed to find input file: {0}", signFilePath);
						return null;
					}

					VirtualFileSystemLog.Info("Signing File: {0}", signFilePath);
					numSignFiles++;
				}
			}

			if (numSignFiles == 0)
			{
				VirtualFileSystemLog.Error("Failed to find any input files");
				return null;
			}

			using (StreamWriter fileStream = File.CreateText(signInputFile))
			{
				using (JsonTextWriter textWriter = new JsonTextWriter(fileStream))
				{
					textWriter.Indentation = 2;
					textWriter.Formatting = Formatting.Indented;
					tokenSignInput.WriteTo(textWriter);
				}
			}
			return signInputFile;
		}

		private bool PreprocessSignRequestFile(CodeSignJob job, JToken tokenSignRequestFile, string signFilePath)
		{
			return DevCenterClient.PreprocessSignRequestFile(job, tokenSignRequestFile, signFilePath);
		}

		private string CreateJobSignPolicyFile(CodeSignJob job)
		{
			return CodeSignUtilities.ExtractResourceToFile(job.TargetFolder, CodeSignResources.SignPolicyJson);
		}
	}
}
