// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.IO;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text.RegularExpressions;
using Microsoft.P4VFS.Extensions.Utilities;

namespace Microsoft.P4VFS.External
{
	public class OpensslModule : Module
	{
		private const string OPENSSL_VERSION = "3.3.1";
		private const string PERL_PACKAGE_NAME = "StrawberryPerl";
		private const string PERL_VERSION = "5.28.0.1";

		public override string Name
		{
			get { return "Openssl"; }
		}

		public override string Description
		{
			get { return String.Format("{0} {1}", Name, OPENSSL_VERSION); }
		}

		public override string Signature
		{ 
			get { return String.Join(";", Name, OPENSSL_VERSION, PERL_PACKAGE_NAME, PERL_VERSION); }
		}

		private string ExtractOpensslArchive(string opensslArchiveFile)
		{
			if (opensslArchiveFile.EndsWith(".tar.gz", StringComparison.InvariantCultureIgnoreCase) == false)
			{
				throw new Exception(String.Format("Invalid openssl archive file: {0}", opensslArchiveFile));
			}

			string opensslExtractFolder = Regex.Replace(opensslArchiveFile, @"\.gz$", "", RegexOptions.IgnoreCase);
			ShellUtilities.RemoveDirectoryRecursive(opensslExtractFolder);
			Directory.CreateDirectory(opensslExtractFolder);
	
			Trace.TraceInformation("Extracting: {0} -> {1}", opensslArchiveFile, opensslExtractFolder);
			if (ProcessInfo.ExecuteWait("tar.exe", String.Format("-x -C {0} -z -f {1}", opensslExtractFolder, opensslArchiveFile), echo:true, log:true) != 0)
			{
				throw new Exception(String.Format("Failed to exact archive: {0}", opensslArchiveFile));
			}

			string opensslSourceFolder = Path.GetFullPath(String.Format("{0}\\{1}", opensslExtractFolder, Regex.Replace(Path.GetFileName(opensslArchiveFile), @"\.tar\.gz$", "", RegexOptions.IgnoreCase)));
			if (Directory.Exists(opensslSourceFolder) == false)
			{
				throw new Exception(String.Format("Failed to find source folder: {0}", opensslSourceFolder));
			}
			return opensslSourceFolder;
		}
	
