// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;
using System.Security;
using System.Security.Permissions;
using Microsoft.Win32;
using Microsoft.Win32.SafeHandles;

namespace Microsoft.P4VFS.Extensions.Utilities
{
	/// <summary>
	/// Class to help with the nitty-gritty details of accessing the two registries (32 and 64 bit)
	/// from managed code.
	/// </summary>
	public static class RegistryInfo
	{
		[System.ThreadStatic]
		private static string _StrRegError = null;

		/// <summary>
		/// Gets the error message of the last operation (null when no error occurred).
		/// </summary>
		public static string StrRegError
		{
			get { return _StrRegError; }
			private set { _StrRegError = value; }
		}

		/// <summary>
		/// Retrieves the specified key/value and casts to type T, where possible. Expected errors (such as could not open key)
		/// stored in StrRegError. Use GetValueType() or T=object if uncertain what type of value the key stores.
		/// </summary>
		/// <typeparam name="T">The type to cast the key value to.</typeparam>
		/// <param name="hive_key">Identifies which hive to look in, such as Microsoft.Win32.Registry.LocalMachine.</param>
		/// <param name="sub_key">Path from hive key to target node. Such as "SOFTWARE\\MICROSOFT\\Notepad\DefaultFonts.</param>
		/// <param name="name">The key to act on. Such as "ifFaceName".</param>
		/// <param name="value">Stores the casted value if RegistryCmd access succeeded.</param>
		/// <returns>Success true/false.</returns>
		/// <exception cref="System.ArgumentNullException">A null argument was passed in.</exception>
		/// <exception cref="System.ArgumentException">sub_key or name is longer than the maximum length allowed (255 characters).</exception>
		/// <exception cref="System.ObjectDisposedException">The Microsoft.Win32.RegistryKey is closed (closed keys cannot be accessed).</exception>
		/// <exception cref="System.Security.SecurityException">The user does not have the permissions required to read the registry key.</exception>
		/// <exception cref="System.IO.IOException">The Microsoft.Win32.RegistryKey that contains the specified key has been marked for deletion.</exception>
		/// <exception cref="System.UnauthorizedAccessException">The user does not have the necessary registry rights.</exception>
		/// <exception cref="System.InvalidCastException">The key value could not be cast to requested type.</exception>
		public static bool GetTypedValue<T>(RegistryKey hive_key, string sub_key, string name, ref T value)
		{
			return RegistryInfo.GetTypedValueWithOptions<T>(hive_key, sub_key, name, RegistryValueOptions.None, ref value);
		}

		/// <summary>
		/// Retrieves the specified key/value and casts to type T, where possible. Expected errors (such as could not open key)
		/// stored in StrRegError. Use GetValueType() or T=object if uncertain what type of value the key stores.
		/// </summary>
		/// <typeparam name="T">The type to cast the key value to.</typeparam>
		/// <param name="hive_key">Identifies which hive to look in, such as Microsoft.Win32.Registry.LocalMachine.</param>
		/// <param name="sub_key">Path from hive key to target node. Such as "SOFTWARE\\MICROSOFT\\Notepad\DefaultFonts.</param>
		/// <param name="name">The key to act on. Such as "ifFaceName".</param>
		/// <param name="options">The options to use when querying the registry.</param>
		/// <param name="value">Stores the casted value if RegistryCmd access succeeded.</param>
		/// <returns>Success true/false.</returns>
		/// <exception cref="System.ArgumentNullException">A null argument was passed in.</exception>
		/// <exception cref="System.ArgumentException">sub_key or name is longer than the maximum length allowed (255 characters).</exception>
		/// <exception cref="System.ObjectDisposedException">The Microsoft.Win32.RegistryKey is closed (closed keys cannot be accessed).</exception>
		/// <exception cref="System.Security.SecurityException">The user does not have the permissions required to read the registry key.</exception>
		/// <exception cref="System.IO.IOException">The Microsoft.Win32.RegistryKey that contains the specified key has been marked for deletion.</exception>
		/// <exception cref="System.UnauthorizedAccessException">The user does not have the necessary registry rights.</exception>
		/// <exception cref="System.InvalidCastException">The key value could not be cast to requested type.</exception>
		public static bool GetTypedValueWithOptions<T>(RegistryKey hive_key, string sub_key, string name, RegistryValueOptions options, ref T value)
		{
			object obj_data = RegistryInfo.GetValue(hive_key, sub_key, name, options);
			if (obj_data != null)
			{
				value = (T)obj_data;
				return true;
			}

			return false;
		}

