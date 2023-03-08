// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.IO;
using System.Linq;
using Microsoft.P4VFS.CoreInterop;

namespace Microsoft.P4VFS.Extensions.Utilities
{
	public static class Impersonation
	{
		public static bool ImpersonateLoggedOnUser(UserContext context)
		{
			return NativeMethods.ImpersonateLoggedOnUser(context);
		}

		public static bool RevertToSelf()
		{
			return NativeMethods.RevertToSelf();
		}

		public class ImpersonateLoggedOnUserScope : IDisposable
		{
			public ImpersonateLoggedOnUserScope(UserContext context)
			{
				ImpersonateLoggedOnUser(context);
			}

			public void Dispose()
			{
				RevertToSelf();
			}
		}
	}
}

