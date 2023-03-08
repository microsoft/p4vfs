// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "TestFactory.h"
#include "ServiceOperations.h"

using namespace Microsoft::P4VFS::FileCore;
using namespace Microsoft::P4VFS::TestCore;
using namespace Microsoft::P4VFS;

void TestServiceOperationsStartStop(const TestContext& context)
{
	const wchar_t* serviceTitle = TEXT("P4VFS.Service");
	
	Assert(ServiceOperations::GetLocalServiceState(serviceTitle) == SERVICE_RUNNING);
	Assert(SUCCEEDED(ServiceOperations::StopLocalService(serviceTitle)));
	Assert(ServiceOperations::GetLocalServiceState(serviceTitle) == SERVICE_STOPPED);
	Assert(SUCCEEDED(ServiceOperations::StopLocalService(serviceTitle)));
	Assert(ServiceOperations::GetLocalServiceState(serviceTitle) == SERVICE_STOPPED);

	Assert(SUCCEEDED(ServiceOperations::StartLocalService(serviceTitle)));
	Assert(ServiceOperations::GetLocalServiceState(serviceTitle) == SERVICE_RUNNING);
	Assert(SUCCEEDED(ServiceOperations::StartLocalService(serviceTitle)));
	Assert(ServiceOperations::GetLocalServiceState(serviceTitle) == SERVICE_RUNNING);

	Assert(ServiceOperations::GetLocalServiceState(TEXT("UnknownBogusService")) == 0);
}
