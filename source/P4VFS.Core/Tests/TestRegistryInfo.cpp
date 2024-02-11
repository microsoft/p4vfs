// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "TestFactory.h"
#include "DriverVersion.h"

using namespace Microsoft::P4VFS::FileCore;
using namespace Microsoft::P4VFS::TestCore;

struct TestRegistryInfo
{
	static void AssertGetKeyValue(const wchar_t* keyName, const wchar_t* valueName, const String& value)
	{
		LSTATUS status = -1;
		RegistryValue regValue = RegistryInfo::GetKeyValue(HKEY_LOCAL_MACHINE, keyName, valueName, &status);
		Assert(status == ERROR_SUCCESS && regValue.IsValid());
		String regValueString = regValue.ToString(&status);
		Assert(status == ERROR_SUCCESS && regValueString == value);
	};

	static void AssertGetKeyValue(const wchar_t* keyName, const wchar_t* valueName, const StringArray& value)
	{
		LSTATUS status = -1;
		RegistryValue regValue = RegistryInfo::GetKeyValue(HKEY_LOCAL_MACHINE, keyName, valueName, &status);
		Assert(status == ERROR_SUCCESS && regValue.IsValid());
		StringArray regValueStringArray = regValue.ToStringArray(&status);
		Assert(status == ERROR_SUCCESS && regValueStringArray == value);
	};

	static void AssertSetKeyValue(const wchar_t* keyName, const wchar_t* valueName, const String& value)
	{
		LSTATUS status = -1;
		RegistryValue regValue = RegistryValue::FromString(value);
		Assert(regValue.IsValid() && regValue.ToString() == value);
		status = RegistryInfo::SetKeyValue(HKEY_LOCAL_MACHINE, keyName, valueName, regValue);
		Assert(status == ERROR_SUCCESS);
	};

	static void AssertSetKeyValue(const wchar_t* keyName, const wchar_t* valueName, const StringArray& value)
	{
		LSTATUS status = -1;
		RegistryValue regValue = RegistryValue::FromStringArray(value);
		Assert(regValue.IsValid() && regValue.ToStringArray() == value);
		status = RegistryInfo::SetKeyValue(HKEY_LOCAL_MACHINE, keyName, valueName, regValue);
		Assert(status == ERROR_SUCCESS);
	};

	template <typename ValueType>
	static void AssertSetGetKeyValue(const wchar_t* keyName, const wchar_t* valueName, const ValueType& value)
	{
		AssertSetKeyValue(keyName, valueName, value);
		AssertGetKeyValue(keyName, valueName, value);
	}

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

	TestRegistryInfo::AssertGetKeyValue(hklmAppKeyName, TEXT("DisplayIcon"), StringInfo::Format(TEXT("%s\\P4VFS.Setup.exe"), appInstallLocation.c_str()));
	TestRegistryInfo::AssertGetKeyValue(hklmAppKeyName, TEXT("DisplayName"), TEXT("P4VFS"));
	TestRegistryInfo::AssertGetKeyValue(hklmAppKeyName, TEXT("DisplayVersion"), P4VFS_VER_VERSION_STRING);
	TestRegistryInfo::AssertGetKeyValue(hklmAppKeyName, TEXT("UninstallString"), StringInfo::Format(TEXT("\"%s\\P4VFS.Setup.exe\" uninstall"), appInstallLocation.c_str()));
	TestRegistryInfo::AssertGetKeyValue(hklmAppKeyName, TEXT("Publisher"), TEXT("Microsoft Corporation"));
}

void TestRegistryInfoKeyValue(const TestContext& context)
{
	const wchar_t* hklmTestKeyName = TEXT("SOFTWARE\\Wow6432Node\\Microsoft\\P4VFS\\Test");

	TestRegistryInfo::AssertResetTestKey(hklmTestKeyName);

	TestRegistryInfo::AssertSetGetKeyValue(hklmTestKeyName, TEXT("MultiTest"), StringArray({TEXT("Saguenay"),TEXT("Trois-Riviéres")}));
	TestRegistryInfo::AssertSetGetKeyValue(hklmTestKeyName, TEXT("MultiTest"), StringArray({TEXT("Saguenay")}));
	TestRegistryInfo::AssertSetGetKeyValue(hklmTestKeyName, TEXT("MultiTest"), StringArray());
	TestRegistryInfo::AssertSetGetKeyValue(hklmTestKeyName, TEXT("MultiTest"), StringArray({TEXT("one"),TEXT("two"),TEXT("three"),TEXT("four")}));

	TestRegistryInfo::AssertSetGetKeyValue(hklmTestKeyName, TEXT("StringTest"), String(TEXT("Sherbrooke")));
	TestRegistryInfo::AssertSetGetKeyValue(hklmTestKeyName, TEXT("StringTest"), String(TEXT("")));
	TestRegistryInfo::AssertSetGetKeyValue(hklmTestKeyName, TEXT("StringTest"), String(TEXT("Saint-Augustin-de-Desmaures,L'Ancienne-Lorette")));
	TestRegistryInfo::AssertSetGetKeyValue(hklmTestKeyName, TEXT("StringTest"), String(TEXT("one;two;three;four")));
}
