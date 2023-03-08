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
	public class Configuration
	{
		[XmlArray("IncludeFiles"), XmlArrayItem("Path", typeof(string))]
		public string[] IncludeFiles;

		public static Configuration LoadFromFile(string fileName)
		{
			try
			{
				using (FileStream fs = File.OpenRead(fileName))
				{
					XmlSerializer xml = new XmlSerializer(typeof(Configuration));
					Configuration config = xml.Deserialize(fs) as Configuration;
					return config;
				}
			}
			catch {}
			return null;
		}
	}
}

