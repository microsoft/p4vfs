// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "ExtensionsServiceHost.h"

namespace Microsoft {
namespace P4VFS {
namespace ExtensionsProtocol {

	long
	InitializeServiceHost(
		ExtensionsInterop::ServiceHost* srvHost
		);

	long
	ShutdownServiceHost(
		);

	void
	LaunchAttachDebugger(
		);
}}}
