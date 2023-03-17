// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Collections.Generic;
using System.Diagnostics.Tracing;

namespace Microsoft.P4VFS.Extensions
{
	[EventSource(Guid = "{06C0739A-3DC4-4A9C-87AC-B2F85436D453}", Name = "P4VFS")]
	public class EventSourceLog : EventSource
	{
		private static EventSource _Instance = new EventSourceLog();

		public static EventSource Instance
		{
			get { return _Instance; }
		}

		public static void Trace<T>(string eventName, EventLevel level, T data)
		{
			Instance.Write(eventName,
				new EventSourceOptions { Level = level, Opcode = EventOpcode.Info },
				data);
		}

		public static void Error(string message)
		{
			Instance.Write("Error",
				new EventSourceOptions { Level = EventLevel.Error, Opcode = EventOpcode.Info },
				new { Message = message });
		}

		public static void Info(string message)
		{
			Instance.Write("Info",
				new EventSourceOptions { Level = EventLevel.Informational, Opcode = EventOpcode.Info },
				new { Message = message });
		}
	}
}
