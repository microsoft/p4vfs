// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Linq;
using System.Diagnostics;
using System.IO;
using System.Security;
using System.Reflection;
using System.Runtime.InteropServices;
using Microsoft.P4VFS.Extensions;
using Microsoft.P4VFS.Extensions.Utilities;

namespace Microsoft.P4VFS.CodeSign
{
	public interface ICodeSignClient : IDisposable
	{
		bool Submit(CodeSignJob job);
	}

	public static class CodeSignInterop
	{
		[DllImport("cabapi.dll", EntryPoint = "Cab_Extract", CallingConvention = CallingConvention.Cdecl, SetLastError = true, CharSet = CharSet.Auto)]
		public static extern int ExtractCabFileToFolder(string filename, string outputDir);

		[DllImport("cabapi.dll", EntryPoint = "Cab_CreateCab", CallingConvention = CallingConvention.Cdecl, SetLastError = true, CharSet = CharSet.Auto)]
		public static extern int CreateCabFileFromFolder(string filename, string rootDirectory, string tempWorkingFolder, string filterToSelectFiles, int compressionType);
	}

	public static class CodeSignResources
	{
		public const string HardwareLabConsolePs1 = "HardwareLabConsole.ps1";
		public const string HardwareLabPlaylistXml = "HardwareLabPlaylist.xml";
		public const string SignAuthJson = "SignAuth.json";
		public const string SignInputSetupJson = "SignInputSetup.json";
		public const string SignPolicyJson = "SignPolicy.json";
	};

	public static class CodeSignUtilities
	{
		public static string ExtractResourceToFile(string folderPath, string resourceName)
		{
			string filePath = Path.GetFullPath(String.Format("{0}\\{1}", folderPath, resourceName));
			FileUtilities.CreateDirectory(folderPath);
			using (Stream resourceStream = ExtractResourceToStream(resourceName))
			{
				using (FileStream fileStream = File.Open(filePath, FileMode.Create))
					resourceStream.CopyTo(fileStream);
			}
			return filePath;
		}

		public static string ExtractResourceToString(string resourceName)
		{
			using (StreamReader resource = new StreamReader(ExtractResourceToStream(resourceName)))
			{
				return resource.ReadToEnd();
			}
		}

		public static Stream ExtractResourceToStream(string resourceName)
		{
			string fullResourceName = String.Format("{0}.Resource.{1}", Path.GetFileNameWithoutExtension(Assembly.GetExecutingAssembly().Location), resourceName);
			Stream stream = Assembly.GetExecutingAssembly().GetManifestResourceStream(fullResourceName);
			if (stream == null)
			{
				throw new Exception(String.Format("Failed to find resource: {0}", fullResourceName));
			}
			return stream;
		}

		public static int ExecuteAsUserWait(string fileName, string arguments, string username, SecureString password)
		{
			object receiverLock = new Object();
			DataReceivedEventHandler outputReceiver = new System.Diagnostics.DataReceivedEventHandler((s, e) =>
			{
				lock (receiverLock)
				{
					VirtualFileSystemLog.Info(e.Data);
				}
			});

			try
			{
				string[] userTokens = username.Split('\\');
				using (Process p = new Process())
				{
					p.StartInfo.FileName = fileName;
					p.StartInfo.Arguments = arguments;
					p.StartInfo.WorkingDirectory = Environment.CurrentDirectory;
					p.StartInfo.UseShellExecute = false;
					p.StartInfo.CreateNoWindow = true;
					p.StartInfo.WindowStyle = ProcessWindowStyle.Hidden;
					p.StartInfo.UserName = userTokens.Length > 1 ? userTokens[1] : userTokens[0];
					p.StartInfo.Domain = userTokens.Length > 1 ? userTokens[0] : Environment.UserDomainName;
					p.StartInfo.Password = password;
					p.StartInfo.RedirectStandardOutput = true;
					p.StartInfo.RedirectStandardError = true;
					p.OutputDataReceived += outputReceiver;
					p.ErrorDataReceived += outputReceiver;

					p.Start();

					p.BeginOutputReadLine();
					p.BeginErrorReadLine();

					p.WaitForExit();
					return p.ExitCode;
				}
			}
			catch (Exception e)
			{
				VirtualFileSystemLog.Info(String.Format("Failed to run '{0}' '{1}' : {2}", fileName, arguments, e));
			}
			return -1;
		}


		public static SecureString ReadConsolePassword(string promptText)
		{
			Console.Write(promptText);
			return ConsoleReader.ReadSecureLine();
		}

		public static SecureString CreateSecureString(string text)
		{
			SecureString secureText = new SecureString();
			foreach (char c in text)
			{
				secureText.AppendChar(c);
			}
			return secureText;
		}
	}

	public class CodeSignJob
	{
		public string EsrpClientFolder { get; set; }
		public string SignInputName { get; set; }
		public string SourceFolder { get; set; }
		public string TargetFolder { get; set; }
		public string NugetFolder { get; set; }
	};
}
