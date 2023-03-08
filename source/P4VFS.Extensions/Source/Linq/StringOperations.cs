// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Microsoft.P4VFS.Extensions.Linq
{
	public static class StringOperations
	{
		public static bool IEquals(this string a, string b)
		{
			return a.Equals(b, StringComparison.InvariantCultureIgnoreCase);
		}

		public static bool IStartsWith(this string a, string b)
		{
			return a.StartsWith(b, StringComparison.InvariantCultureIgnoreCase);
		}

		public static bool IEndsWith(this string a, string b)
		{
			return a.EndsWith(b, StringComparison.InvariantCultureIgnoreCase);
		}

		public static bool IContains(this string a, string b)
		{
			return a.IndexOf(b, StringComparison.InvariantCultureIgnoreCase) >= 0;
		}

		public static string IReplace(this string a, string b, string c)
		{
			return a.Replace(b, c, StringComparison.InvariantCultureIgnoreCase);
		}

		public static string Replace(this string a, string b, string c, StringComparison comparison)
		{
			int index = a.IndexOf(b, comparison);
			if (index >= 0)
				return a.Replace(a.Substring(index, b.Length), c);
			return a;
		}

		public static string NullIfEmpty(this string text)
		{
			return String.IsNullOrEmpty(text) ? null : text;
		}

		public static string ToNiceString(this IEnumerable<string> values)
		{
			if (values != null && values.Any())
				return String.Format("[\"{0}\"]", String.Join("\",\"", values));
			return "[]";
		}

		public static int ToInt32(this string input, int defaultValue)
		{
			return Utilities.Converters.ToInt32(input, defaultValue).Value;
		}

		public static string ToStatusString(this bool status)
		{
			return status ? "SUCCEEDED" : "FAILED";
		}
	}
}