// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Linq;
using System.Text;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Threading.Tasks;
using System.Collections.Generic;
using Microsoft.P4VFS.Extensions.Utilities;
using Microsoft.P4VFS.CoreInterop;
using System.Runtime.InteropServices;
using Newtonsoft.Json;
using Newtonsoft.Json.Converters;

namespace Microsoft.P4VFS.Extensions.SocketModel
{
	public class SocketModelServer
	{
		private const int _PortNumber = 46982;
		private CancellationTokenSource _Cancellation;
		private TcpListener _TcpListener;
		private Thread _ListenThread;

		public bool Initialize()
		{
			try
			{
				_Cancellation = new CancellationTokenSource();
				_TcpListener = new TcpListener(SocketModelServer.EndPoint);
				_ListenThread = new Thread(new ThreadStart(ListenForClients));
				_ListenThread.IsBackground = true;
				_ListenThread.Start();
				VirtualFileSystemLog.Info("SocketModelServer initialized");
				return true;
			}
			catch (Exception e)
			{
				VirtualFileSystemLog.Error("SocketModelServer.Initialize exception: {0}", e.Message);
				Shutdown();
			}
			return false;
		}

		public void Shutdown()
		{
			try
			{
				if (_Cancellation != null)
					_Cancellation.Cancel();

				if (_TcpListener != null)
				{
					_TcpListener.Server.Dispose();
					_TcpListener = null;
				}
			}
			catch (Exception e)
			{
				VirtualFileSystemLog.Error("SocketModelServer.Shutdown _TcpListener exception: {0}", e.Message);
			}

			try
			{
				if (_ListenThread != null)
				{
					if (_ListenThread.IsAlive)
						_ListenThread.Join(500);
					_ListenThread = null;
				}
			}
			catch (Exception e)
			{
				VirtualFileSystemLog.Error("SocketModelServer.Shutdown _ListenThread exception: {0}", e.Message);
			}
		}

		public static IPEndPoint EndPoint
		{
			get { return new IPEndPoint(IPAddress.Loopback, _PortNumber); }
		}

		private void ListenForClients()
		{
			try
			{
				_TcpListener.Start();
				while (_Cancellation.IsCancellationRequested == false)
				{
					TcpClient client = _TcpListener.AcceptTcpClient();
					Thread clientThread = new Thread(new ParameterizedThreadStart(HandleClientConnection));
					clientThread.Start(client);
				}
			}
			catch (Exception e)
			{
				if (_Cancellation.IsCancellationRequested == false)
					VirtualFileSystemLog.Error("SocketModelServer.ListenForClients exiting with exception: {0}", e.Message);
			}
		}

		private void HandleClientConnection(object param)
		{
			try
			{
				VirtualFileSystemLog.Verbose("SocketModelServer.HandleClientConnection recieving incoming connection");
				using (TcpClient client = (TcpClient)param)
				{
					using (NetworkStream stream = client.GetStream())
					{
						SocketModelMessage msg = SocketModelProtocol.ReceiveMessage<SocketModelMessage>(stream);
						if (msg == null)
						{
							VirtualFileSystemLog.Info("SocketModelServer.HandleClientConnection exiting from null message");
							return;
						}

						VirtualFileSystemLog.Verbose("SocketModelServer.HandleClientMessage recieved message Type: {0}", msg.Type);
						HandleSocketModelMessage(stream, msg);
					}
				}
			}
			catch (Exception e)
			{
				VirtualFileSystemLog.Error("SocketModelServer.HandleClientConnection exiting with exception: {0}", e.Message);
			}
		}

