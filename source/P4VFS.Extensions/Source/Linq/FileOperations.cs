// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System.Text;
using System.Text.RegularExpressions;

namespace Microsoft.P4VFS.Extensions.Linq
{
	public static class FileOperations
	{
		private static readonly string RegExMetaChars = @"*?(){}[]+-^$.|\";

		public static Regex WildcardToRegex(this string wildcard)
		{
			StringBuilder regex = new StringBuilder();
			for (int i = 0; i < wildcard.Length; ++i)
			{
				int metaIndex = RegExMetaChars.IndexOf(wildcard[i]);
				if (metaIndex >= 0)
				{
					if (metaIndex == 0)
					{
						regex.Append(".*");
					}
					else if (metaIndex == 1)
					{
						regex.Append(".");
					}
					else
					{
						regex.Append("\\");
						regex.Append(wildcard[i]);
					}
				}
				else
				{
					regex.Append(wildcard[i]);
				}
			}

			return new Regex(regex.ToString(), RegexOptions.IgnoreCase);
		}
	}
}
