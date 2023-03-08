// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Linq;

namespace Microsoft.P4VFS.Extensions
{
	public interface ServiceHost
	{
		bool IsDriverConnected();
		DateTime GetLastRequestTime();
		DateTime GetLastModifiedTime();
		bool GarbageCollect(Int64 timeout);
	}
}
