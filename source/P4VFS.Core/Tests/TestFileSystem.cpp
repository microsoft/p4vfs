// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "TestFactory.h"
#include "DepotClient.h"
#include "DepotResultWhere.h"
#include "DepotResultFStat.h"
#include "DepotOperations.h"
#include "DriverOperations.h"
#include "DriverVersion.h"
#include "FileSystem.h"

using namespace Microsoft::P4VFS::FileCore;
using namespace Microsoft::P4VFS::TestCore;
using namespace Microsoft::P4VFS::P4;

void TestResolveFileResidency(const TestContext& context)
{
	auto ResolveFile = [&context](FDepotClient& client, const String& depotFile, const String& fileType) -> void
	{
		const String p4vfsExe = context.GetEnvironment(L"P4VFS_EXE");
		Assert(FileInfo::Exists(p4vfsExe.c_str()));
		const String p4Options = context.GetEnvironment(L"P4VFS_EXE_CONFIG");
		Assert(p4Options.empty() == false);

		Assert(TestUtilities::ExecuteWait(context, StringInfo::Format(L"\"%s\" %s sync -f %s", p4vfsExe.c_str(), p4Options.c_str(), depotFile.c_str())) == 0);
		const String clientFile = StringInfo::ToWide(client.Run<DepotResultWhere>("where", DepotStringArray{ StringInfo::ToAnsi(depotFile) })->Node().LocalPath());
		Assert(FileInfo::IsRegular(clientFile.c_str()));
		Assert(context.m_IsPlaceholderFile(clientFile));
		Assert(client.Run<DepotResultFStat>("fstat", DepotStringArray{ StringInfo::ToAnsi(depotFile) })->Node().HeadType() == StringInfo::ToAnsi(fileType));

		BYTE fileResidencyPolicy = P4VFS_RESIDENCY_POLICY_UNDEFINED;
		Assert(context.m_FileContext != nullptr);
		Assert(SUCCEEDED(Microsoft::P4VFS::FileSystem::ResolveFileResidency(*context.m_FileContext, clientFile.c_str(), &fileResidencyPolicy)));
		Assert(fileResidencyPolicy == P4VFS_RESIDENCY_POLICY_RESIDENT);
		Assert(context.m_ReconcilePreviewAny(clientFile) == false);
	};

	TestUtilities::WorkspaceReset(context);

	FDepotClient client;
	Assert(client.Connect(context.GetDepotConfig()));
	ResolveFile(client, L"//depot/gears1/Development/Src/Core/Src/BitArray.cpp", L"text");
	ResolveFile(client, L"//depot/gears1/Development/External/nvDXT/Lib/nvDXTlib.vc7.lib", L"binary");
	ResolveFile(client, L"//depot/gears1/WarGame/Localization/INT/Human_Myrrah_Dialog.int", L"utf16");
	ResolveFile(client, L"//depot/tools/dev/external/EsrpClient/1.1.5/Enable-EsrpClientLog.ps1", L"utf8");
}

