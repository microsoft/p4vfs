// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "ServiceOperationsInterop.h"
#include "ServiceOperations.h"
#include "CoreInterop.h"
#include "CoreMarshal.h"

namespace Microsoft {
namespace P4VFS {
namespace CoreInterop {

System::Int32 
ServiceOperations::InstallLocalService(
	System::String^		pathToServiceBinary,
	System::String^		serviceName,
	System::String^		friendlyServiceName,
	System::String^		serviceDescription
	)
{
	return P4VFS::ServiceOperations::InstallLocalService(
		marshal_as_wchar(pathToServiceBinary), 
		marshal_as_wchar(serviceName), 
		marshal_as_wchar(friendlyServiceName),
		marshal_as_wchar(serviceDescription)
		);
}

System::Int32 
ServiceOperations::UninstallLocalService(
	System::String^		serviceName,
	UninstallFlags		flags
	)
{
	return P4VFS::ServiceOperations::UninstallLocalService(
		marshal_as_wchar(serviceName),
		safe_cast<P4VFS::ServiceOperations::UninstallFlags::Enum>(flags)
		);
}

System::Int32 
ServiceOperations::StartLocalService(
	System::String^		serviceName
	)
{
	return P4VFS::ServiceOperations::StartLocalService(
		marshal_as_wchar(serviceName)
		);
}

System::Int32 
ServiceOperations::StopLocalService(
	System::String^		serviceName
	)
{
	return P4VFS::ServiceOperations::StopLocalService(
		marshal_as_wchar(serviceName)
		);
}

System::UInt32 
ServiceOperations::GetLocalServiceState(
	System::String^		serviceName
	)
{
	return P4VFS::ServiceOperations::GetLocalServiceState(
		marshal_as_wchar(serviceName)
		);
}

}}}

