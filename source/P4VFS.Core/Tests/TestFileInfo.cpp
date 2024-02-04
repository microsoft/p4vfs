// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "TestFactory.h"

using namespace Microsoft::P4VFS::FileCore;
using namespace Microsoft::P4VFS::TestCore;

void TestFileInfoPath(const TestContext& context)
{
	Assert(FileInfo::FolderPath(TEXT("C:\\foo\\bar")) == TEXT("C:\\foo"));
	Assert(FileInfo::FolderPath(TEXT("C:\\foo\\\\bar.xml")) == TEXT("C:\\foo"));
	Assert(FileInfo::FolderPath(TEXT("C:\\bar.xml")) == TEXT("C:"));
	Assert(FileInfo::FolderPath(TEXT("foo///bar\\bar.xml")) == TEXT("foo///bar"));
	Assert(FileInfo::FolderPath(TEXT("foo.xml")) == TEXT(""));
	Assert(FileInfo::FolderPath(nullptr) == TEXT(""));

	Assert(FileInfo::FileTitle(TEXT("C:\\foo\\bar.txt")) == TEXT("bar"));
	Assert(FileInfo::FileTitle(TEXT("bar.txt")) == TEXT("bar"));
	Assert(FileInfo::FileTitle(TEXT("bar")) == TEXT("bar"));
	Assert(FileInfo::FileTitle(TEXT("bar.txt")) == TEXT("bar"));

	Assert(FileInfo::FileName(nullptr) == TEXT(""));
	Assert(FileInfo::FileName(TEXT("bar.txt")) == TEXT("bar.txt"));
	Assert(FileInfo::FileName(TEXT("c:/foo/star/bar.txt")) == TEXT("bar.txt"));
	Assert(FileInfo::FileName(TEXT("c:/foo/star/bar")) == TEXT("bar"));
	Assert(FileInfo::FileName(TEXT("c:/foo/star/")) == TEXT(""));

	Assert(FileInfo::FileExtension(TEXT("")) == TEXT(""));
	Assert(FileInfo::FileExtension(TEXT("bar.txt")) == TEXT(".txt"));
	Assert(FileInfo::FileExtension(TEXT("c:/foo/star/bar.txt")) == TEXT(".txt"));
	Assert(FileInfo::FileExtension(TEXT("c:/foo/star/bar")) == TEXT(""));
	Assert(FileInfo::FileExtension(TEXT("c:/foo/star/.txt")) == TEXT(".txt"));

	Assert(FileInfo::FullPath(TEXT("c:/foo/star/bar.txt")) == TEXT("c:\\foo\\star\\bar.txt"));
	Assert(FileInfo::FullPath(TEXT("c:/foo/star")) == TEXT("c:\\foo\\star"));
	Assert(FileInfo::FullPath(TEXT("c:/foo/star/")) == TEXT("c:\\foo\\star\\"));
	Assert(FileInfo::FullPath(TEXT("\\\\foo/star/bar.txt")) == TEXT("\\\\foo\\star\\bar.txt"));

	Assert(FileInfo::FullFolderPath(TEXT("c:/foo/star/bar.txt")) == TEXT("c:\\foo\\star"));
	Assert(FileInfo::FullFolderPath(TEXT("c:/foo/star")) == TEXT("c:\\foo"));
	Assert(FileInfo::FullFolderPath(TEXT("c:/foo/star/")) == TEXT("c:\\foo"));
	Assert(FileInfo::FullFolderPath(TEXT("\\\\foo/star/bar.txt")) == TEXT("\\\\foo\\star"));

	Assert(StringInfo::Stricmp(FileInfo::RootPath(TEXT("c:/foo/star/bar.txt")).c_str(), TEXT("c:")) == 0);
	Assert(StringInfo::Stricmp(FileInfo::RootPath(TEXT("c:\\foo\\star/bar.txt")).c_str(), TEXT("c:")) == 0);
	Assert(StringInfo::Stricmp(FileInfo::RootPath(TEXT("c:\\")).c_str(), TEXT("c:")) == 0);
	Assert(StringInfo::Stricmp(FileInfo::RootPath(TEXT("c:")).c_str(), TEXT("c:")) == 0);
	Assert(StringInfo::Stricmp(FileInfo::RootPath(TEXT("\\\\foo/star/bar.txt")).c_str(), TEXT("")) == 0);
}