		private void HandleSocketModelMessage(NetworkStream stream, SocketModelMessage msg)
		{
			if (msg.Type == typeof(SocketModelRequestSync).Name)
			{
				SocketModelRequestSync request = msg.GetData<SocketModelRequestSync>();
				SocketModelReplySync reply = Sync(stream, request);
				SocketModelProtocol.SendMessage(stream, new SocketModelMessage(reply));
			}
			else if (msg.Type == typeof(SocketModelRequestServiceStatus).Name)
			{
				SocketModelReplyServiceStatus reply = new SocketModelReplyServiceStatus();
				if (VirtualFileSystem.ServiceHost != null)
				{
					reply.IsDriverConnected = VirtualFileSystem.ServiceHost.IsDriverConnected();
					reply.LastModifiedTime = VirtualFileSystem.ServiceHost.GetLastModifiedTime();
					reply.LastRequestTime = VirtualFileSystem.ServiceHost.GetLastRequestTime();
				}
				SocketModelProtocol.SendMessage(stream, new SocketModelMessage(reply));
			}
			else if (msg.Type == typeof(SocketModelRequestSetServiceSetting).Name)
			{
				SocketModelRequestSetServiceSetting request = msg.GetData<SocketModelRequestSetServiceSetting>();
				SocketModelReply reply = new SocketModelReply() { Success = ServiceSettings.SetNode(request.Value, request.Name) };
				SocketModelProtocol.SendMessage(stream, new SocketModelMessage(reply));
			}
			else if (msg.Type == typeof(SocketModelRequestGetServiceSetting).Name)
			{
				SocketModelRequestGetServiceSetting request = msg.GetData<SocketModelRequestGetServiceSetting>();
				SocketModelReplyGetServiceSetting reply = new SocketModelReplyGetServiceSetting() { Value = ServiceSettings.GetNode(request.Name) };
				SocketModelProtocol.SendMessage(stream, new SocketModelMessage(reply));
			}
			else if (msg.Type == typeof(SocketModelRequestGetServiceSettings).Name)
			{
				SocketModelReplyGetServiceSettings reply = new SocketModelReplyGetServiceSettings() { Settings = ServiceSettings.GetMap() };
				SocketModelProtocol.SendMessage(stream, new SocketModelMessage(reply));
			}
			else if (msg.Type == typeof(SocketModelRequestGarbageCollect).Name)
			{
				SocketModelRequestGarbageCollect request = msg.GetData<SocketModelRequestGarbageCollect>();
				SocketModelReply reply = new SocketModelReply() { Success = VirtualFileSystem.ServiceHost != null ? VirtualFileSystem.ServiceHost.GarbageCollect(request.Timeout) : false };
				SocketModelProtocol.SendMessage(stream, new SocketModelMessage(reply));
			}
			else
			{
				VirtualFileSystemLog.Error("SocketModelServer.HandleSocketModelMessage unexpected message Type: {0}", msg.Type);
			}
		}

		private SocketModelReplySync Sync(NetworkStream stream, SocketModelRequestSync request)
		{
			using (var impersonate = new Impersonation.ImpersonateLoggedOnUserScope(request.SyncOptions.Context))
			{
				LogDevice log = new SocketModelLogDevice(stream, _Cancellation.Token);
				if (request.SyncOptions.SyncType.HasFlag(DepotSyncType.Quiet) == false)
				{
					LogDeviceAggregate aggregateLog = new LogDeviceAggregate();
					aggregateLog.Devices.Add(log);
					aggregateLog.Devices.Add(VirtualFileSystemLog.Instance);
					log = aggregateLog;
				}

				FileContext context = new FileContext{ LogDevice=log, UserContext=request.SyncOptions.Context };
				using (DepotClient depotClient = new DepotClient(context))
				{
					if (depotClient.Connect(request.Config) == false)
					{
						log.Error("SocketModelServer.Sync [managed] failed to connect to perforce");
						return new SocketModelReplySync{ Status = DepotSyncStatus.Error };
					}

					DepotSyncResult result = VirtualFileSystem.Sync(depotClient, request.SyncOptions);
					DepotSyncStatus status = result != null ? result.Status : DepotSyncStatus.Success;
					return new SocketModelReplySync{ Status = status };
				}
			}
		}
	}

	public class SocketModelLogDevice : LogDevice
	{
		private bool _HasError = false;
		private object _StreamMutex = new object();
		private NetworkStream _NetworkStream;
		private CancellationToken _CancelationToken;

		public SocketModelLogDevice(NetworkStream networkStream, CancellationToken token)
		{
			_NetworkStream = networkStream;
			_CancelationToken = token;
		}

		public override bool IsFaulted()
		{
			return _HasError || _CancelationToken.IsCancellationRequested;
		}

		public override void Write(LogElement element)
		{
			try 
			{ 
				if (IsFaulted() == false)
				{
					SocketModelMessage msg = new SocketModelMessage(new SocketModelRequestLog(){ Element = element });
					lock (_StreamMutex)
					{
						SocketModelProtocol.SendMessage(_NetworkStream, msg);
					}
				}	
			}
			catch (Exception e)
			{ 
				VirtualFileSystemLog.Error("SocketModelLogDevice.WriteDevice exception: {0}", e.Message);
				_HasError = true; 
			}
		}
	}

	public class SocketModelClient
	{ 
		public DepotSyncStatus Sync(DepotConfig config, DepotSyncOptions syncOptions)
		{
			List<SocketModelMessage> result = new List<SocketModelMessage>();
			if (SendCommand(new SocketModelRequestSync{ Config=config, SyncOptions=syncOptions }, result) == false)
				return DepotSyncStatus.Error;
			SocketModelReplySync reply = SocketModelMessage.GetData<SocketModelReplySync>(result);
			return reply != null ? reply.Status : DepotSyncStatus.Error;
		}

