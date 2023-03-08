// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Runtime.InteropServices;

namespace Microsoft.P4VFS.Monitor
{
	public class NativeMethods
	{
		[StructLayout(LayoutKind.Sequential)]
		public struct ICONDIRHEADER 
		{
			public short idReserved;
			public short idType;
			public short idCount;
		};

		[StructLayout(LayoutKind.Sequential)]
		public struct ICONDIRENTRY 
		{
			public byte bWidth;
			public byte bHeight;
			public byte bColorCount;
			public byte bReserved;
			public short wPlanes;
			public short wBitCount;
			public int dwBytesInRes;
			public int dwImageOffset;
		};

		[DllImport("user32.dll", CharSet = CharSet.Auto)]
		public extern static bool DestroyIcon(IntPtr handle);
	}
}

