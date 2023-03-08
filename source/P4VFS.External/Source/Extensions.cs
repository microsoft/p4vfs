// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Linq;
using System.Diagnostics;

namespace Microsoft.P4VFS.Extensions
{
	public class VirtualFileSystemLog
	{
		public static void Info(string text)
		{
			Trace.TraceInformation(text);
		}

		public static void Info(string format, params object[] args)
		{
			Trace.TraceInformation(format, args);
		}

		public static void Error(string text)
		{
			Trace.TraceError(text);
		}

		public static void Error(string format, params object[] args)
		{
			Trace.TraceError(format, args);
		}
	}

	public class NamedConsoleTraceListener : ConsoleTraceListener
	{
		public NamedConsoleTraceListener(string name)
		{
			SourceName = name;
		}

		public string SourceName
		{
			get;
		}

		public override void TraceEvent(TraceEventCache eventCache, string source, TraceEventType eventType, int id, string message) 
		{
			WriteLine(String.Format("{0} {1}: {2}", SourceName, eventType.ToString(), message));
		}

		public override void TraceEvent(TraceEventCache eventCache, string source, TraceEventType eventType, int id, string format, params object[] args)
		{
			TraceEvent(eventCache, source, eventType, id, args == null ? format : String.Format(format, args));
		}
	}
}
