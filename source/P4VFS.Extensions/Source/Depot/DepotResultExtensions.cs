// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Diagnostics;
using System.Linq;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using Microsoft.P4VFS.CoreInterop;

namespace Microsoft.P4VFS.Extensions
{
	public static class DepotResultExtensions
	{
		public static DepotResultViewType ToView<DepotResultViewType>(this DepotResult result) where DepotResultViewType : IDepotResultView, new()
		{
			return new DepotResultViewType{ Result=result };
		}

		public static DepotResultView ToView(this DepotResult result)
		{
			return new DepotResultView{ Result=result };
		}
	}

	public interface IDepotResultView
	{
		DepotResult Result 
		{ 
			get; 
			set; 
		}
	}

	public class DepotResultView<NodeType> : IDepotResultView where NodeType : DepotResultNode, new()
	{
		public DepotResult Result 
		{ 
			get; 
			set; 
		}

		public int Count
		{
			get { return Result != null ? Result.TagList.Count : 0; }
		}

		public NodeType this[int index]
		{
			get 
			{ 
				DepotResultTag tag = Result != null && index >= 0 && index < Result.TagList.Count ? Result.TagList[index] : null;
				return new NodeType{ Tag = tag }; 
			}
		}

		public NodeType First
		{
			get { return this[0]; }
		}

		public IEnumerable<NodeType> Nodes
		{
			get { return Result != null ? Result.TagList.Select(tag => new NodeType{ Tag = tag }) : new NodeType[0]; }
		}

		public bool HasError
		{
			get { return Result == null || Result.HasError; }
		}

		public string ErrorText
		{
			get { return Result?.GetError()?.Trim() ?? ""; }
		}
	}

	public class DepotResultView : DepotResultView<DepotResultNode>
	{
	}

	public class DepotResultNode
	{
		public DepotResultTag Tag
		{
			get; 
			set;
		}

		public bool TryGetValue<T>(string field, out T value)
		{
			if (Tag != null && Tag.TryGetValue(field, out string stringValue))
			{
				Utilities.Converters.Result<T> parsedValue = Utilities.Converters.ParseType<T>(stringValue);
				if (parsedValue.Success)
				{
					value = parsedValue.Value;
					return true;
				}
			}
			value = default(T);
			return false;
		}

		public T GetValue<T>(string field, T defaultValue = default(T))
		{
			if (TryGetValue<T>(field, out T value))
			{
				return value;
			}
			return defaultValue;
		}

		public string GetValue(string field, string defaultValue = null)
		{
			return GetValue<string>(field, defaultValue);
		}

		public void SetValue(string field, string value)
		{
			Tag?.SetValue(field, value);
		}

		public string this[string field]
		{
			get { return GetValue(field); }
			set { SetValue(field, value); }
		}

		public bool ContainsKey(string field)
		{
			return Tag?.ContainsKey(field) == true;
		}

		public void RemoveKey(string field)
		{
			Tag?.RemoveKey(field);
		}

		public IDictionary<string, string> Fields
		{
			get { return Tag?.Fields; }
		}

		public IEnumerable<T> GetMultiValue<T>(string field)
		{
			for (int index = 0;; ++index)
			{
				if (TryGetValue<T>(String.Format("{0}{1}", field, index), out T value) == false)
					break;
				yield return value;
			}
		}

		public static DepotResultNodeType Create<DepotResultNodeType>(DepotResultTag tag) where DepotResultNodeType : DepotResultNode, new()
		{
			return new DepotResultNodeType{ Tag = tag };
		}
	}

	public class DepotResultFStat : DepotResultView<DepotResultFStat.Node>
	{
		[Flags]
		public enum FieldType
		{
			Default			= 0,
			DepotFile		= 1<<0,
			ClientFile		= 1<<1,
			FileSize		= 1<<2,
			HaveRev			= 1<<3,
			HeadRev			= 1<<4,
			HeadType		= 1<<5,
		}

