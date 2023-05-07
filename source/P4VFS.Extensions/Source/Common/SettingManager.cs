// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Linq;
using Microsoft.P4VFS.CoreInterop;

namespace Microsoft.P4VFS.Extensions
{
	public static class SettingManagerExtensions
	{
		public static LogChannel Verbosity
		{ 
			get => ServiceSettings.GetPropertyEnum(LogChannel.Info); 
			set => ServiceSettings.SetPropertyEnum(value); 
		} 

		public static FilePopulateMethod PopulateMethod
		{ 
			get => ServiceSettings.GetPropertyEnum(FilePopulateMethod.Stream); 
			set => ServiceSettings.SetPropertyEnum(value); 
		} 

		public static DepotFlushType DefaultFlushType
		{ 
			get => ServiceSettings.GetPropertyEnum(DepotFlushType.Atomic); 
			set => ServiceSettings.SetPropertyEnum(value); 
		} 

		public static DepotServerConfig DepotServerConfig
		{ 
			get => DepotServerConfig.FromNode(ServiceSettings.GetProperty()); 
			set => ServiceSettings.SetProperty(value?.ToNode()); 
		} 
	}
}
