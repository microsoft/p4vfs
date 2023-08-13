// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "CoreInterop.h"
#include "DepotSyncActionInterop.h"
#include "DepotSyncOptions.h"

namespace Microsoft {
namespace P4VFS {
namespace CoreInterop {

public enum class DepotSyncMethod : System::Int32
{
	Regular			= P4::DepotSyncMethod::Regular,
	Virtual			= P4::DepotSyncMethod::Virtual,
};

public ref class DepotSyncOptions
{
public:
	DepotSyncOptions();

	P4::FDepotSyncOptions ToNative();

	array<System::String^>^ Files;
	System::String^ Revision;
	DepotSyncFlags SyncFlags;
	DepotFlushType FlushType;
	DepotSyncMethod SyncMethod;
	System::String^ SyncResident;
	UserContext^ Context;
};

}}}