		/// <summary>
		/// Sets/creates the specified String value. Exceptions/errors stored in StrRegError.
		/// </summary>
		/// <param name="hive_key">Identifies which hive to look in, such as Microsoft.Win32.Registry.LocalMachine.</param>
		/// <param name="sub_key">Path from hive key to target node. Such as "SOFTWARE\\MICROSOFT\\Notepad\DefaultFonts.</param>
		/// <param name="name">The key to act on. Such as "ifFaceName".</param>
		/// <param name="value">String data value to write.</param>
		/// <exception cref="System.ArgumentNullException">A null argument was passed in.</exception>
		/// <exception cref="System.ArgumentException">value is an unsupported data type.  -or- name is longer than the maximum length allowed (255 characters).</exception>
		/// <exception cref="System.ObjectDisposedException">The Microsoft.Win32.RegistryKey is closed (closed keys cannot be accessed).</exception>
		/// <exception cref="System.Security.SecurityException">The user does not have the permissions required to read the registry key.</exception>
		/// <exception cref="System.IO.IOException">The Microsoft.Win32.RegistryKey that contains the specified key has been marked for deletion.</exception>
		/// <exception cref="System.UnauthorizedAccessException">The user does not have the necessary registry rights.</exception>
		public static void SetStringValue(RegistryKey hive_key, string sub_key, string name, string value)
		{
			RegistryInfo.SetValue(hive_key, sub_key, name, value, RegistryValueKind.String);
		}

		/// <summary>
		/// Sets/creates the specified ExpandString value. Exceptions/errors stored in StrRegError.
		/// </summary>
		/// <param name="hive_key">Identifies which hive to look in, such as Microsoft.Win32.Registry.LocalMachine.</param>
		/// <param name="sub_key">Path from hive key to target node. Such as "SOFTWARE\\MICROSOFT\\Notepad\DefaultFonts.</param>
		/// <param name="name">The key to act on. Such as "ifFaceName".</param>
		/// <param name="value">String data value to write.</param>
		/// <exception cref="System.ArgumentNullException">A null argument was passed in.</exception>
		/// <exception cref="System.ArgumentException">value is an unsupported data type.  -or- name is longer than the maximum length allowed (255 characters).</exception>
		/// <exception cref="System.ObjectDisposedException">The Microsoft.Win32.RegistryKey is closed (closed keys cannot be accessed).</exception>
		/// <exception cref="System.Security.SecurityException">The user does not have the permissions required to read the registry key.</exception>
		/// <exception cref="System.IO.IOException">The Microsoft.Win32.RegistryKey that contains the specified key has been marked for deletion.</exception>
		/// <exception cref="System.UnauthorizedAccessException">The user does not have the necessary registry rights.</exception>
		public static void SetStringExpandValue(RegistryKey hive_key, string sub_key, string name, string value)
		{
			RegistryInfo.SetValue(hive_key, sub_key, name, value, RegistryValueKind.ExpandString);
		}

		/// <summary>
		/// Sets/creates the specified DWORD value. Exceptions/errors stored in StrRegError.
		/// </summary>
		/// <param name="hive_key">Identifies which hive to look in, such as Microsoft.Win32.Registry.LocalMachine.</param>
		/// <param name="sub_key">Path from hive key to target node. Such as "SOFTWARE\\MICROSOFT\\Notepad\DefaultFonts.</param>
		/// <param name="name">The key to act on. Such as "iPointSize".</param>
		/// <param name="value">DWORD data value to write.</param>
		/// <exception cref="System.ArgumentNullException">A null argument was passed in.</exception>
		/// <exception cref="System.ArgumentException">value is an unsupported data type.  -or- name is longer than the maximum length allowed (255 characters).</exception>
		/// <exception cref="System.ObjectDisposedException">The Microsoft.Win32.RegistryKey is closed (closed keys cannot be accessed).</exception>
		/// <exception cref="System.Security.SecurityException">The user does not have the permissions required to read the registry key.</exception>
		/// <exception cref="System.IO.IOException">The Microsoft.Win32.RegistryKey that contains the specified key has been marked for deletion.</exception>
		/// <exception cref="System.UnauthorizedAccessException">The user does not have the necessary registry rights.</exception>
		public static void SetDWORDValue(RegistryKey hive_key, string sub_key, string name, int value)
		{
			RegistryInfo.SetValue(hive_key, sub_key, name, value, RegistryValueKind.DWord);
		}

