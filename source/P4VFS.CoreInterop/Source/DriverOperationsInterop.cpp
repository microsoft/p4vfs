// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "DriverOperationsInterop.h"
#include "DriverOperations.h"
#include "CoreInterop.h"
#include "CoreMarshal.h"

namespace Microsoft {
namespace P4VFS {
namespace CoreInterop {

System::Int32 
DriverOperations::LoadFilter(
	System::String^ driverName
	)
{
	return P4VFS::DriverOperations::LoadFilter(marshal_as_wstring_c_str(driverName));
}

System::Int32
DriverOperations::UnloadFilter(
	System::String^ driverName
	)
{
	return P4VFS::DriverOperations::UnloadFilter(marshal_as_wstring_c_str(driverName));
}

System::Boolean
DriverOperations::IsFilterLoaded(
	System::String^ driverName
	)
{
	return P4VFS::DriverOperations::IsFilterLoaded(marshal_as_wstring_c_str(driverName));
}

System::Int32
DriverOperations::GetLoadedFilters(
	[System::Runtime::InteropServices::Out] array<System::String^>^% driverNames
	)
{
	driverNames = nullptr;
	FileCore::StringArray srcNames;

	HRESULT hr = P4VFS::DriverOperations::GetLoadedFilters(srcNames);
	if (SUCCEEDED(hr))
	{
		driverNames = Marshal::FromNativeWide(srcNames);
	}
	return hr;
}

System::Int32
DriverOperations::SetDevDriveFilterAllowed(
	System::String^ driverName,
	System::Boolean isAllowed
	)
{
	return P4VFS::DriverOperations::SetDevDriveFilterAllowed(marshal_as_wstring_c_str(driverName), isAllowed);
}

}}}

