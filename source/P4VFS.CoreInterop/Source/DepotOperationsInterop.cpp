// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "DepotOperationsInterop.h"
#include "DepotConstantsInterop.h"
#include "FileCore.h"
#include "FileContext.h"
#include "CoreMarshal.h"

namespace Microsoft {
namespace P4VFS {
namespace CoreInterop {

DepotSyncResult^
DepotOperations::Sync(
	DepotClient^ depotClient,
	DepotSyncOptions^ syncOptions
	)
{
	P4::FDepotSyncOptions options = syncOptions->ToNative();
	P4::DepotClient client = depotClient->ToNative();
	P4::DepotSyncResult syncResult = P4::DepotOperations::Sync(client, options);
	return DepotSyncResult::FromNative(syncResult);
}

DepotSyncResult^
DepotOperations::Hydrate(
	DepotClient^ depotClient,
	DepotSyncOptions^ syncOptions
	)
{
	P4::FDepotSyncOptions options = syncOptions->ToNative();
	P4::DepotClient client = depotClient->ToNative();
	P4::DepotSyncResult syncResult = P4::DepotOperations::Hydrate(client, options);
	return DepotSyncResult::FromNative(syncResult);
}

bool
DepotOperations::Reconfig(
	DepotClient^ depotClient, 
	DepotReconfigOptions^ reconfigOptions
	)
{
	P4::FDepotReconfigOptions options = reconfigOptions->ToNative();
	P4::DepotClient client = depotClient->ToNative();
	return P4::DepotOperations::Reconfig(client, options);
}

System::String^
DepotOperations::GetHeadRevisionChangelist(
	DepotClient^ depotClient
	)
{
	P4::DepotRevision revision = P4::DepotOperations::GetHeadRevisionChangelist(depotClient->ToNative());
	if (revision.get() != nullptr)
	{
		return gcnew System::String(P4::FDepotRevision::ToString(revision).c_str());
	}
	return nullptr;
}

bool
DepotOperations::InstallPlaceholderFile(
	DepotClient^ depotClient,
	DepotSyncActionInfo^ modification
	)
{
	P4::DepotClient nativeClient = depotClient->ToNative();
	return P4::DepotOperations::InstallPlaceholderFile(
		nativeClient, 
		nativeClient->Config(),
		modification->ToNative());
}

bool
DepotOperations::UninstallPlaceholderFile(
	DepotSyncActionInfo^ modification
	)
{
	return P4::DepotOperations::UninstallPlaceholderFile(
		modification->ToNative());
}

System::String^
DepotOperations::CreateFileSpec(
	System::String^ filePath, 
	System::String^ revision, 
	CreateFileSpecFlags flags
	)
{
	P4::DepotString result = P4::DepotOperations::CreateFileSpec(
		marshal_as_astring(filePath),
		P4::FDepotRevision::FromString(marshal_as_astring(revision)),
		safe_cast<P4::DepotOperations::CreateFileSpecFlags::Enum>(flags));

	return gcnew System::String(result.c_str());
}

array<System::String^>^
DepotOperations::CreateFileSpecs(
	DepotClient^ depotClient,
	array<System::String^>^ filePaths, 
	System::String^ revision, 
	CreateFileSpecFlags flags
	)
{
	P4::DepotStringArray result = P4::DepotOperations::CreateFileSpecs(
		depotClient->ToNative(),
		Marshal::ToNativeAnsi(filePaths),
		P4::FDepotRevision::FromString(marshal_as_astring(revision)),
		safe_cast<P4::DepotOperations::CreateFileSpecFlags::Enum>(flags));

	return Marshal::FromNative(result);
}

bool
DepotOperations::IsFileTypeAlwaysResident(
	System::String^ syncResident, 
	System::String^ depotFile
	)
{
	return P4::DepotOperations::IsFileTypeAlwaysResident(
		marshal_as_astring(syncResident),
		marshal_as_astring(depotFile));
}

System::String^
DepotOperations::GetSymlinkTargetPath(
	DepotClient^ depotClient,
	System::String^ filePath, 
	System::String^ revision
	)
{
	P4::DepotString result = P4::DepotOperations::GetSymlinkTargetPath(
		depotClient->ToNative(),
		marshal_as_astring(filePath),
		P4::FDepotRevision::FromString(marshal_as_astring(revision)));

	return gcnew System::String(result.c_str());
}

System::String^
DepotOperations::GetSymlinkTargetDepotFile(
	DepotClient^ depotClient,
	System::String^ filePath, 
	System::String^ revision
	)
{
	P4::DepotString result = P4::DepotOperations::GetSymlinkTargetDepotFile(
		depotClient->ToNative(),
		marshal_as_astring(filePath),
		P4::FDepotRevision::FromString(marshal_as_astring(revision)));

	return gcnew System::String(result.c_str());
}

System::String^
DepotOperations::NormalizePath(
	System::String^ path
	)
{
	P4::DepotString result = P4::DepotOperations::NormalizePath(
		marshal_as_astring(path));
		
	return gcnew System::String(result.c_str());
}

System::String^ 
DepotOperations::ResolveDepotServerName(
	System::String^ sourceName
	)
{
	P4::DepotString targetName;
	if (P4::DepotOperations::ResolveDepotServerName(marshal_as_astring(sourceName), targetName))
	{
		return gcnew System::String(targetName.c_str());
	}
	return sourceName;
}

System::String^
DepotOperations::GetClientOwnerUserName(
	DepotClient^ depotClient,
	System::String^ clientName, 
	System::String^ portName
	)
{
	P4::DepotString userName = depotClient->ToNative()->GetClientOwnerUserName(
		marshal_as_astring(clientName),
		marshal_as_astring(portName));

	return gcnew System::String(userName.c_str());
}

}}}

