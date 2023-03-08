// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Linq;
using System.IO;
using System.Reflection;
using System.Xml;
using System.Xml.Serialization;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using Microsoft.P4VFS.CoreInterop;
using Newtonsoft.Json.Linq;

namespace Microsoft.P4VFS.Extensions
{
	[XmlRoot("Settings", Namespace="")]
	public static class ServiceSettings
	{
		public static string FileLoggerRemoteDirectory			{ get => GetString(""); set => SetString(value); } 
		public static string FileLoggerLocalDirectory			{ get => GetString(""); set => SetString(value); } 
		public static bool AllowSymlinkResidencyPolicy			{ get => GetBool(false); set => SetBool(value); } 
		public static bool ReportUsageExternally				{ get => GetBool(false); set => SetBool(value); } 
		public static bool ImmediateLogging						{ get => GetBool(false); set => SetBool(value); } 
		public static bool ConsoleImmediateLogging				{ get => GetBool(false); set => SetBool(value); } 
		public static LogChannel Verbosity						{ get => GetEnum(LogChannel.Info); set => SetEnum(value); } 
		public static bool RemoteLogging						{ get => GetBool(false); set => SetBool(value); } 
		public static bool ConsoleRemoteLogging					{ get => GetBool(false); set => SetBool(value); } 
		public static int MaxSyncConnections					{ get => GetInt32(8); set => SetInt32(value); } 
		public static FilePopulateMethod PopulateMethod			{ get => GetEnum(FilePopulateMethod.Stream); set => SetEnum(value); } 
		public static DepotFlushType DefaultFlushType			{ get => GetEnum(DepotFlushType.Atomic); set => SetEnum(value); } 
		public static DepotServerConfig DepotServerConfig		{ get => DepotServerConfig.FromNode(GetNode()); set => SetNode(value?.ToNode()); } 
		public static string SyncResidentPattern				{ get => GetString(""); set => SetString(value); } 
		public static bool SyncDefaultQuiet						{ get => GetBool(false); set => SetBool(value); } 
		public static bool Unattended							{ get => GetBool(false); set => SetBool(value); } 
		public static string ExcludedProcessNames				{ get => GetString(""); set => SetString(value); } 

		public static void Reset()
		{
			SettingManager.Reset();
			if (LoadFromFile(VirtualFileSystem.LocalSettingsFilePath) == false)
			{
				LoadFromFile(VirtualFileSystem.InstalledSettingsFilePath);
			}
		}

		public static string GetString(string defaultValue, [CallerMemberName] string memberName = "")
		{
			return GetNode(memberName).ToString(defaultValue);
		}

		public static int GetInt32(int defaultValue, [CallerMemberName] string memberName = "")
		{
			return GetNode(memberName).ToInt32(defaultValue);
		}

		public static bool GetBool(bool defaultValue, [CallerMemberName] string memberName = "")
		{
			return GetNode(memberName).ToBool(defaultValue);
		}

		public static TEnum GetEnum<TEnum>(TEnum defaultValue, [CallerMemberName] string memberName = "")
		{
			return Utilities.Converters.ParseType(GetNode(memberName).ToString(""), defaultValue).Value;
		}

		public static JToken GetJson([CallerMemberName] string memberName = "")
		{
			return SettingNodeToJson(GetNode(memberName));
		}

		public static SettingNode GetNode([CallerMemberName] string memberName = "")
		{
			return SettingManager.GetProperty(memberName);
		}

		public static bool SetString(string value, [CallerMemberName] string memberName = "")
		{
			return SetNode(SettingNode.FromString(value), memberName);
		}

		public static bool SetInt32(int value, [CallerMemberName] string memberName = "")
		{
			return SetNode(SettingNode.FromInt32(value), memberName);
		}

		public static bool SetBool(bool value, [CallerMemberName] string memberName = "")
		{
			return SetNode(SettingNode.FromBool(value), memberName);
		}

		public static bool SetEnum<TEnum>(TEnum value, [CallerMemberName] string memberName = "")
		{
			return SetNode(SettingNode.FromString(value.ToString()), memberName);
		}

		public static bool SetJson(JToken value, [CallerMemberName] string memberName = "")
		{
			return SetNode(SettingNodeFromJson(value), memberName);
		}

		public static bool SetNode(SettingNode value, [CallerMemberName] string memberName = "")
		{
			return SettingManager.SetProperty(memberName, value);
		}

		public static JToken SettingNodeToJson(SettingNode node)
		{
			if (node == null)
			{
				return null;
			}

			XmlElement element = SettingNodeToXmlElement(new XmlDocument(), node);
			if (element == null)
			{
				return new JObject();
			}
			return JToken.FromObject(element);
		}

		public static SettingNode SettingNodeFromJson(JToken token)
		{
			if (token == null)
			{
				return null;
			}

			SettingNode node = XmlElementToSettingNode(token.ToObject<XmlDocument>()?.DocumentElement);
			if (node == null)
			{
				return new SettingNode();
			}
			return node;
		}