		private void BuildOpensslLibrary(string opensslArchiveFolder, string opensslTargetFolder, string configuration)
		{
			string visualStudioEdition = Context.Properties.Get(ReservedProperty.VisualStudioEdition);
			string visualStudioInstallFolder = Context.Properties.Get(ReservedProperty.VisualStudioInstallFolder);

			string opensslLibName = $"x64.vs{visualStudioEdition}.dyn.{configuration}";
			string opensslConfigurationFolder = $"{opensslArchiveFolder}-{configuration}";
			string vcvarsScriptPath = $"{visualStudioInstallFolder}\\VC\\Auxiliary\\Build\\vcvars64.bat";
			if (File.Exists(vcvarsScriptPath) == false)
			{
				throw new Exception(String.Format("Failed to find Visual Studio environment: {0}", vcvarsScriptPath));
			}

			string perlExe = GetPerlExe();
			string buildScriptPath = Path.GetFullPath(String.Format("{0}\\{1}.bat", opensslArchiveFolder, opensslLibName));
			string[] buildScriptLines = new[] {
				$"@ECHO ON",
				$"CALL \"{vcvarsScriptPath}\"",
				$"CD /D \"{opensslArchiveFolder}\"",
				$"\"{perlExe}\" Configure VC-WIN64A no-asm \"--prefix={opensslConfigurationFolder}\" \"--openssldir={opensslConfigurationFolder}-ssl\" --{configuration}",
				$"IF %ERRORLEVEL% NEQ 0 EXIT /B 1",
				$"nmake clean",
				$"IF %ERRORLEVEL% NEQ 0 EXIT /B 1",
				$"nmake",
				$"IF %ERRORLEVEL% NEQ 0 EXIT /B 1",
				//$"nmake test",
				//$"IF %ERRORLEVEL% NEQ 0 EXIT /B 1",
				$"nmake install_sw",
				$"IF %ERRORLEVEL% NEQ 0 EXIT /B 1",
				$"xcopy /f /y /r \"{opensslConfigurationFolder}\\lib\\*.lib\" \"{opensslTargetFolder}\\lib\\{opensslLibName}\\\"",
				$"IF %ERRORLEVEL% NEQ 0 EXIT /B 1",
				$"xcopy /f /y /r \"{opensslConfigurationFolder}\\bin\\*libssl*\" \"{opensslTargetFolder}\\lib\\{opensslLibName}\\\"",
				$"IF %ERRORLEVEL% NEQ 0 EXIT /B 1",
				$"xcopy /f /y /r \"{opensslConfigurationFolder}\\bin\\*libcrypto*\" \"{opensslTargetFolder}\\lib\\{opensslLibName}\\\"",
				$"IF %ERRORLEVEL% NEQ 0 EXIT /B 1",
				$"xcopy /f /y /r /s \"{opensslConfigurationFolder}\\include\" \"{opensslTargetFolder}\\include\\\"",
				$"IF %ERRORLEVEL% NEQ 0 EXIT /B 1",
				$"EXIT /B 0" };

			Trace.TraceInformation("Writing: {0}", buildScriptPath);
			File.WriteAllLines(buildScriptPath, buildScriptLines);
		
			Trace.TraceInformation("Building: {0}", opensslLibName);
			if (ProcessInfo.ExecuteWait("cmd.exe", String.Format("/c \"{0}\"", buildScriptPath), echo:true, log:true) != 0)
			{
				throw new Exception(String.Format("Failed to build: {0}", opensslLibName));
			}
		}

		private string GetPerlExe()
		{
			string perlPackageFolder = ModuleInfo.NugetInstall(Context, PERL_PACKAGE_NAME, PERL_VERSION);
			string perlExe = Path.GetFullPath(String.Format("{0}\\bin\\perl.exe", perlPackageFolder));
			if (File.Exists(perlExe) == false)
			{
				throw new Exception(String.Format("Failed to find perl.exe: {0}", perlExe));
			}
			return perlExe;
		}

		public override void Restore()
		{
			string opensslModuleFolder = Context.Properties.Get(ReservedProperty.ModuleDir);
			string opensslArchiveUrl = String.Format("https://www.openssl.org/source/openssl-{0}.tar.gz", OPENSSL_VERSION);
			string opensslTargetFolder = String.Format("{0}\\{1}", opensslModuleFolder, OPENSSL_VERSION);
			string workingFolder = String.Format("{0}\\Temp", opensslModuleFolder);
    
			ShellUtilities.RemoveDirectoryRecursive(opensslTargetFolder);
			ShellUtilities.RemoveDirectoryRecursive(workingFolder);
        
			// Download the checksums file
			string checksumFilePath = ModuleInfo.DownloadFileToFolder(String.Format("{0}.sha256", opensslArchiveUrl), workingFolder);
			Dictionary<string, string> checksums = ModuleInfo.LoadChecksumFile(checksumFilePath);

			// Download and extract the Openssl source archive
			string opensslArchiveFile = ModuleInfo.DownloadFileToFolder(opensslArchiveUrl, workingFolder, checksums);
			string opensslArchiveFolder = ExtractOpensslArchive(opensslArchiveFile);

			// Build and deploy the release Openssl library
			BuildOpensslLibrary(opensslArchiveFolder, opensslTargetFolder, "release");
    
			// Build and deploy the debug Openssl library
			BuildOpensslLibrary(opensslArchiveFolder, opensslTargetFolder, "debug");

			// Remove temporary working folder
			ShellUtilities.RemoveDirectoryRecursive(workingFolder);
		}
	}
}
