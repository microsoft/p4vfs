// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Diagnostics;
using System.Linq;
using System.IO;
using System.Xml;
using System.Text;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.P4VFS.Extensions;
using Microsoft.P4VFS.Extensions.Utilities;
using Microsoft.P4VFS.Extensions.Linq;
using Microsoft.P4VFS.CoreInterop;

namespace Microsoft.P4VFS.UnitTest
{
	[TestClass, BasePriority(500)]
	public class UnitTestServer : UnitTestBase
	{
		[TestMethod, Priority(0)]
		public void StartupLocalPerforceServerTest()
		{
			ServerWorkspaceReset();

			string serverRootFolder = GetServerRootFolder();
			string serverP4dExe = GetServerP4dExe();
			Assert(FileUtilities.CopyFile(P4dExe, Path.GetDirectoryName(serverP4dExe)));
			Assert(File.Exists(serverP4dExe));

			XmlDocument recipeDocument = LoadServerRecipeXmlDocument();
			XmlDocument historyDocument = LoadServerHistoryXmlDocument();
			MergeHistoryXmlDocument(recipeDocument, historyDocument);

			uint serverPortNumber = GetServerPortNumber(_P4Port);
			string serverDatabaseFolder = String.Format("{0}\\db", serverRootFolder);
			FileUtilities.CreateDirectory(serverDatabaseFolder);
			string serverLogFile = String.Format("{0}\\p4d.log", serverRootFolder);
			string serverDescription = GetServerDescription(_P4Port);
			string serverArgs = String.Format("-L \"{0}\" -r \"{1}\" -p {2} -Id {3} -J off", serverLogFile, serverDatabaseFolder, serverPortNumber, serverDescription);
			
			ProcessStartInfo serverStartInfo = new ProcessStartInfo{ 
				FileName = serverP4dExe, 
				Arguments = serverArgs, 
				CreateNoWindow = true, 
				WindowStyle = ProcessWindowStyle.Hidden,
				UseShellExecute = false };

			// Start a new local perforce server, completely empty, and configure basic Xbox Game Studio options
			Process serverProcess = Process.Start(serverStartInfo);
			Assert(serverProcess != null, String.Format("Failed to start server process ({0} {1})", serverP4dExe, serverArgs));
			Assert(!serverProcess.HasExited, "Server process quit prematuraly");
			AssertRetry(() => ProcessInfo.ExecuteWait(P4Exe, String.Format("-p {0} info", _P4Port)) == 0);

			string[] serverConfigVariables = new[] {
				"auth.sso.allow.passwd=1",
				"dm.user.noautocreate=2", 
				"dm.user.resetpassword=0",
				"lbr.autocompress=1",
				"monitor=10",
				"net.parallel.submit.threads=8",
				"net.parallel.max=8",
				"server=2",
				"submit.unlocklocked=1",
			};
			foreach (string configVariable in serverConfigVariables)
			{
				StringBuilder output = new StringBuilder();
				Assert(!serverProcess.HasExited);
				Assert(ProcessInfo.ExecuteWait(P4Exe, String.Format("-p {0} configure set {1}", _P4Port, configVariable), echo: true, stdout: output) == 0, output.ToString());
			}

			// Create the server P4LOGINSSO script
			string serverLoginSSOFilePath = GetServerLoginSSOFilePath();
			WriteServerLoginSSOScript(serverLoginSSOFilePath, recipeDocument);

			// Create triggers for our basic test configuration. Allows for both SSO authentication and perforce password authentication
			string[] serverTriggerCmds = new[] {
				String.Format("sso auth-check-sso auth \"%quote%{0}%quote% %user%\"", serverLoginSSOFilePath),
			};
			string serverTriggers = String.Format("Triggers:\n{0}", String.Join("", serverTriggerCmds.Select(s => "\t"+s)));
			Assert(ProcessInfo.ExecuteWait(P4Exe, String.Format("-p {0} triggers -i", _P4Port), echo:true, stdin:serverTriggers) == 0);

			// Get the default generated admin for this new database. This user will be deleted afterwards
			StringBuilder defaultUsersOutput = new StringBuilder();
			Assert(ProcessInfo.ExecuteWait(P4Exe, String.Format("-p {0} -ztag users", _P4Port), echo:true, stdout:defaultUsersOutput) == 0);
			string[] defaultUsers = defaultUsersOutput.ToString().Split('\n').Select(s => Regex.Match(s, "^... User (.+)")?.Groups[1].Value).Where(u => !String.IsNullOrEmpty(u)).ToArray();
			Assert(defaultUsers.Length == 1);
			string defaultUser = defaultUsers[0];

			// Set a password for the admin, then restart the server for the SSO login options to take effect, then change the admin password as required 
			Assert(ProcessInfo.ExecuteWait(P4Exe, String.Format("-p {0} -u {1} passwd -P {2}{2}", _P4Port, defaultUser, DefaultP4Passwd), echo:true) == 0);
			Assert(ProcessInfo.ExecuteWait(P4Exe, String.Format("-p {0} admin restart", _P4Port), echo:true) == 0);
			Assert(ProcessInfo.ExecuteWait(P4Exe, String.Format("-p {0} -u {1} passwd", _P4Port, defaultUser), echo:true, stdin:String.Format("{0}{0}\n{0}\n{0}\n", DefaultP4Passwd)) == 0);
			Assert(ProcessInfo.ExecuteWait(P4Exe, String.Format("-p {0} -u {1} set P4PASSWD=", _P4Port, defaultUser), echo:true) == 0);

			// Perform initial administrator operations using the default generated admin user (typically current session user name).
			using (DepotClient adminClient = new DepotClient()) 
			{
				adminClient.Connect(depotServer:_P4Port, depotUser:defaultUser, depotPasswd:DefaultP4Passwd);
				Assert(adminClient.IsConnected());

				// Create all users specified in the recipe, and generate tickets. This must also contain the predefined _P4User!
				foreach (XmlElement userElement in recipeDocument.SelectNodes("./server/user").OfType<XmlElement>())
				{
					string userName = userElement.GetAttribute("name");
					Assert(String.IsNullOrEmpty(userName) == false);
					string password = userElement.GetAttribute("password").NullIfEmpty() ?? DefaultP4Passwd;

					string userSpec = String.Join("\n", new[]{
						String.Format("User:\t{0}", userName),
						String.Format("FullName:\t{0}", userElement.GetAttribute("fullname").NullIfEmpty() ?? userName),
						String.Format("Email:\t{0}", userElement.GetAttribute("email").NullIfEmpty() ?? String.Format("{0}@localhost.com", userName))});

					DepotResultView userResult = adminClient.Run(new DepotCommand{ Name="user", Args=new[]{"-i", "-f"}, Input=userSpec }).ToView();
					Assert(userResult.HasError == false);
					Assert(adminClient.Users().Nodes.Any(n => n.User == _P4User));
					Assert(adminClient.Run(new DepotCommand{ Name="passwd", Args=new[]{userName}, Prompt=(_) => password }).HasError == false);

					using (DepotClient userClient = new DepotClient())
					{
						userClient.Connect(depotServer:_P4Port, depotUser:userName, depotPasswd:password);
						Assert(userClient.IsConnected());
					}
				}

				// Find and create all depots specified in the recipe
				HashSet<string> depotNames = new HashSet<string>();
				foreach (XmlElement clientElement in recipeDocument.SelectNodes("./server/client").OfType<XmlElement>())
				{
					foreach (string depotPath in clientElement.GetAttribute("view").Split(new[]{','}, StringSplitOptions.RemoveEmptyEntries))
					{
						Match m = Regex.Match(depotPath, @"^.*?//(.+?)(/\.\.\.)?$");
						Assert(m.Success);
						depotNames.Add(m.Groups[1].Value);
					}
				}

				foreach (string depotName in depotNames)
				{
					string clientSpec = String.Join("\n", new[]{
						$"Depot:\t{depotName}",
						$"Owner:\t{_P4User}",
						$"Type:\tlocal",
						$"Map:\t{depotName}/...",
					});
					AssertLambda(() => adminClient.Run(new DepotCommand{ Name="depot", Args=new[]{"-i"}, Input=clientSpec }).HasError == false);
				}

				// Create spec depot and update it
				string depotSpec = String.Join("\n", new[]{
					$"Depot:\tspec",
					$"Owner:\t{_P4User}",
					$"Type:\tspec",
					$"Map:\tspec/...",
				});
				AssertLambda(() => adminClient.Run(new DepotCommand{ Name="depot", Args=new[]{"-i"}, Input=depotSpec }).HasError == false);
				Assert(adminClient.Run("admin", new[]{"updatespecdepot", "-a"}).HasError == false);

				// Create all clients specified in the recipe. This must also contain the predefined _P4Client!
				foreach (XmlElement clientElement in recipeDocument.SelectNodes("./server/client").OfType<XmlElement>())
				{
					string clientName = clientElement.GetAttribute("name");
					Assert(String.IsNullOrEmpty(clientName) == false);
					string clientOwner = clientElement.GetAttribute("owner");
					Assert(String.IsNullOrEmpty(clientOwner) == false);
					IEnumerable<string> clientView = clientElement.GetAttribute("view").Split(new[]{','}, StringSplitOptions.RemoveEmptyEntries);
					Assert(clientView.Any());
						
					clientView = clientView.Select(p => { 
						Match m = Regex.Match(p, @"^.*?//(.+)"); 
						Assert(m.Success); 
						return String.Format("{0} //{1}/{2}", p, Regex.Escape(clientName), m.Groups[1].Value); 
					});

					string clientSpec = String.Join("\n", new[]{
						String.Format("Client:\t{0}", clientName),
						String.Format("Owner:\t{0}", clientOwner),
						String.Format("Host:\t{0}", ""),
						String.Format("Root:\t{0}", GetServerClientRootFolder(clientName)),
						String.Format("Options:\t{0}", "noallwrite clobber nocompress unlocked nomodtime rmdir"),
						String.Format("LineEnd:\t{0}", "local"),
						String.Format("Description:\n\t{0}", "Created by " + nameof(StartupLocalPerforceServerTest)),
						String.Format("View:\n\t{0}", String.Join("\n\t", clientView))
					});
					Assert(adminClient.Run(new DepotCommand{ Name="client", Args=new[]{"-i"}, Input=clientSpec }).HasError == false);
				}

				// Verify that the hardcoded _P4User now exists
				Assert(adminClient.Users().Nodes.Any(n => n.User == _P4User));

				// Verify that the hardcoded _P4Client now exists
				Assert(adminClient.Clients().Nodes.Any(n => n.Client == _P4Client));

				// Ensure that the _P4User is a super user
				string protectSpec = String.Join("\n", new[]{
					$"Protections:",
					$"    super user {_P4User} * //...",
					$"    write user * * //...",
				});
				Assert(adminClient.Run(new DepotCommand{ Name="protect", Args=new[]{"-i"}, Input=protectSpec }).HasError == false);
			}

			// Delete the default user created for the empty database
			using (DepotClient depotClient = new DepotClient())
			{
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));
				Assert(depotClient.Run("user", new[]{"-d", "-f", defaultUser}).HasError == false);
			}

