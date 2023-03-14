// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Linq;
using System.IO;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.P4VFS.Extensions;
using Microsoft.P4VFS.Extensions.Utilities;
using Microsoft.P4VFS.CoreInterop;
using Newtonsoft.Json.Linq;
using Newtonsoft.Json;

namespace Microsoft.P4VFS.UnitTest
{
	[TestClass, BasePriority(5000)]
	public class UnitTestUtilities : UnitTestBase
	{
		[TestMethod, Priority(0)]
		public void FileAttributeTest()
		{
			foreach (ServiceSettingsScope settings in EnumerateCommonServicePopulateSettings())
			{
				using (settings) {
				WorkspaceReset();

				string clientRoot = GetClientRoot();
				string clientFile = String.Format(@"{0}\depot\tools\dev\source\CinematicCapture\DefaultSettings.xml", clientRoot);
				string clientFolder = Path.GetDirectoryName(clientFile);
				string clientFileMissing = String.Format("{0}\\does_not_exist.txt", clientFolder);
				string revision = "@16";

				List<TimeSpan> getAttrTimes = new List<TimeSpan>();
				foreach (var getAttr in new Func<string, FileAttributes>[]{ File.GetAttributes, FileUtilities.GetAttributes })
				{
					Assert(ProcessInfo.ExecuteWait(P4Exe, String.Format("{0} sync -f \"{1}{2}\"", ClientConfig, clientFile, revision), echo:true) == 0);
					Assert(File.Exists(clientFile));
					Assert(IsPlaceholderFile(clientFile) == false);
					Assert(getAttr(clientFile).HasFlag(FileAttributes.ReadOnly|FileAttributes.Archive));
					Assert(getAttr(clientFolder).HasFlag(FileAttributes.Directory));
					try { getAttr(clientFileMissing); Assert(false); } catch {}

					AssertLambda(() => FileUtilities.DeleteDirectoryAndFiles(clientRoot));
					AssertRetry(() => Directory.Exists(clientRoot) == false, String.Format("directory exists {0}", clientRoot));

					Assert(ProcessInfo.ExecuteWait(P4vfsExe, String.Format("{0} sync -f \"{1}{2}\"", ClientConfig, clientFile, revision), echo:true) == 0);
					Assert(File.Exists(clientFile));
					Assert(IsPlaceholderFile(clientFile));
					Assert(getAttr(clientFile).HasFlag(FileAttributes.ReadOnly|FileAttributes.Offline));
					Assert(getAttr(clientFolder).HasFlag(FileAttributes.Directory));
					try { getAttr(clientFileMissing); Assert(false); } catch {}
				
					Assert(ReconcilePreview(clientFolder).Any() == false);
					Assert(IsPlaceholderFile(clientFile) == false);
					Assert(getAttr(clientFile) == FileAttributes.ReadOnly);
					File.SetAttributes(clientFile, FileAttributes.Normal);
					Assert(getAttr(clientFile) == FileAttributes.Normal);

					DateTime startTime = DateTime.Now;
					for (int i = 0; i < 100000; ++i)
						getAttr(clientFile);
					getAttrTimes.Add(DateTime.Now - startTime);
				}
				
				// alexboc: Disable timing test
				//float getAttrTimeThreshold = 10.0f;
				//Assert(getAttrTimes[1].TotalMilliseconds < getAttrTimes[0].TotalMilliseconds * getAttrTimeThreshold, string.Format("FileUtilities.GetAttributes is un unacceptably slow {0} > {1}!", getAttrTimes[1].TotalMilliseconds, getAttrTimes[0].TotalMilliseconds * getAttrTimeThreshold));
				//VirtualFileSystemLog.Info("GetAttributes Times: [{0}]", String.Join(", ", getAttrTimes.Select(span => span.TotalMilliseconds.ToString())));
			}}
		}

