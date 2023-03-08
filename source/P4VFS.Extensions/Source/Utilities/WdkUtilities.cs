// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.IO;
using System.Text.RegularExpressions;

namespace Microsoft.P4VFS.Extensions.Utilities
{
	public class WdkUtilities
	{
		public static string GetWdkRootFolder()
		{
			string windowsKitRoot = "";
			if (RegistryInfo.GetTypedValue(Microsoft.Win32.Registry.LocalMachine, @"SOFTWARE\Microsoft\Windows Kits\Installed Roots", "KitsRoot10", ref windowsKitRoot) == false)
			{
				throw new Exception("Failed to find installed Windows Driver Kit key");
			}

			string windowsKitBin = Path.Combine(windowsKitRoot, "bin");
			if (Directory.Exists(windowsKitBin) == false)
			{
				throw new DirectoryNotFoundException(String.Format("Windows Driver Kit root directory not found: {0}", windowsKitBin));
			}

			Version maxVersion = null;
			string maxVersionFolder = null;
			foreach (string versionFolder in Directory.EnumerateDirectories(windowsKitBin))
			{
				string versionString = Path.GetFileName(versionFolder);
				if (Regex.IsMatch(versionString, @"^\d+\.\d+\.\d+\.\d+$") && Version.TryParse(versionString, out Version version) && Directory.Exists(Path.Combine(versionFolder,"x64")))
				{
					if (maxVersion == null || version > maxVersion)
					{
						maxVersion = version;
						maxVersionFolder = versionFolder;
					}
				}
			}

			if (Directory.Exists(maxVersionFolder) == false)
			{
				throw new DirectoryNotFoundException("Latest Windows Driver Kit not found");
			}
			return maxVersionFolder;
		}

		public static string GetSignToolExe()
		{
			string signToolExe = String.Format(@"{0}\x64\signtool.exe", GetWdkRootFolder());
			if (File.Exists(signToolExe) == false)
			{
				throw new FileNotFoundException(String.Format("WDK signtool is not installed: {0}", signToolExe));
			}
			return signToolExe;
		}		
	}
}
