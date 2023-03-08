// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "ServiceOperations.h"

namespace Microsoft {
namespace P4VFS {
namespace CoreInterop {

public ref class ServiceOperations abstract sealed
{
public:

	static System::Int32 
	InstallLocalService(
		System::String^		pathToServiceBinary,
		System::String^		serviceName,
		System::String^		friendlyServiceName,
		System::String^		serviceDescription
		);

	[System::FlagsAttribute]
	enum class UninstallFlags : System::Int32
	{
		None		= P4VFS::ServiceOperations::UninstallFlags::None,
		NoDelete	= P4VFS::ServiceOperations::UninstallFlags::NoDelete,
	};

	static System::Int32 
	UninstallLocalService(
		System::String^		serviceName,
		UninstallFlags		flags
		);

	static System::Int32 
	StartLocalService(
		System::String^		serviceName
		);

	static System::Int32 
	StopLocalService(
		System::String^		serviceName
		);

	static System::UInt32 
	GetLocalServiceState(
		System::String^		serviceName
		);
};

}}}

