// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Collections.Generic;
using Microsoft.P4VFS.Extensions;
using Microsoft.Practices.EnterpriseLibrary.SemanticLogging.Utility;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Microsoft.P4VFS.UnitTest
{
	[TestClass, BasePriority(8000)]
	public class UnitTestEventSource : UnitTestBase
	{
		[TestMethod, Priority(0)]
		public void BasicEventSourcesTest()
		{
			AssertLambda(() => EventSourceAnalyzer.InspectAll(EventSource.Log));
		}
	}
}
