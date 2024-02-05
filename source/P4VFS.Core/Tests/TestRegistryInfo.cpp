// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "TestFactory.h"
#include "DriverVersion.h"

using namespace Microsoft::P4VFS::FileCore;
using namespace Microsoft::P4VFS::TestCore;

struct TestRegistryInfo
{
	static void AssertGetKeyValueString(const wchar_t* keyName, const wchar_t* valueName, const wchar_t* value)
	{
		LSTATUS status = -1;
		RegistryValue regValue = RegistryInfo::GetKeyValue(HKEY_LOCAL_MACHINE, keyName, valueName, &status);
		Assert(status == ERROR_SUCCESS);
		String regValueString = regValue.ToString(&status);
		Assert(status == ERROR_SUCCESS && StringInfo::Strcmp(regValueString.c_str(), value) == 0);
	};

	static void AssertGetKeyValueStringArray(const wchar_t* keyName, const wchar_t* valueName, const StringArray& value)
	{
		LSTATUS status = -1;
		RegistryValue regValue = RegistryInfo::GetKeyValue(HKEY_LOCAL_MACHINE, keyName, valueName, &status);
		Assert(status == ERROR_SUCCESS);
		StringArray regValueStringArray = regValue.ToStringArray(&status);
		Assert(status == ERROR_SUCCESS && regValueStringArray == value);
	};

	static void AssertResetTestKey(const wchar_t* keyName)
	{
		HKEY hRootTestKey = NULL;
		LSTATUS status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, FileInfo::FolderPath(keyName).c_str(), 0, KEY_ALL_ACCESS, &hRootTestKey);
		Assert(status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND);
		if (status == ERROR_SUCCESS)
		{
			status = RegDeleteTreeW(hRootTestKey, FileInfo::FileName(keyName).c_str());
			Assert(status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND);
			RegCloseKey(hRootTestKey);
		}
	};
};

void TestRegistryInfoInstallKeys(const TestContext& context)
{
	const wchar_t* hklmAppKeyName = TEXT("SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\P4VFS");
	
	const String appInstallLocation = RegistryInfo::GetKeyValue(HKEY_LOCAL_MACHINE, hklmAppKeyName, TEXT("InstallLocation")).ToString();
	Assert(FileInfo::IsDirectory(appInstallLocation.c_str()));

	TestRegistryInfo::AssertGetKeyValueString(hklmAppKeyName, TEXT("DisplayIcon"), StringInfo::Format(TEXT("%s\\P4VFS.Setup.exe"), appInstallLocation.c_str()).c_str());
	TestRegistryInfo::AssertGetKeyValueString(hklmAppKeyName, TEXT("DisplayName"), TEXT("P4VFS"));
	TestRegistryInfo::AssertGetKeyValueString(hklmAppKeyName, TEXT("DisplayVersion"), P4VFS_VER_VERSION_STRING);
	TestRegistryInfo::AssertGetKeyValueString(hklmAppKeyName, TEXT("UninstallString"), StringInfo::Format(TEXT("\"%s\\P4VFS.Setup.exe\" uninstall"), appInstallLocation.c_str()).c_str());
	TestRegistryInfo::AssertGetKeyValueString(hklmAppKeyName, TEXT("Publisher"), TEXT("Microsoft Corporation"));
}

void TestRegistryInfoKeyValue(const TestContext& context)
{
	const wchar_t* hklmTestKeyName = TEXT("SOFTWARE\\Wow6432Node\\Microsoft\\P4VFS\\Test");

	TestRegistryInfo::AssertResetTestKey(hklmTestKeyName);

	const String cmdMV0 = StringInfo::Format(TEXT("reg add \"HKLM\\%s\" /v MultiTest0 /t REG_MULTI_SZ /d \"Saguenay\\0Trois-Riviéres\""), hklmTestKeyName);
	Assert(Process::Execute(cmdMV0.c_str(), nullptr, Process::ExecuteFlags::WaitForExit|Process::ExecuteFlags::HideWindow).m_ExitCode == 0);
	TestRegistryInfo::AssertGetKeyValueStringArray(hklmTestKeyName, TEXT("MultiTest0"), StringArray({TEXT("Saguenay"),TEXT("Trois-Riviéres")}));
}
