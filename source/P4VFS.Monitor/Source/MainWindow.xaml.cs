// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.IO;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Windows;
using System.Windows.Threading;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using Microsoft.P4VFS.Extensions;
using Forms = System.Windows.Forms;

namespace Microsoft.P4VFS.Monitor
{
	public partial class MainWindow : Window
	{
		public const int SampleIntervalMs = 200;
		public const int SampleDurationMs = 1000;
		public readonly int[] SampleBitmapSizes = new int[]{ 16 /*, 18, 20, 22, 24*/ };
		public readonly Color SampleHighliteColor = Color.FromArgb(30, 60, 200);
		public readonly Color SampleBackgroundColor = Color.FromArgb(143, 158, 228);
		public readonly Color SampleBorderColor = Color.FromArgb(130, 130, 130);
		public readonly Color IconBackgroundColor = Color.FromArgb(240, 240, 240);

		private Forms.NotifyIcon _NotifyIcon;
		private SampleImage[] _SampleImages;
		private DateTime _LogFileCheckTime;
		private DateTime _LogNotifyIconCheckTime;
		private string _LogFile;
		private string[] _LogFolders;
		private DispatcherTimer _LogUpdateTimer;
		private ServiceMonitor _ServiceMonitor;

		public MainWindow()
		{
			InitializeComponent();
		}

		protected override void OnInitialized(EventArgs e)
		{
			base.OnInitialized(e);

			_NotifyIcon = new Forms.NotifyIcon();
			_NotifyIcon.MouseClick += OnNotifyIconClick;
			_NotifyIcon.MouseDoubleClick += OnNotifyIconDoubleClick;
			_NotifyIcon.ContextMenu = CreateNotifyIconContextMenu();
			_NotifyIcon.Visible = true;

			_SampleImages = new SampleImage[SampleBitmapSizes.Length];
			for (int imageIndex = 0; imageIndex < SampleBitmapSizes.Length; ++imageIndex)
			{
				_SampleImages[imageIndex] = new SampleImage();
				_SampleImages[imageIndex].Bitmap = new Bitmap(SampleBitmapSizes[imageIndex], SampleBitmapSizes[imageIndex], PixelFormat.Format24bppRgb);
			}

			_LogFolders = new string[] 
			{
				Environment.ExpandEnvironmentVariables(Path.Combine(ServiceSettings.FileLoggerRemoteDirectory, Environment.UserName, VirtualFileSystem.ServiceTitle)),
				Environment.ExpandEnvironmentVariables(Path.Combine(ServiceSettings.FileLoggerLocalDirectory, VirtualFileSystem.ServiceTitle))
			};

			_LogFile = FindLatestLogFilePath();
			_LogFileCheckTime = DateTime.MinValue;
			_LogNotifyIconCheckTime = DateTime.MinValue;

			_LogUpdateTimer = new DispatcherTimer();
			_LogUpdateTimer.Interval = TimeSpan.FromMilliseconds(SampleIntervalMs);
			_LogUpdateTimer.Tick += OnLogUpdateTimedEvent;
			_LogUpdateTimer.IsEnabled = true;
			_LogUpdateTimer.Start();

			_ServiceMonitor = new ServiceMonitor(SampleBitmapSizes.Max(), TimeSpan.FromMilliseconds(SampleIntervalMs), TimeSpan.FromMilliseconds(SampleDurationMs));
			_ServiceMonitor.Start();

			Visibility = Visibility.Hidden;
			ShowInTaskbar = false;
			Topmost = true;
			System.Drawing.Size windowSize = Forms.SystemInformation.PrimaryMonitorMaximizedWindowSize;
			Left = windowSize.Width - this.Width - 16;
			Top = windowSize.Height - this.Height - 16;
		}

		protected override void OnClosed(EventArgs e)
		{
			_ServiceMonitor.Stop();
		}

		private string FindLatestLogFilePath()
		{
			DateTime lastWriteTime = DateTime.MinValue;
			string lastWriteFile = null;
			foreach (string logFolder in _LogFolders)
			{
				foreach (string logFile in GetLogFilesInFolder(logFolder))
				{
					DateTime writeTime = GetFileLastWriteTime(logFile);
					if (writeTime > lastWriteTime)
					{
						lastWriteTime = writeTime;
						lastWriteFile = logFile;
					}
				}
			}
			return lastWriteFile;
		}

