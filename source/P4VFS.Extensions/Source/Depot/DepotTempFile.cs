// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Diagnostics;
using System.Linq;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text.RegularExpressions;
using Microsoft.P4VFS.Extensions.Utilities;

namespace Microsoft.P4VFS.Extensions
{
    public class DepotTempFile : IDisposable
    {
		private string _LocalFilePath;

		public DepotTempFile(string filePath)
		{
			_LocalFilePath = filePath;
		}

		public string LocalFilePath
		{
			get { return _LocalFilePath; }
		}

		public void Dispose()
		{
			FileUtilities.DeleteFile(_LocalFilePath);
		}
	}
}