void TestFileInfoDirectory(const TestContext& context)
{
	TestUtilities::WorkspaceReset(context);

	const String rootFolder = context.GetEnvironment(TEXT("P4ROOT"));
	Assert(rootFolder.size() > 0);
	Assert(StringInfo::Split(rootFolder.c_str(), TEXT("\\/"), StringInfo::SplitFlags::RemoveEmptyEntries).size() >= 2);

	const String localRootFolder = StringInfo::Format(TEXT("%s\\TestFileInfo"), rootFolder.c_str());
	Assert(FileInfo::IsDirectory(localRootFolder.c_str()) == false);
	Assert(FileInfo::CreateDirectory((localRootFolder + TEXT("//Test\\UnitTest\\Test0")).c_str()));
	Assert(FileInfo::IsDirectory(localRootFolder.c_str()));

	Assert(FileInfo::CreateDirectory((localRootFolder + TEXT("//Test\\UnitTest//Test1")).c_str()));
	Assert(FileInfo::CreateDirectory((localRootFolder + TEXT("//Test\\UnitTest//Test1\\Test11")).c_str()));
	const String subFile0 = localRootFolder + TEXT("//Test\\UnitTest//Test1\\Test0.txt");
	const String subFile1 = localRootFolder + TEXT("//Test\\UnitTest//Test1\\Test1.txt");
	Assert(FileInfo::CreateWritableFile(subFile0.c_str()));
	Assert(FileInfo::IsReadOnly(subFile1.c_str()) == false);
	Assert(FileInfo::CreateWritableFile(subFile1.c_str()));
	Assert(FileInfo::SetReadOnly(subFile1.c_str(), true));
	Assert(FileInfo::IsReadOnly(subFile1.c_str()) == true);
	Assert(FileInfo::IsDirectoryEmpty((localRootFolder + TEXT("//Test\\UnitTest//Test1\\Test11")).c_str()));
	Assert(FileInfo::IsDirectoryEmpty((localRootFolder + TEXT("//Test\\UnitTest//Test1")).c_str()) == false);
	Assert(FileInfo::IsDirectoryEmpty((localRootFolder + TEXT("//Test\\UnitTest")).c_str()) == false);
	{ 
		StringArray files; 
		FileInfo::FindFiles(files, localRootFolder.c_str(), nullptr, FileInfo::Find::kFiles|FileInfo::Find::kRecursive); 
		Assert(files.size() == 2);
		Assert(StringInfo::Stricmp(files[0].c_str(), FileInfo::FullPath(subFile0.c_str()).c_str()) == 0);
		Assert(StringInfo::Stricmp(files[1].c_str(), FileInfo::FullPath(subFile1.c_str()).c_str()) == 0);
	}

	Assert(FileInfo::DeleteEmptyDirectories(localRootFolder.c_str()));
	Assert(FileInfo::IsDirectory(localRootFolder.c_str()));
	Assert(FileInfo::DeleteDirectoryRecursively(localRootFolder.c_str()));
	Assert(FileInfo::IsDirectory(localRootFolder.c_str()) == false);

	Assert(FileInfo::CreateDirectory((localRootFolder + TEXT("//Test\\UnitTest//Test1")).c_str()));
	Assert(FileInfo::CreateDirectory((localRootFolder + TEXT("//Test\\UnitTest//Test1\\Test11")).c_str()));
	Assert(FileInfo::DeleteEmptyDirectories(localRootFolder.c_str()));
	Assert(FileInfo::IsDirectory((localRootFolder + TEXT("//Test\\UnitTest//Test1\\Test11")).c_str()));
	Assert(FileInfo::DeleteEmptyDirectories((localRootFolder + TEXT("//Test\\UnitTest//Test1\\Test11")).c_str()));
	Assert(FileInfo::IsDirectory(localRootFolder.c_str()) == false);

	const wchar_t* rootFolderSubPath = StringInfo::Strchr(rootFolder.c_str(), TEXT('\\'));
	Assert(rootFolderSubPath != nullptr);
	const String uncRootFolder = StringInfo::Format(TEXT("\\\\localhost\\%c$%s"), *rootFolder.c_str(), rootFolderSubPath);
	Assert(FileInfo::CreateDirectory(rootFolder.c_str()));
	AssertMsg(FileInfo::IsDirectory(uncRootFolder.c_str()), TEXT("Expecting test development machine to have default administrative shares enabled"));
	const String uncLocalRootFolder = StringInfo::Format(TEXT("%s\\TestFileInfo"), uncRootFolder.c_str());
	Assert(FileInfo::IsDirectory(uncLocalRootFolder.c_str()) == false);
	Assert(FileInfo::CreateDirectory((uncLocalRootFolder + TEXT("//Test\\UnitTest\\Test0")).c_str()));
	Assert(FileInfo::IsDirectory(uncLocalRootFolder.c_str()));
	const String uncSubFile0 = uncLocalRootFolder + TEXT("//Test\\UnitTest//Test0\\Test0.txt");
	Assert(FileInfo::CreateWritableFile(uncSubFile0.c_str()));
	{ 
		StringArray files; 
		FileInfo::FindFiles(files, uncLocalRootFolder.c_str(), nullptr, FileInfo::Find::kFiles|FileInfo::Find::kRecursive); 
		Assert(files.size() == 1);
		Assert(StringInfo::Stricmp(files[0].c_str(), FileInfo::FullPath(uncSubFile0.c_str()).c_str()) == 0);
	}
	Assert(FileInfo::DeleteDirectoryRecursively(uncLocalRootFolder.c_str()));
	Assert(FileInfo::IsDirectory(uncLocalRootFolder.c_str()) == false);
	Assert(FileInfo::IsDirectory(uncRootFolder.c_str()));
}

