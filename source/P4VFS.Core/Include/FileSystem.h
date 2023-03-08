// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "FileCore.h"
#include "FileContext.h"
#include "DriverData.h"
#pragma managed(push, off)

namespace Microsoft {
namespace P4VFS {
namespace FileSystem {

	struct FilePopulateMethod
	{
		enum Enum
		{
			Copy	= P4VFS_POPULATE_METHOD_COPY,
			Move	= P4VFS_POPULATE_METHOD_MOVE,
			Stream	= P4VFS_POPULATE_METHOD_STREAM,
		};

		P4VFS_CORE_API static FileCore::AString ToString(Enum value);
		P4VFS_CORE_API static Enum FromString(const FileCore::AString& value);
	};

	struct FilePopulatePolicy
	{
		enum Enum
		{
			Undefined   = P4VFS_POPULATE_POLICY_UNDEFINED,
			Depot		= P4VFS_POPULATE_POLICY_DEPOT,    
			Share		= P4VFS_POPULATE_POLICY_SHARE,
		};
	};

	struct FileResidencyPolicy
	{
		enum Enum
		{
			Undefined   = P4VFS_RESIDENCY_POLICY_UNDEFINED,
			Resident    = P4VFS_RESIDENCY_POLICY_RESIDENT,
			Symlink     = P4VFS_RESIDENCY_POLICY_SYMLINK,
			RemoveFile  = P4VFS_RESIDENCY_POLICY_REMOVE_FILE,
		};
	};
	
	P4VFS_CORE_API HRESULT 
	ResolveFileResidency(
		FileCore::FileContext& context,
		const WCHAR* filePath,
		BYTE* fileResidencyPolicy
		);

	P4VFS_CORE_API bool
	IsExcludedProcessId(
		ULONG processId
		);

	P4VFS_CORE_API FileCore::String
	GetModuleVersion(
		);

	P4VFS_CORE_API FileCore::String
	GetDriverVersion(
		);

	P4VFS_CORE_API HRESULT 
	SetupInstallHinfSection(
		const WCHAR* sectionName,
		const WCHAR* filePath
		);

}}}
#pragma managed(pop)
