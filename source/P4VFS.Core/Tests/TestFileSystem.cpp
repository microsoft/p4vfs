// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "TestFactory.h"
#include "DepotClient.h"
#include "DepotResultWhere.h"
#include "DepotResultFStat.h"
#include "DepotOperations.h"
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

		Assert(TestUtilities::ExecuteWait(p4vfsExe, StringInfo::Format(L"%s sync -f %s", p4Options.c_str(), depotFile.c_str())) == 0);
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
