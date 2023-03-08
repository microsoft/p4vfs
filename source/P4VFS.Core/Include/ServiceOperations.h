// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "FileCore.h"
#pragma managed(push, off)

namespace Microsoft {
namespace P4VFS {
namespace ServiceOperations {

	static constexpr DWORD DefaultServiceTimeout = 30000;

	P4VFS_CORE_API HRESULT 
	InstallLocalService(
		const WCHAR* pathToServiceBinary,
		const WCHAR* serviceName,
		const WCHAR* friendlyServiceName,
		const WCHAR* serviceDescription
		);

	struct UninstallFlags
	{
		enum Enum
		{
			None		= 0,
			NoDelete	= 1<<0,
		};
	};

	P4VFS_CORE_API HRESULT 
	UninstallLocalService(
		const WCHAR* serviceName,
		UninstallFlags::Enum flags = UninstallFlags::None
		);

	P4VFS_CORE_API HRESULT
	StartLocalService(
		const WCHAR* serviceName,
		DWORD dwTimeout = DefaultServiceTimeout
		);

	P4VFS_CORE_API HRESULT
	StopLocalService(
		const WCHAR* serviceName,
		DWORD dwTimeout = DefaultServiceTimeout
		);

	P4VFS_CORE_API DWORD
	GetLocalServiceState(
		const WCHAR* serviceName
		);
	
}}}
#pragma managed(pop)
