// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "DepotRevision.h"
#include "FileCore.h"

namespace Microsoft {
namespace P4VFS {
namespace P4 {

	struct DepotReconfigFlags
	{
		enum Enum
		{
			None		= 0,
			Preview		= (1<<0),
			P4Port		= (1<<1),
			P4Client	= (1<<2),
			P4User		= (1<<3),
		};

		static DepotString ToString(DepotReconfigFlags::Enum value);
	};

	struct FDepotReconfigOptions
	{
		DepotStringArray m_Files;
		DepotReconfigFlags::Enum m_Flags;

		P4VFS_CORE_API FDepotReconfigOptions();
		P4VFS_CORE_API ~FDepotReconfigOptions();
	};

}}}
