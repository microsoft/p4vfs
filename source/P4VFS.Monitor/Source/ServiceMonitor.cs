// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.IO;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Windows;
using System.Windows.Threading;
using System.Runtime.InteropServices;
using Microsoft.P4VFS.Extensions.Linq;
using Forms = System.Windows.Forms;

namespace Microsoft.P4VFS.Monitor
{
	public class ServiceMonitor
	{
		private DateTime _StartTime;
		private Sample[] _Samples;
		private TimeSpan _SampleInterval; 
		private TimeSpan _SampleDuration;
		private int _SamplesEnd;
		private object _SampleLock;
		private System.Threading.Thread _SampleThread;
		private System.Threading.CancellationTokenSource _SampleThreadCancellation;

		public ServiceMonitor(int numSamples, TimeSpan sampleInterval, TimeSpan sampleDuration)
		{
			_StartTime = DateTime.MinValue;
			_Samples = new Sample[numSamples];
			_SampleInterval = sampleInterval;
			_SampleDuration = sampleDuration;
			_SamplesEnd = 0;
			_SampleLock = new object();
		}

		public void Start()
		{
			if (_SampleThread != null)
				throw new Exception("ServiceMonitor.Start SampleThread already started");

			_SampleThreadCancellation = new System.Threading.CancellationTokenSource();
			_SampleThread = new System.Threading.Thread(new System.Threading.ThreadStart(ProcessThread));
			_SampleThread.IsBackground = true;
			_SampleThread.Start();
		}

		public void Stop()
		{
			if (_SampleThread == null)
				throw new Exception("ServiceMonitor.Stop SampleThread not started");

			_SampleThreadCancellation.Cancel();
			_SampleThread.Join();
			_SampleThread = null;
		}

		public float[] GetSampleValues()
		{
			lock (_SampleLock)
			{
				float[] values = new float[_Samples.Length];
				for (int i = 0; i < _Samples.Length; ++i)
				{
					Sample s = _Samples[(_SamplesEnd+i+1) % _Samples.Length];
					values[i] = s.Count > 0 ? (float)s.Value / (float)s.Count : 0.0f;
				}
				return values;
			}
		}

		private void ProcessThread()
		{
			System.Threading.CancellationToken token = _SampleThreadCancellation.Token;
			while (token.IsCancellationRequested == false)
			{
				UpdateSample();
				System.Threading.Thread.Sleep((int)_SampleInterval.TotalMilliseconds);
			}
		}

		private void UpdateSample()
		{
			try
			{
				Extensions.SocketModel.SocketModelClient serviceClient = new Extensions.SocketModel.SocketModelClient();
				Extensions.SocketModel.SocketModelReplyServiceStatus status = serviceClient.GetServiceStatus();
				if (status != null)
				{
					DateTime now = DateTime.Now;
					DateTime sampleTime = status.LastModifiedTime;
					lock (_SampleLock)
					{
						_Samples[_SamplesEnd].Value += sampleTime.Add(_SampleDuration) > now ? 1 : 0;
						_Samples[_SamplesEnd].Count += 1;
						
						if (now > _StartTime.Add(_SampleDuration))
						{
							_StartTime = now;
							_SamplesEnd = (_SamplesEnd+1) % _Samples.Length;
							_Samples[_SamplesEnd].Value = 0;
							_Samples[_SamplesEnd].Count = 0;
						}
					}
				}
			}
			catch {}
		}

		private struct Sample
		{
			public int Value;
			public int Count;
		}
	}
}
