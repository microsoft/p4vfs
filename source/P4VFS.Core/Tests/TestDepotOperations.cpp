// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "TestFactory.h"
#include "DepotClient.h"
#include "DepotResultWhere.h"
#include "FileSystem.h"
#include "DepotOperations.h"

using namespace Microsoft::P4VFS::FileCore;
using namespace Microsoft::P4VFS::TestCore;
using namespace Microsoft::P4VFS::P4;

void TestDepotOperationsSync(const TestContext& context)
{
	TestUtilities::WorkspaceReset(context);

	DepotClient client = FDepotClient::New(context.m_FileContext);
	Assert(client->Connect(context.GetDepotConfig()));
	DepotResult changes = client->Run(DepotCommand("changes"));
	Assert(changes->HasError() == false);
	Assert(changes->TagList().size() > 0);
	Assert(changes->TagList().size() == size_t(std::count_if(changes->TagList().begin(), changes->TagList().end(), [](const DepotResultTag& t) -> bool { return t->m_Fields.find("change") != t->m_Fields.end(); })));

	DepotRevision headChange = DepotOperations::GetHeadRevisionChangelist(client);
	Assert(headChange.get());
	int32_t headChangeNumber = atoi(StringInfo::TrimLeft(headChange->ToString().c_str(), "@").c_str());
	Assert(headChangeNumber >= changes->TagList().size());
	int32_t lastChangeNumber = atoi(StringInfo::TrimLeft(changes->TagList().front()->GetValue("change").c_str(), "@").c_str());
	Assert(lastChangeNumber == headChangeNumber);

	const char* fileSpecs[] = {
		"//depot/tools/dev/source/CinematicCapture/...@16",
		"//depot/tools/dev/source/CinematicCapture/...@18",
		"//depot/tools/dev/source/Hammer/...@19",
		"//depot/tools/dev/external/EsrpClient/1.1.5/Enable-EsrpClientLog.ps1",
		"//depot/gears1/WarGame/Localization/INT/Human_Myrrah_Dialog.int",
	};

	for (DepotSyncMethod::Enum syncMethod : { DepotSyncMethod::Virtual, DepotSyncMethod::Regular })
	{
		for (DepotSyncType::Enum syncType : { DepotSyncType::Normal, DepotSyncType::Quiet })
		{
			TestUtilities::WorkspaceReset(context);
			for (const char* fileSpec : fileSpecs)
			{
				DepotSyncResult syncResult = DepotOperations::Sync(client, DepotStringArray{ fileSpec }, nullptr, syncType, syncMethod);
				Assert(syncResult.get() != nullptr);
				Assert(syncResult->m_Status == DepotSyncStatus::Success);
				Assert(syncResult->m_Modifications.get() && syncResult->m_Modifications->size() > 0);
				Assert(client->Run(DepotCommand("flush", DepotStringArray{ "-f", fileSpec }))->HasError() == false);
				Assert(context.m_ReconcilePreviewAny(TEXT("//...")) == false);
			}
		}
	}
}

void TestDepotOperationsSyncStatus(const TestContext& context)
{
	TestUtilities::WorkspaceReset(context);
	DepotClient client = FDepotClient::New(context.m_FileContext);

	auto AssertStatus = [&client, &context](const DepotString& fileSpec, DepotSyncStatus::Enum status, int32_t modificationCount) -> void
	{
		for (DepotSyncMethod::Enum syncMethod : { DepotSyncMethod::Virtual, DepotSyncMethod::Regular })
		{
			for (DepotSyncType::Enum syncType : { DepotSyncType::Normal, DepotSyncType::Quiet })
			{
				DepotSyncResult syncResult = DepotOperations::Sync(client, DepotStringArray{ fileSpec }, nullptr, syncType, syncMethod);
				Assert(syncResult.get() != nullptr);
				Assert(syncResult->m_Status == status);
				Assert((modificationCount < 0 && syncResult->m_Modifications.get() == nullptr) || (syncResult->m_Modifications.get() && int32_t(syncResult->m_Modifications->size()) == modificationCount));
				Assert(context.m_ReconcilePreviewAny(TEXT("//...")) == false);
			}
		}
	};
	
	const DepotString depotFile = "//depot/tools/dev/source/CinematicCapture/DefaultSettings.xml";
	const DepotString clientFile = client->Run<DepotResultWhere>("where", DepotStringArray{ depotFile })->Node().LocalPath();
	Assert(clientFile.size() > 0);

	AssertStatus("", DepotSyncStatus::Error, -1);
	AssertStatus(depotFile+"#1", DepotSyncStatus::Success, 1);
	AssertStatus(depotFile+"#512", DepotSyncStatus::Error, 0);
	Assert(FileInfo::IsRegular(CSTR_ATOW(clientFile)));
	Assert(FileInfo::IsReadOnly(CSTR_ATOW(clientFile)));
	Assert(FileInfo::SetReadOnly(CSTR_ATOW(clientFile), false));
	AssertStatus(depotFile+"#2", DepotSyncStatus::Error, 1);
	Assert(FileInfo::SetReadOnly(CSTR_ATOW(clientFile), true));
	AssertStatus(depotFile+"#2", DepotSyncStatus::Success, 1);
}

