// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "DepotRevision.h"
#include "DepotConfig.h"
#include "FileContext.h"

namespace Microsoft {
namespace P4VFS {
namespace P4 {

	struct DepotSyncFlags
	{
		enum Enum
		{
			Normal			= 0,
			Force			= 1<<0,
			Flush			= 1<<1,
			Preview			= 1<<2,
			IgnoreOutput	= 1<<3,
			Quiet			= 1<<4,
			Writeable		= 1<<5,
			ClientSize		= 1<<6,
		};

		P4VFS_CORE_API static DepotString ToString(Enum value);
	};

	struct DepotFlushType
	{
		enum Enum
		{
			Single,
			Atomic,
		};

		P4VFS_CORE_API static DepotString ToString(Enum value);
	};

	struct DepotSyncActionType
	{
		enum Enum
		{
			None,
			Added,
			Deleted,
			Updated,
			Refreshed,
			Replaced,
			UpToDate,
			NoFilesFound,
			NoFileAtRevision,
			InvalidPattern,
			NotInClientView,
			OpenedNotChanged,
			CantClobber,
			NeedsResolve,
			GenericError,
		};

		P4VFS_CORE_API static bool IsError(Enum value);
		P4VFS_CORE_API static bool IsChanged(Enum value);
		P4VFS_CORE_API static bool IsLocalChanged(Enum value);
		P4VFS_CORE_API static DepotString ToString(Enum value);
		P4VFS_CORE_API static Enum FromString(const DepotString& value);
	};

	struct DepotSyncActionFlags
	{
		enum Enum
		{
			None			= 0,
			FileWrite		= 1<<0,
			HaveFileWrite	= 1<<1,
			ClientWrite		= 1<<2,
			ClientClobber	= 1<<3,
			FileSymlink		= 1<<4,
		};

		P4VFS_CORE_API static DepotString ToString(Enum value);
	};

	typedef std::shared_ptr<struct FDepotSyncActionInfo> DepotSyncActionInfo;
	typedef std::shared_ptr<Array<DepotSyncActionInfo>> DepotSyncActionInfoArray;

	struct FDepotSyncActionInfo
	{
		static const std::basic_regex<char> m_ErrorRegex;
		static const std::basic_regex<char> m_InfoRegex;

		DepotString m_DepotFile;
		DepotString m_ClientFile;
		int64_t m_FileSize;
		DepotRevision m_Revision;
		DepotSyncActionType::Enum m_SyncActionType;
		DepotSyncActionFlags::Enum m_SyncActionFlags;
		DepotSyncFlags::Enum m_SyncFlags;
		DepotFlushType::Enum m_FlushType;
		int64_t m_DiskFileSize;
		int64_t m_VirtualFileSize;
		bool m_IsAlwaysResident;
		int64_t m_PlaceholderTime;
		int64_t m_FlushTime;
		int64_t m_SyncTime;
		DepotString m_Message;
		FileCore::Array<DepotSyncActionInfo> m_SubActions;

		P4VFS_CORE_API FDepotSyncActionInfo();
		P4VFS_CORE_API ~FDepotSyncActionInfo();

		DepotString ToString() const;
		DepotString ToFileSpecString() const;

		int32_t RevisionNumber() const;
		bool CanModifyWritableFile() const;
		bool CanSetWritableFile() const;
		bool IsPreview() const;

		static DepotSyncActionInfo FromInfoOutput(const DepotString& infoText, FileCore::LogDevice* log = nullptr);
		static DepotSyncActionInfo FromTaggedOutput(const FDepotResultTag& tag, FileCore::LogDevice* log = nullptr);
		static DepotSyncActionInfo FromErrorOutput(const DepotString& errorText, FileCore::LogDevice* log = nullptr);
	};

	struct DepotSyncStatus
	{
		enum Enum
		{
			Success		= 0,
			Warning		= 1<<0,
			Error		= 1<<1,
		};

		P4VFS_CORE_API static DepotString ToString(Enum value);
		P4VFS_CORE_API static Enum FromLogElement(const LogElement& element);
		P4VFS_CORE_API static Enum FromLog(const LogDeviceMemory& log);
	};

	struct FDepotSyncResult
	{
		P4VFS_CORE_API FDepotSyncResult(DepotSyncStatus::Enum status = DepotSyncStatus::Success, DepotSyncActionInfoArray modifications = nullptr);
		P4VFS_CORE_API ~FDepotSyncResult();

		DepotSyncStatus::Enum m_Status;
		DepotSyncActionInfoArray m_Modifications;
	};

	typedef std::shared_ptr<FDepotSyncResult> DepotSyncResult;

	DEFINE_ENUM_FLAG_OPERATORS(DepotSyncFlags::Enum);
	DEFINE_ENUM_FLAG_OPERATORS(DepotSyncActionFlags::Enum);
	DEFINE_ENUM_FLAG_OPERATORS(DepotSyncStatus::Enum);
}}}
