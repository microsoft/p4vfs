// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace Microsoft.P4VFS.Extensions.Linq
{
	public static class ListOperations
	{
		public static int AddRange(this IList collection, IEnumerable items)
		{
			int count = 0;
			foreach (var t in items)
			{
				collection.Add(t);
				++count;
			}
			return count;
		}

		public static bool AddUnique<TSource>(this IList<TSource> collection, TSource item, IEqualityComparer<TSource> comparer = null)
		{
			if (collection.Contains(item, comparer ?? EqualityComparer<TSource>.Default))
			{
				collection.Add(item);
				return true;
			}
			return false;
		}
	}
}