			// Populate the database with files!
			Dictionary<string, int> headRevisions = new Dictionary<string, int>();
			foreach (XmlElement changeElement in recipeDocument.SelectNodes("./server/change").OfType<XmlElement>())
			{
				string changeClient = changeElement.GetAttribute("client").NullIfEmpty() ?? _P4Client;
				string changeUser = changeElement.GetAttribute("user").NullIfEmpty() ?? _P4User;
				WorkspaceReset(_P4Port, changeClient, changeUser);
				
				using (DepotClient depotClient = new DepotClient()) {
				Assert(depotClient.Connect(_P4Port, changeClient, changeUser));
				string depotClientRoot = depotClient.ConnectionClient().Root;
				Assert(depotClientRoot?.StartsWith(serverRootFolder, StringComparison.InvariantCultureIgnoreCase) == true);

				string fstatSpec = changeElement.GetAttribute("fstat");
				if (String.IsNullOrEmpty(fstatSpec) == false)
				{
					string fstatDepotSpec = Regex.Replace(fstatSpec, @"([@#].*)?", "");
					string fstatDepotPath = GetFileSpecPath(fstatSpec);
					DepotResultWhere fstatPathWhere = depotClient.Where(fstatDepotPath);
					Assert(fstatPathWhere.Count == 1);
					string fstatClientPath = fstatPathWhere[0].LocalPath;
					Assert(fstatClientPath?.StartsWith(depotClientRoot, StringComparison.InvariantCultureIgnoreCase) == true);
					
					Dictionary<string, string> depotPathTypeMap = new Dictionary<string, string>(StringComparer.InvariantCultureIgnoreCase);
					XmlElement[] fileElements = changeElement.SelectNodes("./file").OfType<XmlElement>().ToArray();
					foreach (XmlElement fileElement in fileElements)
					{
						string fileType = fileElement.GetAttribute("type");
						Assert(String.IsNullOrEmpty(fileType) == false);
						Int64 fileSize = Converters.ToInt64(fileElement.GetAttribute("size"), -1).Value;
						Assert(fileSize >= 0);

						string depotFilePath = fileElement.GetAttribute("path");
						Assert(String.IsNullOrEmpty(depotFilePath) == false);
						DepotResultWhere depotFileWhere = depotClient.Where(depotFilePath);
						Assert(depotFileWhere.Count == 1);
						string clientFilePath = depotFileWhere[0].LocalPath;
						Assert(String.Compare(fstatClientPath, clientFilePath, StringComparison.InvariantCultureIgnoreCase) == 0 || clientFilePath.StartsWith(fstatClientPath+"\\", StringComparison.InvariantCultureIgnoreCase) == true);
						string symlinkTarget = fileElement.GetAttribute("target");
						depotPathTypeMap[depotFilePath] = fileType;

						int fileRev = Int32.Parse(fileElement.GetAttribute("rev"));
						if (headRevisions.TryGetValue(depotFilePath, out int lastFileRev))
						{
							Assert(lastFileRev <= fileRev, "recipe revisions must be in order");
							if (lastFileRev == fileRev)
							{
								Assert(depotClient.Run("sync", new[]{"-f", depotFilePath}).HasError == false);
								continue;
							}
						}

						headRevisions[depotFilePath] = fileRev;
						GenerateServerUnitTestFile(clientFilePath, fileSize, fileType, symlinkTarget);
					}

					// Flush this filespec to #head
					DepotResultView resultFlush = depotClient.Run("flush", new[]{ "-f", fstatDepotSpec });
					Assert(resultFlush.HasError == false || resultFlush.ErrorText.EndsWith(" - no such file(s)."));

					// Reconcile this filespec add/edit/delete
					DepotResultView resultReconcile = depotClient.Run("reconcile", new[]{ "-I", fstatDepotSpec });
					Assert(resultReconcile.HasError == false || resultReconcile.ErrorText.EndsWith(" - no file(s) to reconcile."));

					// Nothing to submit?
					DepotResultOpened resultOpened = depotClient.Opened();
					if (resultOpened.Count == 0)
					{
						continue;
					}

					// Ensure all correct fileType's
					foreach (DepotResultOpened.Node openedNode in resultOpened.Nodes)
					{
						if (openedNode.Action != "delete")
						{
							Assert(depotPathTypeMap.TryGetValue(openedNode.DepotFile, out string fileType));
							Assert(depotClient.Run("reopen", new[]{ "-t", fileType, openedNode.DepotFile }).HasError == false);
						}
					}

					// Submit all open files in this filespec
					string changeDescription = String.Format("change fstat={0}", fstatSpec);
					VirtualFileSystemLog.Info("Submitting to {0}: {1}", depotClient.Config().Port, changeDescription);
					Assert(depotClient.Run("submit", new[]{ "--parallel=0", "-d", changeDescription, fstatDepotSpec }).HasError == false);
				}}
			}
		}

		private void GenerateServerUnitTestFile(string filePath, long fileSize, string fileType, string symlinkTarget = null)
		{
			if (ServerText == null)
			{
				string serverTextFile = String.Format("{0}\\ServerText.txt", GetServerRootFolder());
				Assert(ExtractResourceToFile(Path.GetFileName(serverTextFile), serverTextFile));
				ServerText = File.ReadAllLines(serverTextFile);
			}
			if (ServerRandom == null)
			{
				ServerRandom = new RandomFast(291481);
			}

			FileUtilities.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(filePath)));
			FileUtilities.DeleteFile(filePath);

			string baseFileType = fileType.Split('+')[0];
			switch (baseFileType)
			{
				case "text":
				case "utf8":
				case "utf16":
				{
					Encoding encoding = baseFileType == "utf16" ? Encoding.Unicode : Encoding.UTF8;
					using (StreamWriter file = new StreamWriter(filePath, false, encoding))
					{
						file.Flush();
						file.AutoFlush = true;
						file.BaseStream.Position = 0;
						for (long position = 0; position < fileSize;)
						{
							uint lineIndex = ServerRandom.Next() % (uint)(ServerText.Length-1);
							file.WriteLine(ServerText[lineIndex]);
							long nextPosition = file.BaseStream.Position;
							Assert(nextPosition > position);
							position = nextPosition;
						}
						file.BaseStream.SetLength(fileSize);
					}
					break;
				}
				case "binary":
				{
					using (FileStream file = File.Create(filePath))
					{
						uint data = 0;
						for (long position = 0; position < fileSize; ++position)
						{
							if (data == 0)
							{
								data = ServerRandom.Next();
							}
							file.WriteByte((byte)(data & 0xFF));
							data >>= 8;
						}
					}
					break;
				}
				case "symlink":
				{
					Assert(String.IsNullOrEmpty(symlinkTarget) == false);
					Assert(CoreInterop.NativeMethods.CreateSymlinkFile(filePath, symlinkTarget) == 0);
					break;
				}
				default:
				{
					Assert(false, "unsupported file type");
					break;
				}
			}

			Assert(File.Exists(filePath));
			Assert((baseFileType == "symlink") != String.IsNullOrEmpty(symlinkTarget));
			Assert(baseFileType == "symlink" || FileUtilities.GetFileLength(filePath) == fileSize);
		}

		[TestMethod, Priority(1), TestExplicit]
		public void GenerateServerHistoryTest()
		{
			// Private settings for recipe history generation. Enter before running once.
			string recipeUser = "";
			string recipePort = "";
			string recipeClient = "";
			int recipeMaxPath = 135;

			Assert(!String.IsNullOrEmpty(recipeUser));
			Assert(!String.IsNullOrEmpty(recipePort));
			Assert(!String.IsNullOrEmpty(recipeClient));

			ServerWorkspaceReset();
			
			string serverRootFolder = GetServerRootFolder();
			XmlDocument recipeDocument = LoadServerRecipeXmlDocument();

			XmlDocument historyDocument = new XmlDocument();
			XmlComment historyComment = historyDocument.AppendChild(historyDocument.CreateComment("")) as XmlComment;
			XmlElement serverElement = historyDocument.AppendChild(historyDocument.CreateElement(recipeDocument.DocumentElement.Name)) as XmlElement;

			string commentBorder = "".PadRight(73, '=');
			historyComment.Value = String.Format("\n{0}\n", String.Join("\n", new[]{
				String.Format("  {0}", commentBorder),
				String.Format("  Do not edit this file! It is auto-generated by {0}.", nameof(GenerateServerHistoryTest)),
				String.Format("  This file only contains unmutable changlist history and should probably"),
				String.Format("  never need to change. Any new content should be added to ServerRecipe.xml"),
				String.Format("  {0}", commentBorder),
			}));

			Dictionary<string, int> headRevisions = new Dictionary<string, int>();
			using (DepotClient depotClient = new DepotClient()) 
			{
				Assert(depotClient.Connect(recipePort, recipeClient, recipeUser));
				foreach (XmlElement recipeChangeElement in recipeDocument.SelectNodes("./server/change").OfType<XmlElement>())
				{
					if (recipeChangeElement.ChildNodes.Count > 0)
					{
						continue;
					}

					XmlElement changeElement = serverElement.AppendChild(historyDocument.CreateElement(recipeChangeElement.Name)) as XmlElement;
					foreach (XmlAttribute changeAttribute in recipeChangeElement.Attributes)
					{
						changeElement.SetAttribute(changeAttribute.Name, changeAttribute.Value);
					}

					string fstatSpec = changeElement.GetAttribute("fstat");
					if (String.IsNullOrEmpty(fstatSpec) == false)
					{
						DepotResultFStat fstat = depotClient.FStat(new[]{ fstatSpec }, optionArgs:new[]{"-Ol"});
						Assert(fstat.Count > 0);
						foreach (DepotResultFStat.Node node in fstat.Nodes)
						{
							string filePath = node.DepotFile;
							if (filePath.Length > recipeMaxPath)
							{
								VirtualFileSystemLog.Warning("Skipping long path: {0}", filePath);
								continue;
							}

							int fileRev = node.HeadRev;
							if (headRevisions.TryGetValue(filePath, out int lastFileRev))
							{
								Assert(lastFileRev <= fileRev, "history revisions must be in order");
							}

							headRevisions[filePath] = fileRev;
							if (node.ContainsKey(DepotResultFStat.FieldName.FileSize) == false)
							{
								Assert(node.HeadAction.Contains("delete"));
								continue;
							}

							string fileType = changeElement.GetAttribute("filetype").NullIfEmpty() ?? node.HeadType;
							string fileSize = node.FileSize.ToString();

							XmlElement fileElement = changeElement.AppendChild(historyDocument.CreateElement("file")) as XmlElement;
							fileElement.SetAttribute("path", filePath);
							fileElement.SetAttribute("size", fileSize);
							fileElement.SetAttribute("type", fileType);
							fileElement.SetAttribute("rev", fileRev.ToString());
							
							if (fileType == "symlink")
							{
								string symlinkTargetPath = DepotOperations.GetSymlinkTargetPath(depotClient, String.Format("{0}#{1}", filePath, fileRev), null);
								Assert(String.IsNullOrEmpty(symlinkTargetPath) == false);
								fileElement.SetAttribute("target", symlinkTargetPath);
							}
						}
						continue;
					}
					Assert(false);
				}
			}

			string historyFile = String.Format("{0}\\ServerHistory.xml", serverRootFolder);
			using (XmlTextWriter historyWriter = new XmlTextWriter(historyFile, Encoding.UTF8))
			{
				historyWriter.Indentation = 2;
				historyWriter.Formatting = Formatting.Indented;
				historyDocument.Save(historyWriter);
			}
			
			string resourcesFolder = String.Format("{0}\\Microsoft.P4VFS.UnitTest\\Resource", UnitTestInstall.GetSourceRootFolder());
			VirtualFileSystemLog.Info("Copying: {0} -> {1}", historyFile, resourcesFolder);
			FileUtilities.CopyFile(historyFile, resourcesFolder, overwrite:true);
		}

		public static string GetFileSpecPath(string fileSpec)
		{
			return Regex.Replace(fileSpec, @"(/\.\.\.)?([@#].*)?", "");
		}

		public static string GetServerRootFolder(string p4Port = null)
		{
			return String.Format("{0}\\{1}", UnitTestInstall.GetIntermediateRootFolder(), GetServerPortNumber(p4Port));
		}

		public static string GetServerClientRootFolder(string clientName, string p4Port = null)
		{
			return String.Format("{0}\\{1}", GetServerRootFolder(p4Port), clientName);
		}

		public static string GetServerDescription(string p4Port = null)
		{
			return String.Format("P4VFS_TEST_{0}", GetServerPortNumber(p4Port));
		}

		public static string GetServerP4dExe(string p4Port = null)
		{
			return String.Format("{0}\\p4d.exe", GetServerRootFolder(p4Port));
		}

		private static string[] ServerText
		{
			get; set;
		}

		private static RandomFast ServerRandom
		{
			get; set;
		}

		public static string DefaultP4Passwd
		{
			get { return "P@33w0rD"; }
		}

		public static uint DefaultP4PortNumber
		{
			get { return 1666; }
		}

		public static string GetUserP4Passwd(string username)
		{
			return LoadServerRecipeXmlDocument()
				.SelectNodes("./server/user")
				.OfType<XmlElement>()
				.Where(u => u.GetAttribute("name") == username)
				.Select(u => u.GetAttribute("password"))
				.FirstOrDefault() ?? DefaultP4Passwd;
		}

		public static string GetServerLoginSSOFilePath(string p4Port = null)
		{
			return String.Format("{0}\\ServerLoginSSO.bat", GetServerRootFolder(p4Port));
		}

		private static void WriteServerLoginSSOScript(string filePath, XmlDocument recipeDocument)
		{
			Assert(String.IsNullOrEmpty(filePath) == false);
			FileUtilities.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(filePath)));
			VirtualFileSystemLog.Info("Writing server SSO script: {0}", filePath);
			using (StreamWriter stream = new StreamWriter(filePath, false, Encoding.ASCII))
			{
				stream.WriteLine("@ECHO OFF");
				stream.WriteLine("SETLOCAL ENABLEDELAYEDEXPANSION");
				stream.WriteLine("SET P4USER=%1");
				stream.WriteLine("SET P4PASSWD=");
				stream.WriteLine("SET EXIT_CODE=1");
				stream.WriteLine("FOR /F \"tokens=*\" %%A in ('more') do (");
				stream.WriteLine("    SET P4PASSWD=%%A");
				stream.WriteLine("    IF NOT \"%P4PASSWD%\"==\"\" SET P4PASSWD=!P4PASSWD: =!");
				stream.WriteLine(")");
				foreach (XmlElement userElement in recipeDocument.SelectNodes("./server/user").OfType<XmlElement>())
				{
					string userName = userElement.GetAttribute("name");
					string password = userElement.GetAttribute("password").NullIfEmpty() ?? DefaultP4Passwd;
					stream.WriteLine(String.Format("IF \"%P4USER%\"==\"{0}\" IF \"%P4PASSWD%\"==\"{1}\" SET EXIT_CODE=0", userName, password));
				}
				stream.WriteLine("ECHO %~nx0 \"%P4USER%\" \"%P4PASSWD%\" [%EXIT_CODE%]");
				stream.WriteLine("EXIT /B %EXIT_CODE%");
			}
		}

		public static void CreateDuplicatePerforceServer(string targetPort)
		{
			ServerWorkspaceReset(targetPort);

			string sourceRootFolder = GetServerRootFolder();
			string sourceDatabaseFolder = String.Format("{0}\\db", sourceRootFolder);
			
			// Checkpoint the source server before we duplicate it
			string checkpointFile;
			List<string> depotNames;
			using (DepotClient depotClient = new DepotClient()) 
			{
				Assert(depotClient.Connect(_P4Port, _P4Client, _P4User));
				DepotResultDepots depots = depotClient.Depots();
				Assert(depots?.HasError == false);
				depotNames = depots.Nodes.Select(n => n.Name).ToList();
				Assert(depots.Count == 3);
				
				Assert(depotClient.Run("admin", new[]{"checkpoint"}).HasError == false);
				checkpointFile = Directory.GetFiles(sourceDatabaseFolder)
					.Select(n => Regex.Match(Path.GetFileName(n), @"^(?<name>checkpoint.(?<num>\d+))$"))
					.Where(m => m.Success)
					.OrderBy(m => -1*Int32.Parse(m.Groups["num"].Value))
					.Select(m => m.Groups["name"].Value)
					.FirstOrDefault();

				Assert(String.IsNullOrEmpty(checkpointFile) == false);
				Assert(File.Exists(String.Format("{0}\\{1}", sourceDatabaseFolder, checkpointFile)));
				Assert(File.Exists(String.Format("{0}\\{1}.md5", sourceDatabaseFolder, checkpointFile)));
			}

			bool targetUseSSL = targetPort.StartsWith("ssl:", StringComparison.InvariantCultureIgnoreCase);
			uint targetServerPortNumber = GetServerPortNumber(targetPort);
			Assert(targetServerPortNumber != DefaultP4PortNumber);
			string targetServerPort = String.Concat(targetUseSSL ? "ssl:" : "", targetServerPortNumber);

			string targetRootFolder = GetServerRootFolder(targetPort);
			string targetDatabaseFolder = String.Format("{0}\\db", targetRootFolder);
			string targetServerLogFile = String.Format("{0}\\p4d.log", targetRootFolder);
			string targetServerDescription = GetServerDescription(targetPort);

			Action<string> copyFileToTarget = (string sourceFilePath) => 
			{
				Assert(File.Exists(sourceFilePath));
				Assert(sourceFilePath.StartsWith(String.Format("{0}\\", sourceRootFolder), StringComparison.InvariantCultureIgnoreCase));
				string targetFilePath = Path.GetFullPath(String.Format("{0}\\{1}", targetRootFolder, sourceFilePath.Substring(sourceRootFolder.Length+1)));
				AssertLambda(() => Directory.CreateDirectory(Path.GetDirectoryName(targetFilePath)));
				AssertLambda(() => File.Copy(NativeMethods.GetFileExtendedPath(sourceFilePath), NativeMethods.GetFileExtendedPath(targetFilePath)));
				Assert(File.Exists(targetFilePath));
			};

			// Copy depot (non-metadata) files from source to target
			Directory.GetFiles(sourceRootFolder, "*", SearchOption.TopDirectoryOnly).ForEach(copyFileToTarget);
			foreach (string depotName in depotNames)
			{
				string sourceDepotDatabaseFolder = String.Format("{0}\\{1}", sourceDatabaseFolder, depotName);
				Assert(Directory.Exists(sourceDepotDatabaseFolder));
				Directory.GetFiles(sourceDepotDatabaseFolder, "*", SearchOption.AllDirectories).ForEach(copyFileToTarget);
			}

			copyFileToTarget(String.Format("{0}\\{1}", sourceDatabaseFolder, checkpointFile));
			copyFileToTarget(String.Format("{0}\\{1}.md5", sourceDatabaseFolder, checkpointFile));

			string targetP4dExe = String.Format("{0}\\{1}", targetRootFolder, Path.GetFileName(GetServerP4dExe()));
			Assert(File.Exists(targetP4dExe));
			string targetServerArgs = String.Format("-L \"{0}\" -r \"{1}\" -p {2} -Id {3} -J off", targetServerLogFile, targetDatabaseFolder, targetServerPort, targetServerDescription);
			Assert(ProcessInfo.ExecuteWait(targetP4dExe, String.Format("{0} -jr {1}", targetServerArgs, checkpointFile), echo: true) == 0);

			Dictionary<string,string> targetEnvironment = new Dictionary<string, string>();
			if (targetUseSSL)
			{
				string targetP4SSLDIR = String.Format("{0}\\ssl", targetRootFolder);
				Directory.CreateDirectory(targetP4SSLDIR);
				targetEnvironment["P4SSLDIR"] = targetP4SSLDIR;

				// Generate a new self-signed SSL certificate and for TLS 1.2 or greater 
				Assert(ProcessInfo.ExecuteWait(new ProcessInfo.ExecuteParams{ FileName=targetP4dExe, Arguments=String.Format("{0} -Gc", targetServerArgs), LogCommand=true, Environment=targetEnvironment }).ExitCode == 0);
				Assert(ProcessInfo.ExecuteWait(targetP4dExe, String.Format("{0} -c \"set ssl.tls.version.min=12\"", targetServerArgs), echo: true) == 0);
			}
			
			ProcessStartInfo targetServerStartInfo = new ProcessStartInfo{ 
				FileName = targetP4dExe, 
				Arguments = targetServerArgs, 
				CreateNoWindow = true, 
				WindowStyle = ProcessWindowStyle.Hidden,
				UseShellExecute = false	};

			targetEnvironment.ForEach(kv => targetServerStartInfo.EnvironmentVariables[kv.Key] = kv.Value);

			// Start a the duplicate local perforce server on the target port
			Process targetServerProcess = Process.Start(targetServerStartInfo);
			Assert(targetServerProcess != null, String.Format("Failed to start duplicate server process ({0} {1})", targetP4dExe, targetServerArgs));
			Assert(!targetServerProcess.HasExited, "Duplicate server process quit prematuraly");
			AssertRetry(() => { var r = ProcessInfo.ExecuteWaitOutput(P4Exe, String.Format("-p {0} info", targetPort)); return r.ExitCode == 0 || r.Data.Any(s => s.Text.Contains("use the 'p4 trust' command")); });

			// Login and generate tickets for all users
			using (DepotClient depotClient = new DepotClient()) 
			{
				string password = GetUserP4Passwd(_P4User);
				Assert(String.IsNullOrEmpty(password) == false);
				Assert(depotClient.Connect(depotServer:targetPort, depotUser:_P4User, depotPasswd:password));

				DepotResultUsers users = depotClient.Users();
				Assert(users?.HasError == false);
				Assert(users.Nodes.Any(u => u.User == _P4User));
				foreach (string userName in users.Nodes.Select(u => u.User))
				{
					if (userName != _P4User)
					{
						// Super user ticket generation for each other user
						Assert(depotClient.Run("login", new[]{ userName }).HasError == false);
					}
				}
			}

			// Update clients to have unique root for this server
			using (DepotClient depotClient = new DepotClient()) 
			{
				Assert(depotClient.Connect(depotServer:targetPort, depotUser:_P4User));
				DepotResultClients clients = depotClient.Clients();
				Assert(clients?.HasError == false);
				Assert(clients.Nodes.Count() > 1);
				Assert(clients.Nodes.Any(n => n.Client == _P4Client));
				foreach (string clientName in clients.Nodes.Select(c => c.Client))
				{
					DepotResultClient.Node client = depotClient.Client(clientName);
					Assert(client?.Client == clientName);
					client[DepotResultClient.FieldName.Root] = GetServerClientRootFolder(client.Client, p4Port:targetPort);
					Assert(depotClient.UpdateClient(client).HasError == false);
				}
			}
		}

		[TestMethod, Priority(2)]
		public void CreateDuplicatePerforceServerTest()
		{
			foreach (bool duplicateUseSSL in new[]{ false, true })
			{
				string duplicatePort = String.Format("{0}localhost:2666", duplicateUseSSL ? "ssl:" : "");
				CreateDuplicatePerforceServer(duplicatePort);

				using (DepotClient depotClient = new DepotClient()) 
				{
					Assert(depotClient.Connect(depotServer:duplicatePort, depotUser:_P4User));
					Assert(depotClient.Run("info").HasError == false);
				}

				ServerWorkspaceReset(duplicatePort);
				Assert(Directory.Exists(GetServerRootFolder(duplicatePort)) == false);
			}
		}

		private static void ServerWorkspaceReset(string p4Port = null)
		{
			string serverRootFolder = GetServerRootFolder(p4Port);
			bool terminateAllServers = p4Port == null;

			AssertRetry(() => Process.GetProcessesByName(Path.GetFileNameWithoutExtension(GetServerP4dExe(p4Port)))
				.Where(p => { try { return terminateAllServers || p.MainModule.FileName.StartsWith(String.Format("{0}\\", serverRootFolder), StringComparison.InvariantCultureIgnoreCase); } catch {} return false; })
				.All(p => { try { p.Kill(); return true; } catch {} return false; }));

			AssertRetry(() => { try { FileUtilities.DeleteDirectoryAndFiles(serverRootFolder); return true; } catch {} return false; });
			AssertRetry(() => Directory.Exists(serverRootFolder) == false, String.Format("directory exists {0}", serverRootFolder));
		}

		public static string GetServerPortIPAddress(string p4Port = null)
		{
			string hostname = Regex.Replace(p4Port ?? "", "^(.*?):.*", "$1");
			if (String.IsNullOrEmpty(hostname))
			{
				hostname = System.Net.Dns.GetHostEntry(System.Net.Dns.GetHostName()).HostName;
			}
			if (String.Compare(hostname?.Split('.')[0], "localhost") == 0)
			{
				hostname = Environment.MachineName;
			}

			System.Net.IPHostEntry host = System.Net.Dns.GetHostEntry(hostname);
			foreach (System.Net.IPAddress address in host.AddressList)
			{
				if (System.Net.IPAddress.IsLoopback(address) == false && address.AddressFamily == System.Net.Sockets.AddressFamily.InterNetwork)
				{
					return address.ToString();
				}
			}
			return null;
		}

		public static uint GetServerPortNumber(string p4Port = null)
		{
			Match m = Regex.Match(p4Port ?? "", @"(.*:)?(?<num>\d+)$");
			return m.Success ? uint.Parse(m.Groups["num"].Value) : DefaultP4PortNumber;
		}

		public static XmlDocument LoadServerRecipeXmlDocument()
		{
			return LoadResourceXmlDocument(String.Format("{0}\\ServerRecipe.xml", GetServerRootFolder()));
		}

		public static XmlDocument LoadServerHistoryXmlDocument()
		{
			return LoadResourceXmlDocument(String.Format("{0}\\ServerHistory.xml", GetServerRootFolder()));
		}

		private static XmlDocument LoadXmlDocument(string filePath)
		{
			XmlDocument document = new XmlDocument();
			using (XmlTextReader reader = new XmlTextReader(filePath))
			{
				reader.Namespaces = false;
				document.Load(reader);
			}
			return document;
		}

		private static XmlDocument LoadResourceXmlDocument(string filePath)
		{
			Assert(ExtractResourceToFile(Path.GetFileName(filePath), filePath));
			Assert(File.Exists(filePath));
			return LoadXmlDocument(filePath);
		}

		private void MergeHistoryXmlDocument(XmlDocument recipeDocument, XmlDocument historyDocument)
		{
			Dictionary<string, XmlElement> historyFStatElements = new Dictionary<string, XmlElement>();
			foreach (XmlElement historyChangeElement in historyDocument.SelectNodes("./server/change").OfType<XmlElement>().ToArray())
			{
				string fstat = historyChangeElement.GetAttribute("fstat");
				if (String.IsNullOrEmpty(fstat) == false)
				{
					historyFStatElements.Add(fstat, historyChangeElement);
				}
			}

			foreach (XmlElement recipeChangeElement in recipeDocument.SelectNodes("./server/change").OfType<XmlElement>().ToArray())
			{
				if (recipeChangeElement.ChildNodes.Count > 0)
				{
					continue;
				}

				string fstat = recipeChangeElement.GetAttribute("fstat");
				if (String.IsNullOrEmpty(fstat) == false)
				{
					Assert(historyFStatElements.TryGetValue(fstat, out XmlElement historyChangeElement), "Empty changelist in recipe document");
					Assert(historyChangeElement.ChildNodes.OfType<XmlElement>().Any(), "Empty changelist in history document");
					foreach (XmlElement historyElement in historyChangeElement.ChildNodes.OfType<XmlElement>())
					{
						recipeChangeElement.AppendChild(recipeDocument.ImportNode(historyElement, true));
					}
				}
			}
		}
  	}
}