		public static class FieldName
		{
			public const string DepotFile	= "depotFile";
			public const string ClientFile  = "clientFile";
			public const string FileSize	= "fileSize";
			public const string HaveRev		= "haveRev";
			public const string HeadRev		= "headRev";
			public const string HeadType	= "headType";

			public static IEnumerable<string> ToNames(FieldType types)
			{
				List<string> names = new List<string>();
				if (types.HasFlag(FieldType.DepotFile))
					names.Add(DepotFile);
				if (types.HasFlag(FieldType.ClientFile))
					names.Add(ClientFile);
				if (types.HasFlag(FieldType.FileSize))
					names.Add(FileSize);
				if (types.HasFlag(FieldType.HaveRev))
					names.Add(HaveRev);
				if (types.HasFlag(FieldType.HeadRev))
					names.Add(HeadRev);
				if (types.HasFlag(FieldType.HeadType))
					names.Add(HeadType);
				return names;
			}
		}

		public class Node : DepotResultNode
		{
			public bool InDepot			{ get { return String.IsNullOrEmpty(DepotFile) == false && HeadAction != "delete"; } }
			public string ClientFile	{ get { return GetValue<string>(FieldName.ClientFile); } }
			public string DepotFile		{ get { return GetValue<string>(FieldName.DepotFile); } }
			public string MovedFile		{ get { return GetValue<string>("movedFile"); } }
			public string Path			{ get { return GetValue<string>("path"); } }
			public bool IsMapped		{ get { return Fields.ContainsKey("isMapped"); } }
			public bool Shelved			{ get { return Fields.ContainsKey("shelved"); } }
			public string HeadAction	{ get { return GetValue<string>("headAction"); } }
			public int HeadChange		{ get { return GetValue<int>("headChange"); } }
			public int HeadRev			{ get { return GetValue<int>(FieldName.HeadRev); } }
			public string HeadType		{ get { return GetValue<string>(FieldName.HeadType); } }
			public int HeadTime			{ get { return GetValue<int>("headTime"); } }
			public int HeadModTime		{ get { return GetValue<int>("headModTime"); } }
			public int MovedRev			{ get { return GetValue<int>("movedRev"); } }
			public int HaveRev			{ get { return GetValue<int>(FieldName.HaveRev); } }
			public string Desc			{ get { return GetValue<string>("desc"); } }
			public string Digest		{ get { return GetValue<string>("digest"); } }
			public long FileSize		{ get { return GetValue<long>(FieldName.FileSize); } }
			public string Action		{ get { return GetValue<string>("action"); } }
			public string Type			{ get { return GetValue<string>("type"); } }
			public string ActionOwner	{ get { return GetValue<string>("actionOwner"); } }
			public int Change			{ get { return GetValue<int>("change"); } }
			public string Resolved		{ get { return GetValue<string>("resolved"); } }
			public string Unresolved	{ get { return GetValue<string>("unresolved"); } }
			public bool OtherOpen		{ get { return Fields.ContainsKey("otherOpen"); } }
			public bool OtherLock		{ get { return Fields.ContainsKey("otherLock"); } }
			public bool OurLock			{ get { return Fields.ContainsKey("ourLock"); } }
		}
	}

	public class DepotResultWhere : DepotResultView<DepotResultWhere.Node>
	{
		public static class FieldName
		{
			public const string LocalPath		= "path";
			public const string DepotPath		= "depotFile";
			public const string WorkspacePath	= "clientFile";
		}

		public class Node : DepotResultNode
		{
			public string LocalPath			{ get { return GetValue<string>(FieldName.LocalPath); } }
			public string DepotPath			{ get { return GetValue<string>(FieldName.DepotPath); } }
			public string WorkspacePath		{ get { return GetValue<string>(FieldName.WorkspacePath); } }
		}
	}

	public class DepotResultClient : DepotResultView<DepotResultClient.Node>
	{
		public struct LineEnd
		{
			public const string Local	= "local";
			public const string Unix	= "unix";
			public const string Mac		= "mac";
			public const string Win		= "win";
			public const string Share	= "share";
		}