		public static SettingNode XmlElementToSettingNode(XmlElement element)
		{
			SettingNode node = null;
			if (element != null)
			{
				List<SettingNode> childNodes = new List<SettingNode>();
				foreach (XmlAttribute childAttribute in element.Attributes)
				{
					if (childAttribute.Name == "Value" && element.Attributes.Count == 1)
					{
						childNodes.Add(new SettingNode(childAttribute.Value));
					}
					else
					{
						childNodes.Add(new SettingNode(childAttribute.Name, childAttribute.Value));
					}
				}

				foreach (XmlNode childNode in element.ChildNodes)
				{
					XmlText childText = childNode as XmlText;
					if (childText != null)
					{
						if (String.IsNullOrEmpty(childText.Value) == false)
						{
							childNodes.Add(new SettingNode(childText.Value));
						}
						continue;
					}

					XmlElement childElement = childNode as XmlElement;
					if (childElement != null)
					{
						childNodes.Add(XmlElementToSettingNode(childElement));
						continue;
					}
				}
				node = new SettingNode(XmlConvert.DecodeName(element.Name), childNodes.Where(n => n != null).ToArray());
			}
			return node;
		}

		public static XmlElement SettingNodeToXmlElement(XmlDocument document, SettingNode node)
		{
			XmlElement nodeElement = null;
			if (String.IsNullOrEmpty(node?.m_Data) == false)
			{
				nodeElement = document.CreateElement(XmlConvert.EncodeName(node.m_Data));
				if (node.m_Nodes != null)
				{
					if (node.m_Nodes.Length == 1 && node.m_Nodes[0]?.m_Nodes?.FirstOrDefault() == null)
					{
						nodeElement.AppendChild(document.CreateTextNode(node.m_Nodes[0].m_Data));
					}
					else
					{
						foreach (SettingNode childNode in node.m_Nodes.Where(n => !String.IsNullOrEmpty(n?.m_Data)))
						{
							XmlElement childElement = SettingNodeToXmlElement(document, childNode);
							if (childElement != null)
							{
								nodeElement.AppendChild(childElement);
							}
						}
					}
				}
			}
			return nodeElement;
		}

		public static string SettingNodeToText(SettingNode node)
		{
			string text = null;
			if (node != null)
			{
				if (node.m_Nodes?.FirstOrDefault() == null)
				{
					text = node.m_Data;
				}
				else
				{
					text = Newtonsoft.Json.JsonConvert.SerializeObject(SettingNodeToJson(node));
				}
			}
			return text ?? "";
		}

		public static SettingNodeMap GetMap()
		{
			SettingNodeMap nodeMap = new SettingNodeMap();
			foreach (PropertyInfo property in typeof(ServiceSettings).GetProperties(BindingFlags.Static|BindingFlags.Public|BindingFlags.GetProperty))
			{
				SettingNode node = GetNode(property.Name);
				if (node != null)
				{
					nodeMap[property.Name] = node;
				}
			}
			return nodeMap;
		}

		public static bool SetMap(SettingNodeMap nodeMap)
		{
			bool success = true;
			foreach (KeyValuePair<string, SettingNode> nodeProperty in nodeMap)
			{
				success &= SetNode(nodeProperty.Value, nodeProperty.Key);
			}
			return success;
		}

		public static bool LoadFromFile(string fileName)
		{
			try
			{
				if (File.Exists(fileName))
				{
					VirtualFileSystemLog.Verbose("ServiceSettings.LoadFromFile \"{0}\"", fileName);
					XmlDocument document = new XmlDocument();
					using (XmlTextReader reader = new XmlTextReader(fileName))
					{
						reader.Namespaces = false;
						document.Load(reader);
					}
					XmlElement settingsElement = document.DocumentElement;
					if (settingsElement.Name == "Settings")
					{
						foreach (XmlElement settingElement in settingsElement.ChildNodes.OfType<XmlElement>())
						{
							SettingNode node = XmlElementToSettingNode(settingElement);
							if (node != null)
							{
								VirtualFileSystemLog.Verbose("ServiceSettings.LoadFromFile reading {0}", SettingNodeToText(node));
								ServiceSettings.SetNode(node.m_Nodes?.FirstOrDefault(), node.m_Data);
							}
						}
					}
					return true;
				}
			}
			catch (Exception e)
			{
				VirtualFileSystemLog.Error("ServiceSettings.LoadFromFile failed to load file \"{0}\" {1}", fileName, e.Message);
			}
			return false;
		}

		public static bool SaveToFile(string fileName)
		{
			try
			{
				VirtualFileSystemLog.Verbose("ServiceSettings.SaveToFile \"{0}\"", fileName);
				XmlDocument document = new XmlDocument();
				XmlElement settingsElement = document.AppendChild(document.CreateElement("Settings")) as XmlElement;
				foreach (PropertyInfo property in typeof(ServiceSettings).GetProperties(BindingFlags.Static|BindingFlags.Public|BindingFlags.GetProperty))
				{
					SettingNode value = ServiceSettings.GetNode(property.Name);
					if (value != null)
					{
						SettingNode node = new SettingNode(property.Name, new[]{ value });
						VirtualFileSystemLog.Verbose("ServiceSettings.SaveToFile writing {0}", SettingNodeToText(node));
						settingsElement.AppendChild(SettingNodeToXmlElement(document, node));
					}
				}

				Utilities.FileUtilities.DeleteFile(fileName);
				using (XmlTextWriter textWriter = new XmlTextWriter(fileName, System.Text.Encoding.UTF8))
				{
					textWriter.Formatting = System.Xml.Formatting.Indented;
					document.Save(textWriter);
				}
				return true;
			}
			catch (Exception e)
			{
				VirtualFileSystemLog.Error("ServiceSettings.SaveToFile failed to write file \"{0}\" {1}", fileName, e.Message);
			}
			return false;
		}
	}

	public class SettingNodeMap : SortedDictionary<string, SettingNode>
	{
	}
}