void TestDepotOperationsToString(const TestContext& context)
{
	Assert(DepotOperations::ToString(DepotStringArray{}) == "[]");
	Assert(DepotOperations::ToString(DepotStringArray{"//foo/bar"}) == "[\"//foo/bar\"]");
	Assert(DepotOperations::ToString(DepotStringArray{"//foo/bar", "//foo/star"}) == "[\"//foo/bar\",\"//foo/star\"]");
	Assert(DepotOperations::ToString(DepotStringArray{"//foo/bar", ""}) == "[\"//foo/bar\",\"\"]");

	Assert(DepotSyncType::ToString(DepotSyncType::Flush) == "Flush");
	Assert(DepotSyncType::ToString(DepotSyncType::Enum(DepotSyncType::Flush|DepotSyncType::Quiet)) == "Flush|Quiet");
	Assert(DepotSyncType::ToString(DepotSyncType::Enum(DepotSyncType::Flush|DepotSyncType::Quiet|DepotSyncType::IgnoreOutput)) == "Flush|IgnoreOutput|Quiet");
	Assert(DepotSyncType::ToString(DepotSyncType::Enum(0)) == "Normal");
	Assert(DepotSyncType::ToString(DepotSyncType::Enum(1<<16)) == "");

	Assert(DepotSyncMethod::ToString(DepotSyncMethod::Regular) == "Regular");
	Assert(DepotSyncMethod::ToString(DepotSyncMethod::Virtual) == "Virtual");
	Assert(DepotSyncMethod::ToString(DepotSyncMethod::Enum(-1)) == "");

	Assert(DepotFlushType::ToString(DepotFlushType::Single) == "Single");
	Assert(DepotFlushType::ToString(DepotFlushType::Atomic) == "Atomic");
	Assert(DepotFlushType::ToString(DepotFlushType::Enum(-1)) == "");

	Assert(DepotSyncAction::ToString(DepotSyncAction::Deleted) == "Deleted");
	Assert(DepotSyncAction::ToString(DepotSyncAction::NoFilesFound) == "NoFilesFound");
	Assert(DepotSyncAction::ToString(DepotSyncAction::Enum(-1)) == "");

	Assert(DepotSyncOption::ToString(DepotSyncOption::FileWrite) == "FileWrite");
	Assert(DepotSyncOption::ToString(DepotSyncOption::Enum(DepotSyncOption::FileWrite|DepotSyncOption::ClientWrite)) == "FileWrite|ClientWrite");
	Assert(DepotSyncOption::ToString(DepotSyncOption::Enum(DepotSyncOption::FileWrite|DepotSyncOption::ClientWrite|DepotSyncOption::FileSymlink)) == "FileWrite|ClientWrite|FileSymlink");
	Assert(DepotSyncOption::ToString(DepotSyncOption::Enum(0)) == "None");
	Assert(DepotSyncOption::ToString(DepotSyncOption::Enum(1<<16)) == "");

	Assert(FDepotRevision::ToString(FDepotRevision::FromString("@32")) == "@32");
	Assert(FDepotRevision::ToString(FDepotRevision::FromString("#16")) == "#16");
	Assert(FDepotRevision::ToString(nullptr) == "");
	Assert(FDepotRevision::ToString(DepotRevision()) == "");
}

