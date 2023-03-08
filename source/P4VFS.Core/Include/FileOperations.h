// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "FileCore.h"
#include "FileContext.h"
#include "DriverData.h"
#pragma managed(push, off)

namespace Microsoft {
namespace P4VFS {
namespace FileOperations {

	bool 
	SleepAndVerifyFileExists(
		const WCHAR* filePath, 
		int32_t totalMilisecondsToWait = -1
		);

	HRESULT 
	GetReparseDataSize( 
		_In_ HANDLE handle,
		_Out_ DWORD* dwReparseBufferSize
		);

	HRESULT
	GetReparsePointOfHandle(
		HANDLE handle,
		REPARSE_GUID_DATA_BUFFER* pReparseHeader,
		DWORD dwReparsePointSize
		);

	HRESULT
	RemoveReparsePointOnHandle(
		HANDLE handle
		);

	HRESULT
	SetReparsePointOnHandle(
		HANDLE handle,
		USHORT majorVersion,
		USHORT minorVersion,
		USHORT buildversion,
		BYTE residencyPolicy,
		BYTE populatePolicy,
		ULONG fileRevision,
		INT64 fileSize,
		const WCHAR* depotPath,
		const WCHAR* depotServer,
		const WCHAR* depotClient,
		const WCHAR* depotUser
		);

	HRESULT
	SetSparseFileSizeOnHandle(
		HANDLE handle,
		INT64 fileSize
		);

	HRESULT
	RemoveSparseFileSizeOnHandle(
		HANDLE handle
		);

	HRESULT
	GetFileReparseData(
		const REPARSE_GUID_DATA_BUFFER* pReparsePoint,
		FileCore::GAllocPtr<P4VFS_REPARSE_DATA_2>& pReparseData
		);

	HRESULT
	GetFileReparseData(
		HANDLE hFile,
		FileCore::GAllocPtr<P4VFS_REPARSE_DATA_2>& pReparseData
		);

	P4VFS_CORE_API HRESULT 
	GetFileReparseData(
		const WCHAR* fileName,
		FileCore::GAllocPtr<P4VFS_REPARSE_DATA_2>& pReparseData
		);

	P4VFS_CORE_API HRESULT 
	InstallReparsePointOnFile(
		USHORT majorVersion,
		USHORT minorVersion,
		USHORT buildversion,
		const WCHAR* fileName,
		BYTE residencyPolicy,
		ULONG fileRevision,
		INT64 fileSize,
		DWORD fileAttributes,
		const WCHAR* depotPath,
		const WCHAR* depotServer,
		const WCHAR* depotClient,
		const WCHAR* depotUser
		);

	P4VFS_CORE_API HRESULT
	CreateSymlinkFile(
		const WCHAR* symlinkFileName,
		const WCHAR* targetFileName
		);

	P4VFS_CORE_API HRESULT
	RemoveReparsePoint(
		const WCHAR* fileName
		);

	P4VFS_CORE_API P4VFS_FLT_FILE_HANDLE
	OpenReparsePointFile(
		const WCHAR* fileName,
		DWORD desiredAccess,
		DWORD shareMode
		);

	P4VFS_CORE_API HRESULT
	CloseReparsePointFile(
		const P4VFS_FLT_FILE_HANDLE& handle
		);

	P4VFS_CORE_API HRESULT 
	PopulateFile(
		const WCHAR* dstFileName,
		const WCHAR* srcFileName,
		BYTE populateMethod
		);

	P4VFS_CORE_API HRESULT 
	PopulateFile(
		const WCHAR* dstFileName,
		FileCore::FileStream* srcFileStream
		);

	P4VFS_CORE_API HRESULT
	ImpersonateFileAppend(
		const WCHAR* fileName,
		const WCHAR* text,
		const FileCore::UserContext* context = nullptr
		);

	P4VFS_CORE_API HRESULT
	FileAppend(
		const WCHAR* fileName,
		const WCHAR* text
		);

	P4VFS_CORE_API HRESULT
	ImpersonateLoggedOnUser(
		const FileCore::UserContext* context = nullptr
		);

	P4VFS_CORE_API FileCore::String
	GetImpersonatedUserName(
		const FileCore::UserContext* context = nullptr,
		HRESULT* pHR = nullptr
		);

	P4VFS_CORE_API FileCore::String
	GetUserName(
		HRESULT* pHR = nullptr
		);

	P4VFS_CORE_API FileCore::String
	GetSessionUserName(
		DWORD sessionId,
		HRESULT* pHR = nullptr
		);

	P4VFS_CORE_API HRESULT
	GetImpersonatedEnvironmentStrings(
		const WCHAR* srcText,
		WCHAR* dstText,
		DWORD dstTextSize,
		const FileCore::UserContext* context
		);

	P4VFS_CORE_API FileCore::String
	GetImpersonatedEnvironmentStrings(
		const WCHAR* srcText,
		const FileCore::UserContext* context = nullptr,
		HRESULT* pHR = nullptr
		);

	P4VFS_CORE_API bool
	IsSystemUserContext(
		const FileCore::UserContext* context,
		HRESULT* pHR = nullptr
		);

	P4VFS_CORE_API bool
	IsCurrentProcessUserContext(
		const FileCore::UserContext* context
		);

	P4VFS_CORE_API HRESULT
	GetExpandedEnvironmentStrings(
		const WCHAR* srcText,
		WCHAR* dstText,
		DWORD dstTextSize
		);

	P4VFS_CORE_API FileCore::String
	GetExpandedEnvironmentStrings(
		const WCHAR* srcText,
		HRESULT* pHR = nullptr
		);

	P4VFS_CORE_API HANDLE
	GetLoggedOnUserToken(
		const FileCore::UserContext* context
		);

	P4VFS_CORE_API HANDLE
	GetPreferredLoggedOnUserToken(
		);

	P4VFS_CORE_API HRESULT
	CreateProcessImpersonated(
		const WCHAR* commandLine,
		const WCHAR* currentDirectory,
		BOOL waitForExit,
		FileCore::String* stdOutput = nullptr,
		const FileCore::UserContext* context = nullptr
		);

	P4VFS_CORE_API HRESULT
	SendDriverControlMessage(
		P4VFS_CONTROL_MSG& message,
		P4VFS_CONTROL_REPLY& reply
		);

	P4VFS_CORE_API HRESULT
	SendDriverControlMessage(
		P4VFS_CONTROL_MSG* message,
		DWORD messageSize,
		P4VFS_CONTROL_REPLY* reply,
		DWORD replySize,
		DWORD* replySizeWritten = nullptr
		);

	P4VFS_CORE_API HRESULT
	SetDriverTraceEnabled(
		DWORD channels
		);

	P4VFS_CORE_API HRESULT
	SetDriverFlag(
		const wchar_t* flagName,
		ULONG flagValue
		);

	P4VFS_CORE_API HRESULT
	GetDriverIsConnected(
		bool& connected
		);

	P4VFS_CORE_API HRESULT
	GetDriverVersion(
		USHORT& major,
		USHORT& minor,
		USHORT& build,
		USHORT& revision
		);

	P4VFS_CORE_API HRESULT
	AssignUnicodeStringReference(
		P4VFS_UNICODE_STRING* dstString,
		const WCHAR* srcReference,
		ULONG srcReferenceBytes
		);

	P4VFS_CORE_API HRESULT
	AppendUnicodeStringReference(
		FileCore::Array<BYTE>& dstDataBuffer,
		size_t dstUnicodeStringOffset,
		const WCHAR* srcString
		);
}}}
#pragma managed(pop)
