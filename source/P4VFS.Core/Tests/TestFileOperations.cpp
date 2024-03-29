// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "TestFactory.h"
#include "FileOperations.h"

using namespace Microsoft::P4VFS::FileCore;
using namespace Microsoft::P4VFS::TestCore;
using namespace Microsoft::P4VFS;

void TestFileOperationsUnicodeString(const TestContext& context)
{
	const UCHAR sentinalByte = 0xCD;
	const WCHAR* shortFilePath = L"C:\\memory.dmp";
	const WCHAR* typicalFilePath = L"C:\\depot\\tools\\dev\\source\\Hammer\\Hammer.Interfaces\\BaseClasses\\Launcher\\ApplicationDescription.cs";
	const WCHAR* veryLongFilePath = L"C:\\depot\\tools\\dev\\external\\packages\\thirdparty\\microsoft\\Windows Kits\\10\\References\\10.0.17134.0\\Windows.ApplicationModel.Activation.WebUISearchActivatedEventsContract\\1.0.0.0\\zh-hans\\2019\\Enterprise\\VSSDK\\VisualStudioIntegration\\Common\\Source\\CPP\\VSL\\VSLArcitecture_files\\Microsoft Azure Tools\\Visual Studio 16.0\\2.9\\RemoteDebuggerConnector\\Connector\\MSBuild\\Microsoft\\Microsoft.NET.Build.Extensions\\net471\\Windows.ApplicationModel.Activation.WebUISearchActivatedEventsContract\\References\\Windows.ApplicationModel.CommunicationBlocking.CommunicationBlockingContract\\2.0.0.0\\Windows.ApplicationModel.CommunicationBlocking.CommunicationBlockingContract.WinMD\\Windows.ApplicationModel.Background.BackgroundAlarmApplicationContract.xml";

	auto AssertAppendUnicodeStringReference = [&](const WCHAR* srcText, const ULONG dstPadding = 0) -> void 
	{
		Array<UCHAR> dstBuffer(dstPadding+sizeof(sentinalByte), 0);
		size_t dstStringOffset = dstBuffer.size() - (dstPadding > 0 ? dstPadding % 2 : 0);
		Assert(dstStringOffset > 0);
		dstBuffer[dstStringOffset-1] = sentinalByte;

		Assert(SUCCEEDED(FileOperations::AppendUnicodeStringReference(dstBuffer, dstStringOffset, srcText)));

		P4VFS_UNICODE_STRING* dstString = reinterpret_cast<P4VFS_UNICODE_STRING*>(dstBuffer.data()+dstStringOffset);
		Assert(StringInfo::Strcmp(dstString->c_str(), srcText) == 0);
		Assert(dstString->sizeBytes == (StringInfo::Strlen(srcText)+1)*sizeof(WCHAR));
		Assert(dstBuffer[dstStringOffset-1] == sentinalByte);
	};

	AssertAppendUnicodeStringReference(typicalFilePath);
	AssertAppendUnicodeStringReference(typicalFilePath, 39);
	AssertAppendUnicodeStringReference(typicalFilePath, 15);
	AssertAppendUnicodeStringReference(veryLongFilePath);
	AssertAppendUnicodeStringReference(veryLongFilePath, 50);
	AssertAppendUnicodeStringReference(veryLongFilePath);
	AssertAppendUnicodeStringReference(veryLongFilePath, 50);
	AssertAppendUnicodeStringReference(shortFilePath);
	AssertAppendUnicodeStringReference(shortFilePath, 17);
	AssertAppendUnicodeStringReference(L"");
	AssertAppendUnicodeStringReference(L"", 19);
}