		/// <summary>
		/// Sets/creates the specified Binary value. Exceptions/errors stored in StrRegError.
		/// </summary>
		/// <param name="hive_key">Identifies which hive to look in, such as Microsoft.Win32.Registry.LocalMachine.</param>
		/// <param name="sub_key">Path from hive key to target node. Such as "SOFTWARE\\MICROSOFT\\Notepad\DefaultFonts.</param>
		/// <param name="name">The key to act on. Such as "iPointSize" sic.</param>
		/// <param name="value">Binary data value to write.</param>
		/// <exception cref="System.ArgumentNullException">A null argument was passed in.</exception>
		/// <exception cref="System.ArgumentException">value is an unsupported data type.  -or- name is longer than the maximum length allowed (255 characters).</exception>
		/// <exception cref="System.ObjectDisposedException">The Microsoft.Win32.RegistryKey is closed (closed keys cannot be accessed).</exception>
		/// <exception cref="System.Security.SecurityException">The user does not have the permissions required to read the registry key.</exception>
		/// <exception cref="System.IO.IOException">The Microsoft.Win32.RegistryKey that contains the specified key has been marked for deletion.</exception>
		/// <exception cref="System.UnauthorizedAccessException">The user does not have the necessary registry rights.</exception>
		public static void SetBinaryValue(RegistryKey hive_key, string sub_key, string name, byte[] value)
		{
			RegistryInfo.SetValue(hive_key, sub_key, name, value, RegistryValueKind.Binary);
		}

		/// <summary>
		/// Sets/creates the specified String Array value. Exceptions/errors stored in StrRegError.
		/// </summary>
		/// <param name="hive_key">Identifies which hive to look in, such as Microsoft.Win32.Registry.LocalMachine.</param>
		/// <param name="sub_key">Path from hive key to target node. Such as "SOFTWARE\\MICROSOFT\\Notepad\DefaultFonts.</param>
		/// <param name="name">The key to act on. Such as "iPointSize" sic.</param>
		/// <param name="value">String Array value to write.</param>
		/// <exception cref="System.ArgumentNullException">A null argument was passed in.</exception>
		/// <exception cref="System.ArgumentException">value is an unsupported data type.  -or- name is longer than the maximum length allowed (255 characters).</exception>
		/// <exception cref="System.ObjectDisposedException">The Microsoft.Win32.RegistryKey is closed (closed keys cannot be accessed).</exception>
		/// <exception cref="System.Security.SecurityException">The user does not have the permissions required to read the registry key.</exception>
		/// <exception cref="System.IO.IOException">The Microsoft.Win32.RegistryKey that contains the specified key has been marked for deletion.</exception>
		/// <exception cref="System.UnauthorizedAccessException">The user does not have the necessary registry rights.</exception>
		public static void SetStringArrayValue(RegistryKey hive_key, string sub_key, string name, string[] value)
		{
			RegistryInfo.SetValue(hive_key, sub_key, name, value, RegistryValueKind.MultiString);
		}

		/// <summary>
		/// Sets/creates the specified value. Casts where able to RegistryValueKind. Exceptions/errors stored in StrRegError.
		/// </summary>
		/// <param name="hive_key">Identifies which hive to look in, such as Microsoft.Win32.Registry.LocalMachine.</param>
		/// <param name="sub_key">Path from hive key to target node. Such as "SOFTWARE\\MICROSOFT\\Notepad\DefaultFonts.</param>
		/// <param name="name">The key to act on. Such as "iPointSize" sic.</param>
		/// <param name="value">Binary data value to write.</param>
		/// <param name="kind">Microsoft.Win32.RegistryValueKind to store the value as.</param>
		/// <exception cref="System.ArgumentNullException">A null argument was passed in.</exception>
		/// <exception cref="System.ArgumentException">value is an unsupported data type.  -or- name is longer than the maximum length allowed (255 characters).</exception>
		/// <exception cref="System.ObjectDisposedException">The Microsoft.Win32.RegistryKey is closed (closed keys cannot be accessed).</exception>
		/// <exception cref="System.Security.SecurityException">The user does not have the permissions required to read the registry key.</exception>
		/// <exception cref="System.IO.IOException">The Microsoft.Win32.RegistryKey that contains the specified key has been marked for deletion.</exception>
		/// <exception cref="System.UnauthorizedAccessException">The user does not have the necessary registry rights.</exception>
		public static void SetValue(RegistryKey hive_key, string sub_key, string name, object value, RegistryValueKind kind)
		{
			StrRegError = null;
			using (RegistryKey subKey = RegistryInfo.CreateSubKey(hive_key, sub_key))
			{
				if (subKey == null)
				{
					StrRegError = "Cannot create/open the specified sub-key";
					return;
				}

				subKey.SetValue(name, value, kind);
			}
		}

