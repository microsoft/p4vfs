// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Collections.Generic;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.P4VFS.Extensions;
using System.Diagnostics.Tracing;

namespace Microsoft.P4VFS.UnitTest
{
	[TestClass, BasePriority(8000)]
	public class UnitTestEventSource : UnitTestBase
	{
		[TestMethod, Priority(0)]
		public void BasicEventSourcesTest()
		{
			Action<EventSource> verifyEventSource = (EventSource eventSource) =>
			{
				Assert(String.IsNullOrEmpty(EventSource.GenerateManifest(eventSource.GetType(), null)) == false);
				using (EventListener listener = new EventListener())
				{
					listener.EnableEvents(eventSource, EventLevel.LogAlways);
					listener.DisableEvents(eventSource);
				}
			};

			AssertLambda(() => verifyEventSource(EventSourceLog.Instance));
			
			string id = Guid.NewGuid().ToString();
			EventSourceLog.Trace("BasicEventSourcesTest", EventLevel.Informational, new { Root = GetRepositoryRootFolder(), Id = id });

			// TODO: Start a trace log and verify that this event id arrives
			// tracelog.exe -start p4vfslog -guid #{Extensions.EventSource.Log.Guid} -f p4vfslog.etl
			//  ....  trace some log info ... 
			// tracelog.exe -flush p4vfslog
			// tracelog.exe -stop p4vfslog
			// tracefmt.exe p4vfslog.etl -nosummary
		}
	}
}
