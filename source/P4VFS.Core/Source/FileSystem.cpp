// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "FileSystem.h"
#include "FileAssert.h"
#include "FileOperations.h"
#include "DriverVersion.h"
#include "DepotClient.h"
#include "DepotClientCache.h"
#include "DepotResultPrint.h"
#include "DepotOperations.h"
#include "SettingManager.h"

namespace Microsoft {
namespace P4VFS {
namespace FileSystem {
using namespace FileCore;

class DepotPrintFileStream : public FileCore::FileStream
{
public:
	DepotPrintFileStream(P4::FDepotClient* depotClient, const P4::DepotString& depotFileSpec) :
		m_DepotClient(depotClient),
		m_DepotFileSpec(depotFileSpec)
	{}

	bool CanWrite() override
	{
		return false;
	}

	bool CanRead() override
	{
		return true;
	}

	HRESULT Read(HANDLE hWriteHandle, UINT64* bytesWritten) override
	{
		P4::FDepotResultPrintHandle printResult(hWriteHandle);
		m_DepotClient->Run(P4::DepotCommand("print", P4::DepotStringArray{"-a",m_DepotFileSpec}), printResult);
		if (printResult.HasError())
		{
			m_DepotClient->Log(LogChannel::Error, StringInfo::Format("DepotPrintFileStream failed '%s' with error [%s]", m_DepotFileSpec.c_str(), printResult.GetError().c_str()));
			return E_FAIL;
		}
		return S_OK;
	}

private:
	P4::FDepotClient* m_DepotClient;
	P4::DepotString m_DepotFileSpec;
};

HRESULT 
MakeFileResident(
	P4::FDepotClient& depotClient, 
	const wchar_t* filePath,
	const P4VFS_REPARSE_DATA_2* populateInfo
	)
{
	if (StringInfo::IsNullOrEmpty(filePath))
	{
		return E_INVALIDARG;
	}

	if (populateInfo == nullptr)
	{
		return E_POINTER;
	}

	const String fileSpec = StringInfo::Format(L"%s#=%u", populateInfo->depotPath.c_str(), uint32_t(populateInfo->fileRevision));
	const FilePopulateMethod::Enum populateMethod = FilePopulateMethod::FromString(StringInfo::ToAnsi(SettingManager::StaticInstance().PopulateMethod.GetValue()));
	
	switch (populateMethod)
	{
		case P4VFS_POPULATE_METHOD_COPY:
		case P4VFS_POPULATE_METHOD_MOVE:
		{
			const char* populateMethodName = populateMethod == P4VFS_POPULATE_METHOD_MOVE ? "MOVE" : "COPY";
			depotClient.Log(LogChannel::Verbose, StringInfo::Format("MakeFileResident '%s' by %s", CSTR_WTOA(fileSpec), populateMethodName));

			String tempPrintFolder = populateMethod == P4VFS_POPULATE_METHOD_MOVE ? FileInfo::FolderPath(filePath) : String();
			AutoTempFile tempPrintFile(FileInfo::CreateTempFile(tempPrintFolder.c_str()).c_str());
			
			if (FileInfo::IsRegular(tempPrintFile.GetFilePath().c_str()) == false)
			{
				depotClient.Log(LogChannel::Error, StringInfo::Format("MakeFileResident '%s' failed to create tempfile for PopulateFile by %s", CSTR_WTOA(fileSpec), populateMethodName));
				return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
			}

			P4::DepotResult print = depotClient.Run("print", P4::DepotStringArray{"-a","-o", StringInfo::ToAnsi(tempPrintFile.GetFilePath()), StringInfo::ToAnsi(fileSpec)});
			if (print.get() == nullptr || print->HasError())
			{
				depotClient.Log(LogChannel::Error, StringInfo::Format("MakeFileResident '%s' failed print tempfile '%s' for PopulateFile by %s", CSTR_WTOA(fileSpec), CSTR_WTOA(tempPrintFile.GetFilePath()), populateMethodName));
				return HRESULT_FROM_WIN32(ERROR_INVALID_PRINTER_COMMAND);
			}

			HRESULT hr = FileOperations::PopulateFile(filePath, tempPrintFile.GetFilePath().c_str(), BYTE(populateMethod));
			if (FAILED(hr))
			{
				depotClient.Log(LogChannel::Error, StringInfo::Format("MakeFileResident '%s' failed PopulateFile by %s", CSTR_WTOA(fileSpec), populateMethodName));
				return hr;
			}
			break;
		}
		case P4VFS_POPULATE_METHOD_STREAM:
		default:
		{
			depotClient.Log(LogChannel::Verbose, StringInfo::Format("MakeFileResident '%s' by STREAM", CSTR_WTOA(fileSpec)));
			DepotPrintFileStream depotStream(&depotClient, StringInfo::WtoA(fileSpec));

			HRESULT hr = FileOperations::PopulateFile(filePath, &depotStream);
			if (FAILED(hr))
			{
				depotClient.Log(LogChannel::Error, StringInfo::Format("MakeFileResident '%s' failed PopulateFile by STREAM", CSTR_WTOA(fileSpec)));
				return hr;
			}
			break;
		}
	}
	return S_OK;
}

HRESULT 
ExecuteFileResidencyPolicy(
	P4::FDepotClient& depotClient, 
	const wchar_t* filePath,
	const P4VFS_REPARSE_DATA_2* populateInfo
	)
{
	if (populateInfo == nullptr)
	{
		return E_POINTER;
	}

	switch (populateInfo->residencyPolicy)
	{
		case P4VFS_RESIDENCY_POLICY_RESIDENT:
		{
			return MakeFileResident(depotClient, filePath, populateInfo);
		}
		case P4VFS_RESIDENCY_POLICY_REMOVE_FILE:
		{
			depotClient.Log(LogChannel::Info, StringInfo::Format("ExecuteFileResidencyPolicy RemoveFile Start '%s'", CSTR_WTOA(filePath)));
			FileCore::FileInfo::Delete(filePath);
			depotClient.Log(LogChannel::Info, StringInfo::Format("ExecuteFileResidencyPolicy RemoveFile End '%s'", CSTR_WTOA(filePath)));
			return S_OK;
		}
	}
	return E_FAIL;
}

HRESULT 
ResolveFileResidency(
	FileContext& context,
	const wchar_t* filePath,
	BYTE* fileResidencyPolicy
	)
{
	BYTE fileResidencyPolicyTmp = 0;
	if (fileResidencyPolicy == nullptr)
	{
		fileResidencyPolicy = &fileResidencyPolicyTmp;
	}

	if (StringInfo::IsNullOrEmpty(filePath))
	{
		if (context.m_LogDevice)
		{
			context.m_LogDevice->Info(L"ResolveFile skipped empty filePath");
		}
		*fileResidencyPolicy = P4VFS_RESIDENCY_POLICY_UNDEFINED;
		return E_FAIL;
	}

	FileCore::GAllocPtr<P4VFS_REPARSE_DATA_2> populateInfo;
	HRESULT hr = FileOperations::GetFileReparseData(filePath, populateInfo);
	if (FAILED(hr) || populateInfo.get() == nullptr)
	{
		if (context.m_LogDevice)
		{
			context.m_LogDevice->Info(StringInfo::Format(L"ResolveFile GetFileReparseData skipped '%s' with error [%s]", filePath, StringInfo::ToString(hr).c_str()));
		}
		*fileResidencyPolicy = P4VFS_RESIDENCY_POLICY_RESIDENT;
		return hr == HRESULT_FROM_WIN32(ERROR_OPEN_FAILED) ? hr : S_OK;
	}

	if (populateInfo->residencyPolicy == P4VFS_RESIDENCY_POLICY_UNDEFINED || StringInfo::IsNullOrEmpty(populateInfo->depotPath.c_str()))
	{
		if (context.m_LogDevice)
		{
			context.m_LogDevice->Error(StringInfo::Format(L"ResolveFile GetFileReparseData failed '%s'", filePath));
		}
		*fileResidencyPolicy = P4VFS_RESIDENCY_POLICY_UNDEFINED;
		return hr;
	}

	// Specify a DepotConfig that will be used as a key for similar connections
	P4::DepotConfig config;
	config.m_Port = P4::DepotOperations::ResolveDepotServerName(StringInfo::ToAnsi(populateInfo->depotServer.c_str()));
	config.m_User = StringInfo::ToAnsi(populateInfo->depotUser.c_str());
	config.m_Client = StringInfo::ToAnsi(populateInfo->depotClient.c_str());
	config.m_Directory = StringInfo::ToAnsi(FileInfo::FolderPath(filePath));
	const P4::DepotConfig& configKey = config;

	// Make sure we go through all existing clients and a new one before giving up
	Assert(context.m_DepotClientCache != nullptr);
	const size_t maxRetryCount = context.m_DepotClientCache->GetFreeCount() + 1; 
	for (size_t retryIndex = 0; retryIndex < maxRetryCount; ++retryIndex)
	{
		P4::DepotClient client = context.m_DepotClientCache->Alloc(configKey, context);
		if (client.get() == nullptr)
		{
			hr = HRESULT_FROM_WIN32(ERROR_BROKEN_PIPE);
			if (context.m_LogDevice)
			{
				context.m_LogDevice->Error(StringInfo::Format(L"ResolveFile failed to connect '%s' with error [%s]", filePath, CSTR_ATOW(configKey.ToConnectionString())));
			}
			// This DepotClient failed, don't return it.
			continue;
		}

		hr = ExecuteFileResidencyPolicy(*client, filePath, populateInfo.get());
		if (FAILED(hr))
		{
			if (context.m_LogDevice)
			{
				context.m_LogDevice->Error(StringInfo::Format(L"ResolveFile ExecuteFileResidencyPolicy failed '%s' with error [%s]", filePath, StringInfo::ToString(hr).c_str()));
			}
			// This DepotClient failed, don't return it.
			continue;
		}

		// This DepotClient was successfull, return it to the cache using the exact key that it was allocated with.
		context.m_DepotClientCache->Free(configKey, client);
		*fileResidencyPolicy = populateInfo->residencyPolicy;

		if (context.m_LogDevice)
		{
			context.m_LogDevice->Info(StringInfo::Format(L"%s#%u - hydrated as %s [%s,%s,%s] process [%d.%d]", populateInfo->depotPath.c_str(), uint32_t(populateInfo->fileRevision), filePath, CSTR_ATOW(configKey.m_Port), populateInfo->depotUser.c_str(), populateInfo->depotClient.c_str(), context.ProcessId(), context.ThreadId()));
		}
		return S_OK;
	}

	*fileResidencyPolicy = P4VFS_RESIDENCY_POLICY_UNDEFINED;
	return hr;
}

bool
IsExcludedProcessId(
	ULONG processId
	)
{
	String excludedNames = SettingManager::StaticInstance().ExcludedProcessNames.GetValue();
	if (excludedNames.empty() == false)
	{
		String processName = FileInfo::FileName(Process::GetProcessNameById(processId).c_str());
		if (processName.empty() == false && StringInfo::ContainsToken(TEXT(';'), excludedNames.c_str(), processName.c_str(), StringInfo::SearchCase::Insensitive))
		{
			return true;
		}		
	}
	return false;
}

FileCore::String
GetModuleVersion(
	)
{
	return P4VFS_VER_VERSION_STRING;
}

FileCore::String
GetDriverVersion(
	)
{
	USHORT driverMajor(0), driverMinor(0), driverBuild(0), driverRevision(0);
	if (SUCCEEDED(FileOperations::GetDriverVersion(driverMajor, driverMinor, driverBuild, driverRevision)) == false)
		return String();

	return StringInfo::Format(L"%u.%u.%u.%u", driverMajor, driverMinor, driverBuild, driverRevision);
}

HRESULT 
SetupInstallHinfSection(
	const WCHAR* sectionName,
	const WCHAR* filePath
	)
{
	if (StringInfo::IsNullOrEmpty(sectionName))
	{
		return HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
	}

	if (FileInfo::IsRegular(filePath) == false)
	{
		return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	}

	HRESULT hr = E_FAIL;
	HMODULE hSetupApiModule = LoadLibraryA("setupapi.dll");
	if (hSetupApiModule != NULL)
	{
		typedef BOOL (WINAPI *FSetupSetNonInteractiveMode)(
			BOOL NonInteractiveFlag
			);

		typedef VOID (WINAPI *FInstallHinfSectionW)(
			HWND Window,
			HINSTANCE ModuleHandle,
			PCWSTR CommandLine,
			INT ShowCommand
			);

		FSetupSetNonInteractiveMode pSetupSetNonInteractiveMode = (FSetupSetNonInteractiveMode)GetProcAddress(hSetupApiModule, "SetupSetNonInteractiveMode");
		FInstallHinfSectionW pInstallHinfSectionW = (FInstallHinfSectionW)GetProcAddress(hSetupApiModule, "InstallHinfSectionW");
		
		if (pSetupSetNonInteractiveMode && pInstallHinfSectionW)
		{
			// Set the non-interactive flag in setup API
			pSetupSetNonInteractiveMode(TRUE);

			// High level routine to perform right-click install action on INFs
			const String cmd = StringInfo::Format(TEXT("%s 128 %s"), sectionName, filePath);
			pInstallHinfSectionW(NULL, NULL, cmd.c_str(), 0);
			hr = S_OK;
		}

		FreeLibrary(hSetupApiModule);
	}
	return hr;
}

AString 
FilePopulateMethod::ToString(
	FilePopulateMethod::Enum value
	)
{
	P4VFS_ENUM_TO_STRING_RETURN(value, FilePopulateMethod, Copy);
	P4VFS_ENUM_TO_STRING_RETURN(value, FilePopulateMethod, Move);
	P4VFS_ENUM_TO_STRING_RETURN(value, FilePopulateMethod, Stream);
	return AString();
}

FilePopulateMethod::Enum 
FilePopulateMethod::FromString(
	const AString& value
	)
{
	P4VFS_STRING_TO_ENUM_RETURN(value, FilePopulateMethod, Copy);
	P4VFS_STRING_TO_ENUM_RETURN(value, FilePopulateMethod, Move);
	P4VFS_STRING_TO_ENUM_RETURN(value, FilePopulateMethod, Stream);
	return FilePopulateMethod::Stream;
}

}}}
