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

public enum class DepotSyncAction : System::Int32
{
	None				= P4::DepotSyncAction::None,
	Added				= P4::DepotSyncAction::Added,
	Deleted				= P4::DepotSyncAction::Deleted,
	Updated				= P4::DepotSyncAction::Updated,
	Refreshed			= P4::DepotSyncAction::Refreshed,
	Replaced			= P4::DepotSyncAction::Replaced,
	UpToDate			= P4::DepotSyncAction::UpToDate,
	NoFilesFound		= P4::DepotSyncAction::NoFilesFound,
	NoFileAtRevision	= P4::DepotSyncAction::NoFileAtRevision,
	InvalidPattern		= P4::DepotSyncAction::InvalidPattern,
	NotInClientView		= P4::DepotSyncAction::NotInClientView,
	OpenedNotChanged	= P4::DepotSyncAction::OpenedNotChanged,
	CantClobber			= P4::DepotSyncAction::CantClobber,
	NeedsResolve		= P4::DepotSyncAction::NeedsResolve,
	GenericError		= P4::DepotSyncAction::GenericError,
};

public value class FDepotSyncAction
{
public:
	static bool IsError(DepotSyncAction value);
	static bool IsChanged(DepotSyncAction value);
	static bool IsLocalChanged(DepotSyncAction value);
};

[System::FlagsAttribute]
public enum class DepotSyncOption : System::Int32
{
	None				= P4::DepotSyncOption::None,
	FileWrite			= P4::DepotSyncOption::FileWrite,
	HaveFileWrite		= P4::DepotSyncOption::HaveFileWrite,
	ClientWrite			= P4::DepotSyncOption::ClientWrite,
	ClientClobber		= P4::DepotSyncOption::ClientClobber,
	FileSymlink			= P4::DepotSyncOption::FileSymlink,
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
	DepotSyncAction SyncAction;
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

