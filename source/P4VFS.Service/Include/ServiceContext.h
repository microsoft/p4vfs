// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "FileContext.h"
#include "DepotClientCache.h"
#include "ServiceLog.h"

namespace Microsoft {
namespace P4VFS {

class ServiceContext : public FileCore::FileContext
{
public:
	ServiceContext(HANDLE hCancelationEvent);

public:
	ServiceLogDevice m_ServiceLogDevice;
	static P4::DepotClientCache m_StaticDepotClientCache;
};

}}