void TestRequireFilterOpLock(const TestContext& context)
{
	TestUtilities::WorkspaceReset(context);

	const String p4Options = context.GetEnvironment(L"P4VFS_EXE_CONFIG");
	Assert(p4Options.empty() == false);

	DepotClient client = FDepotClient::New(context.m_FileContext);
	Assert(client->Connect(context.GetDepotConfig()));
	const DepotString depotFile = "//depot/gears1/Development/Src/Core/Src/BitArray.cpp";
	DepotSyncResult syncResult = DepotOperations::Sync(client, DepotStringArray{ depotFile }, nullptr);
	Assert(syncResult.get() != nullptr);
	Assert(syncResult->m_Status == DepotSyncStatus::Success);
	Assert(syncResult->m_Modifications.get() && syncResult->m_Modifications->size() > 0);

	const String clientFile = StringInfo::ToWide(client->Run<DepotResultWhere>("where", DepotStringArray{ depotFile })->Node().LocalPath());
	Assert(FileInfo::IsRegular(clientFile.c_str()));
	Assert(context.m_IsPlaceholderFile(clientFile));

	// Open the clientFile for overlapped IO without reparse
	OVERLAPPED clientOverlapped = {0};
	AutoHandle hClientFile = CreateFile(clientFile.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_OVERLAPPED, &clientOverlapped);
	Assert(hClientFile.IsValid());

	// TODO: At this instant, another process may race to open clientFile and fail from Overlapped IO in progress

	// Request filter opportunistic lock on a clientFile, which could prevent the P4VFS service from hydrating it
	OVERLAPPED oplockOverlapped = {0};
	oplockOverlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	Assert(oplockOverlapped.hEvent != NULL);
	Assert(DeviceIoControl(hClientFile.Handle(), FSCTL_REQUEST_FILTER_OPLOCK, NULL, 0, NULL, 0, NULL, &oplockOverlapped) == FALSE);
	Assert(GetLastError() == ERROR_IO_PENDING);
	Assert(CloseHandle(oplockOverlapped.hEvent));

	struct OverlappedRead : OVERLAPPED
	{
		Array<uint8_t> data;
		uint8_t buffer[256];

		OverlappedRead()
		{
			memset(this, 0, sizeof(OVERLAPPED));
			memset(buffer, 0, sizeof(buffer));
			hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
			Assert(hEvent != NULL);
		}

		~OverlappedRead()
		{
			Assert(CloseHandle(hEvent));
		}

		static void CompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
		{
			OverlappedRead* overlapped = reinterpret_cast<OverlappedRead*>(lpOverlapped);
			Assert(overlapped != nullptr);
			Assert(dwNumberOfBytesTransfered <= DWORD(sizeof(buffer)));
			Algo::Append(overlapped->data, overlapped->buffer, dwNumberOfBytesTransfered);
			lpOverlapped->Offset += dwNumberOfBytesTransfered;
			SetEvent(lpOverlapped->hEvent);
		}
	};

	// Perform and asyncronous read of the entire file using this handle and OPLOCK
	OverlappedRead overlappedRead;
	BOOL asyncReadStatus = TRUE;
	while (asyncReadStatus)
	{
		asyncReadStatus = ReadFileEx(hClientFile.Handle(), overlappedRead.buffer, sizeof(overlappedRead.buffer), &overlappedRead, &OverlappedRead::CompletionRoutine);
		if (asyncReadStatus == FALSE)
		{
			break;
		}
	
		Assert(overlappedRead.hEvent != NULL);;
		WaitForSingleObjectEx(overlappedRead.hEvent, INFINITE, TRUE);

		DWORD dwBytesRead = 0;
		asyncReadStatus = GetOverlappedResult(hClientFile.Handle(), &overlappedRead, &dwBytesRead, TRUE);
		if (asyncReadStatus == FALSE && GetLastError() == ERROR_HANDLE_EOF)
		{
			asyncReadStatus = TRUE;
			break;
		}
	}

	hClientFile.Close();
	Assert(asyncReadStatus);

	// If this fails (SPARSE size=1742) then the file did not hydrate when OPLOCK was aquired
	Assert(overlappedRead.data.size() == 1768); 

	Array<uint8_t> syncReadData;
	Assert(FileInfo::ReadFile(clientFile.c_str(), syncReadData));
	Assert(overlappedRead.data == syncReadData);

	// The file will no longer be a placeholder
	Assert(context.m_IsPlaceholderFile(clientFile) == false);
	Assert(context.m_ReconcilePreviewAny(clientFile) == false);
}

