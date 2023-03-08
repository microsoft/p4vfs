// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "ExtensionsInterop.h"
#include "ExtensionsProtocol.h"
#pragma unmanaged

namespace Microsoft {
namespace P4VFS {

HRESULT  
ExtensionsInterop::InitializeServiceHost(
	ServiceHost* srvHost
	)
{
	return HRESULT(ExtensionsProtocol::InitializeServiceHost(srvHost));
}

HRESULT  
ExtensionsInterop::ShutdownServiceHost(
	)
{
	return HRESULT(ExtensionsProtocol::ShutdownServiceHost());
}

void
ExtensionsInterop::LaunchAttachDebugger(
	)
{
	ExtensionsProtocol::LaunchAttachDebugger();
}

}}