void TestFileInfoLongPathSupport(const TestContext& context)
{
	TestUtilities::WorkspaceReset(context);
	AssertMsg(RegistryInfo::GetKeyValue(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Control\\FileSystem"), TEXT("LongPathsEnabled")).ToString() == TEXT("1"), TEXT("Expecting long path support to be enabled on this machine"));

	Assert(FileInfo::IsExtendedPath(FileInfo::ExtendedPathPrefix));
	Assert(FileInfo::IsExtendedPath(StringInfo::Format(TEXT("%sC:\\Temp"), FileInfo::ExtendedPathPrefix).c_str()));
	Assert(FileInfo::IsExtendedPath(StringInfo::Format(TEXT("C:\\Temp")).c_str()) == false);
	Assert(FileInfo::IsExtendedPath(TEXT("")) == false);
	Assert(FileInfo::IsExtendedPath(nullptr) == false);

	Assert(FileInfo::ExtendedPath(FileInfo::ExtendedPathPrefix) == String(FileInfo::ExtendedPathPrefix));
	Assert(FileInfo::ExtendedPath(TEXT("")) == String());
	Assert(FileInfo::ExtendedPath(nullptr) == String());
	Assert(FileInfo::ExtendedPath(StringInfo::Format(TEXT("%sC:\\Temp"), FileInfo::ExtendedPathPrefix).c_str()) == StringInfo::Format(TEXT("%sC:\\Temp"), FileInfo::ExtendedPathPrefix));
	Assert(FileInfo::ExtendedPath(TEXT("C:\\Temp")) == StringInfo::Format(TEXT("%sC:\\Temp"), FileInfo::ExtendedPathPrefix));

	Assert(FileInfo::UnextendedPath(FileInfo::ExtendedPathPrefix) == String());
	Assert(FileInfo::UnextendedPath(TEXT("")) == String());
	Assert(FileInfo::UnextendedPath(nullptr) == String());
	Assert(FileInfo::UnextendedPath(StringInfo::Format(TEXT("%sC:\\Temp"), FileInfo::ExtendedPathPrefix).c_str()) == String(TEXT("C:\\Temp")));
	Assert(FileInfo::UnextendedPath(TEXT("C:\\Temp")) == String(TEXT("C:\\Temp")));

	const String rootFolder = FileInfo::FullPath(context.GetEnvironment(TEXT("P4ROOT")).c_str());
	Assert(rootFolder.size() > 0);
	Assert(StringInfo::Split(rootFolder.c_str(), TEXT("\\/"), StringInfo::SplitFlags::RemoveEmptyEntries).size() >= 2);

	const String localRootFolder = StringInfo::Format(TEXT("%s\\TestFileInfoLongPathSupport"), rootFolder.c_str());
	Assert(FileInfo::DeleteDirectoryRecursively(localRootFolder.c_str()));
	Assert(FileInfo::CreateDirectory(localRootFolder.c_str()));
	
	const String longFolderPath = StringInfo::Format(TEXT("%s\\Microsoft Visual Studio\\2019\\Enterprise\\VSSDK\\VisualStudioIntegration\\Common\\Source\\CPP\\VSL\\VSLArchitecture_files\\Microsoft Azure Tools\\Visual Studio 16.0\\2.9\\RemoteDebuggerConnector\\Connector\\MSBuild\\Microsoft\\Microsoft.NET.Build.Extensions\\net471"), localRootFolder.c_str());
	Assert(longFolderPath.length() > MAX_PATH);
	const String longFolderFullPath = FileInfo::FullPath(longFolderPath.c_str());
	Assert(FileInfo::IsExtendedPath(longFolderFullPath.c_str()));
	Assert(StringInfo::Stricmp(longFolderFullPath.c_str(), StringInfo::Format(TEXT("%s%s"), FileInfo::ExtendedPathPrefix, longFolderPath.c_str()).c_str()) == 0);
	Assert(longFolderFullPath == FileInfo::ExtendedPath(longFolderPath.c_str()));
	Assert(longFolderFullPath == FileInfo::ExtendedPath(FileInfo::ExtendedPath(longFolderPath.c_str()).c_str()));
	Assert(longFolderPath == FileInfo::UnextendedPath(longFolderFullPath.c_str()));
	Assert(longFolderPath == FileInfo::UnextendedPath(FileInfo::UnextendedPath(longFolderFullPath.c_str()).c_str()));
	Assert(longFolderFullPath == FileInfo::FullPath(longFolderFullPath.c_str()));
	
	const String longFilePath = StringInfo::Format(TEXT("%s\\Microsoft.VisualStudio.WindowsAzure.RemoteDebugger.Connector.Core.dll"), longFolderPath.c_str()); 
	Assert(FileInfo::IsExtendedPath(longFilePath.c_str()) == false);
	const String longFileFullPath = FileInfo::FullPath(longFilePath.c_str());
	Assert(FileInfo::IsExtendedPath(longFileFullPath.c_str()));

	Assert(FileInfo::IsDirectory(longFolderPath.c_str()) == false);
	Assert(FileInfo::IsDirectory(longFolderFullPath.c_str()) == false);
	Assert(FileInfo::CreateDirectory(longFolderPath.c_str()));
	Assert(FileInfo::IsDirectory(longFolderFullPath.c_str()) == true);
	
	Assert(FileInfo::CreateWritableFile(longFileFullPath.c_str()));
	Assert(FileInfo::IsRegular(longFileFullPath.c_str()));
	Assert(FileInfo::Exists(longFileFullPath.c_str()));
	Assert(FileInfo::Delete(longFileFullPath.c_str()));
	Assert(FileInfo::Exists(longFileFullPath.c_str()) == false);

	Assert(FileInfo::CreateWritableFile(longFileFullPath.c_str()));
	Assert(FileInfo::IsRegular(longFileFullPath.c_str()));
	Assert(FileInfo::SetReadOnly(longFileFullPath.c_str(), true));
	Assert(FileInfo::IsReadOnly(longFileFullPath.c_str()));
	Assert(FileInfo::SetReadOnly(longFileFullPath.c_str(), false));
	Assert(FileInfo::IsReadOnly(longFileFullPath.c_str()) == false);
	Assert(FileInfo::Delete(longFileFullPath.c_str()));
	Assert(FileInfo::Exists(longFileFullPath.c_str()) == false);
}
