// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Microsoft.P4VFS.Extensions
{
	public class DepotFileReference
	{
		public string DepotFile;
		public DepotRevisionNumber Rev;
		public string ClientFile;
		public string WorkspaceFile;
		public int Change;
		public string Action;
		public string Type;
	}
}