		/// <summary>
		/// Creates a new subkey or opens an existing subkey. Exceptions/errors stored in StrRegError.
		/// </summary>
		/// <param name="hive_key">Identifies which hive to look in, such as Microsoft.Win32.Registry.LocalMachine.</param>
		/// <param name="sub_key">Path from hive key to target node. Such as "SOFTWARE\\MICROSOFT\\Notepad\DefaultFonts.</param>
		/// <exception cref="System.ArgumentNullException">A null argument was passed in.</exception>
		/// <exception cref="System.ArgumentException">sub_key is longer than the maximum length allowed (255 characters).</exception>
		/// <exception cref="System.ObjectDisposedException">The Microsoft.Win32.RegistryKey is closed (closed keys cannot be accessed).</exception>
		/// <exception cref="System.Security.SecurityException">The user does not have the permissions required to read the registry key.</exception>
		/// <exception cref="System.IO.IOException">The Microsoft.Win32.RegistryKey that contains the specified key has been marked for deletion.</exception>
		/// <exception cref="System.UnauthorizedAccessException">The user does not have the necessary registry rights.</exception>
		/// <returns>The Microsoft.Win32.RegistryKey created from hive_key and sub_key.</returns>
		public static RegistryKey CreateSubKey(RegistryKey hive_key, string sub_key)
		{
			StrRegError = null;
			RegistryKey subKey = hive_key.CreateSubKey(sub_key);
			if (subKey == null)
			{
				StrRegError = "Cannot create the specified sub-key";
			}

			return subKey;
		}

		/// <summary>
		/// Deletes a subkey and any child subkeys recursively. Exceptions/errors stored in StrRegError.
		/// </summary>
		/// <param name="hive_key">Identifies which hive to look in, such as Microsoft.Win32.Registry.LocalMachine.</param>
		/// <param name="sub_key">Path from hive key to target node. Such as "SOFTWARE\\MICROSOFT\\Notepad\DefaultFonts.</param>
		/// <exception cref="System.ArgumentNullException">A null argument was passed in.</exception>
		/// <exception cref="System.ArgumentException">Deletion of a root hive is attempted.  -or- subkey does not specify a valid registry subkey.</exception>
		/// <exception cref="System.ObjectDisposedException">The Microsoft.Win32.RegistryKey is closed (closed keys cannot be accessed).</exception>
		/// <exception cref="System.Security.SecurityException">The user does not have the permissions required to read the registry key.</exception>
		/// <exception cref="System.UnauthorizedAccessException">The user does not have the necessary registry rights.</exception>
		public static void DeleteSubKeyTree(RegistryKey hive_key, string sub_key)
		{
			StrRegError = null;
			hive_key.DeleteSubKeyTree(sub_key);
		}

		/// <summary>
		/// Deletes the specified value from this (current) key. Exceptions/errors stored in StrRegError.
		/// </summary>
		/// <param name="hive_key">Identifies which hive to look in, such as Microsoft.Win32.Registry.LocalMachine.</param>
		/// <param name="sub_key">Path from hive key to target node. Such as "SOFTWARE\\MICROSOFT\\Notepad\DefaultFonts.</param>
		/// <param name="name">The key to act on. Such as "iPointSize".</param>
		/// <exception cref="System.ArgumentNullException">A null argument was passed in.</exception>
		/// <exception cref="System.ArgumentException">name is not a valid reference to a value.</exception>
		/// <exception cref="System.ObjectDisposedException">The Microsoft.Win32.RegistryKey is closed (closed keys cannot be accessed).</exception>
		/// <exception cref="System.Security.SecurityException">The user does not have the permissions required to read the registry key.</exception>
		/// <exception cref="System.IO.IOException">The Microsoft.Win32.RegistryKey that contains the specified key has been marked for deletion.</exception>
		/// <exception cref="System.UnauthorizedAccessException">The user does not have the necessary registry rights.</exception>
		public static void DeleteValue(RegistryKey hive_key, string sub_key, string name)
		{
			StrRegError = null;
			using (RegistryKey subKey = hive_key.OpenSubKey(sub_key, true))
			{
				if (subKey == null)
				{
					StrRegError = String.Format("Cannot open the specified sub-key: {0}", sub_key);
					return;
				}

				if (subKey.GetValue(name) == null)
				{
					StrRegError = String.Format("Cannot open the specified value \"{0}\" on sub-key \"{1}\"", name, sub_key);
					return;
				}

				subKey.DeleteValue(name);
			}
		}

