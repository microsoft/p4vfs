// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.IO;
using System.Linq;
using System.Collections.Generic;
using System.Xml.Serialization;

namespace Microsoft.P4VFS.Setup
{
	[XmlRoot("Configuration", Namespace="")]
	public class SetupConfiguration
	{
		[XmlArray("IncludeFiles"), XmlArrayItem("Path", typeof(string))]
		public List<string> IncludeFiles;

		[XmlArray("RegistryKeys"), XmlArrayItem("Key", typeof(SetupRegistryKey))]
		public List<SetupRegistryKey> RegistryKeys;

		public static SetupConfiguration LoadFromFile(string fileName)
		{
			try
			{
				using (FileStream fs = File.OpenRead(fileName))
				{
					XmlSerializer xml = new XmlSerializer(typeof(SetupConfiguration));
					SetupConfiguration config = xml.Deserialize(fs) as SetupConfiguration;
					return config;
				}
			}
			catch {}
			return null;
		}

		public void Import(SetupConfiguration config)
		{
			if (IncludeFiles == null)
				IncludeFiles = new List<string>();
			if (config?.IncludeFiles != null)
				IncludeFiles.AddRange(config.IncludeFiles);

			if (RegistryKeys == null)
				RegistryKeys = new List<SetupRegistryKey>();
			if (config?.RegistryKeys != null)
				RegistryKeys.AddRange(config.RegistryKeys);
		}
	}

	public class SetupRegistryKey
	{
		[XmlAttribute]
		public string Name;

		[XmlAttribute]
		public string Value;
	}
}

