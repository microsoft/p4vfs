// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Management;
using System.Text.RegularExpressions;
using Microsoft.P4VFS.Extensions.Linq;

namespace Microsoft.P4VFS.Extensions.Utilities
{
	public class FileUtilities
	{
		public static string GetExtension(string filePath)
		{
			string extension = Path.GetExtension(filePath);
			if (extension.StartsWith("."))
			{
				extension = extension.Substring(1);
			}
			return extension;
		}

		public static string GetTempFileWithExtension(string extension)
		{
			return GetTempFileWithExtension(Path.GetTempPath(), extension);
		}

		public static string GetTempFileWithExtension(string directory, string extension)
		{
			RandomEx random = new RandomEx();
			if (!extension.StartsWith("."))
			{
				extension = "." + extension;
			}

			CreateDirectory(directory);

			string path = null;
			while (true)
			{
				do
				{
					path = Path.Combine(directory, random.GetRandomString(12) + extension);
				}
				while (File.Exists(path));

				try
				{
					File.Create(path).Close();
					return path;
				}
				catch (IOException) 
				{
				}
			}
		}

		public static void CreateDirectory(string directory)
		{
			Directory.CreateDirectory(directory);
		}

		public static void DeleteDirectoryAndFiles(string directory)
		{
			if (Directory.Exists(directory))
			{
				try
				{
					foreach (var filePath in Directory.GetFiles(directory, "*", SearchOption.AllDirectories))
					{
						File.SetAttributes(filePath, FileAttributes.Normal);
						File.Delete(filePath);
					}
					Directory.Delete(directory, true);
				}
				catch
				{
					if (Directory.Exists(directory))
					{
						throw;
					}
				}
			}
		}

		public static long GetDirectorySize(string directory)
		{
			DirectoryInfo di = new DirectoryInfo(directory);
			return di.EnumerateFiles("*", SearchOption.AllDirectories).Sum(fi => fi.Length);
		}

		public static int GetDirectoryFileCount(string directory)
		{
			DirectoryInfo di = new DirectoryInfo(directory);
			return di.EnumerateFiles("*", SearchOption.AllDirectories).Count();
		}

		public static bool IsDirectoryEmpty(string directory)
		{
			return !Directory.EnumerateFiles(directory).Any() && !Directory.EnumerateFiles(directory, "*", SearchOption.AllDirectories).Any();
		}

		public static void DeleteFile(string filePath)
		{
			if (File.Exists(filePath))
			{
				File.SetAttributes(filePath, FileUtilities.GetAttributes(filePath) & ~FileAttributes.ReadOnly);
				File.Delete(filePath);
			}
		}

		public static long GetFileLength(string filePath)
		{
			if (File.Exists(filePath))
			{
				var info = new FileInfo(filePath);
				return info.Length;
			}
			return -1;
		}

		public static FileAttributes GetAttributes(string filePath)
		{
			return CoreInterop.NativeMethods.GetFileAttributes(filePath);
		}

		public static string GetFileHash(string filePath)
		{
			if (File.Exists(filePath))
			{
				using (System.Security.Cryptography.MD5 hasher = System.Security.Cryptography.MD5.Create())
				{
					using (FileStream stream = File.OpenRead(filePath))
					{
						byte[] hash = hasher.ComputeHash(stream);
						return BitConverter.ToString(hash).Replace("-", "").ToLowerInvariant();
					}
				}
			}
			return String.Empty;
		}

		public static IReadOnlyDictionary<string, string> GetFileProperties(string filePath)
		{
			Dictionary<string, string> properties = new Dictionary<string, string>(StringComparer.InvariantCultureIgnoreCase);
			if (File.Exists(filePath))
			{
				ManagementObjectSearcher mos = new ManagementObjectSearcher(String.Format("select * from CIM_DataFile where Name=\"{0}\"", Path.GetFullPath(filePath).Replace("\\","\\\\")));
				if (mos != null)
				{
					foreach (ManagementObject mo in mos.Get())
					{
						foreach (PropertyData pd in mo.Properties)
						{
							if (String.IsNullOrEmpty(pd.Name))
							{
								continue;
							}
							object value = mo[pd.Name];
							if (value != null)
								properties[pd.Name] = value.ToString();
						}
					}
				}
			}
			return properties;
		}