		/// <summary>
		/// Retrieves the type of the specified RegistryCmd value. Exceptions/errors stored in StrRegError.
		/// </summary>
		/// <param name="hive_key">Identifies which hive to look in, such as Microsoft.Win32.Registry.LocalMachine.</param>
		/// <param name="sub_key">Path from hive key to target node. Such as "SOFTWARE\\MICROSOFT\\Notepad\DefaultFonts.</param>
		/// <param name="name">The key to act on. Such as "iPointSize".</param>
		/// <returns>System.Type. Exceptions/errors stored in StrRegError.</returns>
		/// <exception cref="System.ArgumentNullException">A null argument was passed in.</exception>
		/// <exception cref="System.ArgumentException">sub_key or name is longer than the maximum length allowed (255 characters).</exception>
		/// <exception cref="System.ObjectDisposedException">The Microsoft.Win32.RegistryKey is closed (closed keys cannot be accessed).</exception>
		/// <exception cref="System.Security.SecurityException">The user does not have the permissions required to read the registry key.</exception>
		/// <exception cref="System.IO.IOException">The Microsoft.Win32.RegistryKey that contains the specified key has been marked for deletion.</exception>
		/// <exception cref="System.UnauthorizedAccessException">The user does not have the necessary registry rights.</exception>
		public static Type GetValueType(RegistryKey hive_key, string sub_key, string name)
		{
			return RegistryInfo.GetValue(hive_key, sub_key, name, RegistryValueOptions.None).GetType();
		}

		/// <summary>
		/// Determines the existence of a value. Exceptions/errors stored in StrRegError.
		/// </summary>
		/// <param name="hive_key">Identifies which hive to look in, such as Microsoft.Win32.Registry.LocalMachine.</param>
		/// <param name="sub_key">Path from hive key to target node. Such as "SOFTWARE\\MICROSOFT\\Notepad\DefaultFonts.</param>
		/// <param name="name">The key to act on. Such as "iPointSize".</param>
		/// <returns>Whether the value exists by boolean. Exceptions/errors stored in StrRegError.</returns>
		/// <exception cref="System.ArgumentNullException">A null argument was passed in.</exception>
		/// <exception cref="System.ArgumentException">sub_key or name is longer than the maximum length allowed (255 characters).</exception>
		/// <exception cref="System.ObjectDisposedException">The Microsoft.Win32.RegistryKey is closed (closed keys cannot be accessed).</exception>
		/// <exception cref="System.Security.SecurityException">The user does not have the permissions required to read the registry key.</exception>
		/// <exception cref="System.IO.IOException">The Microsoft.Win32.RegistryKey that contains the specified key has been marked for deletion.</exception>
		/// <exception cref="System.UnauthorizedAccessException">The user does not have the necessary registry rights.</exception>
		public static bool ValueExists(RegistryKey hive_key, string sub_key, string name)
		{
			StrRegError = null;
			using (RegistryKey subKey = hive_key.OpenSubKey(sub_key))
			{
				if (subKey == null)
				{
					StrRegError = "Cannot open the specified sub-key";
					return false;
				}

				string[] value_names = subKey.GetValueNames();
				foreach (string name_key in value_names)
				{
					if (0 == String.Compare(name_key, name, System.StringComparison.InvariantCultureIgnoreCase))
					{
						return true;
					}
				}
			}

			return false;
		}

		/// <summary>
		/// Wraps up the Win32 RegistryCmd access to a single line. OpenSubKey and GetValue can throw several exceptions,
		/// but for now we will let those hit the user (exceptions listed on public accessors to this function).
		/// </summary>
		/// <param name="hive_key">Identifies which hive to look in, such as Microsoft.Win32.RegistryCmd.LocalMachine.</param>
		/// <param name="sub_key">Path from hive key to target node. Such as "SOFTWARE\\MICROSOFT\\Notepad\DefaultFonts.</param>
		/// <param name="name">The key to act on. Such as "iPointSize".</param>
		/// <returns>Object from RegistryKey.OpenSubKey.</returns>
		private static object GetValue(RegistryKey hive_key, string sub_key, string name, RegistryValueOptions options)
		{
			object objData = null;
			StrRegError = null;

