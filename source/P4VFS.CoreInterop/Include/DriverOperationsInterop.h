// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "DriverOperations.h"

namespace Microsoft {
namespace P4VFS {
namespace CoreInterop {

public ref class DriverOperations abstract sealed
{
public:

	static System::Int32 
	LoadFilter(
		System::String^ driverName
		);

	static System::Int32
	UnloadFilter(
		System::String^ driverName
		);

	static System::Boolean
	IsFilterLoaded(
		System::String^ driverName
		);

	static System::Int32
	GetLoadedFilters(
		[System::Runtime::InteropServices::Out] array<System::String^>^% driverNames
		);

	static System::Int32
	SetDevDriveFilterAllowed(
		System::String^ driverName,
		System::Boolean isAllowed
	);
};

}}}

