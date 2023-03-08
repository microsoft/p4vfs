// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Diagnostics;
using System.Linq;
using System.IO;
using System.Threading;
using System.Collections.Generic;
using System.Text.RegularExpressions;

namespace Microsoft.P4VFS.Extensions
{
	public class VersionDescriptor
	{
		public VersionDescriptor(string versionString)
		{
			string[] tokens = versionString?.Split('.');
			if (tokens != null)
			{
				Major = ParseVersionNumber(tokens.Length > 0 ? tokens[0] : null);
				Minor = ParseVersionNumber(tokens.Length > 1 ? tokens[1] : null);
				Build = ParseVersionNumber(tokens.Length > 2 ? tokens[2] : null);
				Revision = ParseVersionNumber(tokens.Length > 3 ? tokens[3] : null);
			}
		}

		public VersionDescriptor(ushort major, ushort minor, ushort build, ushort revision)
		{
			Major = major;
			Minor = minor;
			Build = build;
			Revision = revision;
		}

		public ushort Major
		{
			get;
			private set;
		}

		public ushort Minor
		{
			get;
			private set;
		}

		public ushort Build
		{
			get;
			private set;
		}

		public ushort Revision
		{
			get;
			private set;
		}

		public override string ToString()
		{
			return String.Format("{0}.{1}.{2}.{3}", Major, Minor, Build, Revision);
		}

		private static ushort ParseVersionNumber(string text)
		{
			ushort value = 0;
			if (String.IsNullOrWhiteSpace(text) == false)
				ushort.TryParse(text, out value);
			return value;
		}
	}	
}
