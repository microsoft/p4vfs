// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "DepotSyncOptions.h"

namespace Microsoft {
namespace P4VFS {
namespace P4 {

DepotString DepotSyncMethod::ToString(DepotSyncMethod::Enum value)
{
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncMethod, Regular);
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncMethod, Virtual);
	return DepotString();
}

FDepotSyncOptions::FDepotSyncOptions() :
	m_SyncFlags(DepotSyncFlags::Normal),
	m_SyncMethod(DepotSyncMethod::Virtual),
	m_FlushType(DepotFlushType::Atomic)
{
}

FDepotSyncOptions::~FDepotSyncOptions()
{
}

}}}

