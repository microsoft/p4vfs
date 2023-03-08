// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Collections.Generic;
using System.Linq;

namespace Microsoft.P4VFS.Extensions.Linq
{
	public static class TimeSpanOperations
	{
		public static TimeSpan Sum(this IEnumerable<TimeSpan> source)
		{
			return new TimeSpan(source.Select(timeSpan => timeSpan.Ticks).Sum());
		}

		public static TimeSpan Sum<T>(this IEnumerable<T> source, Func<T, TimeSpan> selector)
		{
			return new TimeSpan(source.Select(t => selector(t).Ticks).Sum());
		}
	}
}
