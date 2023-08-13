// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Linq;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using Microsoft.P4VFS.CoreInterop;
using Microsoft.P4VFS.Extensions.Linq;

namespace Microsoft.P4VFS.Extensions
{
	public static class DepotClientExtensions
	{
		public static DepotResultViewType Run<DepotResultViewType>(this DepotClient depotClient, string name, IEnumerable<string> args = null) where DepotResultViewType : IDepotResultView, new()
		{
			return depotClient.Run<DepotResultViewType>(new DepotCommand{ Name=name, Args=args?.ToArray() });
		}

		public static DepotResultView Run(this DepotClient depotClient, string name, IEnumerable<string> args = null)
		{
			return depotClient.Run<DepotResultView>(new DepotCommand{ Name=name, Args=args?.ToArray() });
		}

		public static DepotResultViewType Run<DepotResultViewType>(this DepotClient depotClient, DepotCommand command) where DepotResultViewType : IDepotResultView, new()
		{
			return depotClient.Run(command).ToView<DepotResultViewType>();
		}

		public static DepotSyncResult Sync(this DepotClient depotClient, string files, DepotRevision revision = null, DepotSyncFlags syncFlags = DepotSyncFlags.Normal, DepotSyncMethod syncMethod = DepotSyncMethod.Virtual, DepotFlushType flushType = DepotFlushType.Atomic, string syncResident = null)
		{
			return depotClient.Sync(new string[]{ files }, revision, syncFlags, syncMethod, flushType, syncResident);
		}

		public static DepotSyncResult Sync(this DepotClient depotClient, IEnumerable<string> files, DepotRevision revision = null, DepotSyncFlags syncFlags = DepotSyncFlags.Normal, DepotSyncMethod syncMethod = DepotSyncMethod.Virtual, DepotFlushType flushType = DepotFlushType.Atomic, string syncResident = null)
		{
			DepotSyncOptions syncOptions = new DepotSyncOptions();
			syncOptions.Files = files?.ToArray();
			syncOptions.Revision = revision?.ToString();
			syncOptions.SyncFlags = syncFlags;
			syncOptions.SyncMethod = syncMethod;
			syncOptions.SyncResident = syncResident;
			syncOptions.FlushType = flushType;

			return depotClient.Sync(syncOptions);
		}

		public static DepotSyncResult Sync(this DepotClient depotClient, DepotSyncOptions syncOptions)
		{
			return DepotOperations.Sync(depotClient, syncOptions);
		}

		public static bool Connect(this DepotClient client, string depotServer = null, string depotClient = null, string depotUser = null, string directoryPath = null, string depotPasswd = null, string host = null)
		{
			return client.Connect(new DepotConfig{ Port=depotServer, Client=depotClient, User=depotUser, Directory=directoryPath, Passwd=depotPasswd, Host=host });
		}

		public static DepotResultFiles Files(this DepotClient depotClient, IEnumerable<string> fileSpecs, bool includeDeleted = false)
		{
			if (!fileSpecs.IsEmpty())
			{
				List<string> filesArgs = new List<string>();
				if (!includeDeleted)
				{
					filesArgs.Add("-e");
				}

				filesArgs.AddRange(fileSpecs);
				return depotClient.Run<DepotResultFiles>("files", filesArgs);
			}
			return new DepotResultFiles();
		}

		public static DepotResultFStat FStat(this DepotClient depotClient, string fileSpec)
		{
			return depotClient.FStat(new string[]{ fileSpec });
		}

		public static DepotResultFStat FStat(this DepotClient depotClient, IEnumerable<string> files, string filterType = "", DepotResultFStat.FieldType fields = DepotResultFStat.FieldType.Default, IEnumerable<string> optionArgs = null)
		{
			if (files != null && files.Any())
			{
				IEnumerable<string> fieldNames = DepotResultFStat.FieldName.ToNames(fields);
				List<string> fstatArgs = new List<string>();

				if (optionArgs != null)
					fstatArgs.AddRange(optionArgs);
				if (fields.HasFlag(DepotResultFStat.FieldType.FileSize))
					fstatArgs.Add("-Ol");
				if (fieldNames.Any())
					fstatArgs.AddRange(new string[]{"-T", String.Join(",", fieldNames)});
				if (String.IsNullOrEmpty(filterType) == false)
					fstatArgs.AddRange(new string[]{"-F", filterType});

				fstatArgs.AddRange(files);
				return depotClient.Run<DepotResultFStat>("fstat", fstatArgs);
			}
			return new DepotResultFStat();
		}

