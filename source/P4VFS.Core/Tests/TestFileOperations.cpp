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
	const String p4vfsExe = context.GetEnvironment(TEXT("P4VFS_EXE"));
	Assert(FileInfo::IsRegular(p4vfsExe.c_str()));
	const String sysInternalsFolder = context.GetEnvironment(TEXT("SYSINTERNALS_FOLDER"));
	Assert(FileInfo::IsDirectory(sysInternalsFolder.c_str()));
	const String psexecExe = StringInfo::Format(TEXT("%s\\psexec.exe"), sysInternalsFolder.c_str());
	Assert(FileInfo::IsRegular(psexecExe.c_str()));

	// Confirm successfull run of elevated process
	Assert(TestUtilities::ExecuteWait(TEXT("fltmc.exe")) == 0);
	// Confirm successfull psexec run of elevated process
	Assert(TestUtilities::ExecuteWait(StringInfo::Format(TEXT("\"%s\" -i -accepteula -nobanner fltmc.exe"), psexecExe.c_str())) == 0);
	// Confirm unsuccessfull psexec run of unelevated process
	Assert(TestUtilities::ExecuteWait(StringInfo::Format(TEXT("\"%s\" -i -l -accepteula -nobanner fltmc.exe"), psexecExe.c_str())) != 0);

	// Confirm successfull run of p4vfs test TestFileOperationsAccessElevated
	int32_t testElevatedPriority = P4VFS_FIND_TEST(TestFileOperationsAccessElevated).m_Priority;
	Assert(TestUtilities::ExecuteLogWait(StringInfo::Format(TEXT("\"%s\" -b test -e %d"), p4vfsExe.c_str(), testElevatedPriority), context, TEXT("[AccessElevated] ")) == 0);

	// Confirm successfull run of p4vfs test TestFileOperationsAccessUnelevated
	int32_t testUnelevatedPriority = P4VFS_FIND_TEST(TestFileOperationsAccessUnelevated).m_Priority;
	Assert(TestUtilities::ExecuteLogWait(StringInfo::Format(TEXT("\"%s\" -i -l -accepteula -nobanner \"%s\" -b test -e %d"), psexecExe.c_str(), p4vfsExe.c_str(), testUnelevatedPriority), context, TEXT("[AccessUnelevated] ")) == 0);
}

void AssertFileOperationsAccessInternal(const TestContext& context, bool isElevated)
{
	// Confirm successfull run of elevated process if expected
	Assert((TestUtilities::ExecuteWait(TEXT("fltmc.exe")) == 0) == isElevated);

	// Attempt to open read/write an an existing file under an elevation protected folder
	const String adminFilePath = FileOperations::GetExpandedEnvironmentStrings(TEXT("%ProgramFiles%\\P4VFS\\P4VFS.Notes.txt"));
	Assert(FileInfo::IsRegular(adminFilePath.c_str()));
	P4VFS_FLT_FILE_HANDLE adminFileHandle = FileOperations::OpenReparsePointFile(adminFilePath.c_str(), FILE_GENERIC_READ|FILE_GENERIC_WRITE, 0);
	if (isElevated)
	{
		Assert(adminFileHandle.fileHandle != NULL && adminFileHandle.fileHandle != INVALID_HANDLE_VALUE && adminFileHandle.fileObject != nullptr);
		Assert(SUCCEEDED(FileOperations::CloseReparsePointFile(adminFileHandle)));
	}
	else
	{
		Assert(adminFileHandle.fileHandle == NULL && adminFileHandle.fileObject == nullptr);
	}
}

void TestFileOperationsAccessElevated(const TestContext& context)
{
	AssertFileOperationsAccessInternal(context, true);
}

void TestFileOperationsAccessUnelevated(const TestContext& context)
{
	AssertFileOperationsAccessInternal(context, false);
}
