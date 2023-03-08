// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "ServiceContext.h"
#include "ExtensionsInterop.h"

namespace Microsoft {
namespace P4VFS {

P4::DepotClientCache ServiceContext::m_StaticDepotClientCache;

ServiceContext::ServiceContext(HANDLE hCancelationEvent) :
	m_ServiceLogDevice(hCancelationEvent)
{
	m_LogDevice = &m_ServiceLogDevice;
	m_DepotClientCache = &m_StaticDepotClientCache;
}

}}
