// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "DepotOperations.h"
#include "DepotDateTime.h"
#include "DepotResultPrint.h"
#include "DepotConstants.h"
#include "DriverVersion.h"
#include "FileSystem.h"
#include "FileOperations.h"
#include "SettingManager.h"
#include "ThreadPool.h"

namespace Microsoft {
namespace P4VFS {
namespace P4 {

DepotSyncResult
DepotOperations::Sync(
	DepotClient& depotClient, 
	const DepotStringArray& files, 
	const DepotRevision revision, 
	DepotSyncFlags::Enum syncFlags,
	DepotSyncMethod::Enum syncMethod,
	DepotFlushType::Enum flushType,
	const DepotString& syncResident
)
{
	FDepotSyncOptions syncOptions;
	syncOptions.m_Files = files;
	syncOptions.m_Revision = revision;
	syncOptions.m_SyncFlags = syncFlags;
	syncOptions.m_SyncMethod = syncMethod;
	syncOptions.m_FlushType = flushType;
	syncOptions.m_SyncResident = syncResident;

	return Sync(depotClient, syncOptions);
}

DepotSyncResult
DepotOperations::Sync(
	DepotClient& depotClient, 
	const FDepotSyncOptions& syncOptions
	)
{
	if (syncOptions.m_SyncMethod == DepotSyncMethod::Regular || syncOptions.m_SyncFlags & (DepotSyncFlags::Preview|DepotSyncFlags::Flush))
	{
		return SyncRegular(depotClient, syncOptions);
	}
	else
	{
		return SyncVirtual(depotClient, syncOptions);
	}
}

DepotSyncResult
DepotOperations::SyncVirtual(
	DepotClient& depotClient, 
	const FDepotSyncOptions& syncOptions
	)
{
	DepotStopwatch totalTimer(DepotStopwatch::Init::Start);

	DepotRevision revision = syncOptions.m_Revision;
	if (revision.get() == nullptr || revision->IsHeadRevision())
	{
		revision = GetHeadRevisionChangelist(depotClient);
		if (revision.get() == nullptr)
		{
			depotClient->Log(LogChannel::Error, "Missing specific head revision");
			return std::make_shared<FDepotSyncResult>(DepotSyncStatus::Error);
		}
	}

	depotClient->Log(LogChannel::Info, StringInfo::Join(DepotStringArray{ "Virtual Sync:", ToString(syncOptions.m_Files), DepotSyncFlags::ToString(syncOptions.m_SyncFlags), DepotFlushType::ToString(syncOptions.m_FlushType), revision->ToString() }, " "));
	depotClient->Log(LogChannel::Info, StringInfo::Format("Started at [%s] version [%s]", DepotDateTime::Now().ToDisplayString().c_str(), CSTR_WTOA(FileSystem::GetModuleVersion())));
	if (depotClient->IsFaulted())
	{
		return std::make_shared<FDepotSyncResult>(DepotSyncStatus::Error);
	}

	DepotSyncFlags::Enum primarySyncFlags = syncOptions.m_SyncFlags & ~DepotSyncFlags::IgnoreOutput;
	if (syncOptions.m_FlushType == DepotFlushType::Single)
	{
		primarySyncFlags |= DepotSyncFlags::Flush;
	}
	else
	{
		primarySyncFlags |= DepotSyncFlags::Preview;
	}

	// Retrieve a list of files to be added, deleted, and updated
	DepotSyncActionInfoArray modifications = SyncCommand(depotClient, syncOptions.m_Files, revision, primarySyncFlags | DepotSyncFlags::Quiet);
	if (modifications.get() == nullptr)
	{
		return std::make_shared<FDepotSyncResult>(DepotSyncStatus::Error);
	}

	depotClient->Log(LogChannel::Info, StringInfo::Format("%I64u Modification message%s to act on.", uint64_t(modifications->size()), modifications->size() ? "s" : ""));
	ThreadPool::ForEach::Execute(
		modifications->data(), 
		modifications->size(), 
		[&syncOptions, primarySyncFlags](DepotSyncActionInfo& modification) -> void
		{
			modification->m_SyncFlags = syncOptions.m_SyncFlags;
			modification->m_FlushType = syncOptions.m_FlushType;
			modification->m_IsAlwaysResident = IsFileTypeAlwaysResident(syncOptions.m_SyncResident, modification->m_DepotFile);

			if (primarySyncFlags & DepotSyncFlags::Writeable)
			{
				modification->m_SyncActionFlags |= DepotSyncActionFlags::ClientClobber;
			}
		});

	if (depotClient->IsFaulted())
	{
		return std::make_shared<FDepotSyncResult>(DepotSyncStatus::Error);
	}

	Array<DepotSyncActionInfo> virtualModifications;
	Array<DepotSyncActionInfo> residentModifications;
	if (syncOptions.m_FlushType == DepotFlushType::Single)
	{
		virtualModifications.reserve(modifications->size());
		residentModifications.reserve(modifications->size());
		for (const DepotSyncActionInfo& modification : *modifications)
		{
			if ((modification->m_SyncActionFlags & DepotSyncActionFlags::FileSymlink) == 0 && modification->m_IsAlwaysResident)
			{
				residentModifications.push_back(modification);
			}
			else
			{
				virtualModifications.push_back(modification);
			}
		}
	}
	else
	{
		virtualModifications = *modifications;
	}

	DepotSyncActionInfoArray resultModifications;
	if ((syncOptions.m_SyncFlags & DepotSyncFlags::IgnoreOutput) == 0)
	{
		resultModifications = std::make_shared<DepotSyncActionInfoArray::element_type>();
	}

	LogDeviceMemory memoryLog;
	LogDeviceAggregate aggregateLog;
	LogDevice* log = &memoryLog;

	if (depotClient->Log() != nullptr)
	{
		aggregateLog.m_Devices.push_back(depotClient->Log());
		aggregateLog.m_Devices.push_back(&memoryLog);
		log = &aggregateLog;
	}

	DepotStopwatch virtualModTimer(DepotStopwatch::Init::Start);
	if (modifications->size() > 0)
	{
		Array<DepotClient> depotClientList;
		depotClientList.push_back(depotClient);

		size_t maxThreads = size_t(std::max(1, SettingManager::StaticInstance().MaxSyncConnections.GetValue()));
		FSyncVirtualModificationParams params(log, depotClient, resultModifications);

		ThreadPool::ForEach::ExecuteImpersonated(
			maxThreads, 
			modifications->data(), 
			modifications->size(), 
			params.m_CancelationEvent.Handle(), 
			params.m_DepotClient->GetUserContext(), 
			[&params](const DepotSyncActionInfo& modification) -> void
			{
				if (params.m_Results.get())
				{
					AutoMutex resultslock(params.m_ResultsMutex);
					params.m_Results->push_back(modification);
				}

				if (SyncVirtualModification(modification, params) == false)
				{
					params.m_DepotClient->Log(LogChannel::Info, "Aborting Sync from SyncVirtualModification");
					SetEvent(params.m_CancelationEvent.Handle());
					return;
				}

				if (params.m_DepotClient->IsFaulted())
				{
					params.m_DepotClient->Log(LogChannel::Info, "Aborting Sync from DepotClient fault");
					SetEvent(params.m_CancelationEvent.Handle());
					return;
				}
			}
		);
	}

	virtualModTimer.Stop();
	DepotStopwatch residentModTimer(DepotStopwatch::Init::Start);

	// Force sync the always resident modifications
	if (residentModifications.size())
	{
		if (resultModifications.get())
		{
			Algo::Append(*resultModifications, residentModifications);
		}

		DepotStringArray residentFileSpecs;
		for (const DepotSyncActionInfo& modification : residentModifications)
		{
			if (modification->IsPreview() == false)
				residentFileSpecs.push_back(modification->ToFileSpecString());
		}
		if (residentFileSpecs.size())
		{
			SyncCommand(depotClient, residentFileSpecs, revision, DepotSyncFlags::Force | DepotSyncFlags::IgnoreOutput);
		}
	}

	// Exit early if we havn't gathered any results
	if (resultModifications.get() == nullptr)
	{
		return std::make_shared<FDepotSyncResult>(DepotSyncStatus::Success);
	}

	residentModTimer.Stop();
	totalTimer.Stop();

	if (depotClient->IsFaulted())
	{
		return std::make_shared<FDepotSyncResult>(DepotSyncStatus::Error, resultModifications);
	}

	// Derive a DepotSyncStatus from the log output for this operation
	DepotSyncStatus::Enum status = DepotSyncStatus::FromLog(memoryLog);
	if (status != DepotSyncStatus::Success)
	{
		LogWarningErrorSummary(depotClient, memoryLog);
	}

	int64_t totalTime = totalTimer.TotalMilliseconds();
	int64_t fileModTime = virtualModTimer.TotalMilliseconds() + residentModTimer.TotalMilliseconds();
	int64_t virtualFileSize = Algo::Sum<int64_t>(*resultModifications, [](const DepotSyncActionInfo& m) -> int64_t { return m->m_VirtualFileSize; });
	int64_t diskFileSize = Algo::Sum<int64_t>(*resultModifications, [](const DepotSyncActionInfo& m) -> int64_t { return m->m_DiskFileSize; });
	int64_t flushTime = Algo::Sum<int64_t>(*resultModifications, [](const DepotSyncActionInfo& m) -> int64_t { return m->m_FlushTime; });
	int64_t placeholderTime = Algo::Sum<int64_t>(*resultModifications, [](const DepotSyncActionInfo& m) -> int64_t { return m->m_PlaceholderTime; });
	int64_t syncTime = Algo::Sum<int64_t>(*resultModifications, [](const DepotSyncActionInfo& m) -> int64_t { return m->m_SyncTime; });

	depotClient->Log(LogChannel::Info, "Virtual Sync Summary:");
	depotClient->Log(LogChannel::Info,    StringInfo::Format("Total Files:         %u / %u", resultModifications->size(), modifications->size()));
	depotClient->Log(LogChannel::Info,    StringInfo::Format("Total Time:          %s", ToDisplayStringMilliseconds(totalTime).c_str()));
	depotClient->Log(LogChannel::Info,    StringInfo::Format("Virtual Mod Time:    %s", ToDisplayStringMilliseconds(virtualModTimer.TotalMilliseconds()).c_str()));
	depotClient->Log(LogChannel::Info,    StringInfo::Format("Resident Mod Time:   %s", ToDisplayStringMilliseconds(residentModTimer.TotalMilliseconds()).c_str()));
	depotClient->Log(LogChannel::Info,    StringInfo::Format("Virtual File Size:   %s", ToDisplayStringBytes(virtualFileSize).c_str()));
	depotClient->Log(LogChannel::Info,    StringInfo::Format("Disk File Size:      %s", ToDisplayStringBytes(diskFileSize).c_str()));
	depotClient->Log(LogChannel::Verbose, StringInfo::Format("File Time:           %s", ToDisplayStringMilliseconds(fileModTime).c_str()));
	depotClient->Log(LogChannel::Verbose, StringInfo::Format("Flush Time:          %s", ToDisplayStringMilliseconds(flushTime).c_str()));
	depotClient->Log(LogChannel::Verbose, StringInfo::Format("Placeholder Time:    %s", ToDisplayStringMilliseconds(placeholderTime).c_str()));
	depotClient->Log(LogChannel::Verbose, StringInfo::Format("Sync Time:           %s", ToDisplayStringMilliseconds(syncTime).c_str()));

	return std::make_shared<FDepotSyncResult>(status, resultModifications);
}

bool
DepotOperations::SyncVirtualModification(
	const DepotSyncActionInfo& modification,
	FSyncVirtualModificationParams& params
	)
{
	if ((modification->m_IsAlwaysResident == false) && 
		(modification->m_SyncActionFlags & DepotSyncActionFlags::FileSymlink) == 0 &&
		(modification->m_FlushType == DepotFlushType::Single))
	{
		ApplyVirtualModification(DepotClient(), params.m_Config, modification, params.m_Log);
		return true;
	}

	DepotClient depotClient;
	{
		AutoMutex clientListlock(params.m_ClientListMutex);
		if (params.m_DepotClientList.size())
		{
			depotClient = params.m_DepotClientList.back();
			params.m_DepotClientList.pop_back();
		}
	}

	if (depotClient.get() == nullptr)
	{
		depotClient = FDepotClient::New(params.m_DepotClient->GetContext());
		if (depotClient->Connect(params.m_DepotClient->Config()) == false)
		{
			LogDevice::WriteLine(params.m_Log, LogChannel::Error, "DepotClient failed to connect");
			return false;
		}
	}

	if (depotClient->IsConnected() == false)
	{
		LogDevice::WriteLine(params.m_Log, LogChannel::Error, "DepotClient not connected");
		return false;
	}

	if (depotClient->IsFaulted())
	{
		LogDevice::WriteLine(params.m_Log, LogChannel::Error, "DepotClient faulted");
		return false;
	}

	ApplyVirtualModification(depotClient, params.m_Config, modification, params.m_Log);
	{
		AutoMutex clientListlock(params.m_ClientListMutex);
		params.m_DepotClientList.push_back(depotClient);
	}
	return true;
}

void
DepotOperations::ApplyVirtualModification(
	DepotClient& depotClient,
	const DepotConfig& depotConfig,
	const DepotSyncActionInfo& modification,
	LogDevice* parentLog)
{
	// If we have nested actions, buffer up the output.
	LogDeviceMemory memoryLog;
	LogDevice* log = modification->m_SubActions.size() > 0 ? &memoryLog : parentLog;

	// We only handle the Added, Updated, or Deleted events from Perforce
	switch (modification->m_SyncActionType)
	{
		case DepotSyncActionType::Added:
		case DepotSyncActionType::Updated:
		case DepotSyncActionType::Refreshed:
		case DepotSyncActionType::Replaced:
		{
			if (modification->m_IsAlwaysResident)
			{
				LogDevice::WriteLine(log, LogChannel::Info, StringInfo::Format("%s%s - downloaded as %s", modification->m_DepotFile.c_str(), FDepotRevision::ToString(modification->m_Revision).c_str(), modification->m_ClientFile.c_str()));
			}
			else
			{
				LogDevice::WriteLine(log, LogChannel::Info, StringInfo::Format("%s%s - installed as %s", modification->m_DepotFile.c_str(), FDepotRevision::ToString(modification->m_Revision).c_str(), modification->m_ClientFile.c_str()));
			}

			// When Added or Updated simply install a reparse point placeholder file
			const DWORD clientFileAttributes = FileInfo::FileAttributes(CSTR_ATOW(modification->m_ClientFile));
			if (clientFileAttributes == INVALID_FILE_ATTRIBUTES || (clientFileAttributes & FILE_ATTRIBUTE_READONLY) || modification->CanModifyWritableFile())
			{
				if (modification->m_IsAlwaysResident)
				{
					modification->m_DiskFileSize = modification->m_FileSize;
					if (modification->IsPreview() == false)
					{
						SyncCommand(depotClient, DepotStringArray{ modification->m_DepotFile }, modification->m_Revision, DepotSyncFlags::Force | DepotSyncFlags::IgnoreOutput | DepotSyncFlags::Quiet);
					}
				}
				else
				{
					modification->m_VirtualFileSize = modification->m_FileSize;
					if (modification->IsPreview() == false)
					{
						DepotStopwatch timer(DepotStopwatch::Init::Start);
						if (InstallPlaceholderFile(depotClient, depotConfig, modification))
						{
							modification->m_PlaceholderTime = timer.TotalMilliseconds();
							timer.Restart();

							SyncCommand(depotClient, DepotStringArray{ modification->m_DepotFile }, modification->m_Revision, DepotSyncFlags::Flush | DepotSyncFlags::IgnoreOutput | DepotSyncFlags::Quiet);
							modification->m_FlushTime = timer.TotalMilliseconds();
						}
						else
						{
							LogDevice::WriteLine(log, LogChannel::Error, StringInfo::Format("Failed to install %s%s -> %s", modification->m_DepotFile.c_str(), FDepotRevision::ToString(modification->m_Revision).c_str(), modification->m_ClientFile.c_str()));
						}
					}
				}
			}
			else
			{
				LogDevice::WriteLine(log, LogChannel::Error, StringInfo::Format("Can't clobber writeable file %s", modification->m_ClientFile.c_str()));
			}
			break;
		}
		case DepotSyncActionType::Deleted:
		{
			LogDevice::WriteLine(log, LogChannel::Info, StringInfo::Format("%s%s - deleted as %s", modification->m_DepotFile.c_str(), FDepotRevision::ToString(modification->m_Revision).c_str(), modification->m_ClientFile.c_str()));

			// On Deleted we just need to uninstall the placeholder file
			const DWORD clientFileAttributes = FileInfo::FileAttributes(CSTR_ATOW(modification->m_ClientFile));
			if (clientFileAttributes == INVALID_FILE_ATTRIBUTES || (clientFileAttributes & FILE_ATTRIBUTE_READONLY) || modification->CanModifyWritableFile())
			{
				if (modification->IsPreview() == false)
				{
					DepotStopwatch timer(DepotStopwatch::Init::Start);
					if (UninstallPlaceholderFile(modification))
					{
						modification->m_PlaceholderTime = timer.TotalMilliseconds();
						timer.Restart();

						SyncCommand(depotClient, DepotStringArray{ modification->m_DepotFile }, FDepotRevision::New<FDepotRevisionNone>(), DepotSyncFlags::Flush | DepotSyncFlags::IgnoreOutput | DepotSyncFlags::Quiet);
						modification->m_FlushTime = timer.TotalMilliseconds();
					}
					else
					{
						LogDevice::WriteLine(log, LogChannel::Error, StringInfo::Format("Failed to remove file %s", modification->m_ClientFile.c_str()));
					}
				}
			}
			else if (FileInfo::Exists(CSTR_ATOW(modification->m_ClientFile)))
			{
				LogDevice::WriteLine(log, LogChannel::Error, StringInfo::Format("Can't clobber writeable file %s", modification->m_ClientFile.c_str()));
			}
			break;
		}
		case DepotSyncActionType::OpenedNotChanged:
		{
			DepotStopwatch timer(DepotStopwatch::Init::Start);
			SyncCommand(depotClient, DepotStringArray{ modification->m_DepotFile }, modification->m_Revision, DepotSyncFlags::Flush | DepotSyncFlags::IgnoreOutput | DepotSyncFlags::Quiet);
			modification->m_FlushTime = timer.TotalMilliseconds();
			LogDevice::WriteLine(log, LogChannel::Warning, StringInfo::Format("%s - is opened and not being changed", modification->ToFileSpecString().c_str()));
			break;
		}
		case DepotSyncActionType::UpToDate:
		{
			LogDevice::WriteLine(log, LogChannel::Info, StringInfo::Format("File up-to-date %s", modification->ToFileSpecString().c_str()));
			break;
		}
		case DepotSyncActionType::NoFilesFound:
		{
			LogDevice::WriteLine(log, LogChannel::Error, StringInfo::Format("No file at that changelist number %s", modification->ToFileSpecString().c_str()));
			break;
		}
		case DepotSyncActionType::NoFileAtRevision:
		{
			LogDevice::WriteLine(log, LogChannel::Error, StringInfo::Format("No file at that revision %s", modification->ToFileSpecString().c_str()));
			break;
		}
		case DepotSyncActionType::InvalidPattern:
		{
			LogDevice::WriteLine(log, LogChannel::Error, StringInfo::Format("No such file %s", modification->ToFileSpecString().c_str()));
			break;
		}
		case DepotSyncActionType::NotInClientView:
		{
			LogDevice::WriteLine(log, LogChannel::Error, StringInfo::Format("File not in client view %s", modification->ToFileSpecString().c_str()));
			break;
		}
		case DepotSyncActionType::CantClobber:
		{
			LogDevice::WriteLine(log, LogChannel::Error, StringInfo::Format("Can't clobber writable file %s", modification->m_ClientFile.c_str()));
			break;
		}
		case DepotSyncActionType::NeedsResolve:
		{
			LogDevice::WriteLine(log, LogChannel::Error, StringInfo::Format("... %s - must resolve %s before submitting", modification->m_DepotFile.c_str(), FDepotRevision::ToString(modification->m_Revision).c_str()));
			break;
		}
		case DepotSyncActionType::GenericError:
		{
			LogDevice::WriteLine(log, LogChannel::Error, modification->m_Message.c_str());
			break;
		}
		default:
		{
			LogDevice::WriteLine(log, LogChannel::Error, StringInfo::Format("Unsupported sync action: %s", DepotSyncActionType::ToString(modification->m_SyncActionType).c_str()));
			break;
		}
	}

	for (const DepotSyncActionInfo& subaction : modification->m_SubActions)
	{
		ApplyVirtualModification(depotClient, depotConfig, subaction, log);
	}

	// Flush buffered log all at once
	if (log == &memoryLog && parentLog != nullptr)
	{
		for (const LogElement& element : memoryLog.m_Elements)
		{
			parentLog->Write(element.m_Channel, element.m_Text);
		}
	}
}

bool
DepotOperations::CreateSymlinkFile(
	DepotClient& depotClient,
	const DepotSyncActionInfo& modification
	)
{
	if (modification->IsPreview())
	{
		return true;
	}

	DepotString targetFile = GetSymlinkTargetPath(depotClient, modification->m_DepotFile, modification->m_Revision);
	if (targetFile.empty())
	{
		return false;
	}

	bool status = SUCCEEDED(FileOperations::CreateSymlinkFile(
		CSTR_ATOW(modification->m_ClientFile), 
		CSTR_ATOW(targetFile)));
	
	return status;
}

DepotSyncResult
DepotOperations::Hydrate(
	DepotClient& depotClient, 
	const FDepotSyncOptions& syncOptions
	)
{
	DepotStringArray hydrateSpecs = CreateFileSpecs(depotClient, syncOptions.m_Files, FDepotRevision::New<FDepotRevisionHave>(), CreateFileSpecFlags::OverrideRevison);
	if (hydrateSpecs.size() == 0)
	{
		depotClient->Log(LogChannel::Error, "No files specified to hydrate");
		return std::make_shared<FDepotSyncResult>(DepotSyncStatus::Error);
	}

	DepotResultFStat hydrateFStat = FStat(depotClient, hydrateSpecs, "", FDepotResultFStatField::DepotFile | FDepotResultFStatField::ClientFile | FDepotResultFStatField::HaveRev);
	if (hydrateFStat->HasError())
	{
		depotClient->Log(LogChannel::Error, StringInfo::Format("Failed to fstat paths to hydrate: %s", hydrateFStat->GetError().c_str()));
		return std::make_shared<FDepotSyncResult>(DepotSyncStatus::Error);
	}

	DepotSyncActionInfoArray modifications = std::make_shared<DepotSyncActionInfoArray::element_type>();
	DepotSyncStatus::Enum status = DepotSyncStatus::Success;

	if (hydrateFStat->NodeCount() > 0)
	{
		LogDevice* log = depotClient->Log();
		FileCore::AutoHandle modificationsMutex = CreateMutex(NULL, FALSE, NULL);

		ThreadPool::ForEach::Execute(
			hydrateFStat->TagList().data(),
			hydrateFStat->TagList().size(), 
			[log, &syncOptions, &status, &modifications, &modificationsMutex](const DepotResultTag& hydrateFStatTag) -> void
		{
			const FDepotResultFStatNode node = FDepotResultNode::Create<FDepotResultFStatNode>(hydrateFStatTag);
			const int32_t haveRev = node.HaveRev();
			const DepotString& depotFile = node.DepotFile();
			const DepotString& clientFile = node.ClientFile();
			const WString& filePath = StringInfo::ToWide(clientFile);

			DWORD fileAttributes = FileCore::FileInfo::FileAttributes(filePath.c_str());
			if ((fileAttributes == INVALID_FILE_ATTRIBUTES) || (fileAttributes & FILE_ATTRIBUTE_OFFLINE) == 0)
			{
				return;
			}

			bool isAlwaysResident = false;
			if (syncOptions.m_SyncResident.empty() == false)
			{
				isAlwaysResident = IsFileTypeAlwaysResident(syncOptions.m_SyncResident, depotFile);
				if (isAlwaysResident == false)
				{
					return;
				}
			}

			DepotSyncActionInfo modification = std::make_shared<FDepotSyncActionInfo>();
			modification->m_DepotFile = depotFile;
			modification->m_ClientFile = clientFile;
			modification->m_Revision = FDepotRevision::New<FDepotRevisionNumber>(haveRev);
			modification->m_SyncFlags = syncOptions.m_SyncFlags;
			modification->m_IsAlwaysResident = isAlwaysResident;

			LogDevice::WriteLine(log, LogChannel::Info, StringInfo::Format("%s#%d - request hydrate as %s", depotFile.c_str(), haveRev, clientFile.c_str()));
			if (modification->IsPreview() == false)
			{
				HRESULT hr = FileOperations::HydrateFile(filePath.c_str());
				if (hr != S_OK)
				{
					LogDevice::WriteLine(log, LogChannel::Error, StringInfo::Format("Failed to hydrate file '%s' with error [%s]", clientFile.c_str(), CSTR_WTOA(StringInfo::ToString(hr))));
					status = DepotSyncStatus::Error;
				}
			}

			AutoMutex modificationsLock(modificationsMutex.Handle());
			modifications->push_back(modification);
		});
	}

	return std::make_shared<FDepotSyncResult>(status, modifications);
}

bool
DepotOperations::Reconfig(
	DepotClient& depotClient, 
	const FDepotReconfigOptions& reconfigOptions
	)
{
	DepotStringArray reconfigSpecs = CreateFileSpecs(depotClient, reconfigOptions.m_Files, FDepotRevision::New<FDepotRevisionHave>(), CreateFileSpecFlags::OverrideRevison);
	if (reconfigSpecs.size() == 0)
	{
		depotClient->Log(LogChannel::Error, "No files specified to reconfig");
		return false;
	}

	DepotResultFStat reconfigFStat = FStat(depotClient, reconfigSpecs, "", FDepotResultFStatField::DepotFile | FDepotResultFStatField::ClientFile | FDepotResultFStatField::FileSize);
	if (reconfigFStat->HasError())
	{
		depotClient->Log(LogChannel::Error, StringInfo::Format("Failed to fstat paths to reconfig: %s", reconfigFStat->GetError().c_str()));
		return false;
	}

	bool status = true;

	if (reconfigFStat->NodeCount() > 0)
	{
		LogDevice* log = depotClient->Log();
		const DepotConfig depotConfig = depotClient->Config();

		ThreadPool::ForEach::Execute(
			reconfigFStat->TagList().data(),
			reconfigFStat->TagList().size(), 
			[log, depotConfig, &reconfigOptions, &status](const DepotResultTag& reconfigFStatTag) -> void
		{
			const FDepotResultFStatNode node = FDepotResultNode::Create<FDepotResultFStatNode>(reconfigFStatTag);			
			const DepotString& depotFile = node.DepotFile();
			const DepotString& clientFile = node.ClientFile();
			const WString& filePath = StringInfo::ToWide(clientFile);
			const int64_t fileSize = node.FileSize();

			DWORD fileAttributes = FileCore::FileInfo::FileAttributes(filePath.c_str());
			if ((fileAttributes == INVALID_FILE_ATTRIBUTES) || (fileAttributes & FILE_ATTRIBUTE_OFFLINE) == 0)
			{
				return;
			}

			FileCore::GAllocPtr<P4VFS_REPARSE_DATA_2> reparseData;
			HRESULT hr = FileOperations::GetFileReparseData(filePath.c_str(), reparseData);
			if (FAILED(hr) || reparseData.get() == nullptr)
			{
				return;
			}
			
			const DepotString reconfigPort = reconfigOptions.m_Flags & DepotReconfigFlags::P4Port ? depotConfig.m_Port : StringInfo::ToAnsi(reparseData->depotServer.c_str());
			const DepotString reconfigClient = reconfigOptions.m_Flags & DepotReconfigFlags::P4Client ? depotConfig.m_Client : StringInfo::ToAnsi(reparseData->depotClient.c_str());
			const DepotString reconfigUser = reconfigOptions.m_Flags & DepotReconfigFlags::P4User ? depotConfig.m_User : StringInfo::ToAnsi(reparseData->depotUser.c_str());

			LogDevice::WriteLine(log, LogChannel::Info, StringInfo::Format("%s#%d - reconfig as %s [%s %s %s]", depotFile.c_str(), int32_t(reparseData->fileRevision), clientFile.c_str(), reconfigPort.c_str(), reconfigClient.c_str(), reconfigUser.c_str()));
			if ((reconfigOptions.m_Flags & DepotReconfigFlags::Preview) == 0)
			{
				hr = FileOperations::InstallReparsePointOnFile(
					P4VFS_VER_MAJOR,
					P4VFS_VER_MINOR,
					P4VFS_VER_BUILD,
					filePath.c_str(),
					P4VFS_RESIDENCY_POLICY_RESIDENT,
					reparseData->fileRevision,
					fileSize,
					fileAttributes & FILE_ATTRIBUTE_READONLY,
					CSTR_ATOW(depotFile),
					CSTR_ATOW(reconfigPort),
					CSTR_ATOW(reconfigClient),
					CSTR_ATOW(reconfigUser));

				if (hr != S_OK)
				{
					LogDevice::Error(log, StringInfo::Format("Failed to reconfig file '%s' with error [%s]", clientFile.c_str(), CSTR_WTOA(StringInfo::ToString(hr))));
					status = false;
				}
			}
		});
	}

	return status;
}

bool
DepotOperations::InstallPlaceholderFile(
	DepotClient& depotClient,
	const DepotConfig& depotConfig,
	const DepotSyncActionInfo& modification
	)
{
	if (modification->IsPreview())
	{
		return true;
	}

	if (modification->m_SyncActionFlags & DepotSyncActionFlags::FileSymlink)
	{
		return CreateSymlinkFile(depotClient, modification);
	}

	const DWORD fileAttributes = modification->CanSetWritableFile() ? 0 : FILE_ATTRIBUTE_READONLY;
	const WString filePath = StringInfo::ToWide(modification->m_ClientFile);

	bool status = SUCCEEDED(FileOperations::InstallReparsePointOnFile(
		P4VFS_VER_MAJOR,
		P4VFS_VER_MINOR,
		P4VFS_VER_BUILD,
		filePath.c_str(),
		P4VFS_RESIDENCY_POLICY_RESIDENT,
		modification->RevisionNumber(),
		modification->m_FileSize,
		fileAttributes,
		CSTR_ATOW(modification->m_DepotFile),
		CSTR_ATOW(depotConfig.m_Port),
		CSTR_ATOW(depotConfig.m_Client),
		CSTR_ATOW(depotConfig.m_User)));

	return status;
}

bool
DepotOperations::UninstallPlaceholderFile(
	const DepotSyncActionInfo& modification
	)
{
	if (modification->IsPreview())
	{
		return true;
	}

	const WString clientFile = StringInfo::ToWide(modification->m_ClientFile);
	FileInfo::Delete(clientFile.c_str());
	if (FileInfo::Exists(clientFile.c_str()))
	{
		return false;
	}

	WString deletedFromFolder = FileInfo::FolderPath(clientFile.c_str());
	if (StringInfo::IsNullOrWhiteSpace(deletedFromFolder.c_str()) == false)
	{
		FileInfo::DeleteEmptyDirectories(deletedFromFolder.c_str());
	}
	return true;
}

DepotSyncResult
DepotOperations::SyncRegular(
	DepotClient& depotClient, 
	const FDepotSyncOptions& syncOptions
	)
{
	LogDeviceMemory memoryLog;
	LogDeviceAggregate aggregateLog;	
	LogDevice* log = &memoryLog;

	if (depotClient->Log() != nullptr)
	{
		aggregateLog.m_Devices.push_back(depotClient->Log());
		aggregateLog.m_Devices.push_back(&memoryLog);
		log = &aggregateLog;
	}

	DepotRevision revision = syncOptions.m_Revision;
	if (revision.get() == nullptr || revision->IsHeadRevision())
	{
		revision = GetHeadRevisionChangelist(depotClient);
		if (revision.get() == nullptr)
		{
			depotClient->Log(LogChannel::Error, "Missing specific head revision");
			return std::make_shared<FDepotSyncResult>(DepotSyncStatus::Error);
		}
	}

	// Perform a regular (non-virtual) sync
	DepotSyncActionInfoArray modifications = SyncCommand(depotClient, syncOptions.m_Files, revision, syncOptions.m_SyncFlags, log);

	// Derive a DepotSyncStatus from the log output for this operation
	DepotSyncStatus::Enum status = DepotSyncStatus::FromLog(memoryLog);
	if (status != DepotSyncStatus::Success)
	{
		LogWarningErrorSummary(depotClient, memoryLog);
	}

	return std::make_shared<FDepotSyncResult>(status, modifications);
}

DepotSyncActionInfoArray
DepotOperations::SyncCommand(
	DepotClient& depotClient, 
	const DepotStringArray& files, 
	DepotRevision revision,
	DepotSyncFlags::Enum syncFlags,
	FileCore::LogDevice* log
	)
{
	if (depotClient.get() == nullptr)
	{
		return nullptr;
	}

	if (log == nullptr)
	{
		log = depotClient->Log();
	}
	
	LogDeviceFilter quietLog(log, syncFlags & DepotSyncFlags::Flush ? LogChannel::Error : LogChannel::Warning);
	if (syncFlags & DepotSyncFlags::Quiet)
	{
		log = &quietLog;
	}

	auto reportError = [&](const DepotString& context) -> void
	{
		if (log != nullptr)
		{
			log->Error(StringInfo::Format("DepotOperations sync failed for ClientName='%s' files=%s revision='%s' syncFlags='%s'. %s", 
				depotClient->Config().m_Client.c_str(), 
				ToString(files).c_str(), 
				FDepotRevision::ToString(revision).c_str(), 
				DepotSyncFlags::ToString(syncFlags).c_str(), 
				context.c_str()));
		}
	};

	if (revision.get() == nullptr || revision->IsHeadRevision())
	{
		revision = GetHeadRevisionChangelist(depotClient);
		if (revision.get() == nullptr)
		{
			reportError("Missing specific head revision");
			return nullptr;
		}
	}

	DepotStringArray fileSpecs = CreateFileSpecs(depotClient, files, revision);
	if (fileSpecs.size() == 0)
	{
		reportError("No files specified to sync to");
		return nullptr;
	}

	typedef HashSet<DepotString, StringInfo::EqualInsensitive> DepotFileHashSet; 
	typedef Map<DepotString, FDepotResultSizesNode, StringInfo::LessInsensitive> DepotFileClientSizeMapType;

	DepotFileHashSet writeableHeadDepotFiles;
	DepotFileHashSet writeableHaveDepotFiles;
	DepotFileHashSet symlinkDepotFiles;
	DepotFileHashSet diffDepotFiles;
	DepotFileClientSizeMapType depotFileClientSizeMap;

	if ((syncFlags & (DepotSyncFlags::Preview | DepotSyncFlags::Flush)) != 0 && (syncFlags & DepotSyncFlags::IgnoreOutput) == 0)
	{
		DepotRevision haveRevision = (syncFlags & DepotSyncFlags::Force) ? FDepotRevision::New<FDepotRevisionNone>() : FDepotRevision::New<FDepotRevisionHave>();
		for (const DepotString& fileSpec : fileSpecs)
		{
			const DepotString haveFileSpec = CreateFileSpec(fileSpec, haveRevision, CreateFileSpecFlags::OverrideRevison);
			if (haveFileSpec.empty())
			{
				reportError(StringInfo::Format("Invalid haveFileSpec for fileSpec='%s' rev='%s'", fileSpec.c_str(), FDepotRevision::ToString(haveRevision).c_str()));
				return nullptr;
			}

			const DepotString& headFileSpec = fileSpec;
			if (headFileSpec.empty())
			{
				reportError(StringInfo::Format("Invalid headFileSpec for fileSpec='%s' rev='%s'", fileSpec.c_str(), FDepotRevision::ToString(revision).c_str()));
				return nullptr;
			}

			DepotResultDiff2 diff2 = Diff2(depotClient, haveFileSpec, headFileSpec);
			for (size_t diff2NodeIndex = 0; diff2NodeIndex < diff2->NodeCount(); ++diff2NodeIndex)
			{
				FDepotResultDiff2Node node = diff2->Node(diff2NodeIndex);
				diffDepotFiles.insert(node.DepotFile());
				diffDepotFiles.insert(node.DepotFile2());
				
				if (DepotInfo::IsWritableFileType(node.Type()))
					writeableHaveDepotFiles.insert(node.DepotFile());
				if (DepotInfo::IsWritableFileType(node.Type2()))
					writeableHeadDepotFiles.insert(node.DepotFile2());

				if (DepotInfo::IsSymlinkFileType(node.Type()))
					symlinkDepotFiles.insert(node.DepotFile());
				if (DepotInfo::IsSymlinkFileType(node.Type2()))
					symlinkDepotFiles.insert(node.DepotFile2());
			}

			if ((syncFlags & DepotSyncFlags::ClientSize) != 0 && depotClient->GetServerApiLevel() >= DepotProtocol::SERVER_SIZES_C)
			{
				DepotResultSizes sizes = Sizes(depotClient, headFileSpec, SizesFlags::ClientSize);
				for (size_t sizesNodeIndex = 0; sizesNodeIndex < sizes->NodeCount(); ++sizesNodeIndex)
				{
					FDepotResultSizesNode node = sizes->Node(sizesNodeIndex);
					depotFileClientSizeMap[node.DepotFile()] = node;
				}
			}
		}
	}

	DepotClientLogCallback onClientLogCallback = std::make_shared<FDepotClientLogCallback>([log](LogChannel::Enum channel, const char* severity, const char* text) -> void
	{
		if (log != nullptr)
		{
			log->Write(channel, StringInfo::ToWide(text));
		}
	});

	DepotCommand syncCmd;
	syncCmd.m_Name = "sync";

	if (syncFlags & DepotSyncFlags::Force)
		syncCmd.m_Args.push_back("-f");
	if (syncFlags & DepotSyncFlags::Flush)
		syncCmd.m_Args.push_back("-k");
	if (syncFlags & DepotSyncFlags::Preview)
		syncCmd.m_Args.push_back("-n");

	Algo::Append(syncCmd.m_Args, fileSpecs);

	bool useTaggedOutput = (syncFlags & DepotSyncFlags::Quiet) != 0;
	if (useTaggedOutput == false)
	{
		syncCmd.m_Flags |= DepotCommand::Flags::UnTagged;
	}

	depotClient->SetMessageCallback(onClientLogCallback);
	depotClient->SetErrorCallback(onClientLogCallback);
	DepotResult syncResult = depotClient->Run(syncCmd);
	depotClient->SetMessageCallback(nullptr);
	depotClient->SetErrorCallback(nullptr);

	if (syncFlags & DepotSyncFlags::IgnoreOutput)
	{
		return nullptr;
	}

	DepotSyncActionInfoArray modifications = std::make_shared<DepotSyncActionInfoArray::element_type>();
	if (syncResult->TagList().size() > 0)
	{
		for (const DepotResultTag& tag : syncResult->TagList())
		{
			DepotSyncActionInfo actionInfo = FDepotSyncActionInfo::FromTaggedOutput(*tag, log);
			if (actionInfo.get() != nullptr)
			{
				modifications->push_back(actionInfo);
			}
		}
	}

	if (syncResult->TextList().size() > 0)
	{
		DepotSyncActionInfo parentActionInfo;
		for (const DepotResultText& text : syncResult->TextList())
		{
			DepotSyncActionInfo actionInfo;
			if (text->m_Channel == DepotResultChannel::StdOut)
			{
				actionInfo = FDepotSyncActionInfo::FromInfoOutput(text->m_Value, log);
			}
			else if (text->m_Channel == DepotResultChannel::StdErr)
			{
				actionInfo = FDepotSyncActionInfo::FromErrorOutput(text->m_Value, log);
			}

			if (actionInfo.get() == nullptr)
			{
				parentActionInfo = nullptr;
			}
			else if (text->m_Level == 0)
			{
				modifications->push_back(actionInfo);
				parentActionInfo = actionInfo;
			}
			else if (parentActionInfo.get() != nullptr)
			{
				parentActionInfo->m_SubActions.push_back(actionInfo);
			}
		}
	}

	typedef Map<DepotString, FDepotResultFStatNode, StringInfo::LessInsensitive> OpenedHeadDepotFilesType;
	OpenedHeadDepotFilesType openedHeadDepotFiles;

	DepotResultFStat openedFStat = FStat(depotClient, fileSpecs, "", FDepotResultFStatField::DepotFile | FDepotResultFStatField::HeadRev, DepotStringArray{ "-Ro" });
	for (size_t fstatNodeIndex = 0; fstatNodeIndex < openedFStat->NodeCount(); ++fstatNodeIndex)
	{
		const FDepotResultFStatNode& node = openedFStat->Node(fstatNodeIndex);
		openedHeadDepotFiles[node.DepotFile()] = node;
	}

	for (const DepotSyncActionInfo& modification : *modifications)
	{
		if (modification->m_SyncActionType == DepotSyncActionType::OpenedNotChanged && 
			modification->m_DepotFile.empty() == false)
		{
			if (FDepotResultFStatNode* fstatNode = Algo::Find(openedHeadDepotFiles, modification->m_DepotFile))
			{
				modification->m_Revision = FDepotRevision::New<FDepotRevisionNumber>(fstatNode->HeadRev());
			}
		}

		if (FDepotResultSizesNode* sizesNode = Algo::Find(depotFileClientSizeMap, modification->m_DepotFile))
		{
			int64_t clientFileSize = sizesNode->FileSize();
			if (clientFileSize > 0)
			{
				modification->m_FileSize = clientFileSize;
			}
		}
	}

	if (syncFlags & (DepotSyncFlags::Preview | DepotSyncFlags::Flush))
	{
		DepotStringArray identicalHaveDepotFiles;
		for (const DepotSyncActionInfo& modification : *modifications)
		{
			if (DepotSyncActionType::IsLocalChanged(modification->m_SyncActionType) && 
				modification->m_DepotFile.empty() == false && 
				diffDepotFiles.find(modification->m_DepotFile) == diffDepotFiles.end())
			{
				identicalHaveDepotFiles.push_back(StringInfo::Format("%s%s", modification->m_DepotFile.c_str(), revision->ToString().c_str()));
			}
		}

		if (identicalHaveDepotFiles.size() > 0)
		{
			DepotString writableFileTypeFilter = "headType=*+*w*";
			DepotResultFStat identicalFStat = FStat(depotClient, identicalHaveDepotFiles, writableFileTypeFilter, FDepotResultFStatField::DepotFile);
			for (size_t fstatNodeIndex = 0; fstatNodeIndex < identicalFStat->NodeCount(); ++fstatNodeIndex)
			{
				const FDepotResultFStatNode& node = identicalFStat->Node(fstatNodeIndex);
				writeableHaveDepotFiles.insert(node.DepotFile());
				writeableHeadDepotFiles.insert(node.DepotFile());
			}
		}
	}

	DepotSyncActionFlags::Enum commonSyncActionFlags = DepotSyncActionFlags::None;
	FDepotResultClientOption::Enum clientOptions = depotClient->Client()->OptionFlags();
	if (clientOptions & FDepotResultClientOption::Clobber)
	{
		commonSyncActionFlags |= DepotSyncActionFlags::ClientClobber;
	}
	if (clientOptions & FDepotResultClientOption::AllWrite)
	{
		commonSyncActionFlags |= DepotSyncActionFlags::ClientWrite;
	}

	for (const DepotSyncActionInfo& modification : *modifications)
	{
		modification->m_SyncFlags = syncFlags;
		modification->m_SyncActionFlags = commonSyncActionFlags;

		if (Algo::Contains(writeableHeadDepotFiles, modification->m_DepotFile))
		{
			modification->m_SyncActionFlags |= DepotSyncActionFlags::FileWrite;
		}
		if (Algo::Contains(writeableHaveDepotFiles, modification->m_DepotFile))
		{
			modification->m_SyncActionFlags |= DepotSyncActionFlags::HaveFileWrite;
		}
		if (Algo::Contains(symlinkDepotFiles, modification->m_DepotFile))
		{
			modification->m_SyncActionFlags |= DepotSyncActionFlags::FileSymlink;
		}
	}
	return modifications;
}

DepotRevision
DepotOperations::GetHeadRevisionChangelist(
	DepotClient& depotClient
	)
{
	auto reportError = [&depotClient](const DepotString& context) -> void
	{
		depotClient->Log(LogChannel::Error, StringInfo::Format("DepotOperations::GetHeadRevisionChangelist failed for ClientName='%s'. %s", depotClient->Config().m_Client.c_str(), context.c_str()));
	};

	if (depotClient->IsConnected() == false)
	{
		reportError("DepotClient not connected");
		return nullptr;
	}

	DepotResult changes = depotClient->Run("changes", DepotStringArray{ "-m", "1" });
	if (changes->TagList().empty())
	{
		reportError("Unable to run changes command");
		return nullptr;
	}

	const DepotString& changeText = changes->GetTagValue("change");
	if (changeText.empty())
	{
		reportError("Perforce changes output does not include changelist number");
		return nullptr;
	}

	int32_t changeNum = atoi(changeText.c_str());
	if (changeNum <= 0)
	{
		reportError(StringInfo::Format("Unable to parse change number to change='%s'", changeText.c_str()));
		return nullptr;
	}

	return std::make_shared<FDepotRevisionChangelist>(changeNum);
}

DepotString
DepotOperations::CreateFileSpec(
	const DepotString& filePath, 
	const DepotRevision& revision, 
	CreateFileSpecFlags::Enum flags
	)
{
	DepotString fileSpecPath = StringInfo::Trim(filePath.c_str(), "\" ");
	DepotString fileSpecRev = FDepotRevision::ToString(revision);

	const DepotString fileSpecMatch = fileSpecPath;
	const char* fileSpecMatchRev = StringInfo::Strpbrk(fileSpecMatch.c_str(), "@#");
	if (fileSpecMatchRev != nullptr)
	{
		fileSpecPath.assign(fileSpecMatch.c_str(), fileSpecMatchRev-fileSpecMatch.c_str());
		if ((flags & CreateFileSpecFlags::OverrideRevison) == 0 && fileSpecMatchRev[1] != '\0')
			fileSpecRev = fileSpecMatchRev;
	}

	DepotRevision parsedRev;
	if (fileSpecRev.empty() == false)
	{
		parsedRev = FDepotRevision::FromString(fileSpecRev);
		if (parsedRev.get() == nullptr)
			return DepotString();
	}

	return StringInfo::Format("%s%s", fileSpecPath.c_str(), FDepotRevision::ToString(parsedRev).c_str());
}

DepotStringArray
DepotOperations::CreateFileSpecs(
	DepotClient& depotClient,
	const DepotStringArray& filePaths, 
	const DepotRevision& revision, 
	CreateFileSpecFlags::Enum flags
	)
{
	DepotStringArray validFilePaths;
	Algo::AppendIf(validFilePaths, filePaths, [](const DepotString& p) -> bool { return !p.empty(); }); 
	if (validFilePaths.size() == 0)
	{
		validFilePaths.push_back(StringInfo::Format("//%s/...", depotClient->Config().m_Client.c_str()));
	}

	DepotStringArray fileSpecs;
	fileSpecs.reserve(validFilePaths.size());
	for (const DepotString& filePath : validFilePaths)
	{
		DepotString fileSpec = CreateFileSpec(filePath, revision, flags);
		if (fileSpec.empty() == false)
		{
			fileSpecs.push_back(fileSpec);
		}
	}
	return fileSpecs;
}

DepotResultDiff2
DepotOperations::Diff2(
	DepotClient& depotClient,
	const DepotString& fileSpec1, 
	const DepotString& fileSpec2
	)
{
	if (fileSpec1.empty() || fileSpec2.empty() || StringInfo::Stricmp(fileSpec1.c_str(), fileSpec2.c_str()) == 0)
	{
		return std::make_shared<FDepotResultDiff2>();
	}
	return depotClient->Run<DepotResultDiff2>("diff2", DepotStringArray{ DepotString("-q"), fileSpec1, fileSpec2 });
}

DepotResultSizes
DepotOperations::Sizes(
	DepotClient& depotClient,
	const DepotString& fileSpec, 
	SizesFlags::Enum flags
	)
{
	DepotStringArray sizesArgs;
	if (flags & SizesFlags::ClientSize)
	{
		sizesArgs.push_back("-C");
	}
	if (fileSpec.empty() == false)
	{
		sizesArgs.push_back(fileSpec);
	}
	return depotClient->Run<DepotResultSizes>("sizes", sizesArgs);
}

DepotResultFStat 
DepotOperations::FStat(
	DepotClient& depotClient,
	const DepotStringArray& files,
	const DepotString& filterType,
	FDepotResultFStatField::Enum fields,
	const DepotStringArray& optionArgs
	)
{
	if (files.size() == 0)
	{
		return std::make_shared<FDepotResultFStat>();
	}

	DepotStringArray fieldNames = FDepotResultFStatField::ToNames(fields);
	DepotStringArray fstatArgs = optionArgs;

	if (fields & FDepotResultFStatField::FileSize)
		fstatArgs.push_back("-Ol");
	if (fieldNames.size())
		Algo::Append(fstatArgs, DepotStringArray{"-T", StringInfo::Join(fieldNames, ",")});
	if (filterType.empty() == false)
		Algo::Append(fstatArgs, DepotStringArray{"-F", filterType});

	Algo::Append(fstatArgs, files);
	return depotClient->Run<DepotResultFStat>("fstat", fstatArgs);
}

bool
DepotOperations::IsFileTypeAlwaysResident(
	const DepotString& syncResident, 
	const DepotString& depotFile
	)
{
	if (depotFile.empty() == false && syncResident.empty() == false)
	{
		std::match_results<const char*> match;
		return std::regex_search(depotFile.c_str(), match, std::regex(syncResident, std::regex_constants::ECMAScript|std::regex_constants::icase));
	}
	return false;
}

DepotString 
DepotOperations::ToString(
	const DepotStringArray& paths
	)
{
	return paths.size() ? StringInfo::Format("[\"%s\"]", StringInfo::Join(paths, "\",\"").c_str()) : DepotString("[]");
}

DepotString
DepotOperations::GetSymlinkTargetPath(
	DepotClient& depotClient,
	const DepotString& filePath, 
	const DepotRevision& revision
	)
{
	if (depotClient.get() == nullptr)
		return DepotString();

	DepotString fileSpec = CreateFileSpec(filePath, revision);
	if (fileSpec.empty())
		return DepotString();
	
	DepotResultPrintString print = depotClient->Run<DepotResultPrintString>(DepotCommand("print", DepotStringArray{ "-q", fileSpec }, DepotCommand::Flags::UnTagged));
	DepotString targetPath = StringInfo::Trim(print->GetString().c_str());
	return targetPath;
}

DepotString
DepotOperations::GetSymlinkTargetDepotFile(
	DepotClient& depotClient,
	const DepotString& filePath, 
	const DepotRevision& revision
	)
{
	DepotString targetFile = GetSymlinkTargetPath(depotClient, filePath, revision);
	if (targetFile.empty())
		return DepotString();

	if (StringInfo::StartsWith(targetFile.c_str(), "//") == false)
	{
		DepotString fileSpec = CreateFileSpec(filePath, revision);
		if (fileSpec.empty())
			return DepotString();
		FDepotResultFStatNode fstatNode = FStat(depotClient, DepotStringArray{ fileSpec }, "", FDepotResultFStatField::DepotFile|FDepotResultFStatField::HeadType)->Node();
		if (fstatNode.IsEmpty())
			return DepotString();
		if (DepotInfo::IsSymlinkFileType(fstatNode.HeadType()) == false)
			return DepotString();
		targetFile = NormalizePath(StringInfo::Format("%s/../%s", fstatNode.DepotFile().c_str(), targetFile.c_str()));
	}
	return targetFile;
}

DepotString
DepotOperations::NormalizePath(
	DepotString path
	)
{
	path = StringInfo::Trim(path.c_str());
	if (path.empty())
		return path;

	DepotStringArray rsplit = StringInfo::Split(path.c_str(), "\\/", StringInfo::SplitFlags::RemoveEmptyEntries);
	DepotStringArray split;
	for (int32_t i = int32_t(rsplit.size())-1, rel = 0; i >= 0; --i)
	{
		if (rsplit[i] == "..")
			rel++;
		else if (rel > 0)
			rel--;
		else
			split.insert(split.begin(), rsplit[i]);
	}

	if (split.size() > 0)
	{
		if (StringInfo::StartsWith(path.c_str(), "//"))
		{
			path = StringInfo::Format("//%s", StringInfo::Join(split, "/").c_str());
		}
		else
		{
			if (split[0].length() == 2 && StringInfo::IsAlpha(split[0][0]) && split[0][1] == ':')
			{
				split[0] = StringInfo::ToUpper(split[0].c_str());
			}
			path = StringInfo::Join(split, "\\");
		}
	}
	return path;
}

bool 
DepotOperations::ResolveDepotServerName(
	const DepotString& sourceName, 
	DepotString& targetName
	)
{
	const SettingNode dsc = SettingManager::StaticInstance().DepotServerConfig.GetNode();
	if (StringInfo::Stricmp(dsc.Data().c_str(), L"Servers") == 0)
	{
		for (const SettingNode& entry : dsc.Nodes())
		{
			if (StringInfo::Stricmp(entry.Data().c_str(), L"Entry") == 0)
			{
				String pattern = entry.GetAttribute(L"Pattern");
				String address = entry.GetAttribute(L"Address");
				if (pattern.empty() == false && address.empty() == false)
				{
					std::match_results<const wchar_t*> match;
					if (std::regex_search(StringInfo::ToWide(sourceName).c_str(), match, std::wregex(pattern, std::regex_constants::ECMAScript|std::regex_constants::icase)))
					{
						targetName = StringInfo::ToAnsi(address);
						return true;
					}
				}
			}
		}
	}
	return false;
}

DepotString 
DepotOperations::ResolveDepotServerName(
	const DepotString& sourceName
	)
{
	DepotString targetName;
	if (ResolveDepotServerName(sourceName, targetName) == false)
	{
		targetName = sourceName;
	}
	return targetName;
}

DepotString
DepotOperations::ToDisplayStringBytes(
	uint64_t sizeBytes
	)
{
	struct DisplayUnit
	{
		const char* prefix;
		const uint64_t factor;
	};
	static const DisplayUnit units[] = {{"TB",1ULL<<40}, {"GB",1ULL<<30}, {"MB",1ULL<<20}, {"KB",1ULL<<10}};
	DepotString text = StringInfo::Format("%I64u bytes", sizeBytes);
	for (const DisplayUnit& unit : units)
	{
		if (sizeBytes >= unit.factor)
		{
			text += StringInfo::Format(" (%.01f %s)", float(double(sizeBytes)/unit.factor), unit.prefix);
			break;
		}
	}
	return text;
}

DepotString
DepotOperations::ToDisplayStringMilliseconds(
	int64_t totalMilliseconds
	)
{
	uint64_t ms = static_cast<uint64_t>(std::abs(totalMilliseconds));
	const uint64_t hours = ms / (60*60*1000);
	ms -= hours * (60*60*1000);
	const uint64_t mins = ms / (60*1000);
	ms -= mins * (60*1000);
	const uint64_t secs = ms / 1000;
	ms -= secs * 1000;

	DepotString span;
	if (hours > 0)
		span += StringInfo::Format("%I64u:", hours);
	if (mins > 0 || !span.empty())
		span += StringInfo::Format("%0*I64u:", span.empty() ? 1 : 2, mins);
	if (secs > 0 || ms > 0 || !span.empty())
		span += StringInfo::Format("%0*I64u.%03I64u", span.empty() ? 1 : 2, secs, ms);

	DepotString text = StringInfo::Format("%I64d sec", totalMilliseconds / 1000);
	if (!span.empty())
		text += StringInfo::Format(" (%s)", span.c_str());
	return text;
}

void
DepotOperations::LogWarningErrorSummary(
	DepotClient& depotClient,
	const LogDeviceMemory& memoryLog
	)
{
	auto inSummary = [](const LogElement& e) -> bool 
	{ 
		return e.m_Channel == LogChannel::Warning || e.m_Channel == LogChannel::Error;
	};

	if (Algo::Any(memoryLog.m_Elements, std::not_fn(inSummary)))
	{
		bool writeHeader = true;
		for (const LogElement& element : memoryLog.m_Elements)
		{
			if (inSummary(element))
			{
				if (writeHeader)
				{
					depotClient->Log(LogChannel::Warning, "Warning/Error Summary:");
					writeHeader = false;
				}
				depotClient->Log(element.m_Channel, StringInfo::ToAnsi(element.m_Text));
			}
		}
	}
}

}}}
