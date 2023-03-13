// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;
using Microsoft.P4VFS.Extensions.Utilities;

namespace Microsoft.P4VFS.External
{
	public class P4apiModule : Module
	{
		private const string P4API_VERSION = "r21.2";
		private const string P4API_VISUAL_STUDIO_EDITION = "2019"; // TODO: Remove when P4API libs are available for 2022  

		public override string Name
		{
			get { return "P4API"; }
		}

		public override string Description
		{
			get { return String.Format("{0} {1}", Name, P4API_VERSION); }
		}
	
		private void ExtractP4apiSDK(string p4apiZipFile, string targetFolder, string libName)
		{
			string p4apiExtractFolder = ModuleInfo.TrimFileExtension(p4apiZipFile);
			ShellUtilities.RemoveDirectoryRecursive(p4apiExtractFolder);
	
			Trace.TraceInformation("Extracting: {0} -> {1}", p4apiZipFile, p4apiExtractFolder);
			System.IO.Compression.ZipFile.ExtractToDirectory(p4apiZipFile, p4apiExtractFolder);
	
			string srcIncludeFolder = ModuleInfo.FindFirstFolder(p4apiExtractFolder, "include");
			string dstIncludeFolder = Path.Combine(targetFolder, Path.GetFileName(srcIncludeFolder));
			Trace.TraceInformation("Copying: {0} -> {1}", srcIncludeFolder, dstIncludeFolder);
			ShellUtilities.CopyDirectoryRecursive(srcIncludeFolder, dstIncludeFolder);
	
			string srcLibFolder = ModuleInfo.FindFirstFolder(p4apiExtractFolder, "lib");
			string dstLibFolder = Path.Combine(targetFolder, Path.GetFileName(srcLibFolder), libName);
			Trace.TraceInformation("Copying: {0} -> {1}", srcLibFolder, dstLibFolder);
			ShellUtilities.CopyDirectoryRecursive(srcLibFolder, dstLibFolder);
		}
	
		private void ExtractP4apiCore(string p4apiCoreZipFile, string targetFolder)
		{
			string p4apiExtractFolder = ModuleInfo.TrimFileExtension(p4apiCoreZipFile);
			ShellUtilities.RemoveDirectoryRecursive(p4apiExtractFolder);
		
			string p4apiBinFolder = Path.Combine(targetFolder, "bin");
			Directory.CreateDirectory(p4apiBinFolder);
	
			Trace.TraceInformation("Extracting: {0} -> {1}", p4apiCoreZipFile, p4apiExtractFolder);
			System.IO.Compression.ZipFile.ExtractToDirectory(p4apiCoreZipFile, p4apiExtractFolder);

			foreach (string fileName in new[]{"p4.exe", "p4d.exe"})
			{
				string srcPath = Path.Combine(p4apiExtractFolder, fileName);
				string dstPath = Path.Combine(p4apiBinFolder, fileName);
				Trace.TraceInformation("Copying: {0} -> {1}", srcPath, dstPath);
				File.Copy(srcPath, dstPath);
			}
		}

		public override void Restore()
		{
			string visualStudioEdition = Context.Properties.Get(ReservedProperty.VisualStudioEdition);
			string p4apiVisualStudioEdition = P4API_VISUAL_STUDIO_EDITION ?? visualStudioEdition;
			string p4apiModuleFolder = Context.Properties.Get(ReservedProperty.ModuleDir);
			string p4apiUrlRoot = String.Format("http://filehost.perforce.com/perforce/{0}", P4API_VERSION);
			string p4apiTargetFolder = String.Format("{0}\\{1}", p4apiModuleFolder, Regex.Replace(P4API_VERSION, "^r","20"));
			string workingFolder = String.Format("{0}\\Temp", p4apiModuleFolder);
	
			ShellUtilities.RemoveDirectoryRecursive(p4apiTargetFolder);
			ShellUtilities.RemoveDirectoryRecursive(workingFolder);
		
			// Download the checksums file
			string checksumFilePath = ModuleInfo.DownloadFileToFolder($"{p4apiUrlRoot}/bin.ntx64/SHA256SUMS", workingFolder);
			Dictionary<string, string> checksums = ModuleInfo.LoadChecksumFile(checksumFilePath);

			// Download and deploy the debug P4API
			string p4apiDbgFile = ModuleInfo.DownloadFileToFolder($"{p4apiUrlRoot}/bin.ntx64/p4api_vs{p4apiVisualStudioEdition}_dyn_vsdebug_openssl1.1.1.zip", workingFolder, checksums);
			ExtractP4apiSDK(p4apiDbgFile, p4apiTargetFolder, $"x64.vs{visualStudioEdition}.dyn.debug");
	
			// Download and deploy the release P4API
			string p4apiRelFile = ModuleInfo.DownloadFileToFolder($"{p4apiUrlRoot}/bin.ntx64/p4api_vs{p4apiVisualStudioEdition}_dyn_openssl1.1.1.zip", workingFolder, checksums);
			ExtractP4apiSDK(p4apiRelFile, p4apiTargetFolder, $"x64.vs{visualStudioEdition}.dyn.release");
	
			// Download and deploy the perforce test binaries
			string p4apiCoreFile = ModuleInfo.DownloadFileToFolder($"{p4apiUrlRoot}/bin.ntx64/helix-core-server.zip", workingFolder, checksums);
			ExtractP4apiCore(p4apiCoreFile, p4apiTargetFolder);

			// Remove temporary working folder
			ShellUtilities.RemoveDirectoryRecursive(workingFolder);
		}
	}
}