		private static string[] GetLogFilesInFolder(string logFolder)
		{
			try
			{
				if (Directory.Exists(logFolder))
					return Directory.GetFiles(logFolder, "*.log", SearchOption.AllDirectories).ToArray();
			}
			catch {}
			return new string[0];
		}

		private static DateTime GetFileLastWriteTime(string filePath)
		{
			try
			{
				if (File.Exists(filePath))
					return File.GetLastWriteTime(filePath);
			}
			catch {}
			return DateTime.MinValue;
		}

		private Forms.ContextMenu CreateNotifyIconContextMenu()
		{
			Forms.ContextMenu menu = new Forms.ContextMenu();
			menu.MenuItems.Add(new Forms.MenuItem("Close", (s, e) => Close()));
			return menu;
		}

		private void OnNotifyIconClick(object sender, Forms.MouseEventArgs e)
		{
			if (e.Button == Forms.MouseButtons.Left)
				Visibility = Visibility == Visibility.Visible ? Visibility.Hidden : Visibility.Visible;
		}

		private void OnNotifyIconDoubleClick(object sender, Forms.MouseEventArgs e)
		{
			string logFile = _LogFile;
			if (File.Exists(logFile))
				Dispatcher.BeginInvoke(new Action(() => Process.Start(logFile)));
		}

		private void ReloadLogFileTail()
		{
			try
			{
				if (File.Exists(_LogFile))
				{
					using (FileStream stream = System.IO.File.Open(_LogFile, FileMode.Open, FileAccess.Read, FileShare.ReadWrite))
					{
						const int NUM_LINES = 8;
						ReverseFileReader reader = new ReverseFileReader(stream);
						List<byte> textBuffer = new List<byte>();
						for (int count = 0; count <= NUM_LINES;)
						{
							byte c = reader.ReadByte();
							if (c == byte.MaxValue)
								break;
							if (c == '\r')
								continue;
							if (c == '\n')
								count++;
							textBuffer.Add(c);
						}

						textBuffer.Reverse();
						string text = System.Text.Encoding.ASCII.GetString(textBuffer.ToArray()).Trim();
						Dispatcher.BeginInvoke(DispatcherPriority.Background, new Action(() =>
						{
							PopupTextBlock.Text = text;
						}));
					}
				}
			}
			catch (Exception e)
			{
				Console.WriteLine("ReloadLogFileTail exception: {0}", e.Message);
			}
		}

		private void OnLogUpdateTimedEvent(object sender, EventArgs e)
		{
			DateTime now = DateTime.Now;
			if (Visibility == Visibility.Visible)
			{
				if (now > _LogFileCheckTime.AddSeconds(10.0))
				{
					_LogFile = FindLatestLogFilePath();
					_LogFileCheckTime = now;
				}
				ReloadLogFileTail();
			}

			if (now > _LogNotifyIconCheckTime.AddMilliseconds(SampleDurationMs))
			{
				UpdateNotifyIcon();
				_LogNotifyIconCheckTime = now;
			}
		}

		private void UpdateNotifyIcon()
		{
			try
			{
				Pen sampleBackgroundPen = new Pen(SampleBackgroundColor);
				Pen sampleBorderPen = new Pen(SampleBorderColor);
				float[] mipdata = _ServiceMonitor.GetSampleValues();
				for (int imageIndex = 0; imageIndex < _SampleImages.Length; ++imageIndex)
				{
					Bitmap mipmap = _SampleImages[imageIndex].Bitmap;
					System.Drawing.Point[] points = new System.Drawing.Point[mipmap.Width-2];
					using (Graphics g = Graphics.FromImage(mipmap))
					{
						g.Clear(IconBackgroundColor);
						for (int m = 0; m < points.Length; ++m)
						{
							int mi = mipdata.Length - points.Length + m - 1;
							float mv = mi >= 0 && mi < mipdata.Length ? mipdata[mi] : 0.0f;
							points[m].X = m+1;
							points[m].Y = (int)((float)(mipmap.Height-1) - (float)(mipmap.Height-2) * mv);
							g.DrawLine(sampleBackgroundPen, points[m].X, mipmap.Height-1, points[m].X, points[m].Y);
						}
						g.DrawLines(new Pen(SampleHighliteColor), points);
						g.Transform = new System.Drawing.Drawing2D.Matrix(1,0,0,1,0,1);
						g.DrawLines(new Pen(Color.FromArgb(128, SampleHighliteColor)), points);
						g.Transform = new System.Drawing.Drawing2D.Matrix();
						g.DrawRectangle(sampleBorderPen, new Rectangle(0, 0, mipmap.Width - 1, mipmap.Height - 1));
					}
				}

				if (_NotifyIcon.Icon != null)
					NativeMethods.DestroyIcon(_NotifyIcon.Icon.Handle);

				_NotifyIcon.Icon = CreateSampleIcon();
			}
			catch {}
		}

