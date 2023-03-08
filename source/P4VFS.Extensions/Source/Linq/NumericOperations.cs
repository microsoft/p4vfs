// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;

namespace Microsoft.P4VFS.Extensions.Linq
{
	public static class NumericOperations
	{
		public static string PluralCharacter(this int count)
		{
			return count > 1 ? "s" : "";
		}

		public static string PluralCharacter(this long count)
		{
			return count > 1 ? "s" : "";
		}

		public static string PluralCharacter(this double count)
		{
			return count > 1 ? "s" : "";
		}
	}
}
