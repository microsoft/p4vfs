// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "ExtensionsServiceHost.h"
#pragma managed(push, off)

#if defined(P4VFS_EXTENSIONS_INTEROP_EXPORTS)
#define P4VFS_EXTENSIONS_INTEROP_API __declspec(dllexport)
#else
#define P4VFS_EXTENSIONS_INTEROP_API __declspec(dllimport)
#endif

namespace Microsoft {
namespace P4VFS {
namespace ExtensionsInterop {

	P4VFS_EXTENSIONS_INTEROP_API HRESULT  
	InitializeServiceHost(
		ServiceHost* srvHost
		);

	P4VFS_EXTENSIONS_INTEROP_API HRESULT  
	ShutdownServiceHost(
		);

	P4VFS_EXTENSIONS_INTEROP_API void
	LaunchAttachDebugger(
		);
}}}

#pragma managed(pop)
