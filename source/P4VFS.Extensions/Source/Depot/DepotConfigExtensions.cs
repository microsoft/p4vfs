// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Linq;
using System.Reflection;
using System.Collections.Generic;
using System.Xml.Serialization;
using Microsoft.P4VFS.CoreInterop;
using Newtonsoft.Json;

namespace Microsoft.P4VFS.Extensions
{
	public static class DepotConfigExtensions
	{
		public static void ApplyConnection(this DepotConfig dst, DepotConfig src)
		{
			if (dst == null || src == null)
			{
				return;
			}
			if (String.IsNullOrEmpty(src.Port) == false)
			{
				dst.Port = src.Port;
			}
			if (String.IsNullOrEmpty(src.User) == false)
			{
				dst.User = src.User;
			}
			if (String.IsNullOrEmpty(src.Client) == false)
			{
				dst.Client = src.Client;
			}
		}

		public static System.Net.IPAddress PortAddress(this DepotConfig dst)
		{
			try
			{
				var addrs = System.Net.Dns.GetHostAddresses(dst.PortName());
				if (addrs.Length > 0 && addrs[0] != null)
				{
					return addrs[0];
				}
			}
			catch {}
			return System.Net.IPAddress.None;
		}

		public static string PortName(this DepotConfig dst)
		{
			string[] tokens = (dst.Port ?? "").Split(new char[]{':'}, StringSplitOptions.RemoveEmptyEntries);
			return (tokens.Length > 0 ? tokens[0] : "");
		}

		public static string PortNumber(this DepotConfig dst)
		{
			string[] tokens = (dst.Port ?? "").Split(new char[]{':'}, StringSplitOptions.RemoveEmptyEntries);
			return (tokens.Length > 1 ? tokens[1] : "");
		}
	}
}