		public SocketModelReplyServiceStatus GetServiceStatus()
		{
			List<SocketModelMessage> result = new List<SocketModelMessage>();
			if (SendCommand(new SocketModelRequestServiceStatus(), result) == false)
				return null;
			return SocketModelMessage.GetData<SocketModelReplyServiceStatus>(result);
		}

		public SettingNodeMap GetServiceSettings()
		{
			List<SocketModelMessage> result = new List<SocketModelMessage>();
			if (SendCommand(new SocketModelRequestGetServiceSettings(), result) == false)
				return null;
			SocketModelReplyGetServiceSettings reply = SocketModelMessage.GetData<SocketModelReplyGetServiceSettings>(result);
			return reply?.Settings;
		}

		public bool SetServiceSetting(string name, SettingNode value)
		{
			List<SocketModelMessage> result = new List<SocketModelMessage>();
			if (SendCommand(new SocketModelRequestSetServiceSetting{ Name=name, Value=value }, result) == false)
				return false;
			SocketModelReply reply = SocketModelMessage.GetData<SocketModelReply>(result);
			return reply != null && reply.Success;
		}

		public SettingNode GetServiceSetting(string name)
		{
			List<SocketModelMessage> result = new List<SocketModelMessage>();
			if (SendCommand(new SocketModelRequestGetServiceSetting{ Name=name }, result) == false)
				return null;
			SocketModelReplyGetServiceSetting reply = SocketModelMessage.GetData<SocketModelReplyGetServiceSetting>(result);
			return reply?.Value;
		}

		public bool GarbageCollect(Int64 timeout = 0)
		{
			List<SocketModelMessage> result = new List<SocketModelMessage>();
			if (SendCommand(new SocketModelRequestGarbageCollect(){ Timeout = timeout }, result) == false)
				return false;
			SocketModelReply reply = SocketModelMessage.GetData<SocketModelReply>(result);
			return reply != null && reply.Success;
		}

		private bool SendCommand<CommandType>(CommandType cmd, List<SocketModelMessage> result = null)
		{
			try
			{
				using (TcpClient client = new TcpClient())
				{
					client.Connect(SocketModelServer.EndPoint);
					using (NetworkStream stream = client.GetStream())
					{
						SocketModelMessage request = new SocketModelMessage(cmd);
						SocketModelProtocol.SendMessage(stream, request);
						while (client.Connected)
						{
							SocketModelMessage msg = SocketModelProtocol.ReceiveMessage<SocketModelMessage>(stream);
							if (msg == null)
								break;
							if (result != null)
								result.Add(msg);

							HandleSocketModelMessage(stream, msg);
						}
					}
				}
			}
			catch (Exception e)
			{
				VirtualFileSystemLog.Error("SocketModelClient.SendCommand exit with exception: {0}", e.Message);
				return false;
			}
			return true;
		}

		private void HandleSocketModelMessage(NetworkStream stream, SocketModelMessage msg)
		{
			if (msg.Type == typeof(SocketModelRequestLog).Name)
			{
				SocketModelRequestLog options = msg.GetData<SocketModelRequestLog>();
				VirtualFileSystemLog.Instance.Write(options.Element);
			}
		}
	}

	public static class SocketModelProtocol
	{
		private static readonly JsonSerializerSettings _Settings;
		private static readonly Encoding _Encoding;
		private const UInt32 _MagicNumber = 0xD3E142A4;
		private const UInt32 _Version = 1;

		static SocketModelProtocol()
		{
			_Settings = new JsonSerializerSettings();
			_Settings.DefaultValueHandling = DefaultValueHandling.Include;
			_Settings.Converters.Add(new StringEnumConverter());
			_Settings.TypeNameHandling = TypeNameHandling.Auto;
			_Encoding = new UTF8Encoding(false, true);
		}

		public static void SendMessage<MessageType>(NetworkStream stream, MessageType msg)
		{
			SendMessageAsync(stream, msg).Wait();
		}

		public static async Task SendMessageAsync<MessageType>(NetworkStream stream, MessageType msg)
		{
			string content = Serialize(msg);
			if (content == null)
				throw new Exception("Failed to serialize message");
							
			byte[] contentBytes = _Encoding.GetBytes(content);
			
			Header header = new Header();
			header.MagicNumber = _MagicNumber;
			header.Version = _Version;
			header.ContentLength = (UInt32)contentBytes.Length;
			byte[] headerBytes = StructToBytes(header);

			byte[] packetBytes = new byte[contentBytes.Length + headerBytes.Length];
			Array.Copy(headerBytes, 0, packetBytes, 0, headerBytes.Length);
			Array.Copy(contentBytes, 0, packetBytes, headerBytes.Length, contentBytes.Length);

			await stream.WriteAsync(packetBytes, 0, packetBytes.Length);
		}