		public static DepotResultWhere Where(this DepotClient depotClient, string fileSpec)
		{
			return depotClient.Where(new string[]{ fileSpec });
		}

		public static DepotResultWhere Where(this DepotClient depotClient, IEnumerable<string> fileSpecs)
		{
			if (fileSpecs.IsEmpty())
				return null;

			return depotClient.Run<DepotResultWhere>("where", fileSpecs);
		}

		public static DepotResultWhere.Node WhereFolder(this DepotClient depotClient, string folderPath)
		{
			if (String.IsNullOrEmpty(folderPath))
				return null;

			string folderSpec = Regex.IsMatch(folderPath, @"[\\/]\.\.\.$") ? folderPath : String.Format("{0}/...", folderPath);
			DepotResultWhere result = depotClient.Run<DepotResultWhere>("where", new string[]{ DepotOperations.NormalizePath(folderSpec) });
			if (result.HasError)
				return null;

			DepotResultWhere.Node node = result.First;
			node.Fields[DepotResultWhere.FieldName.LocalPath] = node.LocalPath?.TrimEnd('\\','.');
			node.Fields[DepotResultWhere.FieldName.DepotPath] = node.DepotPath?.TrimEnd('/','.');
			node.Fields[DepotResultWhere.FieldName.WorkspacePath] = node.WorkspacePath?.TrimEnd('/','.');
			return node;
		}

		public static DepotResultDiff2 Diff2(this DepotClient depotClient, string fileSpec1, string fileSpec2)
		{
			if (String.IsNullOrEmpty(fileSpec1) == false && String.IsNullOrEmpty(fileSpec2) == false && String.Compare(fileSpec1, fileSpec2) != 0)
			{
				string[] diff2Args = new string[]{ "-q", fileSpec1, fileSpec2 };
				return depotClient.Run<DepotResultDiff2>("diff2", diff2Args);
			}
			return new DepotResultDiff2();
		}

		public static Int64 GetDepotSize(this DepotClient depotClient, IEnumerable<string> files)
		{
			if (files.Any())
			{
				List<string> sizesArgs = new List<string>();
				sizesArgs.Add("-s");
				sizesArgs.AddRange(files);
				
				DepotResultView result = depotClient.Run<DepotResultView>("sizes", sizesArgs);
				if (result.Count > 0)
				{
					return result[0].GetValue("fileSize", -1);
				}
			}
			return -1;
		}

		public static DepotRevision GetHeadRevisionChangelist(this DepotClient depotClient)
		{
			return DepotRevision.FromString(DepotOperations.GetHeadRevisionChangelist(depotClient));
		}

		public static DepotResultInfo.Node Info(this DepotClient depotClient)
		{
			return depotClient.Run<DepotResultInfo>("info").First;
		}

		public static DepotResultClient.Node Client(this DepotClient depotClient, string clientName = null)
		{
			return depotClient.Run<DepotResultClient>("client", new string[]{"-o", clientName}.Where(s => !String.IsNullOrEmpty(s))).First;
		}

		public static DepotResultClient.Node ConnectionClient(this DepotClient depotClient)
		{
			return depotClient.Connection().ToView<DepotResultClient>().First;
		}

		public static DepotResult UpdateClient(this DepotClient depotClient, DepotResultClient.Node client)
		{
			string clientSpec = String.Join("\n", new[]{
				String.Format("Client:\t{0}", client.Client??""),
				String.Format("Owner:\t{0}", client.Owner??""),
				String.Format("Host:\t{0}", client.Host??""),
				String.Format("Root:\t{0}", client.Root??""),
				String.Format("Options:\t{0}", client.Options??""),
				String.Format("LineEnd:\t{0}", client.LineEnd??""),
				String.Format("Description:\n\t{0}", client.Description??""),
				String.Format("View:\n\t{0}", String.Join("\n\t", client.View))
			});
			return depotClient.Run(new DepotCommand{ Name="client", Args=new string[]{"-i"}, Input=clientSpec });
		}

		public static DepotResultClients Clients(this DepotClient depotClient)
		{
			return depotClient.Run<DepotResultClients>("clients");
		}

		public static DepotResultUsers Users(this DepotClient depotClient)
		{
			return depotClient.Run<DepotResultUsers>("users");
		}

		public static DepotResultOpened Opened(this DepotClient depotClient)
		{
			return depotClient.Run<DepotResultOpened>("opened");
		}

		public static DepotResultDepots Depots(this DepotClient depotClient)
		{
			return depotClient.Run<DepotResultDepots>("depots");
		}
	}
}
