// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;

namespace Microsoft.P4VFS.Extensions.Linq
{
	public static class DateTimeOperations
	{
		/// <summary>
		/// Keeps the DateTime value the same, but makes sure that its DateTime.Kind value is set to UTC.
		/// </summary>
		/// <param name="dateTime">DateTime to use as Universal Time.</param>
		/// <returns></returns>
		public static DateTime AsUniversalTime(this DateTime dateTime)
		{
			return new DateTime(dateTime.Ticks, DateTimeKind.Utc);
		}

		public static string ToNiceTimeString(this DateTime dateTime)
		{
			return dateTime.ToString("HH:mm:ss");
		}

		public static string ToNiceDateString(this DateTime dateTime)
		{
			return dateTime.ToString("yyyy-MM-dd");
		}

		public static string ToNiceDateAndTimeString(this DateTime dateTime)
		{
			return String.Format("{0}, {1}", dateTime.ToNiceDateString(), dateTime.ToNiceTimeString());
		}

		public static string ToSortableDateAndTimeString(this DateTime dateTime)
		{
			return dateTime.ToString("yyyy-MM-dd-HH-mm-ss");
		}
	}
}