void TestDepotOperationsCreateFileSpec(const TestContext& context)
{
	struct FLegacyDepotClient
	{
		static DepotString CreateFileSpec(const DepotString& filePath, const DepotRevision& revision, DepotOperations::CreateFileSpecFlags::Enum flags)
		{
			DepotString fileSpecPath = StringInfo::Trim(filePath.c_str(), "\" ");
			DepotString fileSpecRev = FDepotRevision::ToString(revision);

			const DepotString fileSpecMatch = fileSpecPath;
			std::match_results<const char*> match;
			if (std::regex_search(fileSpecMatch.c_str(), match, std::regex("(.*)([@#].*)")))
			{
				fileSpecPath = match[1];
				if ((flags & DepotOperations::CreateFileSpecFlags::OverrideRevison) == 0 && match[2].length() > 1)
					fileSpecRev = match[2];
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
	};

	#define TestCreateFileSpec(...) Assert(DepotOperations::CreateFileSpec(__VA_ARGS__) == FLegacyDepotClient::CreateFileSpec(__VA_ARGS__))
	for (const DepotString& path : DepotStringArray({ "", "//depot/gears1", "//depot/gears1/...", "D:\\depot\\gears1", "D:\\depot\\gears1\\..." }))
	{
		for (const DepotString& rev : DepotStringArray({ "", "@1234", "#have", "@my_label", "#", "@" }))
		{
			TestCreateFileSpec(path, nullptr, DepotOperations::CreateFileSpecFlags::None);
			TestCreateFileSpec(path+rev, nullptr, DepotOperations::CreateFileSpecFlags::None);
			TestCreateFileSpec(path, nullptr, DepotOperations::CreateFileSpecFlags::OverrideRevison);
			TestCreateFileSpec(path+rev, nullptr, DepotOperations::CreateFileSpecFlags::OverrideRevison);

			TestCreateFileSpec(path, FDepotRevision::New<FDepotRevisionHead>(), DepotOperations::CreateFileSpecFlags::None);
			TestCreateFileSpec(path+rev, FDepotRevision::New<FDepotRevisionHead>(), DepotOperations::CreateFileSpecFlags::None);
			TestCreateFileSpec(path, FDepotRevision::New<FDepotRevisionHead>(), DepotOperations::CreateFileSpecFlags::OverrideRevison);
			TestCreateFileSpec(path+rev, FDepotRevision::New<FDepotRevisionHead>(), DepotOperations::CreateFileSpecFlags::OverrideRevison);
		}
	}
	#undef TestCreateFileSpec
}

void TestDepotOperationsToDisplayString(const TestContext& context)
{
	Assert(DepotOperations::ToDisplayStringBytes(uint64_t(0)) == "0 bytes");
	Assert(DepotOperations::ToDisplayStringBytes(uint64_t(1)) == "1 bytes");
	Assert(DepotOperations::ToDisplayStringBytes(uint64_t(1024)) == "1024 bytes (1.0 KB)");
	Assert(DepotOperations::ToDisplayStringBytes(uint64_t(1048576)) == "1048576 bytes (1.0 MB)");
	Assert(DepotOperations::ToDisplayStringBytes(uint64_t(1073741824)) == "1073741824 bytes (1.0 GB)");
	Assert(DepotOperations::ToDisplayStringBytes(uint64_t(1099511627776)) == "1099511627776 bytes (1.0 TB)");
	Assert(DepotOperations::ToDisplayStringBytes(uint64_t(1125899906842624)) == "1125899906842624 bytes (1024.0 TB)");
	Assert(DepotOperations::ToDisplayStringBytes(uint64_t(345)) == "345 bytes");
	Assert(DepotOperations::ToDisplayStringBytes(uint64_t(45056)) == "45056 bytes (44.0 KB)");
	Assert(DepotOperations::ToDisplayStringBytes(uint64_t(8221)) == "8221 bytes (8.0 KB)");
	Assert(DepotOperations::ToDisplayStringBytes(uint64_t(41107456)) == "41107456 bytes (39.2 MB)");
	Assert(DepotOperations::ToDisplayStringBytes(uint64_t(269881344)) == "269881344 bytes (257.4 MB)");
	Assert(DepotOperations::ToDisplayStringBytes(uint64_t(54267904)) == "54267904 bytes (51.8 MB)");
	Assert(DepotOperations::ToDisplayStringBytes(uint64_t(65536)) == "65536 bytes (64.0 KB)");
	Assert(DepotOperations::ToDisplayStringBytes(uint64_t(150364160)) == "150364160 bytes (143.4 MB)");
	Assert(DepotOperations::ToDisplayStringBytes(uint64_t(1190301696)) == "1190301696 bytes (1.1 GB)");
	Assert(DepotOperations::ToDisplayStringBytes(uint64_t(98613489664)) == "98613489664 bytes (91.8 GB)");
	Assert(DepotOperations::ToDisplayStringBytes(uint64_t(2528876743884)) == "2528876743884 bytes (2.3 TB)");
	Assert(DepotOperations::ToDisplayStringBytes(uint64_t(5568777679478548)) == "5568777679478548 bytes (5064.8 TB)");

	Assert(DepotOperations::ToDisplayStringMilliseconds(0) == "0 sec");
	Assert(DepotOperations::ToDisplayStringMilliseconds(16463234) == "16463 sec (4:34:23.234)");
	Assert(DepotOperations::ToDisplayStringMilliseconds(56) == "0 sec (0.056)");
	Assert(DepotOperations::ToDisplayStringMilliseconds(560) == "0 sec (0.560)");
	Assert(DepotOperations::ToDisplayStringMilliseconds(1000) == "1 sec (1.000)");
	Assert(DepotOperations::ToDisplayStringMilliseconds(43320001) == "43320 sec (12:02:00.001)");
	Assert(DepotOperations::ToDisplayStringMilliseconds(43320000) == "43320 sec (12:02:00.000)");
	Assert(DepotOperations::ToDisplayStringMilliseconds(5112891) == "5112 sec (1:25:12.891)");
	Assert(DepotOperations::ToDisplayStringMilliseconds(45083) == "45 sec (45.083)");
	Assert(DepotOperations::ToDisplayStringMilliseconds(3645083) == "3645 sec (1:00:45.083)");
	Assert(DepotOperations::ToDisplayStringMilliseconds(225083) == "225 sec (3:45.083)");
	Assert(DepotOperations::ToDisplayStringMilliseconds(25003) == "25 sec (25.003)");
}