		public static MessageType ReceiveMessage<MessageType>(NetworkStream stream)
		{
			Task<MessageType> msg = ReceiveMessageAsync<MessageType>(stream);
			msg.Wait();
			return msg.Result;
		}

		public static async Task<MessageType> ReceiveMessageAsync<MessageType>(NetworkStream stream)
		{
			int headerSize = Marshal.SizeOf(typeof(Header));
			byte[] headerBytes = new byte[headerSize];
			int headerRead = await stream.ReadAsync(headerBytes, 0, headerBytes.Length);
			if (headerRead == 0)
				return default(MessageType);
			if (headerRead != headerBytes.Length)
				throw new Exception("Failed to read header bytes");

			Header header = BytesToStruct<Header>(headerBytes);
			if (header.MagicNumber != _MagicNumber)
				throw new Exception(String.Format("Invalid header format 0x{0:X8} expected 0x{1:X8}", header.MagicNumber, _MagicNumber));
			if (header.Version != _Version)
				throw new Exception(String.Format("Invalid header version {0} expected {1}", header.Version, _Version));

			byte[] contentBytes = new byte[header.ContentLength];
			if (await stream.ReadAsync(contentBytes, 0, contentBytes.Length) != contentBytes.Length)
				throw new Exception("Failed to read content bytes");
				
			string contentString = _Encoding.GetString(contentBytes);
			return Deserialize<MessageType>(contentString);
		}

		public static string Serialize(object obj)
		{
			return JsonConvert.SerializeObject(obj, _Settings);
		}

		public static T Deserialize<T>(string json)
		{
			return JsonConvert.DeserializeObject<T>(json, _Settings);
		}

		public static byte[] StructToBytes<T>(T value) where T : struct
		{
			int valueSize = Marshal.SizeOf(typeof(T));
			byte[] valueBytes = new byte[valueSize];
			IntPtr valuePtr = Marshal.AllocHGlobal(valueSize);
			try
			{
				Marshal.StructureToPtr(value, valuePtr, false);
				Marshal.Copy(valuePtr, valueBytes, 0, valueSize);
			}
			finally
			{
				Marshal.FreeHGlobal(valuePtr);
			}
			return valueBytes;			
		}

		public static T BytesToStruct<T>(byte[] valueBytes) where T : struct
		{
			int valueSize = Marshal.SizeOf(typeof(T));
			if (valueBytes.Length < valueBytes.Length)
				throw new Exception("Invalid buffer size");

			T value = default(T);
			IntPtr valuePtr = Marshal.AllocHGlobal(valueSize);
			try
			{
				Marshal.Copy(valueBytes, 0, valuePtr, valueSize);
				value = (T)Marshal.PtrToStructure(valuePtr, typeof(T));
			}
			finally
			{
				Marshal.FreeHGlobal(valuePtr);
			}
			return value;
		}

		[StructLayout(LayoutKind.Sequential, Pack=1)]
		private struct Header
		{
			public UInt32 MagicNumber;
			public UInt32 Version;
			public UInt32 ContentLength;
		}
	}

	public class SocketModelMessage
	{
		public string Type;
		public string Data;

		public SocketModelMessage()
		{
		}

		public SocketModelMessage(object contents)
		{
			Type = contents.GetType().Name;
			Data = SocketModelProtocol.Serialize(contents);
		}

		public T GetData<T>()
		{
			return SocketModelProtocol.Deserialize<T>(Data);
		}

		public static T GetData<T>(IEnumerable<SocketModelMessage> messages) where T : class
		{
			return messages.FirstOrDefault(m => m.Type == typeof(T).Name)?.GetData<T>();
		}
	}

	public class SocketModelReply
	{
		public bool Success;
	}

	public class SocketModelRequestSync
	{
		public DepotConfig Config;
		public DepotSyncOptions SyncOptions;
	}

	public class SocketModelReplySync
	{
		public DepotSyncStatus Status;
	}

	public class SocketModelRequestLog
	{
		public LogElement Element;
	}

	public class SocketModelRequestServiceStatus
	{
	}

	public class SocketModelReplyServiceStatus
	{
		public bool IsDriverConnected;
		public DateTime LastRequestTime;
		public DateTime LastModifiedTime;
	}

	public class SocketModelRequestSetServiceSetting
	{
		public string Name;
		public SettingNode Value;
	}

	public class SocketModelRequestGetServiceSetting
	{
		public string Name;
	}

	public class SocketModelReplyGetServiceSetting
	{
		public SettingNode Value;
	}

	public class SocketModelRequestGetServiceSettings
	{
	}

	public class SocketModelReplyGetServiceSettings
	{
		public SettingNodeMap Settings;
	}

	public class SocketModelRequestGarbageCollect
	{
		public Int64 Timeout;
	}
}