void TestFileOperationsOpenReparsePointFile(const TestContext& context)
{
	TestUtilities::WorkspaceReset(context);

	const String rootFolder = context.GetEnvironment(TEXT("P4ROOT"));
	Assert(rootFolder.size() > 0);
	const String localRootFolder = StringInfo::Format(TEXT("%s\\TestOpenReparsePointFile"), rootFolder.c_str());
	Assert(FileInfo::IsDirectory(localRootFolder.c_str()) == false);
	Assert(FileInfo::CreateDirectory(localRootFolder.c_str()));

	// Create a new file with no share modes
	const String reparseFilePath = FileInfo::FullPath((localRootFolder + TEXT("/test.txt")).c_str());
	AutoHandle reparseFileHandle = CreateFile(reparseFilePath.c_str(), FILE_GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	Assert(reparseFileHandle.IsValid());
	Assert(GetFileAttributes(reparseFilePath.c_str()) != INVALID_FILE_ATTRIBUTES);

	// Verify that we cannot open the file for read or write
	Assert(CreateFile(reparseFilePath.c_str(), FILE_GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL) == INVALID_HANDLE_VALUE);
	Assert(CreateFile(reparseFilePath.c_str(), FILE_GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL) == INVALID_HANDLE_VALUE);

	// Open the file for write as a reparse point excluding share access checks
	P4VFS_FLT_FILE_HANDLE fltFile = FileOperations::OpenReparsePointFile(reparseFilePath.c_str(), GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE);
	Assert(fltFile.fileHandle != NULL && fltFile.fileHandle != INVALID_HANDLE_VALUE && fltFile.fileObject != nullptr);
	
	Array<uint8_t> localFileWriteBytes;
	for (uint32_t i = 1; i <= 5693; ++i)
		localFileWriteBytes.push_back(uint8_t(i*17));

	DWORD localFileWritten = 0;
	Assert(WriteFile(fltFile.fileHandle, localFileWriteBytes.data(), DWORD(localFileWriteBytes.size()), &localFileWritten, nullptr) != FALSE);
	Assert(localFileWritten == localFileWriteBytes.size());

	Assert(SUCCEEDED(FileOperations::CloseReparsePointFile(fltFile)));
	Assert(SUCCEEDED(reparseFileHandle.Close()));

	// All handles are closed. Read and verify that the file contains what we expect
	Array<uint8_t> localFileReadBytes;
	Assert(FileInfo::ReadFile(reparseFilePath.c_str(), localFileReadBytes));
	Assert(localFileReadBytes.size() == localFileWriteBytes.size());
	Assert(memcmp(localFileReadBytes.data(), localFileWriteBytes.data(), localFileWriteBytes.size()) == 0);
}

void TestFileOperationsAccess(const TestContext& context)
{
	const String rootFolder = context.GetEnvironment(TEXT("P4ROOT"));
	Assert(rootFolder.size() > 0);

	const String localRootFolder = StringInfo::Format(TEXT("%s\\TestFileOperationsAccess"), rootFolder.c_str());
	Assert(FileInfo::DeleteDirectoryRecursively(localRootFolder.c_str()));
	Assert(FileInfo::IsDirectory(localRootFolder.c_str()) == false);
	Assert(FileInfo::CreateDirectory(localRootFolder.c_str()));

	auto AssertTypeLogFile = [&context](const String& filePath, const wchar_t* logLinePrefix) -> void
	{
		Assert(FileInfo::ReadFileLines(filePath.c_str(), [logLinePrefix, &context](const AString& logLine) -> bool 
		{ 
			String logLineOut(logLinePrefix ? logLinePrefix : TEXT(""));
			logLineOut += StringInfo::ToWide(logLine);
			context.Log()->Info(StringInfo::TrimRight(logLineOut.c_str()));
			return true;
		}));
		LogSystem::StaticInstance().Flush();
	};

	auto AssertFileContains = [](const String& filePath, const wchar_t* findText) -> void
	{
		bool found = false;
		Assert(FileInfo::ReadFileLines(filePath.c_str(), [&found, findText](const AString& logLine) -> bool 
		{ 
			found = StringInfo::Contains(StringInfo::ToWide(logLine).c_str(), findText);
			return !found;
		}));
		Assert(found);
	};

	const String p4vfsExe = context.GetEnvironment(TEXT("P4VFS_EXE"));
	Assert(FileInfo::IsRegular(p4vfsExe.c_str()));

	// Confirm successfull run of elevated process
	Assert(TestUtilities::ExecuteWait(context, TEXT("fltmc.exe")) == 0);
	
	// Confirm unsuccessfull run of unelevated process
	const String fltmcUnelevatedOutputFile = FileInfo::FullPath(StringInfo::Format(TEXT("%s\\fltmc-unelevated.txt"), localRootFolder.c_str()).c_str());
	Assert(TestUtilities::ExecuteWait(context, StringInfo::Format(TEXT("cmd.exe /s /c fltmc.exe > \"%s\" 2>&1"), fltmcUnelevatedOutputFile.c_str()).c_str(), nullptr, Process::ExecuteFlags::Unelevated) != 0);
	AssertFileContains(fltmcUnelevatedOutputFile, TEXT("Access is denied."));

	// Confirm successfull run of p4vfs test TestFileOperationsAccessElevated
	const String testElevatedOutputFile = FileInfo::FullPath(StringInfo::Format(TEXT("%s\\test-elevated.txt"), localRootFolder.c_str()).c_str());
	int32_t testElevatedPriority = P4VFS_FIND_TEST(TestFileOperationsAccessElevated).m_Priority;
	int32_t testElevatedResult = TestUtilities::ExecuteWait(context, StringInfo::Format(TEXT("cmd.exe /s /c %s test -e %d > \"%s\" 2>&1"), p4vfsExe.c_str(), testElevatedPriority, testElevatedOutputFile.c_str()));
	AssertTypeLogFile(testElevatedOutputFile, TEXT("[AccessElevated] "));
	Assert(testElevatedResult == 0);

	// Confirm successfull run of p4vfs test TestFileOperationsAccessUnelevated
	const String testUnelevatedOutputFile = FileInfo::FullPath(StringInfo::Format(TEXT("%s\\test-unelevated.txt"), localRootFolder.c_str()).c_str());
	int32_t testUnelevatedPriority = P4VFS_FIND_TEST(TestFileOperationsAccessUnelevated).m_Priority;
	int32_t testUnelevatedResult = TestUtilities::ExecuteWait(context, StringInfo::Format(TEXT("cmd.exe /s /c %s test -e %d > \"%s\" 2>&1"), p4vfsExe.c_str(), testUnelevatedPriority, testUnelevatedOutputFile.c_str()), nullptr, Process::ExecuteFlags::Unelevated);
	AssertTypeLogFile(testUnelevatedOutputFile, TEXT("[AccessUnelevated] "));
	Assert(testUnelevatedResult == 0);
}

void AssertFileOperationsAccessInternal(const TestContext& context, bool isElevated)
{
	// Confirm successfull run of elevated process if expected
	Assert((TestUtilities::ExecuteWait(context, TEXT("fltmc.exe")) == 0) == isElevated);

	// Attempt to open read/write an an existing file under an elevation protected folder. In the future we may want to restrict this to isElevated only
	const String adminFilePath = FileOperations::GetExpandedEnvironmentStrings(TEXT("%ProgramFiles%\\P4VFS\\P4VFS.Notes.txt"));
	Assert(FileInfo::IsRegular(adminFilePath.c_str()));
	P4VFS_FLT_FILE_HANDLE adminFileHandle = FileOperations::OpenReparsePointFile(adminFilePath.c_str(), FILE_GENERIC_READ|FILE_GENERIC_WRITE, 0);
	Assert(adminFileHandle.fileHandle != NULL && adminFileHandle.fileHandle != INVALID_HANDLE_VALUE && adminFileHandle.fileObject != nullptr);
	Assert(SUCCEEDED(FileOperations::CloseReparsePointFile(adminFileHandle)));

	// Attempt to set a driver control message which should be elevated only (this should be a proper set and restore)
	Assert(isElevated == SUCCEEDED(FileOperations::SetDriverFlag(TEXT("SanitizeAttributes"), 1)));
	Assert(isElevated == SUCCEEDED(FileOperations::SetDriverFlag(TEXT("SanitizeAttributes"), 0)));
}

void TestFileOperationsAccessElevated(const TestContext& context)
{
	AssertFileOperationsAccessInternal(context, true);
}

void TestFileOperationsAccessUnelevated(const TestContext& context)
{
	AssertFileOperationsAccessInternal(context, false);
}
