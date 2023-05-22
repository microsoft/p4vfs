// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "DepotClient.h"
#include "DepotSyncOptions.h"
#include "DepotDateTime.h"
#include "DepotResultDiff2.h"
#include "DepotResultFStat.h"
#pragma managed(push, off)

namespace Microsoft {
namespace P4VFS {
namespace P4 {

	struct P4VFS_CORE_API DepotOperations
	{
		static DepotSyncResult
		Sync(
			DepotClient& depotClient, 
			const DepotStringArray& files, 
			const DepotRevision revision = nullptr,
			DepotSyncType::Enum syncType = DepotSyncType::Normal,
			DepotSyncMethod::Enum syncMethod = DepotSyncMethod::Virtual, 
			DepotFlushType::Enum flushType = DepotFlushType::Atomic,  
			const DepotString& syncResident = DepotString()
			);

		static DepotSyncResult
		Sync(
			DepotClient& depotClient, 
			const FDepotSyncOptions& syncOptions
			);

		static DepotSyncResult
		SyncVirtual(
			DepotClient& depotClient, 
			const FDepotSyncOptions& syncOptions
			);

		struct FSyncVirtualModificationParams
		{
			FSyncVirtualModificationParams(LogDevice* log, DepotClient& depotClient, DepotSyncActionInfoArray& results) :
				m_Log(log),
				m_DepotClient(depotClient),
				m_Config(depotClient->Config()),
				m_Results(results),
				m_CancelationEvent(CreateEvent(NULL, TRUE, FALSE, NULL)),
				m_ClientListMutex(CreateMutex(NULL, FALSE, NULL)),
				m_ResultsMutex(CreateMutex(NULL, FALSE, NULL))
			{
				m_DepotClientList.push_back(depotClient);
			}

			LogDevice* m_Log;
			DepotClient& m_DepotClient;
			DepotConfig m_Config;
			DepotSyncActionInfoArray& m_Results;
			AutoHandle m_CancelationEvent;
			AutoHandle m_ClientListMutex;
			AutoHandle m_ResultsMutex;
			Array<DepotClient> m_DepotClientList;
		};

		static bool
		SyncVirtualModification(
			const DepotSyncActionInfo& modification,
			FSyncVirtualModificationParams& params
			);

		static void
		ApplyVirtualModification(
			DepotClient& depotClient, 
			const DepotConfig& depotConfig,
			const DepotSyncActionInfo& modification, 
			LogDevice* parentLog = nullptr
			);

		static bool
		CreateSymlinkFile(
			DepotClient& depotClient,
			const DepotSyncActionInfo& modification
			);

		static DepotSyncResult
		Hydrate(
			DepotClient& depotClient, 
			const FDepotSyncOptions& syncOptions
			);

		static bool
		InstallPlaceholderFile(
			DepotClient& depotClient,
			const DepotConfig& depotConfig,
			const DepotSyncActionInfo& modification
			);

		static bool
		UninstallPlaceholderFile(
			const DepotSyncActionInfo& modification
			);

		static DepotSyncResult
		SyncRegular(
			DepotClient& depotClient, 
			const FDepotSyncOptions& syncOptions
			);

		static DepotSyncActionInfoArray
		SyncCommand(
			DepotClient& depotClient, 
			const DepotStringArray& files, 
			DepotRevision revision,
			DepotSyncType::Enum syncType,
			FileCore::LogDevice* log = nullptr
			);

		static DepotRevision
		GetHeadRevisionChangelist(
			DepotClient& depotClient
			);

		struct CreateFileSpecFlags { enum Enum {
			None				= 0,
			OverrideRevison		= 1<<0,
		};};

		static DepotString
		CreateFileSpec(
			const DepotString& filePath, 
			const DepotRevision& revision, 
			CreateFileSpecFlags::Enum flags = CreateFileSpecFlags::None
			);

		static DepotStringArray
		CreateFileSpecs(
			DepotClient& depotClient,
			const DepotStringArray& filePaths, 
			const DepotRevision& revision, 
			CreateFileSpecFlags::Enum flags = CreateFileSpecFlags::None
			);

		static DepotResultDiff2
		Diff2(
			DepotClient& depotClient,
			const DepotString& fileSpec1, 
			const DepotString& fileSpec2
			);

		static DepotResultFStat 
		FStat(
			DepotClient& depotClient,
			const DepotStringArray& files,
			const DepotString& filterType = DepotString(),
			FDepotResultFStatField::Enum fields = FDepotResultFStatField::Default, 
			const DepotStringArray& optionArgs = DepotStringArray()
			);

		static bool
		IsFileTypeAlwaysResident(
			const DepotString& syncResident, 
			const DepotString& depotFile
			);

		static DepotString 
		ToString(
			const DepotStringArray& paths
			);

		static DepotString
		GetSymlinkTargetPath(
			DepotClient& depotClient,
			const DepotString& filePath, 
			const DepotRevision& revision = nullptr
			);

		static DepotString
		GetSymlinkTargetDepotFile(
			DepotClient& depotClient,
			const DepotString& filePath, 
			const DepotRevision& revision = nullptr
			);

		static DepotString
		NormalizePath(
			DepotString path
			);

		static bool 
		ResolveDepotServerName(
			const DepotString& sourceName, 
			DepotString& targetName
			);

		static DepotString 
		ResolveDepotServerName(
			const DepotString& sourceName
			);

		static DepotString
		ToDisplayStringBytes(
			uint64_t sizeBytes
			);

		static DepotString
		ToDisplayStringMilliseconds(
			int64_t totalMilliseconds
			);

		static void
		LogWarningErrorSummary(
			DepotClient& depotClient,
			const LogDeviceMemory& memoryLog
			);
	};

}}}

#pragma managed(pop)