		[TestMethod, Priority(1), TestRemote]
		public void ConverterTest()
		{
			var IsEqualFloat = new Func<float, float, bool>((a,b) => Math.Abs(a-b) < 1.0e-6);
			var IsEqualDouble = new Func<double, double, bool>((a,b) => Math.Abs(a-b) < 1.0e-9);
			var AssertLambda = new Action<Func<bool>>(a => Assert(a()));

			AssertLambda(() => { var r = Converters.ToFloat("1.34"); return IsEqualFloat(r.Value, 1.34f) && r.Success == true; });
			AssertLambda(() => { var r = Converters.ParseType<float>("1.34"); return IsEqualFloat(r.Value, 1.34f) && r.Success == true; });
			AssertLambda(() => { var r = Converters.ToFloat("-5.0e-3"); return IsEqualFloat(r.Value, -5.0e-3f) && r.Success == true; });
			AssertLambda(() => { var r = Converters.ParseType<float>("-5.0e-3"); return IsEqualFloat(r.Value, -5.0e-3f) && r.Success == true; });
			AssertLambda(() => { var r = Converters.ToFloat("stuff"); return IsEqualFloat(r.Value, 0) && r.Success == false; });
			AssertLambda(() => { var r = Converters.ToFloat("stuff", 15.0f); return IsEqualFloat(r.Value, 15.0f) && r.Success == false; });
			AssertLambda(() => { var r = Converters.ParseType<float>("stuff"); return IsEqualFloat(r.Value, 0) && r.Success == false; });
			AssertLambda(() => { var r = Converters.ParseType<float>("stuff", 15.0f); return IsEqualFloat(r.Value, 15.0f) && r.Success == false; });
			AssertLambda(() => { var r = Converters.ToFloat(""); return IsEqualFloat(r.Value, 0) && r.Success == false; });
			AssertLambda(() => { var r = Converters.ParseType<float>("stuff"); return IsEqualFloat(r.Value, 0) && r.Success == false; });

			AssertLambda(() => { var r = Converters.ToDouble("1.347893"); return IsEqualDouble(r.Value, 1.347893) && r.Success == true; });
			AssertLambda(() => { var r = Converters.ParseType<double>("1.347893"); return IsEqualDouble(r.Value, 1.347893) && r.Success == true; });
			AssertLambda(() => { var r = Converters.ToDouble("  -5.000045e-3  "); return IsEqualDouble(r.Value, -5.000045e-3) && r.Success == true; });
			AssertLambda(() => { var r = Converters.ParseType<double>("  -5.000045e-3  "); return IsEqualDouble(r.Value, -5.000045e-3) && r.Success == true; });
			AssertLambda(() => { var r = Converters.ToDouble("  stuff "); return IsEqualDouble(r.Value, 0) && r.Success == false; });
			AssertLambda(() => { var r = Converters.ParseType<double>("  stuff "); return IsEqualDouble(r.Value, 0) && r.Success == false; });
			AssertLambda(() => { var r = Converters.ToDouble(null); return IsEqualDouble(r.Value, 0) && r.Success == false; });
			AssertLambda(() => { var r = Converters.ParseType<double>(null); return IsEqualDouble(r.Value, 0) && r.Success == false; });

			AssertLambda(() => { var r = Converters.ToInt32("256"); return r.Value == 256 && r.Success == true; });
			AssertLambda(() => { var r = Converters.ParseType<int>("256"); return r.Value == 256 && r.Success == true; });
			AssertLambda(() => { var r = Converters.ToInt32("-512"); return r.Value == -512 && r.Success == true; });
			AssertLambda(() => { var r = Converters.ParseType<int>("-512"); return r.Value == -512 && r.Success == true; });
			AssertLambda(() => { var r = Converters.ToInt32("-5.0e-3"); return r.Value == 0 && r.Success == false; });
			AssertLambda(() => { var r = Converters.ToInt32("-5.0e-3", 56); return r.Value == 56 && r.Success == false; });
			AssertLambda(() => { var r = Converters.ParseType<int>("-5.0e-3"); return r.Value == 0 && r.Success == false; });
			AssertLambda(() => { var r = Converters.ParseType<int>("-5.0e-3", 56); return r.Value == 56 && r.Success == false; });
			AssertLambda(() => { var r = Converters.ToInt32("bar"); return r.Value == 0 && r.Success == false; });
			AssertLambda(() => { var r = Converters.ParseType<int>("bar"); return r.Value == 0 && r.Success == false; });

			AssertLambda(() => { var r = Converters.ToString("foobar"); return r.Value == "foobar" && r.Success == true; });
			AssertLambda(() => { var r = Converters.ParseType<string>("foobar"); return r.Value == "foobar" && r.Success == true; });
			AssertLambda(() => { var r = Converters.ToString(""); return r.Value == "" && r.Success == true; });
			AssertLambda(() => { var r = Converters.ParseType<string>(""); return r.Value == "" && r.Success == true; });
			AssertLambda(() => { var r = Converters.ToString(null); return r.Value == null && r.Success == false; });
			AssertLambda(() => { var r = Converters.ToString(null, "stuff"); return r.Value == "stuff" && r.Success == false; });
			AssertLambda(() => { var r = Converters.ParseType<string>(null); return r.Value == null && r.Success == false; });
			AssertLambda(() => { var r = Converters.ParseType<string>(null, "stuff"); return r.Value == "stuff" && r.Success == false; });

			AssertLambda(() => { var r = Converters.ToBoolean("false"); return r.Value == false && r.Success == true; });
			AssertLambda(() => { var r = Converters.ToBoolean("False"); return r.Value == false && r.Success == true; });
			AssertLambda(() => { var r = Converters.ToBoolean("true"); return r.Value == true && r.Success == true; });
			AssertLambda(() => { var r = Converters.ToBoolean("True"); return r.Value == true && r.Success == true; });
			AssertLambda(() => { var r = Converters.ParseType<bool>("false"); return r.Value == false && r.Success == true; });
			AssertLambda(() => { var r = Converters.ParseType<bool>("False"); return r.Value == false && r.Success == true; });
			AssertLambda(() => { var r = Converters.ParseType<bool>("true"); return r.Value == true && r.Success == true; });
			AssertLambda(() => { var r = Converters.ParseType<bool>("TRUE"); return r.Value == true && r.Success == true; });
			AssertLambda(() => { var r = Converters.ToBoolean(""); return r.Value == false && r.Success == false; });
			AssertLambda(() => { var r = Converters.ToBoolean("", true); return r.Value == true && r.Success == false; });
			AssertLambda(() => { var r = Converters.ParseType<bool>(""); return r.Value == false && r.Success == false; });
			AssertLambda(() => { var r = Converters.ParseType<bool>(null, true); return r.Value == true && r.Success == false; });
			AssertLambda(() => { var r = Converters.ToBoolean("  stuff "); return r.Value == false && r.Success == false; });
			AssertLambda(() => { var r = Converters.ParseType<bool>("  stuff ", true); return r.Value == true && r.Success == false; });
			AssertLambda(() => { var r = Converters.ToBoolean(null); return r.Value == false && r.Success == false; });

			AssertLambda(() => { var r = Converters.ToEnum<FileAttributes>("Archive"); return r.Value == FileAttributes.Archive && r.Success == true; });
			AssertLambda(() => { var r = Converters.ParseType<FileAttributes>("Archive"); return r.Value == FileAttributes.Archive && r.Success == true; });
			AssertLambda(() => { var r = Converters.ToEnum<FileAttributes>("FileAttributes.Archive", FileAttributes.ReadOnly); return r.Value == FileAttributes.ReadOnly && r.Success == false; });
			AssertLambda(() => { var r = Converters.ToEnum<FileAttributes>("FileAttributes.Archive"); return r.Value == default(FileAttributes) && r.Success == false; });
			AssertLambda(() => { var r = Converters.ParseType<FileAttributes>("FileAttributes.Archive", FileAttributes.ReadOnly); return r.Value == FileAttributes.ReadOnly && r.Success == false; });
			AssertLambda(() => { var r = Converters.ToEnum<FileAttributes>("foobar"); return r.Value == default(FileAttributes) && r.Success == false; });
			AssertLambda(() => { var r = Converters.ParseType<FileAttributes>("foobar"); return r.Value == default(FileAttributes) && r.Success == false; });

			AssertLambda(() => { var r = Converters.ToString((object)true); return r.Value == "true" && r.Success == true; });
			AssertLambda(() => { var r = Converters.ToString((object)false); return r.Value == "false" && r.Success == true; });
			AssertLambda(() => { var r = Converters.ToString((object)32); return r.Value == "32" && r.Success == true; });
			AssertLambda(() => { var r = Converters.ToString((object)0); return r.Value == "0" && r.Success == true; });
			AssertLambda(() => { var r = Converters.ToString((object)3.14159); return r.Value == "3.14159" && r.Success == true; });
			AssertLambda(() => { var r = Converters.ToString("foobar"); return r.Value == "foobar" && r.Success == true; });
			AssertLambda(() => { var r = Converters.ToString(null); return r.Value == null && r.Success == false; });
			AssertLambda(() => { var r = Converters.ToString(null, "bar"); return r.Value == "bar" && r.Success == false; });
		}

