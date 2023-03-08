// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;

namespace Microsoft.P4VFS.Extensions.Linq
{
	public static class CollectionOperations
	{
		[DebuggerNonUserCode]
		public static bool IsEmpty(this ICollection collection)
		{
			return collection.Count == 0;
		}

		public static int AddRange<T, U>(this ICollection<T> collection, IEnumerable<U> items) where U : T
		{
			int count = 0;
			foreach (var t in items)
			{
				collection.Add(t);
				++count;
			}
			return count;
		}

		public static int SetRange<T, U>(this ICollection<T> collection, IEnumerable<U> items) where U : T
		{
			collection.Clear();
			return collection.AddRange(items);
		}

		public static int RemoveRange<T, U>(this ICollection<T> collection, IEnumerable<U> items) where U : T
		{
			return items.Count(item => collection.Remove(item));
		}

		public static int RemoveWhere<T>(this ICollection<T> collection, Func<T, bool> predicate)
		{
			var found = collection.Where(predicate).ToList();
			found.ForEach(a => collection.Remove(a));
			return found.Count;
		}

		public static int RemoveWhere<T>(this IList<T> list, Func<T, bool> predicate)
		{
			int count = 0;
			for (int i = 0; i < list.Count; )
			{
				var element = list.ElementAt(i);
				if (predicate(element))
				{
					list.RemoveAt(i);
					++count;
				}
				else
				{
					++i;
				}
			}
			return count;
		}

	}
}