void TestFileAlternateStream(const TestContext& context)
{
	TestUtilities::WorkspaceReset(context);

	DepotClient client = FDepotClient::New(context.m_FileContext);
	Assert(client->Connect(context.GetDepotConfig()));
	const DepotString depotFile = "//depot/gears1/Development/Src/Core/Src/BitArray.cpp";
	const String clientFile = StringInfo::ToWide(client->Run<DepotResultWhere>("where", DepotStringArray{ depotFile })->Node().LocalPath());
	const String clientFileAlt = StringInfo::Format(L"%s:Zone.Identifier", clientFile.c_str());

	Assert(FileInfo::CreateFileDirectory(clientFile.c_str()));
	Assert(AutoHandle(CreateFile(clientFileAlt.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)).IsValid());
	Assert(FileInfo::IsRegular(clientFile.c_str()));
	Assert(FileInfo::Delete(clientFile.c_str()));

	auto AssertForceSyncFile = [&]() -> void
	{
		DepotSyncResult syncResult = DepotOperations::Sync(client, DepotStringArray{ depotFile }, nullptr, DepotSyncFlags::Force);
		Assert(syncResult.get() && syncResult->m_Status == DepotSyncStatus::Success);
		Assert(FileInfo::IsRegular(clientFile.c_str()));
		Assert(context.m_IsPlaceholderFile(clientFile));
	};

	AssertForceSyncFile();

	uint64_t lastServiceRequestTime = TimeInfo::FileTimeToUInt64(context.m_ServiceLastRequestTime());

	Assert(AutoHandle(CreateFile(clientFile.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)).IsValid());
	Assert(context.m_IsPlaceholderFile(clientFile) == false);
	Assert(context.m_ReconcilePreviewAny(clientFile) == false);

	uint64_t nextServiceRequestTime = TimeInfo::FileTimeToUInt64(context.m_ServiceLastRequestTime());
	Assert(nextServiceRequestTime > lastServiceRequestTime);
	lastServiceRequestTime = nextServiceRequestTime;

	AssertForceSyncFile();
	Assert(FileInfo::SetReadOnly(clientFile.c_str(), false));

	Assert(AutoHandle(CreateFile(clientFileAlt.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)).IsValid() == false);
	Assert(context.m_IsPlaceholderFile(clientFile));
	Assert(lastServiceRequestTime == TimeInfo::FileTimeToUInt64(context.m_ServiceLastRequestTime()));

	Assert(AutoHandle(CreateFile(clientFileAlt.c_str(), GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)).IsValid() == false);
	Assert(context.m_IsPlaceholderFile(clientFile));
	Assert(lastServiceRequestTime == TimeInfo::FileTimeToUInt64(context.m_ServiceLastRequestTime()));

	Assert(FileInfo::SetReadOnly(clientFile.c_str(), true));
	Assert(context.m_ReconcilePreviewAny(clientFile) == false);
	Assert(context.m_IsPlaceholderFile(clientFile) == false);

	nextServiceRequestTime = TimeInfo::FileTimeToUInt64(context.m_ServiceLastRequestTime());
	Assert(nextServiceRequestTime > lastServiceRequestTime);
	lastServiceRequestTime = nextServiceRequestTime;

	Assert(FileInfo::SetReadOnly(clientFile.c_str(), false));
	Assert(AutoHandle(CreateFile(clientFileAlt.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)).IsValid());
	Assert(lastServiceRequestTime == TimeInfo::FileTimeToUInt64(context.m_ServiceLastRequestTime()));
}

void TestDevDriveAttachPolicy(const TestContext& context)
{
	auto GetFsutilAttachPolicy = []() -> StringArray
	{
		const wchar_t* cmd = TEXT("fsutil.exe devdrv query");
		Process::ExecuteResult result = Process::Execute(cmd, nullptr, Process::ExecuteFlags::StdOut|Process::ExecuteFlags::HideWindow);
		Assert(result.m_ExitCode == 0);
		
		StringArray names;
		bool nameLines = false;
		const AStringArray lines = StringInfo::Split(result.m_StdOut.c_str(), "\r\n", StringInfo::SplitFlags::RemoveEmptyEntries);
		for (const AString& line : lines)
		{
			if (nameLines == false)
			{
				nameLines = StringInfo::StartsWith(line.c_str(), "Filters allowed on any developer volume:");
				continue;
			}
			std::match_results<const char*> match;
			if (std::regex_search(line.c_str(), match, std::regex("^\\s+(\\S.*)")))
			{
				Algo::Append(names, StringInfo::Split(StringInfo::ToWide(match[1]).c_str(), TEXT(", "), StringInfo::SplitFlags::RemoveEmptyEntries));
				continue;
			}
			break;
		}
		return names;
	};

	auto SetFsutilAttachPolicy = [](const StringArray& names) -> void
	{
		const String cmd = StringInfo::Format(TEXT("fsutil.exe devdrv setFiltersAllowed \"%s\""), StringInfo::Join(names, TEXT(",")).c_str());
		Assert(Process::Execute(cmd.c_str(), nullptr, Process::ExecuteFlags::WaitForExit|Process::ExecuteFlags::HideWindow).m_ExitCode == 0);
	};

	const StringArray prevNames = GetFsutilAttachPolicy();

	StringArray names;
	SetFsutilAttachPolicy(names);
	Assert(GetFsutilAttachPolicy() == names);

	names = {TEXT("osmiumflt"),TEXT("carbonflt"),TEXT("argondrv")};
	SetFsutilAttachPolicy(names);
	Assert(GetFsutilAttachPolicy() == names);

	names.push_back(TEXT(P4VFS_DRIVER_TITLE));
	Assert(SUCCEEDED(Microsoft::P4VFS::DriverOperations::SetDevDriveFilterAllowed(TEXT(P4VFS_DRIVER_TITLE), true)));
	Assert(GetFsutilAttachPolicy() == names);

	names.push_back(TEXT("ironflt"));
	Assert(SUCCEEDED(Microsoft::P4VFS::DriverOperations::SetDevDriveFilterAllowed(TEXT("ironflt"), true)));
	Assert(GetFsutilAttachPolicy() == names);

	Algo::Remove(names, TEXT(P4VFS_DRIVER_TITLE));
	Assert(SUCCEEDED(Microsoft::P4VFS::DriverOperations::SetDevDriveFilterAllowed(TEXT(P4VFS_DRIVER_TITLE), false)));
	Assert(GetFsutilAttachPolicy() == names);

	SetFsutilAttachPolicy(prevNames);
	Assert(GetFsutilAttachPolicy() == prevNames);
}

