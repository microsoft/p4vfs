// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.IO;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using Microsoft.P4VFS.Extensions.Utilities;

namespace Microsoft.P4VFS.External
{
	public abstract class Module
	{
		public abstract string Name 
		{ 
			get; 
		}

		public abstract string Description 
		{ 
			get; 
		}

		public ModuleContext Context 
		{ 
			get; 
			set; 
		}

		public abstract string Signature
		{ 
			get; 
		}

		public abstract void Restore();
	}

	public class ModuleContext
	{
		public ModuleProperties Properties
		{
			get; set;
		}
	}

	public enum ReservedProperty
	{
		Verbose,
		DebuggerLaunch,
		NugetPath,
		MsBuildPath,
		RootDir,
		ExternalDir,
		ModuleDir,
		PackagesDir,
		VisualStudioEdition,
		VisualStudioInstallFolder,
		ModulePattern,
	}

	public class ModuleProperties
	{
		private Dictionary<string, string> m_PropertyMap;

		public ModuleProperties()
		{
			m_PropertyMap = new Dictionary<string, string>(StringComparer.InvariantCultureIgnoreCase);
		}

		public string Get(string name, string defaultValue = null)
		{
			if (m_PropertyMap.TryGetValue(name, out string value))
			{
				return value;
			}
			return defaultValue;
		}

		public string Get(ReservedProperty name, string defaultValue = null)
		{
			return Get(name.ToString(), defaultValue);
		}

		public bool GetBool(string name, bool defaultValue = false)
		{
			return Converters.ToBoolean(Get(name), defaultValue).Value;
		}

		public bool GetBool(ReservedProperty name, bool defaultValue = false)
		{
			return GetBool(name.ToString(), defaultValue);
		}

		public void Set(string name, string value)
		{
			if (String.IsNullOrEmpty(name) == false)
			{
				m_PropertyMap[name] = value;
			}
		}

		public void Set(ReservedProperty name, string value)
		{
			Set(name.ToString(), value);
		}

		public bool IsSet(string name)
		{
			return m_PropertyMap.ContainsKey(name);
		}

		public bool IsSet(ReservedProperty name)
		{
			return IsSet(name.ToString());
		}

		public IReadOnlyCollection<KeyValuePair<string, string>> Items
		{
			get { return m_PropertyMap; }
		}
	}

	public static class ModuleInfo
	{
		public static Dictionary<string, string> LoadChecksumFile(string checksumFilePath)
		{
			Dictionary<string, string> checksums = new Dictionary<string, string>(StringComparer.InvariantCultureIgnoreCase);
			foreach (string line in File.ReadAllLines(checksumFilePath))
			{
				Match m = Regex.Match(line, @"^\s*(?<sum>[0-9a-f]+)\s+\*(?<name>\S+)\s*$", RegexOptions.IgnoreCase);
				if (m.Success)
				{
					checksums[m.Groups["name"].Value] = m.Groups["sum"].Value;
					continue;
				}

				m = Regex.Match(line, @"^\s*(?<sum>[0-9a-f]+)\s*$", RegexOptions.IgnoreCase);
				if (m.Success)
				{
					checksums[Path.GetFileNameWithoutExtension(checksumFilePath)] = m.Groups["sum"].Value;
					continue;
				}
			}
			Trace.TraceInformation("Loaded {0} checksums", checksums.Count);
			return checksums;
		}
	
		public static string GetFileChecksum(string filePath)
		{
			StringBuilder certutilOutput = new StringBuilder();
			if (ProcessInfo.ExecuteWait("certutil.exe", $"-hashfile \"{filePath}\" SHA256", stdout:certutilOutput) == 0)
			{
				foreach (string line in certutilOutput.ToString().Split('\n'))
				{
					Match m = Regex.Match(line, @"^(?<sum>[0-9a-f]+)\s*$", RegexOptions.IgnoreCase);
					if (m.Success)
					{
						return m.Groups["sum"].Value;
					}
				}
			}
			return "";
		}
	
		public static string DownloadFileToFolder(string url, string folderPath, Dictionary<string, string> checksums=null)
		{
			string filePath = Path.Combine(folderPath, Regex.Replace(url, @".*/",""));
			if (File.Exists(filePath))
			{
				File.Delete(filePath);
			}
			
			Trace.TraceInformation("Downloading {0} -> {1}", url, filePath);
			Directory.CreateDirectory(Path.GetDirectoryName(filePath));
			using (System.Net.WebClient request = new System.Net.WebClient())
			{
				request.DownloadFile(url, filePath);
			}

			if (File.Exists(filePath) == false)
			{
				throw new Exception(String.Format("Failed to download file: {0}", filePath));
			}

			if (checksums != null)
			{
				string localSum = GetFileChecksum(filePath);
				if (String.IsNullOrEmpty(localSum))
				{
					throw new Exception(String.Format("Failed to generate local checksum for file: {0}", filePath));
				}
				if (checksums.TryGetValue(Path.GetFileName(filePath), out string targetSum) == false || String.IsNullOrEmpty(targetSum))
				{
					throw new Exception(String.Format("Failed to find target checksum for file: {0}", Path.GetFileName(filePath)));
				}
				if (String.Compare(localSum, targetSum, StringComparison.InvariantCultureIgnoreCase) != 0)
				{
					throw new Exception(String.Format("Mismatched checksum for file: {0}", Path.GetFileName(filePath)));
				}
				Trace.TraceInformation("Matched checksum: {0}", Path.GetFileName(filePath));
			}
			else
			{
				Trace.TraceInformation("Skipped checksum: {0}", Path.GetFileName(filePath));
			}
			return filePath;
		}
	
