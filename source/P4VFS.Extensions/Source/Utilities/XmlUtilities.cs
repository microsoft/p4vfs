// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.IO;
using System.Diagnostics;
using System.Linq;
using System.Xml;
using System.Xml.Serialization;

namespace Microsoft.P4VFS.Extensions.Utilities
{
	public static class XmlUtilities
	{
		public static XmlElement ToXmlElement(object value)
		{
			try
			{
				if (value != null)
				{
					XmlDocument document = new XmlDocument();
					using (XmlWriter writer = document.CreateNavigator().AppendChild())
					{
						XmlSerializer serializer = new XmlSerializer(value.GetType());
						serializer.Serialize(writer, value);
					}
					return document.DocumentElement;
				}
			}
			catch {}
			return null;
		}

		public static ValueType FromXmlElement<ValueType>(XmlElement element) where ValueType : class
		{
			try
			{
				if (element != null)
				{
					using (XmlReader reader = new XmlNodeReader(element))
					{
						XmlSerializer serializer = new XmlSerializer(typeof(ValueType));
						return serializer.Deserialize(reader) as ValueType;
					}
				}
			}
			catch {}
			return null;
		}

		public static ValueType LoadFromFile<ValueType>(string fileName) where ValueType : class
		{
			try
			{
				if (File.Exists(fileName))
				{
					using (FileStream fs = File.OpenRead(fileName))
					{
						XmlSerializer xml = new XmlSerializer(typeof(ServiceSettings));
						ValueType value = xml.Deserialize(fs) as ValueType;
						return value;
					}
				}
			}
			catch {}
			return null;
		}

		public static bool SaveToFile(object value, string fileName)
		{
			try
			{
				if (value == null)
					return false;
				if (File.Exists(fileName))
					File.Delete(fileName);
				using (XmlTextWriter writer = new XmlTextWriter(fileName, System.Text.Encoding.UTF8))
				{
					writer.Indentation = 2;
					writer.Formatting = Formatting.Indented;
					XmlSerializer xml = new XmlSerializer(value.GetType());
					xml.Serialize(writer, value);
					return true;
				}
			}
			catch {}
			return false;
		}
	}
}
