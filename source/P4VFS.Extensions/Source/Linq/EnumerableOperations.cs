// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using IEnumerable = System.Collections.IEnumerable;

namespace Microsoft.P4VFS.Extensions.Linq
{
	public static class EnumerableOperations
	{
		[DebuggerNonUserCode]
		public static IEnumerable<T> ForEach<T>(this IEnumerable<T> list, Action<T> action)
		{
			foreach (var item in list)
			{
				action(item);
			}
			return list;
		}

		[DebuggerNonUserCode]
		public static IEnumerable<T> Concat<T>(this IEnumerable<T> list, T item)
		{
			return list.Concat(new[] { item });
		}

		[DebuggerNonUserCode]
		public static IEnumerable<T> Flatten<T>(this IEnumerable<T> list, Func<T, IEnumerable<T>> func)
		{
			IEnumerable<T> result = FlattenInternal<T>(list, func);
			return result;
		}

		[DebuggerNonUserCode]
		public static IEnumerable<T> Flatten<T>(this IEnumerable list, Func<T, IEnumerable<T>> func)
		{
			IEnumerable<T> result = FlattenInternal<T>(list.Cast<T>(), func);
			return result;
		}

		[DebuggerNonUserCode]
		private static IEnumerable<T> FlattenInternal<T>(this IEnumerable<T> list, Func<T, IEnumerable<T>> func)
		{
			List<T> result = new List<T>();

			if (list != null)
			{
				foreach (T t in list)
				{
					result.Add(t);
					result.AddRange(FlattenInternal(func(t), func));
				}
			}

			return result;
		}

		// from http://www.claassen.net/geek/blog/2009/06/searching-tree-of-objects-with-linq.html
		[DebuggerNonUserCode]
		public static IEnumerable<T> Flatten<T>(T head, Func<T, IEnumerable<T>> childrenFunc)
		{
			yield return head;
			foreach (var node in childrenFunc(head))
			{
				foreach (var child in Flatten(node, childrenFunc))
				{
					yield return child;
				}
			}
		}

		// from http://www.claassen.net/geek/blog/2009/06/searching-tree-of-objects-with-linq.html
		// prefer the ordinary Flatten() unless you really need breadth-first (this func is a little less efficient, and requires far more visits to find leaves)
		[DebuggerNonUserCode]
		public static IEnumerable<T> FlattenBreadthFirst<T>(T head, Func<T, IEnumerable<T>> childrenFunc)
		{
			yield return head;
			var last = head;
			foreach (var node in FlattenBreadthFirst(head, childrenFunc))
			{
				foreach (var child in childrenFunc(node))
				{
					yield return child;
					last = child;
				}
				if (last.Equals(node)) yield break;
			}
		}

		[DebuggerNonUserCode]
		public static int IndexOf<T>(this IEnumerable<T> enumerable, T elementToFind)
		{
			return IndexOfInternal(enumerable, elementToFind, null);
		}

		[DebuggerNonUserCode]
		public static int IndexOf<T>(this IEnumerable<T> enumerable, Predicate<T> matcher)
		{
			return IndexOfInternal(enumerable, default(T), matcher);
		}

		[DebuggerNonUserCode]
		private static int IndexOfInternal<T>(IEnumerable<T> enumerable, T elementToFind, Predicate<T> matcher)
		{
			int result = -1;
			int index = 0;

			foreach (T t in enumerable)
			{
				if ((matcher != null && matcher.Invoke(t)) || (matcher == null && t.Equals(elementToFind)))
				{
					result = index;
					break;
				}

				index++;
			}

			return result;
		}

