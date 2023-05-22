// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "CoreInterop.h"
#include "DepotOperations.h"
#include "DepotConfigInterop.h"
#include "DepotClientInterop.h"
#include "DepotSyncActionInterop.h"
#include "DepotSyncOptionsInterop.h"

namespace Microsoft {
namespace P4VFS {
namespace CoreInterop {

public ref class DepotOperations abstract sealed
{
public:

	static DepotSyncResult^
	Sync(
		DepotClient^ depotClient,
		DepotSyncOptions^ syncOptions
		);

	static DepotSyncResult^
	Hydrate(
		DepotClient^ depotClient, 
		DepotSyncOptions^ syncOptions
		);

	static bool
	InstallPlaceholderFile(
		DepotClient^ depotClient,
		DepotSyncActionInfo^ modification
		);

	static System::String^
	GetHeadRevisionChangelist(
		DepotClient^ depotClient
		);

	static bool
	UninstallPlaceholderFile(
		DepotSyncActionInfo^ modification
		);

	[System::FlagsAttribute]
	enum class CreateFileSpecFlags : System::Int32
	{
		None				= P4::DepotOperations::CreateFileSpecFlags::None,
		OverrideRevison		= P4::DepotOperations::CreateFileSpecFlags::OverrideRevison,
	};

	static System::String^
	CreateFileSpec(
		System::String^ filePath, 
		System::String^ revision, 
		CreateFileSpecFlags flags
		);

	static array<System::String^>^
	CreateFileSpecs(
		DepotClient^ depotClient,
		array<System::String^>^ filePaths, 
		System::String^ revision, 
		CreateFileSpecFlags flags
		);

	static bool
	IsFileTypeAlwaysResident(
		System::String^ syncResident, 
		System::String^ depotFile
		);

	static System::String^
	GetSymlinkTargetPath(
		DepotClient^ depotClient,
		System::String^ filePath, 
		System::String^ revision
		);

	static System::String^
	GetSymlinkTargetDepotFile(
		DepotClient^ depotClient,
		System::String^ filePath, 
		System::String^ revision
		);

	static System::String^
	NormalizePath(
		System::String^ path
		);

	static System::String^
	ResolveDepotServerName(
		System::String^ sourceName
		);

	static System::String^
	GetClientOwnerUserName(
		DepotClient^ depotClient,
		System::String^ clientName, 
		System::String^ portName
		);
};

}}}

