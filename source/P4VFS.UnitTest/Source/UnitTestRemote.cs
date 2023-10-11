// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Linq;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Reflection;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.P4VFS.Extensions;
using Microsoft.P4VFS.Extensions.Utilities;
using Microsoft.P4VFS.CoreInterop;
using NetFwTypeLib;

namespace Microsoft.P4VFS.UnitTest
{
	[TestClass, BasePriority(3000)]
	public class UnitTestRemote : UnitTestBase
	{
		[TestMethod, Priority(0)]
		public void RemoteExecuteTest()
		{
			if (new[]{ RemoteHost, RemoteUser, RemotePasswd }.Any(String.IsNullOrEmpty))
			{
				VirtualFileSystemLog.Info("Skipping RemoteExecuteTest with unset variable");
				return;
			}

			string remoteActiveUser = GetRemoteActiveLogonUser();
			string remoteExecuteUser = Regex.Replace(RemoteUser, @".*\\", "");
			Assert(String.Compare(remoteActiveUser, remoteExecuteUser, StringComparison.InvariantCultureIgnoreCase) == 0, $"{remoteExecuteUser} not currently logged into {RemoteHost}");

			string setupExe = UnitTestInstall.GetSetupExe();
			Assert(File.Exists(setupExe));
			Assert(RemoteExecuteWait("fltmc.exe", shell:true, admin:true) == 0, "remote user not elevated");
			Assert(new[]{0,128}.Contains(RemoteExecuteWait("taskkill.exe /f /im p4vfs.exe", admin:true)));
			Assert(RemoteExecuteWait(String.Format("{0} -a -c", setupExe), copy:true, admin:true) == 0);

			SortedSet<int> remoteTests = new SortedSet<int>();
			foreach (Type unitTestClassType in Assembly.GetExecutingAssembly().GetTypes().Where(t => t.IsClass && t.GetCustomAttribute<TestClassAttribute>(true) != null))
			{
				foreach (MethodInfo methodInfo in unitTestClassType.GetMethods(BindingFlags.Public|BindingFlags.Instance).Where(m => m.GetCustomAttribute<TestRemoteAttribute>() != null))
				{
					remoteTests.Add(GetPriority(methodInfo));
				}
			}

			uint serverPortNumber = UnitTestServer.GetServerPortNumber(_P4Port);
			string serverPortIPAddress = UnitTestServer.GetServerPortIPAddress(_P4Port);
			AssertLambda(() => System.Net.IPAddress.TryParse(serverPortIPAddress, out System.Net.IPAddress a));
			
			DepotConfig remoteConfig = new DepotConfig() { 
				Port = String.Format("{0}:{1}", serverPortIPAddress, serverPortNumber), 
				Client = _P4Client, 
				User = _P4User,
				Passwd = UnitTestServer.GetUserP4Passwd(_P4User)
			};

			Assert(remoteTests.Count > 0);
			string remoteTestsArgs = String.Join(" ", new[]{"-r"}.Concat(remoteTests.Select(t => t.ToString())));
			AssertLambda(() => CreateRemoteFirewallSettings(remoteConfig));
			Assert(RemoteExecuteWait(String.Format("\"{0}\" {1} login", InstalledP4vfsExe, remoteConfig)) == 0);
			Assert(RemoteExecuteWait(String.Format("\"{0}\" {1} test {2}", InstalledP4vfsExe, remoteConfig, remoteTestsArgs), admin:true) == 0);
			AssertLambda(() => RemoveRemoteFirewallSettings(remoteConfig));
		}

		private int RemoteExecuteWait(string cmd, bool admin=false, bool shell=false, bool copy=false, StringBuilder stdout=null)
		{
			return ProcessInfo.RemoteExecuteWait(cmd, RemoteHost, RemoteUser, RemotePasswd, admin:admin, shell:shell, copy:copy, stdout:stdout, interactive:true, sysInternalsFolder:SysInternalsFolder);
		}

		private string GetRequiredEnvironmentVariable(string name)
		{
			string value = Environment.GetEnvironmentVariable(name);
			if (String.IsNullOrEmpty(value))
			{
				VirtualFileSystemLog.Warning("UnitTestRemote missing required environment variable: {0}", name);
			}
			return value;
		}

