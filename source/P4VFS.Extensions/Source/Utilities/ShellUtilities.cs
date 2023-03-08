// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.IO;
using System.Linq;
using System.Threading;
using System.Security.Principal;

namespace Microsoft.P4VFS.Extensions.Utilities
{
	public static class ShellUtilities
	{
		public static void RemoveDirectoryRecursive(string folderPath, int retryWait = 500, int limitWait = 5000)
		{
			DateTime endTime = DateTime.Now.AddMilliseconds((double)limitWait);
			do
			{
				if (Directory.Exists(folderPath) == false || ProcessInfo.ExecuteWait("cmd.exe", String.Format("/s /c \"rmdir /s /q \"{0}\"\"", folderPath)) != 0)
					return;
				Thread.Sleep(retryWait);
			}
			while (DateTime.Now < endTime);

			if (Directory.Exists(folderPath))
			{
				throw new Exception(String.Format("Failed to delete folder: {0}", folderPath));
			}
		}

		public static void CopyDirectoryRecursive(string srcFolderPath, string dstFolderPath)
		{
			if (ProcessInfo.ExecuteWait("xcopy.exe", String.Format("/q /r /s /y \"{0}\" \"{1}\\\"", srcFolderPath.TrimEnd('\\'), dstFolderPath.TrimEnd('\\'))) != 0)
			{
				throw new Exception(String.Format("Failed to copy folder: {0} -> {1}", srcFolderPath, dstFolderPath));
			}
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
	}
}

