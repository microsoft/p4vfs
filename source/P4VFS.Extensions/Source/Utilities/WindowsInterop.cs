// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;

namespace Microsoft.P4VFS.Extensions.Utilities
{
	public static class WindowsInterop
	{
		public const UInt32 FILE_ATTRIBUTE_READONLY        = 0x00000001;
		public const UInt32 FILE_ATTRIBUTE_HIDDEN          = 0x00000002;
		public const UInt32 FILE_ATTRIBUTE_SYSTEM          = 0x00000004;
		public const UInt32 FILE_ATTRIBUTE_DIRECTORY       = 0x00000010;
		public const UInt32 FILE_ATTRIBUTE_ARCHIVE         = 0x00000020;
		public const UInt32 FILE_ATTRIBUTE_DEVICE          = 0x00000040;
		public const UInt32 FILE_ATTRIBUTE_NORMAL          = 0x00000080;
		public const UInt32 FILE_ATTRIBUTE_TEMPORARY       = 0x00000100;
		public const UInt32 FILE_ATTRIBUTE_SPARSE_FILE     = 0x00000200;
		public const UInt32 FILE_ATTRIBUTE_REPARSE_POINT   = 0x00000400;
		public const UInt32 FILE_ATTRIBUTE_COMPRESSED      = 0x00000800;
		public const UInt32 FILE_ATTRIBUTE_OFFLINE         = 0x00001000;

		public static readonly IntPtr FILE_INVALIDHANDLE   = new IntPtr(-1);

		[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
		public struct WIN32_FIND_DATA
		{
			public uint dwFileAttributes;
			public System.Runtime.InteropServices.ComTypes.FILETIME ftCreationTime;
			public System.Runtime.InteropServices.ComTypes.FILETIME ftLastAccessTime;
			public System.Runtime.InteropServices.ComTypes.FILETIME ftLastWriteTime;
			public uint nFileSizeHigh;
			public uint nFileSizeLow;
			public uint dwReserved0;
			public uint dwReserved1;
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
			public string cFileName;
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 14)]
			public string cAlternateFileName;
		}

