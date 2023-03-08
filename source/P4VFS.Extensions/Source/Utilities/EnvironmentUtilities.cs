// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.IO;
using System.Diagnostics;
using System.Linq;

namespace Microsoft.P4VFS.Extensions.Utilities
{
	public class SetEnvironmentVariableScope : IDisposable
	{
		private string _Name;
		private string _PreviousValue;

		public SetEnvironmentVariableScope(string name, string value)
		{
			_Name = name;
			_PreviousValue = Environment.GetEnvironmentVariable(_Name);
			Environment.SetEnvironmentVariable(_Name, value);
		}

		public void Dispose()
		{
			Environment.SetEnvironmentVariable(_Name, _PreviousValue);
		}
	}

	public class SetCurrentDirectoryScope : IDisposable
	{
		private string _PreviousPath;

		public SetCurrentDirectoryScope(string path)
		{
			if (Directory.Exists(path))
			{
				_PreviousPath = Environment.CurrentDirectory;
				Environment.CurrentDirectory = path;
			}
		}

		public void Dispose()
		{
			if (Directory.Exists(_PreviousPath))
			{
				Environment.CurrentDirectory = _PreviousPath;
			}
		}
	}

	public class SetConsoleTitleScope : IDisposable
	{
		private string _PreviousTitle;

		public SetConsoleTitleScope(string title)
		{
			_PreviousTitle = Console.Title;
			Console.Title = title ?? "";
		}

		public void Dispose()
		{
			Console.Title = _PreviousTitle;
		}
	}
}
