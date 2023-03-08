// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Text;
using System.Linq;
using System.Collections.Generic;
using Microsoft.P4VFS.CoreInterop;

namespace Microsoft.P4VFS.Extensions
{
	public static class LogDeviceOperations
	{
		public static void Verbose(this LogDevice log, string text)
		{
			log.WriteLine(LogChannel.Verbose, text);
		}

		public static void Verbose(this LogDevice log, string format, params object[] args)
		{
			log.WriteLine(LogChannel.Verbose, format, args);
		}

		public static void Info(this LogDevice log, string text)
		{
			log.WriteLine(LogChannel.Info, text);
		}

		public static void Info(this LogDevice log, string format, params object[] args)
		{
			log.WriteLine(LogChannel.Info, format, args);
		}

		public static void Warning(this LogDevice log, string text)
		{
			log.WriteLine(LogChannel.Warning, text);
		}

		public static void Warning(this LogDevice log, string format, params object[] args)
		{
			log.WriteLine(LogChannel.Warning, format, args);
		}

		public static void Error(this LogDevice log, string text)
		{
			log.WriteLine(LogChannel.Error, text);
		}

		public static void Error(this LogDevice log, string format, params object[] args)
		{
			log.WriteLine(LogChannel.Error, format, args);
		}

		public static void WriteLine(this LogDevice log, LogChannel channel, string format, params object[] args)
		{
			log.WriteLine(channel, String.Format(format??"", args));
		}

		public static void WriteLine(this LogDevice log, LogChannel channel, string text)
		{
			log.Write(new LogElement(){ m_Channel=channel, m_Text=(text??"")+"\n", m_Time=LogElement.Now() });
		}
	}

	public class LogDeviceMemory : LogDevice
	{
		private List<LogElement> _Elements = new List<LogElement>(512);

		public override void Write(LogElement element)
		{
			lock (_Elements)
			{
				_Elements.Add(element);
			}
		}

		public override bool IsFaulted()
		{
			return false;
		}

		public void FlushTo(LogDevice log)
		{
			if (log != null)
			{
				_Elements.ForEach(element => log.Write(element));
			}
		}
	}

	public class LogDeviceAggregate : LogDevice
	{
		private List<LogDevice> _Devices = new List<LogDevice>();

		public IList<LogDevice> Devices
		{
			get { return _Devices; }
		}

		public override bool IsFaulted()
		{
			return _Devices.Any(device => device.IsFaulted());
		}

		public override void Write(LogElement element)
		{
			foreach (LogDevice device in _Devices)
			{
				device?.Write(element);
			}
		}
	}

	public class VirtualFileSystemLog : LogDevice
	{
		private static VirtualFileSystemLog _Instance;

		public static VirtualFileSystemLog Instance
		{
			get 
			{ 
				if (_Instance == null)
					_Instance = new VirtualFileSystemLog();
				return _Instance; 
			}
		}

		public override bool IsFaulted()
		{
			return false;
		}

		public override void Write(LogElement element)
		{
			LogSystem.Write(element);
		}

		public static void Verbose(string text)
		{
			LogDeviceOperations.Verbose(Instance, text);
		}

		public static void Verbose(string format, params object[] args)
		{
			LogDeviceOperations.Verbose(Instance, format, args);
		}

		public static void Info(string text)
		{
			LogDeviceOperations.Info(Instance, text);
		}

		public static void Info(string format, params object[] args)
		{
			LogDeviceOperations.Info(Instance, format, args);
		}

		public static void Warning(string text)
		{
			LogDeviceOperations.Warning(Instance, text);
		}

		public static void Warning(string format, params object[] args)
		{
			LogDeviceOperations.Warning(Instance, format, args);
		}

		public static void Error(string text)
		{
			LogDeviceOperations.Error(Instance, text);
		}

		public static void Error(string format, params object[] args)
		{
			LogDeviceOperations.Error(Instance, format, args);
		}

		public static void WriteLine(LogChannel channel, string format, params object[] args)
		{
			LogDeviceOperations.WriteLine(Instance, channel, format, args);
		}

		public static void WriteLine(LogChannel channel, string text)
		{
			LogDeviceOperations.WriteLine(Instance, channel, text);
		}

		public static bool IsPending
		{
			get { return LogSystem.IsPending(); }
		}

		public static void Flush()
		{
			LogSystem.Flush();
		}

		public static void Suspend()
		{
			LogSystem.Suspend();
		}

		public static void Resume()
		{
			LogSystem.Resume();
		}

		public static void Intitialize()
		{
			LogSystem.Initialize(null);
		}

		public static void Shutdown()
		{
			LogSystem.Shutdown();
		}
	}
}