		[DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
		public static extern IntPtr FindFirstFile(
			string lpFileName, 
			ref WIN32_FIND_DATA lpFindFileData
			);

		[DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
		[return: MarshalAs(UnmanagedType.Bool)]
		public static extern bool FindNextFile(
			IntPtr hFindFile, 
			ref WIN32_FIND_DATA lpFindFileData
			);

		[DllImport("kernel32.dll")]
		[return: MarshalAs(UnmanagedType.Bool)]
		public static extern bool FindClose(
			IntPtr hFindFile
			);

		public const int S_OK							= 0;
		public const int NO_ERROR						= 0;
		public const int ERROR_SUCCESS					= 0;
		public const int ERROR_INVALID_FUNCTION			= 1;
		public const int ERROR_FILE_NOT_FOUND			= 2;
		public const int ERROR_PATH_NOT_FOUND			= 3;
		public const int ERROR_TOO_MANY_OPEN_FILES		= 4;
		public const int ERROR_ACCESS_DENIED			= 5;
		public const int ERROR_INVALID_HANDLE			= 6;
		public const int ERROR_ALREADY_EXISTS			= 183;

		[DllImport("kernel32", SetLastError=true)]
		public static extern int CreateDirectory(
			string lpPathName, 
			IntPtr lpSecurityAttributes
			);
		
		[DllImport("shell32.dll")]
		public static extern IntPtr CommandLineToArgvW(
			[MarshalAs(UnmanagedType.LPWStr)] string lpCmdLine, 
			out int pNumArgs
			);
		
		[DllImport("kernel32.dll")]
		public static extern IntPtr LocalFree(
			IntPtr hMem
			);

		[DllImport("kernel32.dll", SetLastError=true)]
		public static extern bool AttachConsole(
			uint dwProcessId
			);

		[DllImport("kernel32", SetLastError=true)]
		public static extern bool FreeConsole(
			);

		[DllImport("kernel32.dll", CharSet=CharSet.Unicode)]
		public static extern int CloseHandle(
			IntPtr hObject
			);

		public const uint ATTACH_PARENT_PROCESS = 0xFFFFFFFF;

		[DllImport("kernel32.dll")]
		public static extern bool CreateHardLink(
			string lpFileName, 
			string lpExistingFileName, 
			IntPtr lpSecurityAttributes
			);

		[StructLayout(LayoutKind.Sequential)]
		public struct IO_COUNTERS
		{
			public UInt64 ReadOperationCount;
			public UInt64 WriteOperationCount;
			public UInt64 OtherOperationCount;
			public UInt64 ReadTransferCount;
			public UInt64 WriteTransferCount;
			public UInt64 OtherTransferCount;
		}

		[StructLayout(LayoutKind.Sequential)]
		public struct JOBOBJECT_BASIC_LIMIT_INFORMATION
		{
			public Int64 PerProcessUserTimeLimit;
			public Int64 PerJobUserTimeLimit;
			public UInt32 LimitFlags;
			public UIntPtr MinimumWorkingSetSize;
			public UIntPtr MaximumWorkingSetSize;
			public UInt32 ActiveProcessLimit;
			public UIntPtr Affinity;
			public UInt32 PriorityClass;
			public UInt32 SchedulingClass;
		}

		[StructLayout(LayoutKind.Sequential)]
		public struct SECURITY_ATTRIBUTES
		{
			public UInt32 nLength;
			public IntPtr lpSecurityDescriptor;
			public Int32 bInheritHandle;
		}

		[StructLayout(LayoutKind.Sequential)]
		public struct JOBOBJECT_EXTENDED_LIMIT_INFORMATION
		{
			public JOBOBJECT_BASIC_LIMIT_INFORMATION BasicLimitInformation;
			public IO_COUNTERS IoInfo;
			public UIntPtr ProcessMemoryLimit;
			public UIntPtr JobMemoryLimit;
			public UIntPtr PeakProcessMemoryUsed;
			public UIntPtr PeakJobMemoryUsed;
		}

		public enum JobObjectInfoType
		{
			BasicLimitInformation				= 2,
			BasicUIRestrictions					= 4,
			SecurityLimitInformation			= 5,
			EndOfJobTimeInformation				= 6,
			AssociateCompletionPortInformation	= 7,
			ExtendedLimitInformation			= 9,
			GroupInformation					= 11
		}

		public const Int16 JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE = 0x2000;

		[DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
		public static extern IntPtr CreateJobObject(
			IntPtr lpSecurityAttributes, 
			string lpName
			);

		[DllImport("kernel32.dll", SetLastError=true)]
		public static extern bool AssignProcessToJobObject(
			IntPtr hJob, 
			IntPtr hProcess
			);

		[DllImport("kernel32.dll", SetLastError=true)]
		public static extern bool SetInformationJobObject(
			IntPtr hJob, 
			JobObjectInfoType JobObjectInfoClass, 
			IntPtr lpJobObjectInfo, 
			uint cbJobObjectInfoLength
			);

		public const uint INFINITE			= 0xFFFFFFFF;
		public const uint STATUS_WAIT_0		= 0x00000000;
		public const uint WAIT_FAILED		= 0xFFFFFFFF;
		public const uint WAIT_OBJECT_0		= STATUS_WAIT_0;

		[DllImport("kernel32.dll", SetLastError=true)]
		public static extern uint WaitForMultipleObjects(
			uint nCount, 
			IntPtr[] lpHandles, 
			bool bWaitAll, 
			uint dwMilliseconds
			);

		public enum ConsoleCtrlEvent
		{
			CTRL_C = 0,
			CTRL_BREAK = 1,
			CTRL_CLOSE = 2,
			CTRL_LOGOFF = 5,
			CTRL_SHUTDOWN = 6
		}

		[Flags]
		public enum ThreadAccess : int
		{
			TERMINATE = 0x00000001,
			SUSPEND_RESUME = 0x00000002,
			GET_CONTEXT = 0x00000008,
			SET_CONTEXT = 0x00000010,
			SET_INFORMATION = 0x00000020,
			QUERY_INFORMATION = 0x00000040,
			SET_THREAD_TOKEN = 0x00000080,
			IMPERSONATE = 0x00000100,
			DIRECT_IMPERSONATION = 0x00000200
		}

		[DllImport("kernel32.dll", SetLastError = true)]
		public static extern bool GenerateConsoleCtrlEvent(
			ConsoleCtrlEvent sigevent, 
			int dwProcessGroupId
			);

		[DllImport("kernel32.dll")]
		public static extern IntPtr OpenThread(
			ThreadAccess dwDesiredAccess, 
			bool bInheritHandle, 
			uint dwThreadId
			);

		[DllImport("kernel32.dll")]
		public static extern uint SuspendThread(
			IntPtr hThread
			);

		public static string[] CommandLineToArgs(string commandLine)
		{
			string[] result = null;
			IntPtr ptrToSplitArgs = CommandLineToArgvW(commandLine, out int numberOfArgs);
			if (ptrToSplitArgs != IntPtr.Zero)
			{
				try
				{
					string[] splitArgs = new string[numberOfArgs];
					for (int i = 0; i < numberOfArgs; i++)
					{
						splitArgs[i] = Marshal.PtrToStringUni(Marshal.ReadIntPtr(ptrToSplitArgs, i * IntPtr.Size));
					}
					result = splitArgs;
				}
				catch {}
				LocalFree(ptrToSplitArgs);
			}
			return result;
		}
	}
}
