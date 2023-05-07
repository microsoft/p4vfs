// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Linq;
using System.IO;
using System.Xml;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using Microsoft.P4VFS.CoreInterop;
using Newtonsoft.Json.Linq;

namespace Microsoft.P4VFS.Extensions
{
	public static class ServiceSettings
	{
		public static void Reset()
		{
			SettingManager.Reset();
			if (LoadFromFile(VirtualFileSystem.LocalSettingsFilePath) == false)
			{
				LoadFromFile(VirtualFileSystem.InstalledSettingsFilePath);
			}
		}

		public static TEnum GetPropertyEnum<TEnum>(TEnum defaultValue, [CallerMemberName] string memberName = "")
		{
			return Utilities.Converters.ParseType(GetProperty(memberName).ToString(""), defaultValue).Value;
		}

		public static bool SetPropertyEnum<TEnum>(TEnum value, [CallerMemberName] string memberName = "")
		{
			return SetProperty(SettingNode.FromString(value.ToString()), memberName);
		}

		public static JToken GetPropertyJson([CallerMemberName] string memberName = "")
		{
			return SettingNodeToJson(GetProperty(memberName));
		}

		public static bool SetPropertyJson(JToken value, [CallerMemberName] string memberName = "")
		{
			return SetProperty(SettingNodeFromJson(value), memberName);
		}

		public static SettingNode GetProperty([CallerMemberName] string memberName = "")
		{
			return SettingManager.GetProperty(memberName);
		}

		public static bool SetProperty(SettingNode value, [CallerMemberName] string memberName = "")
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
								SettingManager.SetProperty(node.m_Data, node.m_Nodes?.FirstOrDefault());
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
				foreach (KeyValuePair<string, SettingNode> property in SettingManager.GetProperties())
				{
					SettingNode value = property.Value;
					if (value != null)
					{
						SettingNode node = new SettingNode(property.Key, new[]{ value });
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
}
