// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Diagnostics;
using System.Linq;
using System.ComponentModel;
using System.Collections.Generic;
using Microsoft.P4VFS.CoreInterop;

namespace Microsoft.P4VFS.Extensions.Utilities
{
	public static class ThreadPool
	{
		public static void ForEach<T>(int maxThreads, IEnumerable<T> items, Action<CancelEventArgs,T> action, Func<bool> initialize = null, Func<bool> shutdown = null)
		{
			int numThreads = Math.Max(0, Math.Min(maxThreads, items.Count()));
			if (numThreads == 0)
				return;

			var itemQueue = new System.Collections.Concurrent.ConcurrentQueue<T>(items);
			bool cancelRequested = false;
			Action threadAction = () => 
			{
				if (initialize != null && initialize() == false)
				{
					VirtualFileSystemLog.Error("ThreadPool.ForEach failed to initialize action");
					return;
				}

				T item;
				while (cancelRequested == false && itemQueue.TryDequeue(out item))
				{
					try
					{
						CancelEventArgs e = new CancelEventArgs(false);
						action(e, item);
						if (e.Cancel)
						{
							VirtualFileSystemLog.Verbose("ThreadPool.ForEach cancel requested from EventArgs");
							cancelRequested = true;
						}
					}
					catch (Exception e)
					{
						VirtualFileSystemLog.Verbose("ThreadPool.ForEach cancel requested from Exception: {0}", e);
						cancelRequested = true;
					}
				}

				if (shutdown != null && shutdown() == false)
				{
					VirtualFileSystemLog.Error("ThreadPool.ForEach failed to shutdown action");
					return;
				}
			};

			var threads = new System.Threading.Thread[numThreads];
			try
			{
				for (int i = 0; i < numThreads; ++i)
				{
					threads[i] = new System.Threading.Thread(new System.Threading.ThreadStart(threadAction));
					threads[i].IsBackground = true;
					threads[i].Start();
				}
			}
			catch (Exception e)
			{
				VirtualFileSystemLog.Error("ThreadPool.ForEach failed to start action threads: {0}", e);
			}

			for (int i = 0; i < numThreads; ++i)
			{
				try
				{		
					if (threads[i] != null)
						threads[i].Join();
				}
				catch (Exception e)
				{
					VirtualFileSystemLog.Error("ThreadPool.ForEach failed to join action thread[{0}]: {1}", i, e);
				}
			}
		}

		public static void ForEachImpersonated<T>(int maxThreads, UserContext context, IEnumerable<T> items, Action<CancelEventArgs,T> action)
		{
			Func<bool> impersonateStart = () => { return Impersonation.ImpersonateLoggedOnUser(context); };
			Func<bool> impersonateEnd = () => { return Impersonation.RevertToSelf(); };
			ForEach(maxThreads, items, action, impersonateStart, impersonateEnd);
		}
	}
}