			using (RegistryKey subKey = hive_key.OpenSubKey(sub_key))
			{
				if (subKey == null)
				{
					StrRegError = "Cannot open the specified sub-key";
					return null;
				}

				objData = subKey.GetValue(name, null, options);
				if (objData == null)
				{
					StrRegError = "Cannot open the specified value";
					return null;
				}
			}

			return objData;
		}
		
		[SuppressUnmanagedCodeSecurity()]
		internal static class NativeMethods
		{
			[DllImport("advapi32.dll")]
			internal static extern int RegOpenKeyEx(
				RegistryHive reg_hive,
				[MarshalAs(UnmanagedType.VBByRefStr)] ref string sub_key,
				int options,
				int sam,
				out SafeRegistryHandle result);

			[DllImport("advapi32.dll", EntryPoint = "RegQueryInfoKey", CallingConvention = CallingConvention.Winapi, SetLastError = true)]
			internal static extern int RegQueryInfoKey(
				SafeRegistryHandle hkey,
				StringBuilder lpClass,
				ref uint lpcbClass,
				IntPtr lpReserved,
				out uint lpcSubKeys,
				out uint lpcbMaxSubKeyLen,
				IntPtr lpcbMaxClassLen,
				IntPtr lpcValues,
				IntPtr lpcbMaxValueNameLen,
				IntPtr lpcbMaxValueLen,
				IntPtr lpcbSecurityDescriptor,
				IntPtr lpftLastWriteTime);

			[DllImport("advapi32.dll", EntryPoint = "RegEnumKeyEx")]
			internal static extern int RegEnumKeyEx(
				SafeRegistryHandle hkey,
				uint index,
				StringBuilder lpName,
				ref uint lpcbName,
				IntPtr reserved,
				IntPtr lpClass,
				IntPtr lpcbClass,
				out long lpftLastWriteTime);

			[DllImport("advapi32.dll", CharSet = CharSet.Unicode, EntryPoint = "RegQueryValueExW")]
			internal static extern int RegQueryValueExString(
				SafeRegistryHandle key,
				string value_name,
				int reserved,
				out uint type,
				System.Text.StringBuilder data,
				ref uint data_count);

			[DllImport("advapi32.dll", CharSet = CharSet.Unicode, EntryPoint = "RegQueryValueExW")]
			internal static extern int RegQueryValueExDWORD(
				SafeRegistryHandle key,
				string value_name,
				int reserved,
				out uint type,
				ref uint data,
				ref uint data_count);

			[DllImport("advapi32.dll")]
			internal static extern int RegCloseKey(
				SafeRegistryHandle hkey);

			[DllImport("kernel32.dll")]
			internal static extern uint FormatMessage(
				uint flags,
				IntPtr source,
				uint message_ID,
				uint language_ID,
				StringBuilder buffer,
				uint size,
				IntPtr args);
		}

		public class DirectRegistryAccess
		{
			// Win32 constants for accessing registry.
			public const int STANDARD_RIGHTS_REQUIRED = 0x000F0000;
			public const int READ_CONTROL = 0x00020000;
			public const int SYNCHRONIZE = 0x00100000;
			public const int STANDARD_RIGHTS_READ = READ_CONTROL;
			public const int STANDARD_RIGHTS_WRITE = READ_CONTROL;
			public const int STANDARD_RIGHTS_EXECUTE = READ_CONTROL;
			public const int STANDARD_RIGHTS_ALL = 0x001F0000;
			public const int KEY_QUERY_VALUE = 0x0001;
			public const int KEY_SET_VALUE = 0x0002;
			public const int KEY_CREATE_SUB_KEY = 0x0004;
			public const int KEY_ENUMERATE_SUB_KEYS = 0x0008;
			public const int KEY_NOTIFY = 0x0010;
			public const int KEY_CREATE_LINK = 0x0020;
			public const int KEY_WOW64_32KEY = 0x0200;
			public const int KEY_WOW64_64KEY = 0x0100;
			public const int FORMAT_MESSAGE_FROM_SYSTEM = 0x00001000;