		private string RemoteHost
		{
			get { return GetRequiredEnvironmentVariable("P4VFS_TEST_REMOTE_HOST"); }
		}

		private string RemoteUser
		{
			get { return GetRequiredEnvironmentVariable("P4VFS_TEST_REMOTE_USER"); }
		}

		private string RemotePasswd
		{
			get { return GetRequiredEnvironmentVariable("P4VFS_TEST_REMOTE_PASSWD"); }
		}

		private static string RemoteFirewallRuleName
		{
			get { return "P4VFS_TEST_PERFORCE_SERVER"; }
		}

		private void CreateRemoteFirewallSettings(DepotConfig config)
		{
			INetFwPolicy2 policy = Activator.CreateInstance(Type.GetTypeFromProgID("HNetCfg.FwPolicy2")) as INetFwPolicy2;
			Assert(policy != null);
			
			INetFwRule rule = policy.Rules.OfType<INetFwRule>().FirstOrDefault(r => r.Name == RemoteFirewallRuleName);
			if (rule == null)
			{
				VirtualFileSystemLog.Info("Creating firewall exemption: {0}", RemoteFirewallRuleName);
				rule = (INetFwRule)Activator.CreateInstance(Type.GetTypeFromProgID("HNetCfg.FWRule"));
				rule.Name = RemoteFirewallRuleName;
				policy.Rules.Add(rule);
			}
			else
			{
				VirtualFileSystemLog.Info("Updating firewall exemption: {0}", RemoteFirewallRuleName);
			}

			rule.Description = RemoteFirewallRuleName;
			rule.ApplicationName = Path.GetFileName(P4dExe);
			rule.Action = NET_FW_ACTION_.NET_FW_ACTION_ALLOW;
			rule.Direction = NET_FW_RULE_DIRECTION_.NET_FW_RULE_DIR_IN;
			rule.Protocol = (int)NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_TCP;
			rule.Profiles = (int)NET_FW_PROFILE_TYPE2_.NET_FW_PROFILE2_ALL;
			rule.InterfaceTypes = "All";
			rule.LocalPorts = config.PortNumber();
			rule.Enabled = true;

			Marshal.FinalReleaseComObject(rule);
			Marshal.FinalReleaseComObject(policy);
		}

		private void RemoveRemoteFirewallSettings(DepotConfig config)
		{
			INetFwPolicy2 policy = Activator.CreateInstance(Type.GetTypeFromProgID("HNetCfg.FwPolicy2")) as INetFwPolicy2;
			Assert(policy != null);
			
			INetFwRule rule = policy.Rules.OfType<INetFwRule>().FirstOrDefault(r => r.Name == RemoteFirewallRuleName);
			if (rule == null)
			{
				VirtualFileSystemLog.Info("Removing firewall exemption: {0}", RemoteFirewallRuleName);
				policy.Rules.Remove(RemoteFirewallRuleName);
				Marshal.FinalReleaseComObject(rule);
			}

			Marshal.FinalReleaseComObject(policy);
		}

		private string GetRemoteActiveLogonUser()
		{
			StringBuilder queryStdout = new StringBuilder();
			Assert(RemoteExecuteWait("query.exe user", stdout:queryStdout) == 1);
			
			int queryIndex = 0;
			string[] queryLines = queryStdout.ToString().Split('\n');
			for (; queryIndex < queryLines.Length; ++queryIndex)
			{
				if (Regex.IsMatch(queryLines[queryIndex], @"^\s*USERNAME\s+SESSIONNAME\s+ID\s+STATE\s+"))
				{
					++queryIndex;
					break;
				}
			}

			for (; queryIndex < queryLines.Length; ++queryIndex)
			{
				Match m = Regex.Match(queryLines[queryIndex], @"^\s*(?<user>\S+).*\s(Active|Disc)\s", RegexOptions.IgnoreCase);
				if (m.Success)
				{
					return m.Groups["user"].Value;
				}
			}
			return null;
		}
	}
}
