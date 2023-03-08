// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "DepotRevision.h"
#include "DepotSyncAction.h"
#include "FileContext.h"

namespace Microsoft {
namespace P4VFS {
namespace P4 {

	struct DepotSyncMethod
	{
		enum Enum
		{
			Regular,
			Virtual,
		};

		static DepotString ToString(DepotSyncMethod::Enum value);
	};

	struct FDepotSyncOptions
	{
		DepotStringArray m_Files;
		DepotRevision m_Revision;
		DepotSyncType::Enum m_SyncType;
		DepotFlushType::Enum m_FlushType;
		DepotSyncMethod::Enum m_SyncMethod;
		DepotString m_SyncResident;
		UserContext m_Context;

		P4VFS_CORE_API FDepotSyncOptions();
		P4VFS_CORE_API ~FDepotSyncOptions();
	};

}}}