			public const int KEY_READ = STANDARD_RIGHTS_READ | KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS | KEY_NOTIFY;
			public const int KEY_ALL_ACCESS = STANDARD_RIGHTS_ALL | KEY_QUERY_VALUE | KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_ENUMERATE_SUB_KEYS | KEY_NOTIFY | KEY_CREATE_LINK & (~SYNCHRONIZE);

			// Win32 Error codes
			public const int ERROR_MORE_DATA = 234;

			/// <summary>
			/// Gets the error message of the last operation (null when no error occurred).
			/// </summary>
			public static string StrRegError
			{
				get;
				private set;
			}

			private static void SetErrorStringFromErrorCode(int error_code)
			{
				StringBuilder buffer = new StringBuilder(512);
				uint message_length = NativeMethods.FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, IntPtr.Zero, (uint)error_code, 0, buffer, (uint)buffer.Capacity, IntPtr.Zero);
				if (message_length == 0)
				{
					StrRegError = "Opening registry/getting value failed, also FormatMessage failed.";
				}
				else
				{
					StrRegError = buffer.ToString();
				}

				// Sample FormatMessage code suggests the following line, for no apparent reason...
				// StrRegError = StrRegError.Substring(0, StrRegError.Length - 2);
			}

			public static SafeRegistryHandle OpenRegistryKeyWithFlags(RegistryHive reg_hive, string key, int flags)
			{
				SafeRegistryHandle return_handle = null;
				StrRegError = null;

				int ret = NativeMethods.RegOpenKeyEx(reg_hive, ref key, 0, KEY_READ | flags, out return_handle);
				if (ret != 0)
				{
					SetErrorStringFromErrorCode(ret);

					return_handle.SetHandleAsInvalid();
				}

				return return_handle;
			}

			public static string[] GetRegistryKeySubkeyNames(SafeRegistryHandle safe_handle)
			{
				StringBuilder string_builder = new StringBuilder();
				uint classid = 0;
				uint key_count = 0;
				uint max_key_length = 0;
				int ret = 0;
				string[] subkey_names = null;

				StrRegError = null;

				if (safe_handle != null && !safe_handle.IsInvalid)
				{
					// Get the number of subkeys
					ret = NativeMethods.RegQueryInfoKey(
						safe_handle,
						string_builder,
						ref classid,
						IntPtr.Zero,
						out key_count,
						out max_key_length,
						IntPtr.Zero,
						IntPtr.Zero,
						IntPtr.Zero,
						IntPtr.Zero,
						IntPtr.Zero,
						IntPtr.Zero);

					if (ret != 0)
					{
						SetErrorStringFromErrorCode(ret);
					}
					else
					{
						subkey_names = new string[key_count];
						if (key_count > 0 && max_key_length > 0)
						{
							for (uint i = 0; i < key_count; ++i)
							{
								string_builder.EnsureCapacity((int)max_key_length + 1);
								uint capacity = (uint)string_builder.Capacity;
								long last_write_time = 0;

								ret = NativeMethods.RegEnumKeyEx(
									safe_handle,
									i,
									string_builder,
									ref capacity,
									IntPtr.Zero,
									IntPtr.Zero,
									IntPtr.Zero,
									out last_write_time);

								if (ret == 0)
								{
									subkey_names[i] = string_builder.ToString();
								}
							}
						}
					}
				}
				else
				{
					StrRegError = "Invalid handle passed to GetRegistryKeySubkeyNames";
				}

				return subkey_names;
			}

			public static bool GetStringValueFromRegistryHandle(SafeRegistryHandle safe_handle, string value, out string return_data)
			{
				StringBuilder data = new StringBuilder();
				uint out_size = 0;
				uint out_type = 0;
				int ret = 0;
				bool success = false;

				StrRegError = null;

				if (safe_handle != null && !safe_handle.IsInvalid)
				{
					// Query to see how much space we need for the string
					// The return value here will be ERROR_MORE_DATA if there is data in the requested key/value.
					ret = NativeMethods.RegQueryValueExString(safe_handle, value, 0, out out_type, data, ref out_size);
					success = (ret == ERROR_MORE_DATA);
					if (success)
					{
						// TODO: [jonask: 2010.06.21] Double check that the out_type is REG_SZ or REG_EXPAND_SZ.
						// (And what about REG_MULTI_SZ?)
						// WinNT.h:
						//#define REG_SZ                      ( 1 )   // Unicode nul terminated string
						//#define REG_EXPAND_SZ               ( 2 )   // Unicode nul terminated string
						//#define REG_MULTI_SZ                ( 7 )   // Multiple Unicode strings

						// Get our data out of the string
						data.EnsureCapacity((int)out_size);
						ret = NativeMethods.RegQueryValueExString(safe_handle, value, 0, out out_type, data, ref out_size);
						success = (ret == 0);
					}

					if (!success)
					{
						SetErrorStringFromErrorCode(ret);
					}
				}
				else
				{
					StrRegError = "Invalid handle passed to GetStringValueFromRegistryHandle";
					success = false;
				}

				return_data = data.ToString();
				return success;
			}