		public struct Options
		{
			public const string AllWrite	= "allwrite";
			public const string NoAllWrite	= "noallwrite";
			public const string Clobber		= "clobber";
			public const string NoClobber	= "noclobber";
			public const string Compress	= "compress";
			public const string NoCompress	= "nocompress";
			public const string Locked		= "locked";
			public const string UnLocked	= "unlocked";
			public const string ModTime		= "modtime";
			public const string NoModTime	= "nomodtime";
			public const string RmDir		= "rmdir";
			public const string NoRmDir		= "normdir";
		}

		public static class FieldName
		{
			public const string Access			= "Access";
			public const string Update			= "Update";
			public const string Root			= "Root";
			public const string Client			= "Client";
			public const string Owner			= "Owner";
			public const string Host			= "Host";
			public const string Description		= "Description";
			public const string Options			= "Options";
			public const string SubmitOptions	= "SubmitOptions";
			public const string LineEnd			= "LineEnd";
			public const string Stream			= "Stream";
			public const string StreamAtChange	= "StreamAtChange";
			public const string ServerID		= "ServerID";
			public const string View			= "View";
		}

		public class Node : DepotResultNode
		{
			public bool Exists					{ get { return ContainsKey(FieldName.Access); } }
			public string Access				{ get { return GetValue<string>(FieldName.Access); } }
			public string Update				{ get { return GetValue<string>(FieldName.Update); } }
			public string Root					{ get { return GetValue<string>(FieldName.Root); } }
			public string Client				{ get { return GetValue<string>(FieldName.Client); } }
			public string Owner					{ get { return GetValue<string>(FieldName.Owner); } }
			public string Host					{ get { return GetValue<string>(FieldName.Host); } }
			public string Description			{ get { return GetValue<string>(FieldName.Description); } }
			public string Options				{ get { return GetValue<string>(FieldName.Options); } }
			public string SubmitOptions			{ get { return GetValue<string>(FieldName.SubmitOptions); } }
			public string LineEnd				{ get { return GetValue<string>(FieldName.LineEnd); } }
			public string Stream				{ get { return GetValue<string>(FieldName.Stream); } }
			public string StreamAtChange		{ get { return GetValue<string>(FieldName.StreamAtChange); } }
			public string ServerID				{ get { return GetValue<string>(FieldName.ServerID); } }
			public IEnumerable<string> View		{ get { return GetMultiValue<string>(FieldName.View); } }

			public string[] OptionNames
			{
				get { return Options?.Split(new char[]{' '}, StringSplitOptions.RemoveEmptyEntries); }
			}
		}
	}

	public class DepotResultDiff2 : DepotResultView<DepotResultDiff2.Node>
	{
		public class Node : DepotResultNode
		{
			public string Status 		{ get { return GetValue<string>("status"); } }
			public string DepotFile 	{ get { return GetValue<string>("depotFile"); } }
			public int Rev 				{ get { return GetValue<int>("rev"); } }
			public string Type 			{ get { return GetValue<string>("type"); } }
			public string DepotFile2 	{ get { return GetValue<string>("depotFile2"); } }
			public int Rev2				{ get { return GetValue<int>("rev2"); } }
			public string Type2 		{ get { return GetValue<string>("type2"); } }
		}
	}

	public class DepotResultFiles : DepotResultView<DepotResultFiles.Node>
	{
		public class Node : DepotResultNode
		{
			public string DepotFile		{ get { return GetValue<string>("depotFile"); } }
			public int Rev				{ get { return GetValue<int>("rev"); } }
			public int Change			{ get { return GetValue<int>("change"); } }
			public string Action		{ get { return GetValue<string>("action"); } }
			public string Type			{ get { return GetValue<string>("type"); } }
		}
	}

