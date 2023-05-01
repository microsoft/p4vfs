// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "TestFactory.h"
#include "DepotClient.h"
#include "DepotResultWhere.h"
#include "DepotOperations.h"
#include "DirectoryOperations.h"

using namespace Microsoft::P4VFS::FileCore;
using namespace Microsoft::P4VFS::TestCore;
using namespace Microsoft::P4VFS::P4;
using namespace Microsoft::P4VFS;

void TestIterateDirectoryParallel(const TestContext& context)
{
	struct Local
	{
		static uint64_t GetPathHash(const String& path)
		{
			return StringInfo::HashMd5(StringInfo::ToLower(FileInfo::UnextendedPath(path.c_str()).c_str()));
		}

		static uint64_t GetDirectoryHash(const TestContext& context, const String& folderPath)
		{
			DepotStopwatch timer(DepotStopwatch::Init::Start);
			StringArray paths;
			paths.push_back(folderPath);
			FileInfo::FindFiles(paths, folderPath.c_str(), nullptr, FileInfo::Find::kDirectories|FileInfo::Find::kFiles|FileInfo::Find::kRecursive);
			
			uint64_t hash = 0;
			for (const String& path : paths)
			{
				hash ^= GetPathHash(path);
			}

			context.Log()->Info(StringInfo::Format(TEXT("GetDirectoryHash folderPath=%s duration=%0.4f (ms)"), folderPath.c_str(), timer.DurationMilliseconds()));
			return hash;
		}

		static uint64_t GetDirectoryHashParallel(const TestContext& context, const String& folderPath, int32_t numThreads, bool verbose = false)
		{
			std::atomic<uint64_t> hash = 0;
			DirectoryOperations::IterateDirectoryVisitor visitor = [&context, &hash, verbose](const String& path, DWORD dwAttributes) -> bool 
			{
				// Note that XOR is both cummutative and associative, so these hash values can be distributed in any order with the same result
				hash ^= GetPathHash(path);
				if (verbose)
				{
					const wchar_t* type = dwAttributes & FILE_ATTRIBUTE_DIRECTORY ? TEXT("Directory") : TEXT("File");
					context.Log()->Verbose(StringInfo::Format(TEXT("[0x%08x] %s: %s"), uint32_t(GetCurrentThreadId()), type, path.c_str()));
				}
				return true;
			};

			DepotStopwatch timer(DepotStopwatch::Init::Start);
			DirectoryOperations::IterateDirectoryParallel(folderPath.c_str(), visitor, numThreads);
			context.Log()->Info(StringInfo::Format(TEXT("GetDirectoryHashParallel folderPath=%s numThreads=%d duration=%0.4f (ms)"), folderPath.c_str(), numThreads, timer.DurationMilliseconds()));
			return hash;
		}

		static void AssertIterateDirectory(const TestContext& context, const String& folderPath)
		{
			// Get the hash of all files and folders recursively using FileInfo::FindFiles
			const String fullFolderPath = FileInfo::FullPath(folderPath.c_str());
			const uint64_t hash = GetDirectoryHash(context, folderPath);

			// Test against the hash of all files and folders recursively using IterateDirectoryParallel with various number of threads
			for (uint32_t count = 0; count < 32; count = std::max<uint32_t>(1, count*2))
			{
				bool verbose = count == 0;
				uint64_t hashParallel = GetDirectoryHashParallel(context, folderPath, count, verbose);
				Assert(hash == hashParallel);
			}
		}
	};

	TestUtilities::WorkspaceReset(context);

	DepotClient client = FDepotClient::New(context.m_FileContext);
	Assert(client->Connect(context.GetDepotConfig()));
	
	FDepotSyncOptions syncOptions;
	syncOptions.m_SyncType = DepotSyncType::Quiet;
	DepotSyncResult syncResult = DepotOperations::Sync(client, syncOptions);
	Assert(syncResult.get() != nullptr);
	Assert(syncResult->m_Status == DepotSyncStatus::Success);
	Assert(context.m_ReconcilePreviewAny(TEXT("//depot/gears1/Development/...")) == false);

	Assert(client->Connection().get() != nullptr);
	const String clientRootFolder = StringInfo::ToWide(client->Connection()->Root());
	Assert(FileInfo::IsDirectory(clientRootFolder.c_str()));

	Local::AssertIterateDirectory(context, StringInfo::Format(L"%s\\depot\\gears1", clientRootFolder.c_str()));
	Local::AssertIterateDirectory(context, StringInfo::Format(L"%s\\depot\\tools", clientRootFolder.c_str()));
	Local::AssertIterateDirectory(context, StringInfo::Format(L"%s\\depot\\gears1\\NON_EXISTING", clientRootFolder.c_str()));
}
