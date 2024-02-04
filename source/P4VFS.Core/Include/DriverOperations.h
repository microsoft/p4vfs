// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "FileCore.h"
#pragma managed(push, off)

namespace Microsoft {
namespace P4VFS {
namespace DriverOperations {

	P4VFS_CORE_API HRESULT
	LoadFilter(
		const WCHAR* driverName
		);

	P4VFS_CORE_API HRESULT
	UnloadFilter(
		const WCHAR* driverName
		);

	P4VFS_CORE_API bool
	IsFilterLoaded(
		const WCHAR* driverName,
		HRESULT* pHR = nullptr
		);

	P4VFS_CORE_API HRESULT
	SetDevDriveFilterAllowed(
		const WCHAR* driverName,
		bool isAllowed
		);

	P4VFS_CORE_API HRESULT
	GetLoadedFilters(
		FileCore::StringArray& driverNames
		);

	P4VFS_CORE_API HRESULT
	EnableProcessTokenPrivilege(
		LONG privilege
		);
	
}}}
#pragma managed(pop)