	public class DepotResultInfo : DepotResultView<DepotResultInfo.Node>
	{
		public class Node : DepotResultNode
		{
			public string UserName			{ get { return GetValue<string>("userName"); } }
			public string ClientName		{ get { return GetValue<string>("clientName"); } }
			public string ClientRoot		{ get { return GetValue<string>("clientRoot"); } }
			public string ClientLock		{ get { return GetValue<string>("clientLock"); } }
			public string ClientCwd			{ get { return GetValue<string>("clientCwd"); } }
			public string ClientHost		{ get { return GetValue<string>("clientHost"); } }
			public string PeerAddress		{ get { return GetValue<string>("peerAddress"); } }
			public string ClientAddress		{ get { return GetValue<string>("clientAddress"); } }
			public string ServerName		{ get { return GetValue<string>("serverName"); } }
			public string ServerAddress		{ get { return GetValue<string>("serverAddress"); } }
			public string ServerRoot		{ get { return GetValue<string>("serverRoot"); } }
			public string ServerDate		{ get { return GetValue<string>("serverDate"); } }
			public string ServerUptime		{ get { return GetValue<string>("serverUptime"); } }
			public string ServerVersion		{ get { return GetValue<string>("serverVersion"); } }
			public string ServerServices	{ get { return GetValue<string>("serverServices"); } }
			public string ServerLicense		{ get { return GetValue<string>("serverLicense"); } }
			public string CaseHandling		{ get { return GetValue<string>("caseHandling"); } }
			public string BrokerAddress		{ get { return GetValue<string>("brokerAddress"); } }
			public string BrokerVersion		{ get { return GetValue<string>("brokerVersion"); } }
		}
	}

	public class DepotResultUser : DepotResultView<DepotResultUser.Node>
	{
		public class Node : DepotResultNode
		{
			public string User						{ get { return GetValue<string>("User"); } }
			public string Email						{ get { return GetValue<string>("Email"); } }
			public string Update					{ get { return GetValue<string>("Update"); } }
			public string Access					{ get { return GetValue<string>("Access"); } }
			public string FullName					{ get { return GetValue<string>("FullName"); } }
			public string Password					{ get { return GetValue<string>("Password"); } }
			public string Type						{ get { return GetValue<string>("Type"); } }
			public string AuthMethod				{ get { return GetValue<string>("AuthMethod"); } }
			public string PasswordChange			{ get { return GetValue<string>("passwordChange"); } }
			public IEnumerable<string> Reviews		{ get { return GetMultiValue<string>("Reviews"); } }
		}
	}

	public class DepotResultUsers : DepotResultView<DepotResultUser.Node>
	{
		public class Node : DepotResultUser.Node
		{
		}
	}

	public class DepotResultOpened : DepotResultView<DepotResultOpened.Node>
	{
		public class Node : DepotResultNode
		{
			public string DepotFile			{ get { return GetValue<string>("depotFile"); } }
			public string ClientFile		{ get { return GetValue<string>("clientFile"); } }
			public string Rev				{ get { return GetValue<string>("rev"); } }
			public string HaveRev			{ get { return GetValue<string>("haveRev"); } }
			public string Action			{ get { return GetValue<string>("action"); } }
			public string Change			{ get { return GetValue<string>("change"); } }
			public string Type				{ get { return GetValue<string>("type"); } }
			public string User				{ get { return GetValue<string>("user"); } }
			public string Client			{ get { return GetValue<string>("client"); } }
		}
	}

	public class DepotResultDepots : DepotResultView<DepotResultDepots.Node>
	{
		public class Node : DepotResultNode
		{
			public string Name				{ get { return GetValue<string>("name"); } }
			public string Time				{ get { return GetValue<string>("time"); } }
			public string Type				{ get { return GetValue<string>("type"); } }
			public string Map				{ get { return GetValue<string>("map"); } }
			public string Desc				{ get { return GetValue<string>("desc"); } }
		}
	}

	public class DepotResultClients : DepotResultView<DepotResultClients.Node>
	{
		public class Node : DepotResultClient.Node
		{
		}
	}
}