			public static bool GetDWORDValueFromRegistryHandle(SafeRegistryHandle safe_handle, string value, out uint return_data)
			{
				uint data = 0;
				uint out_size = 0;
				uint out_type = 0;
				int ret = 0;
				bool success = false;

				StrRegError = null;

				if (safe_handle != null && !safe_handle.IsInvalid)
				{
					// Query to see how much space we need for the string
					// The return value here will be ERROR_MORE_DATA if there is data in the requested key/value. (out_size is initialized to 0)
					ret = NativeMethods.RegQueryValueExDWORD(safe_handle, value, 0, out out_type, ref data, ref out_size);
					success = (ret == 0);
					if (success)
					{
						// TODO: [jonask: 2010.06.21] Double check that the out_type is REG_DWORD
						// (And what about REG_DWORD_LITTLE_ENDIAN or REG_DWORD_BIG_ENDIAN?)
						// WinNT.h:
						//#define REG_DWORD                   ( 4 )   // 32-bit number
						//#define REG_DWORD_LITTLE_ENDIAN     ( 4 )   // 32-bit number (same as REG_DWORD)
						//#define REG_DWORD_BIG_ENDIAN        ( 5 )   // 32-bit number
					}

					if (!success)
					{
						SetErrorStringFromErrorCode(ret);
					}
				}
				else
				{
					StrRegError = "Invalid handle passed to GetDWORDValueFromRegistryHandle";
					success = false;
				}

				return_data = data;
				return success;
			}


			/// <summary>
			/// Horrible stuff to be able to access the 64-bit registry from a managed app, regardless of
			/// the app's bitness.
			/// </summary>
			/// <param name="reg_hive">The hive to read from (HKLM, HKCU, etc).</param>
			/// <param name="key">The key itself.</param>
			/// <param name="value">The value to query.</param>
			/// <param name="flags">Additional flags to pass directly to the Win32 API.</param>
			/// <returns>String value stored in the registry, or empty string if not available.</returns>
			public static string GetStringValueFromRegistryWithFlags(RegistryHive reg_hive, string key, string value, int flags)
			{
				string return_value = String.Empty;

				StrRegError = null;

				// First, open the key
				using (SafeRegistryHandle key_handle = OpenRegistryKeyWithFlags(reg_hive, key, KEY_READ | flags))
				{
					if (key_handle != null && !key_handle.IsInvalid)
					{
						GetStringValueFromRegistryHandle(key_handle, value, out return_value);
					}
				}

				return return_value;
			}

			/// <summary>
			/// Horrible stuff to be able to access the 32-bit registry from a managed app, regardless of
			/// the app's bitness.
			/// </summary>
			/// <param name="reg_hive">The hive to read from (HKLM, HKCU, etc).</param>
			/// <param name="key">The key itself.</param>
			/// <param name="value">The value to query.</param>
			/// <returns>String value stored in the 32-bit registry, or empty string if not available.</returns>
			public string GetStringValueFrom32BitRegistry(RegistryHive reg_hive, string key, string value)
			{
				return GetStringValueFromRegistryWithFlags(reg_hive, key, value, KEY_WOW64_32KEY);
			}

			/// <summary>
			/// Horrible stuff to be able to access the 64-bit registry from a managed app, regardless of
			/// the app's bitness.
			/// </summary>
			/// <param name="reg_hive">The hive to read from (HKLM, HKCU, etc).</param>
			/// <param name="key">The key itself.</param>
			/// <param name="value">The value to query.</param>
			/// <returns>String value stored in the 64-bit registry, or empty string if not available.</returns>
			public string GetStringValueFrom64BitRegistry(RegistryHive reg_hive, string key, string value)
			{
				return GetStringValueFromRegistryWithFlags(reg_hive, key, value, KEY_WOW64_64KEY);
			}
		}
	}
}