		[TestMethod, Priority(2), TestRemote]
		public void ServiceSettingJsonTest()
		{
			Action<SettingNode> TestNodeToFromJson = (node) =>
			{
				JToken t0 = ServiceSettings.SettingNodeToJson(node);
				Assert((node == null && t0 == null) || (node != null && t0 != null));
				SettingNode s1 = ServiceSettings.SettingNodeFromJson(t0);
				Assert((node == null && s1 == null) || (node != null && s1 != null));
				JToken t1 = ServiceSettings.SettingNodeToJson(s1);
				string text0 = JsonConvert.SerializeObject(t0);
				string text1 = JsonConvert.SerializeObject(t1);
				Assert(text0 == text1);
			};
			TestNodeToFromJson(new SettingNode{ m_Data="foo" });
			TestNodeToFromJson(new SettingNode{ });
			TestNodeToFromJson(new SettingNode{ m_Data="foo", m_Nodes=new SettingNode[]{} });
			TestNodeToFromJson(new SettingNode{ m_Data="", m_Nodes=new SettingNode[]{ new SettingNode{ m_Data="bar" } } });
			TestNodeToFromJson(new SettingNode{ m_Data="foo", m_Nodes=new SettingNode[]{ new SettingNode{ m_Data="bar" }, new SettingNode{ m_Data="star" } } });
			TestNodeToFromJson(new SettingNode{ m_Data="foo", m_Nodes=new SettingNode[]{ new SettingNode{ m_Data="bar", m_Nodes=new SettingNode[]{ new SettingNode{ m_Data="stuff" } } }, new SettingNode{ m_Data="star" } } });
		}
	}
}