void TestReadDirectoryChanges(const TestContext& context)
{
	TestUtilities::WorkspaceReset(context);

	DepotClient client = FDepotClient::New(context.m_FileContext);
	Assert(client->Connect(context.GetDepotConfig()));

	Assert(client->Connection().get() != nullptr);
	const String clientRootFolder = StringInfo::ToWide(client->Connection()->Root());
	Assert(FileInfo::IsDirectory(clientRootFolder.c_str()) == false);
	const String clientSearchFolder = StringInfo::Format(TEXT("%s\\depot\\gears1\\Development"), clientRootFolder.c_str());

	FDepotSyncOptions syncOptions;
	syncOptions.m_SyncFlags = DepotSyncFlags::Quiet;
	syncOptions.m_Files.push_back(StringInfo::Format("%s\\...", CSTR_WTOA(clientSearchFolder)));
	DepotSyncResult syncResult = DepotOperations::SyncVirtual(client, syncOptions);
	Assert(syncResult.get() != nullptr);
	Assert(syncResult->m_Status == DepotSyncStatus::Success);
	Assert(FileInfo::IsDirectory(clientSearchFolder.c_str()));

	struct FReadChangesThread
	{
		struct FChange
		{
			String m_File;
			DWORD m_Action;
			bool operator==(const FChange& c) const { return c.m_File == m_File && c.m_Action == m_Action; }
		};

		struct FData
		{
			String m_FolderPath;
			HANDLE m_hCancelationEvent;
			HANDLE m_hBeginReadEvent;
			Array<FChange> m_Changes;
		};

		static DWORD Execute(void* threadData)
		{
			FData* data = reinterpret_cast<FData*>(threadData);
			Assert(data != nullptr);
			Assert(data->m_hCancelationEvent != NULL);

			AutoHandle hFolder = CreateFileW(data->m_FolderPath.c_str(),
					FILE_LIST_DIRECTORY,
					FILE_SHARE_READ | FILE_SHARE_DELETE,
					NULL,
					OPEN_EXISTING,
					FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
					NULL);
			Assert(hFolder.IsValid());

			while (WaitForSingleObject(data->m_hCancelationEvent, 0) != WAIT_OBJECT_0)
			{
				// Watching for notifications on all flags except the Attributes change. We need to prevent 
				// other drivers from setting the Archive bit when hydrating
				const DWORD changesNotifyFlags =
					FILE_NOTIFY_CHANGE_CREATION |
					FILE_NOTIFY_CHANGE_LAST_WRITE |
					FILE_NOTIFY_CHANGE_SIZE |
					//FILE_NOTIFY_CHANGE_ATTRIBUTES |
					FILE_NOTIFY_CHANGE_DIR_NAME |
					FILE_NOTIFY_CHANGE_FILE_NAME;

				BYTE changesBuffer[8*1024] = {0};
				OVERLAPPED changesOverlapped = {};
				Assert(SetEvent(data->m_hBeginReadEvent));

				if (ReadDirectoryChangesW(hFolder.Handle(), changesBuffer, sizeof(changesBuffer), true, changesNotifyFlags, NULL, &changesOverlapped, NULL) == FALSE)
				{
					AssertMsg(false, TEXT("ReadDirectoryChanges failed"));
					return 1;
				}
 
				// Wait indefinitely for any directory changes or a cancellation
				const HANDLE handles[] = { hFolder.Handle(), data->m_hCancelationEvent };
				const DWORD waitResult = WaitForMultipleObjects(_countof(handles), handles, FALSE, INFINITE);

				if (waitResult == WAIT_OBJECT_0)
				{
					for (const BYTE* changesPos = changesBuffer;;)
					{
						if (changesPos >= changesBuffer+sizeof(changesBuffer)-sizeof(FILE_NOTIFY_INFORMATION))
						{
							AssertMsg(false, TEXT("ReadDirectoryChanges changesPos overflow"));
							return 1;
						}

						const FILE_NOTIFY_INFORMATION* fni = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(changesPos); 
						if (reinterpret_cast<const BYTE*>(fni->FileName+fni->FileNameLength) > changesBuffer+sizeof(changesBuffer))
						{
							AssertMsg(false, TEXT("ReadDirectoryChanges FileName overflow"));
							return 1;
						}

						data->m_Changes.push_back(FChange{ String(fni->FileName, fni->FileNameLength/sizeof(WCHAR)).c_str(), fni->Action });
						if (fni->NextEntryOffset == 0)
						{
							break;
						}

						changesPos += fni->NextEntryOffset;
					}
				}
				else if (waitResult == WAIT_OBJECT_0+1)
				{
					// Expected thread cancellation
				}
				else
				{
					AssertMsg(false, TEXT("Unexpected wait result %d"), waitResult);
					return 1;
				}
			}

			return 0;
		}
	};

	// Start the thread to watch for changes under clientSearchFolder
	AutoHandle hThreadCancelation = CreateEvent(NULL, TRUE, FALSE, NULL);
	AutoHandle hThreadBeginReadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	FReadChangesThread::FData threadData{ clientSearchFolder, hThreadCancelation.Handle(), hThreadBeginReadEvent.Handle() };
	AutoHandle hThread = CreateThread(NULL, 0, FReadChangesThread::Execute, &threadData, 0, NULL);

	// Wait for the thread to begin reading for changes, plus a little time for it to wait
	Assert(WaitForSingleObject(hThreadBeginReadEvent.Handle(), 5*60*1000) == WAIT_OBJECT_0);
	Sleep(1000);

	// Reconcile a single readonly file ... there should be no notifications
	const String immutableClientRelativeFile = TEXT("Src\\Engine\\Src\\AnimationUtils.cpp");
	const String immutableClientFile = StringInfo::Format(TEXT("%s\\%s"), clientSearchFolder.c_str(), immutableClientRelativeFile.c_str());
	Assert(context.m_IsPlaceholderFile(immutableClientFile));
	Assert(context.m_ReconcilePreviewAny(immutableClientFile) == false);
	Assert(context.m_IsPlaceholderFile(immutableClientFile) == false);

	// Open a placeholder file without hydrating, and write to it to trigger a notification
	const String mutableClientRelativeFile = TEXT("Src\\Core\\Src\\BitArray.cpp");
	const String mutableClientFile = StringInfo::Format(TEXT("%s\\%s"), clientSearchFolder.c_str(), mutableClientRelativeFile.c_str());
	Assert(context.m_IsPlaceholderFile(mutableClientFile));
	Assert(FileInfo::SetReadOnly(mutableClientFile.c_str(), false));
	AutoHandle hMutableClientFile = CreateFile(mutableClientFile.c_str(), FILE_GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	Assert(hMutableClientFile.IsValid());
	Assert(WriteFile(hMutableClientFile.Handle(), CSTR_WTOA(mutableClientRelativeFile), static_cast<DWORD>(mutableClientRelativeFile.length()), nullptr, nullptr));
	hMutableClientFile.Close();
		
	// Gracefully cancel the thread now that we're done testing for changes
	SetEvent(hThreadCancelation.Handle());
	Assert(WaitForSingleObject(hThread.Handle(), 5*60*1000) == WAIT_OBJECT_0);
	DWORD dwThreadExitCode = 1;
	Assert(GetExitCodeThread(hThread.Handle(), &dwThreadExitCode));
	Assert(dwThreadExitCode == 0);

	// Gather the unique list changes, ignoring directories
	Array<FReadChangesThread::FChange> uniqueChanges;
	Algo::AppendIf(uniqueChanges, threadData.m_Changes, [&](const FReadChangesThread::FChange& c) -> bool
	{
		return !Algo::Contains(uniqueChanges, c) && !FileInfo::IsDirectory(StringInfo::Format(TEXT("%s\\%s"), clientSearchFolder.c_str(), c.m_File.c_str()).c_str());
	});

	// Confirm that only the notifications that we expected were recorded
	Assert(uniqueChanges.size() == 1);
	Assert(uniqueChanges[0].m_File == mutableClientRelativeFile);
}

