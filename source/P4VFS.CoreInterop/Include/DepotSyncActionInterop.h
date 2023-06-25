// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "DepotSyncAction.h"

namespace Microsoft {
namespace P4VFS {
namespace CoreInterop {

[System::FlagsAttribute]
public enum class DepotSyncType : System::Int32
{
	Normal			= P4::DepotSyncType::Normal,
	Force			= P4::DepotSyncType::Force,
	Flush			= P4::DepotSyncType::Flush,
	Preview			= P4::DepotSyncType::Preview,
	IgnoreOutput	= P4::DepotSyncType::IgnoreOutput,
	Quiet			= P4::DepotSyncType::Quiet,
	Writeable		= P4::DepotSyncType::Writeable,
};

public enum class DepotFlushType : System::Int32
{
	Single			= P4::DepotFlushType::Single,
	Atomic			= P4::DepotFlushType::Atomic,
};

public enum class DepotSyncActionType : System::Int32
{
	None				= P4::DepotSyncActionType::None,
	Added				= P4::DepotSyncActionType::Added,
	Deleted				= P4::DepotSyncActionType::Deleted,
	Updated				= P4::DepotSyncActionType::Updated,
	Refreshed			= P4::DepotSyncActionType::Refreshed,
	Replaced			= P4::DepotSyncActionType::Replaced,
	UpToDate			= P4::DepotSyncActionType::UpToDate,
	NoFilesFound		= P4::DepotSyncActionType::NoFilesFound,
	NoFileAtRevision	= P4::DepotSyncActionType::NoFileAtRevision,
	InvalidPattern		= P4::DepotSyncActionType::InvalidPattern,
	NotInClientView		= P4::DepotSyncActionType::NotInClientView,
	OpenedNotChanged	= P4::DepotSyncActionType::OpenedNotChanged,
	CantClobber			= P4::DepotSyncActionType::CantClobber,
	NeedsResolve		= P4::DepotSyncActionType::NeedsResolve,
	GenericError		= P4::DepotSyncActionType::GenericError,
};

public value class FDepotSyncActionType
{
public:
	static bool IsError(DepotSyncActionType value);
	static bool IsChanged(DepotSyncActionType value);
	static bool IsLocalChanged(DepotSyncActionType value);
};

[System::FlagsAttribute]
public enum class DepotSyncActionFlags : System::Int32
{
	None				= P4::DepotSyncActionFlags::None,
	FileWrite			= P4::DepotSyncActionFlags::FileWrite,
	HaveFileWrite		= P4::DepotSyncActionFlags::HaveFileWrite,
	ClientWrite			= P4::DepotSyncActionFlags::ClientWrite,
	ClientClobber		= P4::DepotSyncActionFlags::ClientClobber,
	FileSymlink			= P4::DepotSyncActionFlags::FileSymlink,
};

public ref class DepotSyncActionInfo
{
public:
	DepotSyncActionInfo();

	P4::DepotSyncActionInfo ToNative();
	static DepotSyncActionInfo^ FromNative(const P4::DepotSyncActionInfo& src);

	System::String^ DepotFile;
	System::String^ ClientFile;
	System::String^ Revision;
	DepotSyncActionType SyncActionType;
	System::String^ Message;
};

[System::FlagsAttribute]
public enum class DepotSyncStatus : System::Int32
{
	Success		= P4::DepotSyncStatus::Success,
	Warning		= P4::DepotSyncStatus::Warning,
	Error		= P4::DepotSyncStatus::Error,
};

public ref class DepotSyncResult
{
public:
	DepotSyncResult();

	P4::DepotSyncResult ToNative();
	static DepotSyncResult^ FromNative(const P4::DepotSyncResult& src);

	DepotSyncStatus Status;
	array<DepotSyncActionInfo^>^ Modifications;
};

}}}