		public static string FindFirstFolder(string rootFolderPath, string folderName, bool missingOK=false)
		{
			foreach (string subFolderPath in Directory.EnumerateDirectories(rootFolderPath, "*", SearchOption.AllDirectories))
			{
				if (String.Compare(Path.GetFileName(subFolderPath), folderName, StringComparison.InvariantCultureIgnoreCase) == 0)
				{
					return subFolderPath;
				}
			}
			if (missingOK == false)
			{
				throw new Exception(String.Format("Failed to find folder \"{0}\" under: {1}", folderName, rootFolderPath));
			}
			return "";
		}

		public static string TrimFileExtension(string filePath)
		{
			return Regex.Replace(filePath ?? "", @"\.[^\\/]+$", "");
		}

		public static string NugetInstall(ModuleContext context, string packageName, string packageVersion) 
		{
			string nugetExe = context.Properties.Get(ReservedProperty.NugetPath);
			if (File.Exists(nugetExe) == false)
			{
				throw new Exception(String.Format("Invalid or unspecified nuget.exe path: '{0}'", nugetExe));
			}

			string nugetConfig = Path.Combine(context.Properties.Get(ReservedProperty.RootDir), "nuget.config");
			if (File.Exists(nugetConfig) == false)
			{
				throw new Exception(String.Format("Failed to find P4VFS nuget.config file: '{0}'", nugetConfig));
			}

			string packagesFolder = context.Properties.Get(ReservedProperty.PackagesDir);
			if (String.IsNullOrEmpty(packagesFolder))
			{
				throw new Exception("Unspecified PackagesDir property");
			}

			Directory.CreateDirectory(packagesFolder);

			string nugetArgs = "install";
			nugetArgs += String.Format(" \"{0}\"", packageName);
			nugetArgs += String.Format(" -Version \"{0}\"", packageVersion);
			nugetArgs += String.Format(" -OutputDirectory \"{0}\"", packagesFolder);
			nugetArgs += String.Format(" -ConfigFile \"{0}\"", nugetConfig);
			//nugetArgs += " -NonInteractive";

			if (ProcessInfo.ExecuteWait(nugetExe, nugetArgs, echo:true, log:true) != 0)
			{
				throw new Exception(String.Format("Failed to install package: {0} {1}", packageName, packageVersion));
			}

			string packageInstallFolder = Path.GetFullPath(String.Format("{0}\\{1}.{2}", packagesFolder, packageName, packageVersion));
			if (Directory.Exists(packageInstallFolder) == false)
			{
				throw new Exception(String.Format("Failed to find package install folder: {0}", packageInstallFolder));
			}
			return packageInstallFolder;
		}

		public static string GetVisualStudioInstallFolder(string vsEdition)
		{
			string vsWhereExe = String.Format("{0}\\Microsoft Visual Studio\\Installer\\vswhere.exe", Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86));
			foreach (string line in ProcessInfo.ExecuteWaitOutput(vsWhereExe, "-property installationPath").Lines)
			{
				Match m = Regex.Match(line, String.Format(@"^(?<path>.+\\{0}\\.+)", Regex.Escape(vsEdition)), RegexOptions.IgnoreCase);
				if (m.Success)
				{
					return m.Groups["path"].Value;
				}
			}
			return "";
		}

		public static bool IsModuleRestored(Module module)
		{
			string restoredFile = GetModuleRestoredFile(module.Context);
			if (File.Exists(restoredFile) && String.Compare(module.Signature ?? "", File.ReadAllText(restoredFile)) == 0)
			{
				return true;
			}
			return false;
		}

		public static void SetModuleRestored(Module module, bool restored = true)
		{
			string restoredFile = GetModuleRestoredFile(module.Context);
			if (restored)
			{
				File.WriteAllText(restoredFile, module.Signature ?? "");
			}
			else if (File.Exists(restoredFile))
			{
				File.Delete(restoredFile);
			}
		}

		private static string GetModuleRestoredFile(ModuleContext context)
		{
			return Path.GetFullPath(String.Format("{0}\\.restored", context.Properties.Get(ReservedProperty.ModuleDir)));
		}

		public static string GetEnvironmentFullPath(string fileName)
		{
			foreach (string folderPath in Environment.GetEnvironmentVariable("PATH").Split(Path.PathSeparator))
			{
				string filePath = Path.GetFullPath(String.Format("{0}\\{1}", folderPath, fileName));
				if (File.Exists(filePath))
				{
					return filePath;
				}
			}
			return Path.GetFullPath(fileName);
		}

		public static void TraceContext(ModuleContext context)
		{
			if (context.Properties.GetBool(ReservedProperty.Verbose))
			{
				foreach (var item in context.Properties.Items)
				{
					Trace.TraceInformation("{0}={1}", item.Key, item.Value);
				}
			}
		}
	}
}
