// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "ServiceHost.h"
#include "FileAssert.h"

using namespace Microsoft::P4VFS;
ServiceHost g_SrvHost;

int _tmain()
{
	ServiceHost::StaticInstance().Start();
	return 0;
}
