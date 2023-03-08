// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Diagnostics;
using System.Linq;
using System.Xml;
using System.Xml.Serialization;
using Microsoft.P4VFS.CoreInterop;
using Microsoft.P4VFS.Extensions.Utilities;

namespace Microsoft.P4VFS.Extensions
{
	public class DepotServerConfig
	{
		[XmlArray("Servers"), XmlArrayItem("Entry", typeof(DepotServerConfigEntry))]
		public DepotServerConfigEntry[] Servers;

		public SettingNode ToNode()
		{
			XmlElement documentElement = XmlUtilities.ToXmlElement(this);
			return ServiceSettings.XmlElementToSettingNode(documentElement?.ChildNodes?.OfType<XmlElement>()?.FirstOrDefault());
		}

		public static DepotServerConfig FromNode(SettingNode node)
		{
			XmlDocument document = new XmlDocument();
			document.AppendChild(document.CreateElement(typeof(DepotServerConfig).Name));
			XmlElement value = ServiceSettings.SettingNodeToXmlElement(document, node);
			if (value != null)
			{
				document.DocumentElement.AppendChild(value);
			}
			return XmlUtilities.FromXmlElement<DepotServerConfig>(document.DocumentElement);
		}
	}

	public class DepotServerConfigEntry
	{
		[XmlElement("Pattern")] 
		public string Pattern;

		[XmlElement("Address")] 
		public string Address;
	}
}