		private Icon CreateSampleIcon()
		{
			using (MemoryStream stream = new MemoryStream())
			{
				NativeMethods.ICONDIRHEADER header = new NativeMethods.ICONDIRHEADER();
				header.idType = 1;
				header.idCount = (short)_SampleImages.Length;
				WriteStruct(stream, header);

				long entryPos = stream.Position;
				NativeMethods.ICONDIRENTRY[] entries = new NativeMethods.ICONDIRENTRY[_SampleImages.Length];
				for (int entryIndex = 0; entryIndex < entries.Length; ++entryIndex)
					WriteStruct(stream, entries[entryIndex]);

				for (int entryIndex = 0; entryIndex < entries.Length; ++entryIndex)
				{
					Bitmap mipmap = _SampleImages[entryIndex].Bitmap;
					entries[entryIndex].bHeight = (byte)mipmap.Height;
					entries[entryIndex].bWidth = (byte)mipmap.Width;
					entries[entryIndex].wBitCount = (short)Bitmap.GetPixelFormatSize(mipmap.PixelFormat);
					entries[entryIndex].dwImageOffset = (int)stream.Position;
					entries[entryIndex].dwBytesInRes = WriteImage(stream, mipmap, ImageFormat.Png);
				}

				stream.Seek(entryPos, SeekOrigin.Begin);
				for (int entryIndex = 0; entryIndex < entries.Length; ++entryIndex)
					WriteStruct(stream, entries[entryIndex]);

				stream.Seek(0, SeekOrigin.Begin);
				System.Drawing.Icon icon = new System.Drawing.Icon(stream);
				return icon;
			}
		}

		private static int WriteImage(Stream stream, Bitmap image, ImageFormat format)
		{
			long startPos = stream.Position;
			image.Save(stream, format);
			long length = stream.Position - startPos;
			return (int)length;
		}

		public static int WriteStruct<T>(Stream stream, T value) where T : struct
		{
			byte[] valueBytes = StructToBytes(value);
			stream.Write(valueBytes, 0, valueBytes.Length);
			return valueBytes.Length;
		}

		public static byte[] StructToBytes<T>(T value) where T : struct
		{
			int valueSize = Marshal.SizeOf(typeof(T));
			byte[] valueBytes = new byte[valueSize];
			IntPtr valuePtr = Marshal.AllocHGlobal(valueSize);
			try
			{
				Marshal.StructureToPtr(value, valuePtr, false);
				Marshal.Copy(valuePtr, valueBytes, 0, valueSize);
			}
			finally
			{
				Marshal.FreeHGlobal(valuePtr);
			}
			return valueBytes;			
		}

		private void OnClickClose(object sender, RoutedEventArgs e)
		{
			Visibility = Visibility.Hidden;
		}

		private class ReverseFileReader
		{
			private FileStream _Stream;
			private byte[] _BlockBuffer;
			private long _BlockStart;
			private int _BlockPosition;

			public ReverseFileReader(FileStream stream)
			{
				stream.Seek(0, SeekOrigin.End);
				_Stream = stream;
				_BlockBuffer = new byte[1024];
				_BlockStart = _Stream.Position;
				_BlockPosition = -1;
			}

			public byte ReadByte()
			{
				if (_BlockPosition < 0)
				{
					long nextBlockEnd = _BlockStart;
					long nextBlockStart = Math.Max(0, nextBlockEnd-_BlockBuffer.Length);
					int blockLength = (int)(nextBlockEnd-nextBlockStart);
					_Stream.Position = nextBlockStart;
					_Stream.Read(_BlockBuffer, 0, blockLength);
					_BlockStart = nextBlockStart;
					_BlockPosition = blockLength-1;
				}

				if (_BlockPosition < 0)
					return byte.MaxValue;
				
				return _BlockBuffer[_BlockPosition--];
			}
		}

		private class SampleImage
		{
			public Bitmap Bitmap;
		}
	}
}
