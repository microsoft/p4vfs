// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "DriverFilter.h"

#define P4VFS_RESOLVE_FILE_FLAG_NONE			0
#define P4VFS_RESOLVE_FILE_FLAG_IGNORE_TAG		1

#define P4VFS_REPARSE_ACTION_ALLOC_TAG			'BACA'
#define P4VFS_SERVICE_MSG_ALLOC_TAG				'BACR'
#define P4VFS_REPLY_MSG_ALLOC_TAG				'BACY'
#define P4VFS_REPARSE_BUFFER_ALLOC_TAG			'BACB'
#define P4VFS_FILE_NAME_ALLOC_TAG				'BAFN'
#define P4VFS_SERVICE_PORT_HANDLE_ALLOC_TAG		'BASH'
#define P4VFS_CONTROL_PORT_HANDLE_ALLOC_TAG		'BACH'

NTSTATUS
P4vfsUserModeExecuteDrvRequest(
	_In_ P4VFS_SERVICE_MSG* pRequestMsg
	);

NTSTATUS
P4vfsExecuteResolveFile(
	_In_ PFLT_CALLBACK_DATA pCallbackData,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_In_ ULONG dwResolveFileFlags
	);

NTSTATUS
P4vfsUserModeResolveFile(
	_In_ PFLT_CALLBACK_DATA pData,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_In_ FLT_FILE_NAME_INFORMATION* pFileNameInfo
	);

BOOLEAN
P4vfsIsRequestingReadOrWriteAccessToFile(
	_In_ PFLT_CALLBACK_DATA pData
	);

BOOLEAN
P4vfsIsPlaceholderFile(
	_In_ PFLT_INSTANCE pFltInstance,
	_In_ PFILE_OBJECT pFileObject
	);

BOOLEAN
P4vfsIsReparseGuid(
	_In_ CONST GUID* pGuid
	);

ULONG
P4vfsSanitizeFileAttributes(
	_In_ ULONG fileAttributes
	);

NTSTATUS
P4vfsGetReparseActionFileKey(
	_In_ PFLT_CALLBACK_DATA pData,
	_Out_ PUNICODE_STRING pOutActionFileKey
	);

BOOLEAN
P4vfsIsEqualActionFileKey(
	_In_ PCUNICODE_STRING fileKey0,
	_In_ PCUNICODE_STRING fileKey1
	);

BOOLEAN
P4vfsQueryAnyReparseActionInProgress(
	);

BOOLEAN
P4vfsQueryReparseActionInProgress(
	_In_ PFLT_CALLBACK_DATA pData
	);

VOID
P4vfsPushReparseActionInProgress(
	_In_ PFLT_CALLBACK_DATA pData
	);

VOID
P4vfsPopReparseActionInProgress(
	_In_ PFLT_CALLBACK_DATA pData
	);

NTSTATUS
P4vfsExecuteReparseAction(
	_In_ PFLT_CALLBACK_DATA pData,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_In_ ULONG dwResolveFileFlags
	);

NTSTATUS
P4vfsCopyAssignUnicodeString(
	_In_ P4VFS_UNICODE_STRING* pTargetString,
	_In_ VOID* pTargetBuffer,
	_In_ ULONG targetBufferSize,
	_In_ const VOID* pSourceBuffer,
	_In_ ULONG sourceBufferSize
	);

NTSTATUS
P4vfsAllocateUnicodeString(
	_In_ ULONG poolTag,
	_Inout_ PUNICODE_STRING pString
	);

NTSTATUS
P4vfsToUnicodeString(
	_In_ const P4VFS_UNICODE_STRING* pSrcString,
	_Out_ UNICODE_STRING* pDstString
	);

NTSTATUS
P4vfsSetFileWritable(
	_In_ PFLT_INSTANCE pFltInstance,
	_In_ PUNICODE_STRING pFileIdPath
	);

NTSTATUS
P4vfsGetFileIdByFileName(
	_In_ PUNICODE_STRING pFileName,
	_Out_ PUNICODE_STRING pOutFileIdPath,
	_Outptr_opt_ PFLT_INSTANCE* ppFltInstance
	);

NTSTATUS
P4vfsOpenReparsePoint(
	_In_ PUNICODE_STRING pFileName,
	_In_ ACCESS_MASK desiredAccess,
	_Out_ PHANDLE pTargetHandle,
	_Outptr_ PFILE_OBJECT* ppTargetFileObject
	);

NTSTATUS
P4vfsCloseReparsePoint(
	_In_ HANDLE hHandle,
	_In_ PFILE_OBJECT pFileObject
	);

BOOLEAN
P4vfsIsCurrentProcessElevated(
	);

