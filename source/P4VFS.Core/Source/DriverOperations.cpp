// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "DriverOperations.h"
#include <fltUser.h>

namespace Microsoft {
namespace P4VFS {
namespace DriverOperations {

// km/wdm.h
#define SE_LOAD_DRIVER_PRIVILEGE  (10L)

// km/ntddk.h
LUID
static RtlConvertLongToLuid(
	_In_ LONG Long
	)
{
	LUID TempLuid;
	LARGE_INTEGER TempLi;

	TempLi.QuadPart = Long;
	TempLuid.LowPart = TempLi.u.LowPart;
	TempLuid.HighPart = TempLi.u.HighPart;
	return TempLuid;
}

HRESULT
LoadFilter(
	const WCHAR* driverName
	)
{
	if (FileCore::StringInfo::IsNullOrEmpty(driverName))
	{
		return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	}

	HRESULT hr = EnableProcessTokenPrivilege(SE_LOAD_DRIVER_PRIVILEGE);
	if (FAILED(hr))
	{
		return hr;
	}

	hr = FilterLoad(driverName);
	if (hr == HRESULT_FROM_WIN32(ERROR_SERVICE_ALREADY_RUNNING))
	{
		hr = S_OK;
	}
	return hr;
}

HRESULT
UnloadFilter(
	const WCHAR* driverName
	)
{
	if (FileCore::StringInfo::IsNullOrEmpty(driverName))
	{
		return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	}

	HRESULT hr = EnableProcessTokenPrivilege(SE_LOAD_DRIVER_PRIVILEGE);
	if (FAILED(hr))
	{
		return hr;
	}

	if (IsFilterLoaded(driverName))
	{
		hr = FilterUnload(driverName);
	}
	return hr;
}

bool
IsFilterLoaded(
	const WCHAR* driverName,
	HRESULT* pHR
	)
{
	HRESULT tmpHR = S_OK;
	HRESULT& hr = pHR ? *pHR : tmpHR;
	
	if (FileCore::StringInfo::IsNullOrEmpty(driverName))
	{
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
		return false;
	}

	FileCore::StringArray driverNames;
	hr = GetLoadedFilters(driverNames);
	if (FAILED(hr))
	{
		return false;
	}

	for (const FileCore::String& name : driverNames)
	{
		if (FileCore::StringInfo::Stricmp(name.c_str(), driverName) == 0)
		{
			hr = S_OK;
			return true;
		}
	}

	hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
	return false;
}

HRESULT
SetDevDriveFilterAllowed(
	const WCHAR* driverName,
	bool isAllowed
	)
{
	if (FileCore::StringInfo::IsNullOrEmpty(driverName))
	{
		return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	}

	const WCHAR* fltMgrKeyName = TEXT("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\FilterManager");
	const WCHAR* fltMgrDevDriveAttachPolicyValueName = TEXT("FltmgrDevDriveAttachPolicy");
	FileCore::StringArray devDriveAllowedList;

	LSTATUS status = ERROR_SUCCESS;
	FileCore::RegistryValue devDriveValue = FileCore::RegistryInfo::GetKeyValue(HKEY_LOCAL_MACHINE, 
																				fltMgrKeyName, 
																				fltMgrDevDriveAttachPolicyValueName, 
																				&status);
	
	if (status == ERROR_SUCCESS && devDriveValue.m_Type == REG_MULTI_SZ)
	{
		devDriveAllowedList = devDriveValue.ToStringArray();
	}

	auto equalsDriverName = [driverName](const FileCore::String& V) -> bool
	{
		return FileCore::StringInfo::Stricmp(V.c_str(), driverName) == 0;
	};

	if (isAllowed)
	{
		FileCore::Algo::RemoveIf(devDriveAllowedList, equalsDriverName);
	}
	else if (FileCore::Algo::Any(devDriveAllowedList, equalsDriverName) == false)
	{
		devDriveAllowedList.push_back(driverName);
	}

 //       return RtlWriteRegistryValue( RTL_REGISTRY_ABSOLUTE,
 //                                     fltMgrKeyName,
 //                                     fltMgrDevDriveAttachPolicyValueName,
 //                                     REG_MULTI_SZ,
 //                                     FilterList->Buffer,
 //                                     FilterList->Length );
 //
//	FltiAttachPolicySetAllowList
 //
	return S_OK;
}

HRESULT
GetLoadedFilters(
	FileCore::StringArray& driverNames
	)
{
	HANDLE hFilter = INVALID_HANDLE_VALUE;
	BYTE infoBuffer[512];
	ULONG infoWritten = 0;

	driverNames.clear();

	HRESULT hr = FilterFindFirst(FilterFullInformation, infoBuffer, sizeof(infoBuffer), &infoWritten, &hFilter);
	while (SUCCEEDED(hr))
	{
		for (size_t infoRead = 0; infoRead+sizeof(FILTER_FULL_INFORMATION) < infoWritten;)
		{
			const PFILTER_FULL_INFORMATION info = reinterpret_cast<PFILTER_FULL_INFORMATION>(infoBuffer+infoRead);
			
			size_t nameLength = info->FilterNameLength/sizeof(WCHAR);
			if (nameLength > 0)
			{
				const FileCore::String filterName = FileCore::String(info->FilterNameBuffer, nameLength);
				if (FileCore::Algo::Contains(driverNames, filterName) == false)
				{
					driverNames.push_back(filterName);
				}
			}

			if (info->NextEntryOffset == 0)
			{
				break;
			}

			infoRead += info->NextEntryOffset;
		}

		hr = FilterFindNext(hFilter, FilterFullInformation, infoBuffer, sizeof(infoBuffer), &infoWritten);
	}

	if (hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
	{
		hr = S_OK;
	}

	if (hFilter != INVALID_HANDLE_VALUE) 
	{
		FilterFindClose(hFilter);
	}

	return hr;
}

HRESULT
EnableProcessTokenPrivilege(
	LONG privilege
	)
{
	FileCore::AutoHandle hToken;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, hToken.HandlePtr()) == FALSE)
	{
		return HRESULT_FROM_WIN32(ERROR_NO_TOKEN);
	}
	
	if (hToken.IsValid() == false)
	{
		return HRESULT_FROM_WIN32(ERROR_INVALID_TOKEN);
	}

	TOKEN_PRIVILEGES tokenPrivileges = {0};
	tokenPrivileges.PrivilegeCount = 1;
	tokenPrivileges.Privileges[0].Luid = RtlConvertLongToLuid(privilege);
	tokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (AdjustTokenPrivileges(hToken.Handle(), FALSE, &tokenPrivileges, 0, NULL, NULL) == FALSE)
	{
		return E_FAIL;
	}
	return S_OK;
}


}}}
