// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "TestFactory.h"
#include "DepotClient.h"
#include "DepotResultPrint.h"

using namespace Microsoft::P4VFS::FileCore;
using namespace Microsoft::P4VFS::TestCore;
using namespace Microsoft::P4VFS::P4;

void TestDepotClientPrintToFile(const TestContext& context)
{
	auto PrintToFile = [&context](FDepotClient& client, const String& depotFile) -> void
	{
		DepotResultClient workspace = client.Client();
		Assert(workspace->HasError() == false);
		DepotString rootFolder = workspace->Root();
		Assert(rootFolder.size() > 0);

		String clientFile0 = StringInfo::Format(L"%s\\%s0%s", CSTR_ATOW(rootFolder), FileInfo::FileTitle(depotFile.c_str()).c_str(), FileInfo::FileExtension(depotFile.c_str()).c_str());
		Assert(FileInfo::Delete(clientFile0.c_str()));
		DepotResult print = client.Run("print", DepotStringArray{"-o", StringInfo::ToAnsi(clientFile0), StringInfo::ToAnsi(depotFile)});
		Assert(print.get() && print->HasError() == false);
		Assert(FileInfo::Exists(clientFile0.c_str()));
		Assert(FileInfo::FileSize(clientFile0.c_str()) > 0);

		String clientFile1 = StringInfo::Format(L"%s\\%s1%s", CSTR_ATOW(rootFolder), FileInfo::FileTitle(depotFile.c_str()).c_str(), FileInfo::FileExtension(depotFile.c_str()).c_str());
		FILE* outFile = fopen(StringInfo::WtoA(clientFile1), "wb");
		Assert(outFile);
		FDepotResultPrintFile printFile(outFile);
		client.Run(DepotCommand("print", DepotStringArray{StringInfo::ToAnsi(depotFile)}), printFile);
		Assert(printFile.HasError() == false);
		fclose(outFile);
		Assert(FileInfo::Exists(clientFile1.c_str()));
		Assert(FileInfo::FileSize(clientFile1.c_str()) > 0);

		Assert(FileInfo::FileHashMd5(clientFile0.c_str()) > 0);
		Assert(FileInfo::Compare(clientFile0.c_str(), clientFile1.c_str()) == 0);
	};

	TestUtilities::WorkspaceReset(context);

	FDepotClient client;
	Assert(client.Connect(context.GetDepotConfig()));
	PrintToFile(client, L"//depot/gears1/Development/Src/Core/Src/BitArray.cpp");
	PrintToFile(client, L"//depot/gears1/Development/External/nvDXT/Lib/nvDXTlib.vc7.lib");
}

void TestDepotClientConnectFromClientOwner(const TestContext& context)
{
	DepotConfig clientConfig = context.GetDepotConfig();
	clientConfig.m_User += "_invalid";
	
	FDepotClient client;
	Assert(client.Connect(clientConfig));
	Assert(client.Config().m_User == context.GetDepotConfig().m_User);
	Assert(client.Info()->Node().UserName() == context.GetDepotConfig().m_User);
}

void TestDepotClientIsWritableFileType(const TestContext& context)
{
	struct FLegacyDepotClient
	{
		static bool IsWritableFileType(const DepotString& fileType)
		{
			std::match_results<const char*> match;
			return fileType.empty() == false && std::regex_search(fileType.c_str(), match, std::regex("\\+.*w", std::regex_constants::ECMAScript|std::regex_constants::icase));
		}
	};

	#define TestIsWritableFileType(...) Assert(DepotInfo::IsWritableFileType(__VA_ARGS__) == FLegacyDepotClient::IsWritableFileType(__VA_ARGS__))
	TestIsWritableFileType("");
	TestIsWritableFileType("+");
	TestIsWritableFileType("+w");
	TestIsWritableFileType("+x");
	TestIsWritableFileType("binary+w");
	TestIsWritableFileType("binary+x");
	TestIsWritableFileType("text+wC");
	TestIsWritableFileType("text+WC");
	TestIsWritableFileType("+S12w");
	TestIsWritableFileType("+S12");
	TestIsWritableFileType("binaryw+x");
	TestIsWritableFileType("binaryw+w");
	#undef TestIsWritableFileType
}

void TestDepotClientIsSymlinkFileType(const TestContext& context)
{
	struct FLegacyDepotClient
	{
		static bool IsSymlinkFileType(const DepotString& fileType)
		{
			std::match_results<const char*> match;
			return fileType.empty() == false && std::regex_search(fileType.c_str(), match, std::regex("symlink", std::regex_constants::ECMAScript|std::regex_constants::icase));
		}
	};

	#define TestIsWritableFileType(...) Assert(DepotInfo::IsSymlinkFileType(__VA_ARGS__) == FLegacyDepotClient::IsSymlinkFileType(__VA_ARGS__))
	TestIsWritableFileType("");
	TestIsWritableFileType("+x");
	TestIsWritableFileType("text");
	TestIsWritableFileType("symlink");
	TestIsWritableFileType("SYmLiNK");
	TestIsWritableFileType("binary+x");
	TestIsWritableFileType("symlink+wC");
	#undef TestIsWritableFileType
}