		[DebuggerNonUserCode]
		public static bool ElementsEqual<T>(this IEnumerable<T> enumerableA, IEnumerable<T> enumerableB)
		{
			IEnumerator<T> enumeratorA = enumerableA.GetEnumerator();
			IEnumerator<T> enumeratorB = enumerableB.GetEnumerator();

			bool result = true;

			bool aHasMoreElements = enumeratorA.MoveNext();
			bool bHasMoreElements = enumeratorB.MoveNext();

			while (aHasMoreElements && bHasMoreElements)
			{
				if (!enumeratorA.Current.Equals(enumeratorB.Current))
				{
					result = false;
					break;
				}

				aHasMoreElements = enumeratorA.MoveNext();
				bHasMoreElements = enumeratorB.MoveNext();
			}

			result = result && (aHasMoreElements == bHasMoreElements);

			return result;
		}

		[DebuggerNonUserCode]
		public static Dictionary<K, IEnumerable<T>> ToDictionaryGroup<K, T>(this IEnumerable<T> list, Func<T, K> getKeyFunc)
		{
			Dictionary<K, IEnumerable<T>> result = new Dictionary<K, IEnumerable<T>>();

			foreach (T item in list)
			{
				K key = getKeyFunc(item);
				if (result.ContainsKey(key))
				{
					List<T> existingList = (List<T>)result[key];
					existingList.Add(item);
				}
				else
				{
					List<T> newList = new List<T>();
					newList.Add(item);
					result.Add(key, newList);
				}
			}

			return result;
		}

		[DebuggerNonUserCode]
		public static HashSet<T> ToHashSet<T>(this IEnumerable<T> list, IEqualityComparer<T> comparer)
		{
			return new HashSet<T>(list, comparer);
		}

		[DebuggerNonUserCode]
		public static HashSet<T> ToHashSet<T>(this IEnumerable<T> list)
		{
			return new HashSet<T>(list);
		}

		[DebuggerNonUserCode]
		public static bool IsEmpty(this IEnumerable list)
		{
			// optimization
			var collection = list as ICollection;
			if (collection != null)
			{
				return collection.Count == 0;
			}

			return !list.GetEnumerator().MoveNext();
		}

		[DebuggerNonUserCode]
		public static IEnumerable<T> EmptyIfNull<T>(this IEnumerable<T> list)
		{
			return list ?? Enumerable.Empty<T>();
		}

		[DebuggerNonUserCode]
		public static IEnumerable<T> RepeatAll<T>(this IEnumerable<T> list, int count)
		{
			for (int i = 0; (i < count) || (count < 0); ++i)
			{
				foreach (var item in list)
				{
					yield return item;
				}
			}
		}

		[DebuggerNonUserCode]
		public static IEnumerable<T> RepeatAll<T>(this IEnumerable<T> list)
		{
			return list.RepeatAll(-1);
		}

		// ###scobi $TODO: does this duplicate Enumerable.StartWith? - 06-09-10
		[DebuggerNonUserCode]
		public static IEnumerable<T> Prepend<T>(this IEnumerable<T> list, T previousItem)
		{
			yield return previousItem;

			foreach (var item in list)
			{
				yield return item;
			}
		}

		[DebuggerNonUserCode]
		public static IEnumerable<T> Append<T>(this IEnumerable<T> list, T additionalItem)
		{
			foreach (var item in list)
			{
				yield return item;
			}

			yield return additionalItem;
		}

		public class EqualityComparer<T> : IEqualityComparer<T>
		{
			Func<T, T, bool> _equalityComparer;
			Func<T, int> _getHashCode;

			public EqualityComparer(Func<T, T, bool> equalityComparer, Func<T, int> getHashCode)
			{
				_equalityComparer = equalityComparer;
				_getHashCode = getHashCode;
			}

			public bool Equals(T a, T b)
			{
				return _equalityComparer(a, b);
			}

			public int GetHashCode(T t)
			{
				return _getHashCode(t);
			}
		}

		public static IEnumerable<T> Distinct<T>(this IEnumerable<T> list, Func<T, T, bool> equalityComparer, Func<T, int> getHashCode)
		{
			return list.Distinct(new EqualityComparer<T>(equalityComparer, getHashCode));
		}
	}
}