		public static bool CopyFile(string srcFile, string dstFolder, bool overwrite = false)
		{
			try
			{
				if (String.IsNullOrEmpty(srcFile) || String.IsNullOrEmpty(dstFolder))
				{
					return false;
				}
				if (File.Exists(srcFile) == false)
				{
					return false;
				}
				string dstFile = String.Format("{0}\\{1}", dstFolder, Path.GetFileName(srcFile));
				if (File.Exists(dstFile))
				{
					if (overwrite == false)
					{
						return false;
					}
					MakeWritable(dstFile);
				}
				if (Directory.Exists(dstFolder) == false)
				{
					DirectoryInfo di = Directory.CreateDirectory(dstFolder);
					if (di == null || di.Exists == false)
					{
						return false;
					}
				}

				File.Copy(srcFile, dstFile, true);
				return true;
			}
			catch {}
			return false; 
		}

		public static bool IsReadOnly(string filePath)
		{
			if (String.IsNullOrEmpty(filePath))
				return false;
			FileAttributes attributes = (new FileInfo(filePath)).Attributes;
			return (attributes > 0 && (attributes & FileAttributes.ReadOnly) != 0);
		}

		public static void MakeReadOnly(string filePath)
		{
			if (File.Exists(filePath))
			{
				File.SetAttributes(filePath, FileUtilities.GetAttributes(filePath) | FileAttributes.ReadOnly);
			}
		}

		public static void MakeWritable(string filePath)
		{
			if (File.Exists(filePath))
			{
				File.SetAttributes(filePath, FileUtilities.GetAttributes(filePath) & ~FileAttributes.ReadOnly);
			}
		}

		public static bool CreateWritable(string filePath)
		{
			try
			{
				if (String.IsNullOrEmpty(filePath))
				{
					return false;
				}
				if (File.Exists(filePath))
				{
					File.SetAttributes(filePath, FileUtilities.GetAttributes(filePath) & ~FileAttributes.ReadOnly);
					return true;
				}
				using (var file = File.Open(filePath, FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.ReadWrite)) 
				{
					return true;
				}
			}
			catch {}
			return false;
		}

		public static string[] GetFiles(string sourceFolder, string filters, SearchOption searchOption)
		{
			return filters.Split('|').SelectMany(filter => Directory.GetFiles(sourceFolder, filter, searchOption)).ToArray();
		}

		public static string[] GetFiles(string sourceFolder, string filters)
		{
			return filters.Split('|').SelectMany(filter => Directory.GetFiles(sourceFolder, filter)).ToArray();
		}

		public static string[] GetFiles(string sourceFolder, IEnumerable<string> filters, SearchOption searchOption)
		{
			return filters.SelectMany(filter => Directory.GetFiles(sourceFolder, filter, searchOption)).ToArray();
		}

		public static string[] GetFiles(string sourceFolder, IEnumerable<string> filters)
		{
			return filters.SelectMany(filter => Directory.GetFiles(sourceFolder, filter)).ToArray();
		}

		public static string[] ReadAllLines(string filePath)
		{
			try
			{
				if (File.Exists(filePath) == false)
				{
					return null;
				}
				using (FileStream stream = System.IO.File.Open(filePath, System.IO.FileMode.Open, System.IO.FileAccess.Read, System.IO.FileShare.ReadWrite))
				{
					if (stream == null)
					{
						return null;
					}
					List<string> lines = new List<string>();
					StreamReader reader = new StreamReader(stream);
					for (string line = reader.ReadLine(); line != null; line = reader.ReadLine())
					{
						lines.Add(line);
					}
					return lines.ToArray();
				}
			}
			catch {}
			return null;
		}
	}
}
