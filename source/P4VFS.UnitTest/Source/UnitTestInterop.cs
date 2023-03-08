// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Linq;
using System.Reflection;
using System.Collections.Generic;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.P4VFS.Extensions;
using Microsoft.P4VFS.CoreInterop;

namespace Microsoft.P4VFS.UnitTest
{
	[TestClass, BasePriority(UInt16.MaxValue)]
	public class UnitTestInterop : UnitTestBase
	{
		[TestMethod, Priority(0), TestRemote, TestAlways]
		public void FileInteropFactoryTest()
		{
			using (DepotClient depotClient = new DepotClient())
			{
				depotClient.Unattended = true;
				if (depotClient.Connect(_P4Port, _P4Client, _P4User) == false)
				{
					VirtualFileSystemLog.Warning("Skipping native tests with failed perforce connection");
					return;
				}
			}

			WorkspaceReset();

			CoreInterop.TestContextInterop context = new CoreInterop.TestContextInterop();
			context.m_Environment = new Dictionary<string, string>(StringComparer.InvariantCultureIgnoreCase);
			context.m_Environment["P4USER"] = _P4User;
			context.m_Environment["P4PORT"] = _P4Port;
			context.m_Environment["P4CLIENT"] = _P4Client;
			context.m_Environment["P4ROOT"] = GetClientRoot();
			context.m_Environment["P4VFS_EXE"] = P4vfsExe;
			context.m_Environment["P4VFS_EXE_CONFIG"] = ClientConfig.ToString();
			context.m_ReconcilePreviewAny = (s) => this.ReconcilePreview(s).Any();
			context.m_WorkspaceReset = () => this.WorkspaceReset();
			context.m_IsPlaceholderFile = (s) => this.IsPlaceholderFile(s);
			context.m_ServiceLastRequestTime = () => this.GetServiceLastRequestTime();
				
			MethodInfo currentMethod = MethodBase.GetCurrentMethod() as MethodInfo;
			Assert(currentMethod != null);

			string[] interopArgs = (CommandLineArgs ?? new string[0]).Where(a => a != GetPriority(currentMethod).ToString()).ToArray();
			AssertLambda(() => CoreInterop.TestFactoryInterop.Run(interopArgs, context));
		}
	}
}
