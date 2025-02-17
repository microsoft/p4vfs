/*++
Copyright (c) Microsoft Corporation.
Licensed under the MIT license.

Module Name:
	DriverCore.c

Abstract:
	This is the core utility module of the P4VFS minifilter driver.

Environment:
	Kernel mode

--*/
#ifdef P4VFS_KERNEL_MODE
#include <fltKernel.h>
#include <ntintsafe.h>
#include "DriverCore.h"
#include "DriverCore.tmh"
#endif

#pragma code_seg("PAGE")

NTSTATUS
P4vfsExecuteResolveFile(
	_In_ PFLT_CALLBACK_DATA pCallbackData,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_In_ ULONG dwResolveFileFlags
	)
{
	NTSTATUS					status					= STATUS_SUCCESS;
	FLT_FILE_NAME_INFORMATION*	pFileNameInfo			= NULL;

	PAGED_CODE();

	if (!pCallbackData)
	{
		status = STATUS_INVALID_PARAMETER;
		P4vfsTraceError(Core, L"P4vfsExecuteResolveFile: FLT_CALLBACK_DATA is NULL"); 
		goto CLEANUP;
	}

	if (!pFltObjects)
	{
		status = STATUS_INVALID_PARAMETER;
		P4vfsTraceError(Core, L"P4vfsExecuteResolveFile: PCFLT_RELATED_OBJECTS is NULL"); 
		goto CLEANUP;
	}

	// Get the file name info, so we know the file we are opening
	status = FltGetFileNameInformation(pCallbackData, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT | FLT_FILE_NAME_ALLOW_QUERY_ON_REPARSE, &pFileNameInfo);
	if (!NT_SUCCESS(status)) 
	{ 
		P4vfsTraceError(Core, L"P4vfsExecuteResolveFile: Failed to Get File Name Information [%!STATUS!]", status); 
		goto CLEANUP; 
	}

	status = FltParseFileNameInformation(pFileNameInfo);
	if (!NT_SUCCESS(status)) 
	{ 
		P4vfsTraceError(Core, L"P4vfsExecuteResolveFile: Failed to Parse File Name Information [%!STATUS!]", status); 
		goto CLEANUP; 
	}

	if (pFileNameInfo->Stream.Length > 0) 
	{
		P4vfsTraceInfo(Core, L"P4vfsExecuteResolveFile: Skipping AlternateStream [%wZ] for [%wZ]", &pFileNameInfo->Stream, &pFileNameInfo->Name); 
		goto CLEANUP; 
	}

	P4vfsTraceInfo(Core, L"P4vfsExecuteResolveFile: Start [%wZ]", &pFileNameInfo->Name);
	
	if (!FlagOn(dwResolveFileFlags, P4VFS_RESOLVE_FILE_FLAG_IGNORE_TAG))
	{
		if (!pCallbackData->TagData)
		{
			status = STATUS_DATA_ERROR;
			P4vfsTraceError(Core, L"P4vfsExecuteResolveFile: FLT_CALLBACK_DATA::TagData is NULL [%wZ]", &pFileNameInfo->Name); 
			goto CLEANUP;
		}

		if (pCallbackData->TagData->FileTag != P4VFS_REPARSE_TAG)
		{
			status = STATUS_DATA_ERROR;
			P4vfsTraceError(Core, L"P4vfsExecuteResolveFile: FLT_CALLBACK_DATA::TagData::FileTag is not P4VFS_REPARSE_TAG [%wZ]", &pFileNameInfo->Name); 
			goto CLEANUP;
		}

		if (P4vfsIsReparseGuid(&pCallbackData->TagData->GenericGUIDReparseBuffer.TagGuid) == FALSE)
		{
			status = STATUS_DATA_ERROR;
			P4vfsTraceError(Core, L"P4vfsExecuteResolveFile: FLT_CALLBACK_DATA::TagData::GenericGUIDReparseBuffer is not P4VFS_REPARSE_GUID [%wZ]", &pFileNameInfo->Name); 
			goto CLEANUP;
		}
	}

	status = P4vfsUserModeResolveFile(pCallbackData, pFltObjects, pFileNameInfo);
	if (!NT_SUCCESS(status)) 
	{ 
		P4vfsTraceError(Core, L"P4vfsExecuteResolveFile: Resolve [%wZ] Failed [%!STATUS!]", &pFileNameInfo->Name, status); 
		goto CLEANUP; 
	}

	P4vfsTraceInfo(Core, L"P4vfsExecuteResolveFile: End [%wZ] Status [%!STATUS!]", &pFileNameInfo->Name, status);

CLEANUP:
	if (pFileNameInfo)
	{
		FltReleaseFileNameInformation(pFileNameInfo);
		pFileNameInfo = NULL;
	}

	return status;
}

NTSTATUS 
P4vfsUserModeExecuteRequest(
	_In_ P4VFS_SERVICE_MSG* pRequestMsg
	)
{
	NTSTATUS				status				= STATUS_SUCCESS;
	P4VFS_SERVICE_REPLY*	pReplyMsg			= NULL;
	ULONG					dwReplyMsgSize		= sizeof(*pReplyMsg);

	PAGED_CODE();

	if (!pRequestMsg)
	{
		status = STATUS_INVALID_PARAMETER;
		P4vfsTraceError(Core, L"P4vfsUserModeExecuteRequest: P4VFS_SERVICE_MSG is NULL"); 
		goto CLEANUP;
	}

	if (g_FltContext.pServiceClientPort == NULL)
	{
		status = STATUS_PORT_DISCONNECTED;
		P4vfsTraceError(Core, L"P4vfsUserModeExecuteRequest: g_FltContext.pServiceClientPort is NULL"); 
		goto CLEANUP;
	}

	// Allocate a reply message
	pReplyMsg = (P4VFS_SERVICE_REPLY*)ExAllocatePoolZero( 
											NonPagedPoolNx,
											sizeof(P4VFS_SERVICE_REPLY),
											P4VFS_REPLY_MSG_ALLOC_TAG);

	if (pReplyMsg == NULL) 
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		P4vfsTraceError(Core, L"P4vfsUserModeExecuteRequest: Failed to Allocate P4VFS_SERVICE_REPLY size [%d]", sizeof(P4VFS_SERVICE_REPLY)); 
		goto CLEANUP;
	}

	pReplyMsg->requestID = (ULONG)-1;
	pReplyMsg->requestResult = (ULONG)STATUS_UNSUCCESSFUL;

	// Always use a unique requestID
	pRequestMsg->requestID = InterlockedIncrement(&g_FltContext.nRequestCount);

	// We idle until we receive a response back
	status = FltSendMessage(
				g_FltContext.pFilter,
				&g_FltContext.pServiceClientPort,
				pRequestMsg,
				pRequestMsg->size,
				pReplyMsg,
				&dwReplyMsgSize,
				NULL);

	if (!NT_SUCCESS(status)) 
	{ 
		P4vfsTraceError(Core, L"P4vfsUserModeExecuteRequest: Did not receive a response from our message to User Mode."); 
		goto CLEANUP; 
	}

	status = pReplyMsg->requestResult;

CLEANUP:
	if (pReplyMsg)
	{
		ExFreePoolWithTag(pReplyMsg, P4VFS_SERVICE_MSG_ALLOC_TAG);
		pReplyMsg = NULL;
	}

	return status;
}

NTSTATUS
P4vfsUserModeResolveFile(
	_In_ PFLT_CALLBACK_DATA pData,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_In_ FLT_FILE_NAME_INFORMATION* pFileNameInfo
	)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	ULONG sessionId = (ULONG)-1;
	P4VFS_SERVICE_MSG* pRequestMsg = NULL;

	PAGED_CODE();

	if (!pData)
	{
		status = STATUS_INVALID_PARAMETER;
		P4vfsTraceError(Core, L"P4vfsUserModeResolveFile: FLT_CALLBACK_DATA is NULL"); 
		goto CLEANUP;
	}

	if (!pFltObjects)
	{
		status = STATUS_INVALID_PARAMETER;
		P4vfsTraceError(Core, L"P4vfsUserModeResolveFile: PCFLT_RELATED_OBJECTS is NULL"); 
		goto CLEANUP;
	}

	if (!pFileNameInfo)
	{
		status = STATUS_INVALID_PARAMETER;
		P4vfsTraceError(Core, L"P4vfsUserModeResolveFile: FLT_FILE_NAME_INFORMATION is NULL"); 
		goto CLEANUP;
	}

	P4vfsTraceInfo(Core, L"P4vfsUserModeResolveFile: Start [%wZ]", &pFileNameInfo->Name);

	status = FltGetRequestorSessionId(pData, &sessionId);
	if (!NT_SUCCESS(status)) 
	{ 
		P4vfsTraceError(Core, L"P4vfsUserModeResolveFile: Failed to FltGetRequestorSessionId from initiating thread [%!STATUS!]", status); 
		goto CLEANUP; 
	}

	ULONG requestMsgSize = sizeof(P4VFS_SERVICE_MSG);

	const ULONG volumeNameSize = pFileNameInfo->Volume.Length + sizeof(WCHAR);
	const ULONG volumeNameOffset = requestMsgSize;
	requestMsgSize += volumeNameSize;

	const ULONG dataNameSize = pFileNameInfo->Name.Length + sizeof(WCHAR);
	const ULONG dataNameOffset = requestMsgSize;
	requestMsgSize += dataNameSize;

	pRequestMsg = (P4VFS_SERVICE_MSG*)ExAllocatePoolZero( 
											NonPagedPoolNx,
											requestMsgSize,
											P4VFS_SERVICE_MSG_ALLOC_TAG);

	if (pRequestMsg == NULL) 
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		P4vfsTraceError(Core, L"P4vfsUserModeResolveFile: RequestMsg: Failed to P4VFS_SERVICE_MSG size [%d]", requestMsgSize); 
		goto CLEANUP;
	}

	RtlZeroMemory(pRequestMsg, requestMsgSize);

	pRequestMsg->operation = P4VFS_SERVICE_RESOLVE_FILE;
	pRequestMsg->size = requestMsgSize;

	pRequestMsg->resolveFile.sessionId = sessionId;
	pRequestMsg->resolveFile.processId = (ULONG)PsGetCurrentProcessId(); 
	pRequestMsg->resolveFile.threadId = (ULONG)PsGetCurrentThreadId();
	
	P4vfsCopyAssignUnicodeString(
		&pRequestMsg->resolveFile.volumeName, 
		((CHAR*)pRequestMsg)+volumeNameOffset,
		volumeNameSize,
		pFileNameInfo->Volume.Buffer,
		pFileNameInfo->Volume.Length);

	P4vfsCopyAssignUnicodeString(
		&pRequestMsg->resolveFile.dataName, 
		((CHAR*)pRequestMsg)+dataNameOffset,
		dataNameSize,
		pFileNameInfo->Name.Buffer,
		pFileNameInfo->Name.Length);

	status = P4vfsUserModeExecuteRequest(pRequestMsg);
	P4vfsTraceInfo(Core, L"P4vfsUserModeResolveFile: End [%wZ] Status [%!STATUS!]", &pFileNameInfo->Name, status);

CLEANUP:
	if (pRequestMsg)
	{
		ExFreePoolWithTag(pRequestMsg, P4VFS_SERVICE_MSG_ALLOC_TAG);
		pRequestMsg = NULL;
	}
	return status;
}

BOOLEAN
P4vfsIsRequestingReadOrWriteAccessToFile( 
	_In_ PFLT_CALLBACK_DATA pData
	)
{
	PAGED_CODE();

	BOOLEAN request = FALSE;
	if (pData && pData->Iopb && pData->Iopb->Parameters.Create.SecurityContext)
	{
		if (FlagOn(pData->Iopb->Parameters.Create.SecurityContext->DesiredAccess, FILE_READ_DATA) ||
			FlagOn(pData->Iopb->Parameters.Create.SecurityContext->DesiredAccess, FILE_WRITE_DATA) ||
			FlagOn(pData->Iopb->Parameters.Create.SecurityContext->DesiredAccess, FILE_APPEND_DATA))
		{
			request = TRUE;
		}
	}

	//P4vfsTraceInfo(Core, L"P4vfsIsRequestingReadOrWriteAccessToFile: [%!bool!]", request);
	return request;
}

BOOLEAN
P4vfsIsPlaceholderFile(
	_In_ PFLT_INSTANCE pFltInstance,
	_In_ PFILE_OBJECT pFileObject
	)
{
	BOOLEAN result = FALSE;
	NTSTATUS status = STATUS_SUCCESS;
	REPARSE_GUID_DATA_BUFFER reparseBufferHeader;
	ULONG bytesReturned = 0;

	PAGED_CODE();

	if (!pFltInstance)
	{
		P4vfsTraceError(Core, L"P4vfsIsPlaceholderFile: pFltInstance is NULL"); 
		goto CLEANUP;
	}

	if (!pFileObject)
	{
		P4vfsTraceError(Core, L"P4vfsIsPlaceholderFile: pFileObject is NULL"); 
		goto CLEANUP;
	}

	status = FltFsControlFile(pFltInstance,
				pFileObject,
				FSCTL_GET_REPARSE_POINT,
				NULL,
				0,
				&reparseBufferHeader,
				sizeof(reparseBufferHeader),
				&bytesReturned);

	if (status == STATUS_BUFFER_OVERFLOW && bytesReturned == sizeof(reparseBufferHeader))
	{
		if (reparseBufferHeader.ReparseTag == P4VFS_REPARSE_TAG && P4vfsIsReparseGuid(&reparseBufferHeader.ReparseGuid))
		{
			result = TRUE;
		}
	}

CLEANUP:
	//P4vfsTraceInfo(Core, L"P4vfsIsPlaceholderFile: [%!bool!]", result);
	return result;
}

BOOLEAN
P4vfsIsReparseGuid(
	_In_ CONST GUID* pGuid
	)
{
	BOOLEAN result = FALSE;
	GUID reparseGuid = P4VFS_REPARSE_GUID;

	PAGED_CODE();

	if (pGuid != NULL && RtlCompareMemory(&pGuid, &reparseGuid, sizeof(reparseGuid)) == 0)
	{
		result = TRUE;
	}

	//P4vfsTraceInfo(Core, L"P4vfsIsReparseGuid: [%!bool!]", result);
	return result;
}

ULONG
P4vfsSanitizeFileAttributes(
	_In_ ULONG fileAttributes
	)
{
	PAGED_CODE();

	if ((fileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
	{
		fileAttributes &= ~(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_SPARSE_FILE);
		if (fileAttributes == 0) 
		{
			fileAttributes = FILE_ATTRIBUTE_NORMAL;
		}
	}
	return fileAttributes;
}

NTSTATUS
P4vfsGetReparseActionFileKey(
	_In_ PFLT_CALLBACK_DATA pData,
	_Out_ PUNICODE_STRING pOutActionFileKey
	)
{
	NTSTATUS						status			= STATUS_SUCCESS;
	FLT_FILE_NAME_INFORMATION*		pFileNameInfo	= NULL;
	UNICODE_STRING					actionFileKey	= {0};

	PAGED_CODE();

	status = FltGetFileNameInformation(pData, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT | FLT_FILE_NAME_ALLOW_QUERY_ON_REPARSE, &pFileNameInfo);
	if (!NT_SUCCESS(status)) 
	{ 
		P4vfsTraceWarning(Core, L"P4vfsGetReparseActionFileKey: FltGetFileNameInformation failed [%!STATUS!]", status); 
		goto CLEANUP; 
	}

	status = FltParseFileNameInformation(pFileNameInfo);
	if (!NT_SUCCESS(status)) 
	{ 
		P4vfsTraceError(Core, L"P4vfsGetReparseActionFileKey: FltParseFileNameInformation failed [%!STATUS!]", status); 
		goto CLEANUP; 
	}
	
	status = RtlDowncaseUnicodeString(&actionFileKey, &pFileNameInfo->Name, TRUE);
	if (!NT_SUCCESS(status)) 
	{ 
		P4vfsTraceError(Core, L"P4vfsGetReparseActionFileKey: RtlDowncaseUnicodeString failed [%!STATUS!]", status); 
		goto CLEANUP; 
	}

	if (actionFileKey.Buffer == NULL)
	{
		status = STATUS_MEMORY_NOT_ALLOCATED;
		P4vfsTraceError(Core, L"P4vfsGetReparseActionFileKey: actionFileKey.Buffer is NULL"); 
		goto CLEANUP;
	}

	*pOutActionFileKey = actionFileKey;
	RtlZeroMemory(&actionFileKey, sizeof(actionFileKey));

CLEANUP:
	if (pFileNameInfo)
	{
		FltReleaseFileNameInformation(pFileNameInfo);
		pFileNameInfo = NULL;
	}

	if (actionFileKey.Buffer)
	{
		RtlFreeUnicodeString(&actionFileKey);
	}

	return status;
}

BOOLEAN
P4vfsIsEqualActionFileKey(
	_In_ PCUNICODE_STRING fileKey0,
	_In_ PCUNICODE_STRING fileKey1
	)
{
	PAGED_CODE();

	if (fileKey0->Length == fileKey1->Length && fileKey0->Buffer != NULL && fileKey1->Buffer != NULL)
	{
		if (RtlCompareMemory(fileKey0->Buffer, fileKey1->Buffer, fileKey0->Length) == fileKey0->Length)
			return TRUE;
	}

	return FALSE;
}

BOOLEAN
P4vfsQueryAnyReparseActionInProgress( 
	)
{
	PAGED_CODE();

	BOOLEAN	result	= FALSE;
	ExAcquireFastMutex(&g_FltContext.hReparseActionLock);
	{
		if (g_FltContext.pReparseActionList != NULL)
			result = TRUE;
	}
	ExReleaseFastMutex(&g_FltContext.hReparseActionLock);
	return result;
}

BOOLEAN
P4vfsQueryReparseActionInProgress( 
	_In_ PFLT_CALLBACK_DATA pData
	)
{
	NTSTATUS					status			= STATUS_SUCCESS;
	BOOLEAN						result			= FALSE;
	P4VFS_REPARSE_ACTION*		pAction			= NULL;
	UNICODE_STRING				actionFileKey	= {0};

	PAGED_CODE();

	if (P4vfsQueryAnyReparseActionInProgress())
	{
		status = P4vfsGetReparseActionFileKey(pData, &actionFileKey);
		if (!NT_SUCCESS(status)) 
		{ 
			P4vfsTraceWarning(Core, L"P4vfsQueryReparseActionInProgress: P4vfsGetReparseActionFileKey failed [%!STATUS!]", status); 
			goto CLEANUP; 
		}

		ExAcquireFastMutex(&g_FltContext.hReparseActionLock);
		{
			for (pAction = g_FltContext.pReparseActionList; pAction != NULL; pAction = pAction->pNext)
			{
				if (P4vfsIsEqualActionFileKey(&actionFileKey, &pAction->fileKey))
				{
					P4vfsTraceInfo(Core, L"P4vfsQueryReparseActionInProgress: File in progress [%wZ] RefCount [%d]", &pAction->fileKey, pAction->nRefCount);
					result = TRUE;
					break;
				}
			}
		}
		ExReleaseFastMutex(&g_FltContext.hReparseActionLock);
	}

CLEANUP:
	if (actionFileKey.Buffer)
	{
		RtlFreeUnicodeString(&actionFileKey);
	}

	return result;
}

VOID
P4vfsPushReparseActionInProgress(
	_In_ PFLT_CALLBACK_DATA pData
	)
{
	NTSTATUS					status			= STATUS_SUCCESS;
	P4VFS_REPARSE_ACTION*		pAction			= NULL;
	UNICODE_STRING				actionFileKey	= {0};

	PAGED_CODE();

	status = P4vfsGetReparseActionFileKey(pData, &actionFileKey);
	if (!NT_SUCCESS(status)) 
	{ 
		P4vfsTraceError(Core, L"P4vfsPushReparseActionInProgress: P4vfsGetReparseActionFileKey failed [%!STATUS!]", status); 
		goto CLEANUP; 
	}

	ExAcquireFastMutex(&g_FltContext.hReparseActionLock);
	{
		for (pAction = g_FltContext.pReparseActionList; pAction != NULL; pAction = pAction->pNext)
		{
			if (P4vfsIsEqualActionFileKey(&actionFileKey, &pAction->fileKey))
				break;
		}

		if (pAction == NULL)
		{
			pAction = (P4VFS_REPARSE_ACTION*)ExAllocatePoolZero(
												NonPagedPoolNx, 
												sizeof(P4VFS_REPARSE_ACTION), 
												P4VFS_REPARSE_ACTION_ALLOC_TAG);
			if (pAction == NULL)
			{ 
				P4vfsTraceError(Core, L"P4vfsPushReparseActionInProgress: ExAllocatePoolZero failed [%wZ]", &actionFileKey); 
			}
			else
			{
				RtlZeroMemory(pAction, sizeof(P4VFS_REPARSE_ACTION));
		
				pAction->fileKey = actionFileKey;
				RtlZeroMemory(&actionFileKey, sizeof(actionFileKey));

				pAction->pNext = g_FltContext.pReparseActionList;
				g_FltContext.pReparseActionList = pAction;
			}
		}

		if (pAction != NULL)
		{
			pAction->nRefCount++;
			P4vfsTraceInfo(Core, L"P4vfsPushReparseActionInProgress: File [%wZ] RefCount [%d]", &pAction->fileKey, pAction->nRefCount);
		}
	}
	ExReleaseFastMutex(&g_FltContext.hReparseActionLock);

CLEANUP:
	if (actionFileKey.Buffer)
	{
		RtlFreeUnicodeString(&actionFileKey);
	}
}

VOID
P4vfsPopReparseActionInProgress(
	_In_ PFLT_CALLBACK_DATA pData
	)
{
	NTSTATUS					status			= STATUS_SUCCESS;
	P4VFS_REPARSE_ACTION*		pAction			= NULL;
	P4VFS_REPARSE_ACTION*		pPrevAction		= NULL;
	UNICODE_STRING				actionFileKey	= {0};

	PAGED_CODE();

	status = P4vfsGetReparseActionFileKey(pData, &actionFileKey);
	if (!NT_SUCCESS(status)) 
	{ 
		P4vfsTraceError(Core, L"P4vfsPushReparseActionInProgress: P4vfsGetReparseActionFileKey failed [%!STATUS!]", status); 
		goto CLEANUP; 
	}

	ExAcquireFastMutex(&g_FltContext.hReparseActionLock);
	{
		for (pAction = g_FltContext.pReparseActionList; pAction != NULL; pAction = pAction->pNext)
		{
			if (P4vfsIsEqualActionFileKey(&actionFileKey, &pAction->fileKey))
				break;

			pPrevAction = pAction;
		}

		if (pAction != NULL)
		{
			pAction->nRefCount--;
			P4vfsTraceInfo(Core, L"P4vfsPopReparseActionInProgress: File [%wZ] RefCount [%d]", &pAction->fileKey, pAction->nRefCount);

			if (pAction->nRefCount <= 0)
			{
				if (g_FltContext.pReparseActionList == pAction)
					g_FltContext.pReparseActionList = pAction->pNext;
				else if (pPrevAction != NULL)
					pPrevAction->pNext = pAction->pNext;

				RtlFreeUnicodeString(&pAction->fileKey);
				ExFreePoolWithTag(pAction, P4VFS_REPARSE_ACTION_ALLOC_TAG);
				pAction = NULL;
			}
		}
	}
	ExReleaseFastMutex(&g_FltContext.hReparseActionLock);

CLEANUP:
	if (actionFileKey.Buffer)
	{
		RtlFreeUnicodeString(&actionFileKey);
	}
}

NTSTATUS
P4vfsExecuteReparseAction(
	_In_ PFLT_CALLBACK_DATA pData,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_In_ ULONG dwResolveFileFlags
	)
{
	NTSTATUS status	= STATUS_SUCCESS;

	PAGED_CODE();

	if (!g_FltContext.pServiceClientPort)
	{
		status = STATUS_INVALID_PORT_HANDLE;
		goto CLEANUP;
	}

	P4vfsPushReparseActionInProgress(pData);
	status = P4vfsExecuteResolveFile(pData, pFltObjects, dwResolveFileFlags);
	P4vfsPopReparseActionInProgress(pData);

CLEANUP:
	return status;
}

NTSTATUS
P4vfsCopyAssignUnicodeString(
	_In_ P4VFS_UNICODE_STRING* pTargetString,
	_In_ VOID* pTargetBuffer,
	_In_ ULONG targetBufferSize,
	_In_ const VOID* pSourceBuffer,
	_In_ ULONG sourceBufferSize
	)
{
	NTSTATUS status = STATUS_SUCCESS;

	PAGED_CODE();

	if (!pTargetString)
	{
		status = STATUS_INVALID_PARAMETER;
		P4vfsTraceError(Core, L"P4vfsCopyAssignUnicodeString: pTargetString is NULL"); 
		goto CLEANUP;
	}

	if (!pTargetBuffer)
	{
		status = STATUS_INVALID_PARAMETER;
		P4vfsTraceError(Core, L"P4vfsCopyAssignUnicodeString: pTargetBuffer is NULL"); 
		goto CLEANUP;
	}

	if (!pSourceBuffer)
	{
		status = STATUS_INVALID_PARAMETER;
		P4vfsTraceError(Core, L"P4vfsCopyAssignUnicodeString: pSourceBuffer is NULL"); 
		goto CLEANUP;
	}

	const INT64 ptrOffset = ((CHAR*)pTargetBuffer) - ((CHAR*)pTargetString);
	const LONG longOffset = (LONG)ptrOffset;
	if (((INT64)longOffset) != ptrOffset)
	{
		status = STATUS_INTEGER_OVERFLOW;
		P4vfsTraceError(Core, L"P4vfsCopyAssignUnicodeString: Invalid Offset"); 
		goto CLEANUP;
	}

	RtlZeroMemory(pTargetBuffer, targetBufferSize);
	RtlCopyMemory(pTargetBuffer, pSourceBuffer, min(sourceBufferSize, targetBufferSize));

	pTargetString->sizeBytes = targetBufferSize;
	pTargetString->offsetBytes = longOffset;

CLEANUP:
	return status;
}

NTSTATUS
P4vfsAllocateUnicodeString(
	_In_ ULONG poolTag,
	_Inout_ PUNICODE_STRING pString
	)
{
	PAGED_CODE();

	if (pString == NULL)
	{
		return STATUS_INVALID_PARAMETER;
	}

	if (pString->MaximumLength == 0)
	{
		return STATUS_INVALID_BUFFER_SIZE;
	}

	pString->Buffer = (PWCH)ExAllocatePoolZero(
								NonPagedPoolNx, 
								pString->MaximumLength, 
								poolTag);

	if (pString->Buffer == NULL)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	pString->Length = 0;
	return STATUS_SUCCESS;
}

NTSTATUS
P4vfsToUnicodeString(
	_In_ const P4VFS_UNICODE_STRING* pSrcString,
	_Out_ UNICODE_STRING* pDstString
	)
{
	PAGED_CODE();

	if (pSrcString == NULL || pDstString == NULL)
	{
		return STATUS_INVALID_PARAMETER;
	}

	if (pSrcString->sizeBytes >= UNICODE_STRING_MAX_BYTES)
	{
		return STATUS_BUFFER_OVERFLOW;
	}

	if (pSrcString->sizeBytes % sizeof(WCHAR))
	{
		return STATUS_DATATYPE_MISALIGNMENT;
	}
	
	pDstString->Length = (USHORT)(pSrcString->sizeBytes >= sizeof(WCHAR) ? pSrcString->sizeBytes-sizeof(WCHAR) : 0);
	pDstString->MaximumLength = (USHORT)pSrcString->sizeBytes;
	pDstString->Buffer = (WCHAR*)P4VFS_UNICODE_STRING_CSTR(*pSrcString);
	return STATUS_SUCCESS;
}

NTSTATUS
P4vfsGetFileIdByFileName(
	_In_opt_ PFLT_INSTANCE pFltInstance,
	_In_ PUNICODE_STRING pFileName,
	_Out_ PUNICODE_STRING pOutFileIdPath
	)
{
	NTSTATUS status = STATUS_SUCCESS;
	IO_STATUS_BLOCK	ioStatus = {0};
	OBJECT_ATTRIBUTES objAttributes = {0};
	HANDLE hLocalFile = NULL;
	PFILE_OBJECT pLocalFileObject = NULL;
	FILE_ID_INFORMATION fileIdInfo = {0};
	PFLT_VOLUME pVolume = NULL;
	PFLT_INSTANCE pVolumeInstance = NULL;
	UNICODE_STRING fileIdPath = {0};

	PAGED_CODE();

	if (pFileName == NULL)
	{
		status = STATUS_INVALID_PARAMETER;
		P4vfsTraceError(Core, L"P4vfsGetFileIdByFileName: pFileName is NULL"); 
		goto CLEANUP;
	}

	if (pOutFileIdPath == NULL)
	{
		status = STATUS_INVALID_PARAMETER;
		P4vfsTraceError(Core, L"P4vfsGetFileIdByFileName: pOutFileIdPath is NULL"); 
		goto CLEANUP;
	}

	InitializeObjectAttributes(&objAttributes,
								pFileName,
								OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
								NULL,
								NULL);

	status = FltCreateFileEx2(g_FltContext.pFilter,
								pFltInstance,
								&hLocalFile,
								&pLocalFileObject,
								0,
								&objAttributes,
								&ioStatus,
								NULL,
								FILE_ATTRIBUTE_NORMAL,
								FILE_SHARE_VALID_FLAGS,
								FILE_OPEN,
								FILE_NON_DIRECTORY_FILE | FILE_OPEN_REPARSE_POINT,
								NULL,
								0,
								IO_IGNORE_SHARE_ACCESS_CHECK,
								NULL);

	if (!NT_SUCCESS(status))
	{
		P4vfsTraceError(Core, L"P4vfsGetFileIdByFileName: FltCreateFileEx2 failed [%wZ] [%!STATUS!]", pFileName, status); 
		goto CLEANUP;
	}

	status = FltGetVolumeFromFileObject(g_FltContext.pFilter,
										pLocalFileObject,
										&pVolume);

	if (!NT_SUCCESS(status))
	{
		P4vfsTraceError(Core, L"P4vfsGetFileIdByFileName: FltGetVolumeFromFileObject failed [%wZ] [%!STATUS!]", pFileName, status); 
		goto CLEANUP;
	}

	if (pFltInstance == NULL)
	{
		status = FltGetVolumeInstanceFromName(g_FltContext.pFilter, 
											  pVolume, 
											  NULL, 
											  &pVolumeInstance);

		if (!NT_SUCCESS(status) || pVolumeInstance == NULL)
		{
			P4vfsTraceError(Core, L"P4vfsGetFileIdByFileName: FltGetVolumeInstanceFromName failed [%wZ] [%!STATUS!]", pFileName, status); 
			goto CLEANUP;
		}

		pFltInstance = pVolumeInstance;
	}

	status = FltQueryInformationFile(pFltInstance,
									 pLocalFileObject,
									 &fileIdInfo,
									 sizeof(FILE_ID_INFORMATION),
									 FileIdInformation,
									 NULL);

	if (!NT_SUCCESS(status))
	{
		P4vfsTraceError(Core, L"P4vfsGetFileIdByFileName: FltQueryInformationFile failed [%wZ] [%!STATUS!]", pFileName, status); 
		goto CLEANUP;
	}

	ULONG volumeNameLengthRequired = 0;
	status = FltGetVolumeName(pVolume, NULL, &volumeNameLengthRequired);
	if (!NT_SUCCESS(status) && (status != STATUS_BUFFER_TOO_SMALL))
	{
		P4vfsTraceError(Core, L"P4vfsGetFileIdByFileName: FltGetVolumeName length query failed [%wZ] [%!STATUS!]", pFileName, status); 
		goto CLEANUP;
	}

	status = RtlUShortAdd((USHORT)volumeNameLengthRequired, sizeof(WCHAR)+sizeof(fileIdInfo.FileId), &fileIdPath.MaximumLength);
	if (!NT_SUCCESS(status))
	{
		P4vfsTraceError(Core, L"P4vfsGetFileIdByFileName: RtlUShortAdd failed [%wZ] [%!STATUS!]", pFileName, status); 
		goto CLEANUP;
	}

	status = P4vfsAllocateUnicodeString(P4VFS_FILE_NAME_ALLOC_TAG, &fileIdPath);
	if (!NT_SUCCESS(status))
	{
		P4vfsTraceError(Core, L"P4vfsGetFileIdByFileName: P4vfsAllocateUnicodeString failed [%wZ] [%!STATUS!]", pFileName, status); 
		goto CLEANUP;
	}

	status = FltGetVolumeName(pVolume, &fileIdPath, &volumeNameLengthRequired);
	if (!NT_SUCCESS(status))
	{
		P4vfsTraceError(Core, L"P4vfsGetFileIdByFileName: FltGetVolumeName failed [%wZ] [%!STATUS!]", pFileName, status); 
		goto CLEANUP;
	}

	status = RtlAppendUnicodeToString(&fileIdPath, L"\\");
	if (!NT_SUCCESS(status))
	{
		P4vfsTraceError(Core, L"P4vfsGetFileIdByFileName: RtlAppendUnicodeToString separator failed [%wZ] fileIdPath [%wZ] [%!STATUS!]", pFileName, &fileIdPath, status); 
		goto CLEANUP;
	}

	UNICODE_STRING fileIdString;
	fileIdString.Length = sizeof(fileIdInfo.FileId);
	fileIdString.MaximumLength = sizeof(fileIdInfo.FileId);
	fileIdString.Buffer = (PWCHAR)&fileIdInfo.FileId;

	status = RtlAppendUnicodeStringToString(&fileIdPath, &fileIdString);
	if (!NT_SUCCESS(status))
	{
		P4vfsTraceError(Core, L"P4vfsGetFileIdByFileName: RtlAppendUnicodeToString fileIdString failed [%wZ] fileIdPath [%wZ] [%!STATUS!]", pFileName, &fileIdPath, status); 
		goto CLEANUP;
	}

	*pOutFileIdPath = fileIdPath;
	fileIdPath.Buffer = NULL;

CLEANUP:
	if (hLocalFile != NULL)
	{
		FltClose(hLocalFile);
	}

	if (pLocalFileObject != NULL)
	{
		ObDereferenceObject(pLocalFileObject);
	}

	if (pVolume != NULL)
	{
		FltObjectDereference(pVolume);
	}

    if (pVolumeInstance != NULL) 
	{
        FltObjectDereference(pVolumeInstance);
    }

	if (fileIdPath.Buffer != NULL)
	{
		ExFreePoolWithTag(fileIdPath.Buffer, P4VFS_FILE_NAME_ALLOC_TAG);
	}

	return status;
}

NTSTATUS
P4vfsOpenReparsePoint(
	_In_opt_ PFLT_INSTANCE pFltInstance,
	_In_ PUNICODE_STRING pFileName,
	_In_ ACCESS_MASK desiredAccess,
	_In_ ULONG objectAttributes,
	_Out_ PHANDLE pTargetHandle,
	_Outptr_ PFILE_OBJECT* ppTargetFileObject
	)
{
	NTSTATUS status = STATUS_SUCCESS;
	IO_STATUS_BLOCK	ioStatus = {0};
	OBJECT_ATTRIBUTES objAttributes = {0};
	IO_DRIVER_CREATE_CONTEXT createContext = {0};
	HANDLE hLocalFile = NULL;
	PFILE_OBJECT pLocalFileObject = NULL;
	UNICODE_STRING fileIdPath = {0};

	PAGED_CODE();

	// We wish to open an existing reparse point file by using FILE_OPEN_BY_FILE_ID so as to avoid 
	// directory notifications. Begin by finding the unique fileId by file path.

	status = P4vfsGetFileIdByFileName(pFltInstance, pFileName, &fileIdPath);
	if (!NT_SUCCESS(status))
	{
		P4vfsTraceError(Core, L"P4vfsOpenReparsePoint: P4vfsGetFileIdByFileName failed [%wZ] [%!STATUS!]", pFileName, status); 
		goto CLEANUP;
	}

	IoInitializeDriverCreateContext(&createContext);
#if (NTDDI_VERSION >= NTDDI_WIN10_RS1) && defined(P4VFS_KERNEL_MODE)
	createContext.SiloContext =	PsGetHostSilo();
#endif

	// We use the fileIdPath in place of the pFileName path for FILE_OPEN_BY_FILE_ID
	// The IO_IGNORE_SHARE_ACCESS_CHECK is used to avoid existing share conflicts

	InitializeObjectAttributes(&objAttributes,
							   &fileIdPath,
							   objectAttributes | OBJ_CASE_INSENSITIVE,
							   NULL,
							   NULL);

	status = FltCreateFileEx2(g_FltContext.pFilter,
							  pFltInstance,
							  &hLocalFile,
							  &pLocalFileObject,
							  desiredAccess | SYNCHRONIZE,
							  &objAttributes,
							  &ioStatus,
							  NULL,
							  FILE_ATTRIBUTE_NORMAL,
							  FILE_SHARE_READ |	FILE_SHARE_WRITE | FILE_SHARE_DELETE,
							  FILE_OPEN,
							  FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT | FILE_OPEN_BY_FILE_ID,
							  NULL,
							  0,
							  IO_IGNORE_SHARE_ACCESS_CHECK,
							  &createContext);

	if (!NT_SUCCESS(status))
	{
		P4vfsTraceError(Core, L"P4vfsReopenFile: FltCreateFileEx2 failed [%wZ] fileIdPath [%wZ] [%!STATUS!]", pFileName, &fileIdPath, status); 
		goto CLEANUP;
	}

	P4vfsTraceInfo(Core, L"P4vfsReopenFile: FltCreateFileEx2 success [%wZ] fileIdPath [%wZ] [%!STATUS!]", pFileName, &fileIdPath, status); 

	*pTargetHandle = hLocalFile;
	hLocalFile = NULL;
	*ppTargetFileObject = pLocalFileObject;
	pLocalFileObject = NULL;

CLEANUP:
	if (hLocalFile != NULL)
	{
		FltClose(hLocalFile);
	}

	if (pLocalFileObject != NULL)
	{
		ObDereferenceObject(pLocalFileObject);
	}

	if (fileIdPath.Buffer != NULL)
	{
		ExFreePoolWithTag(fileIdPath.Buffer, P4VFS_FILE_NAME_ALLOC_TAG);
	}

	return status;
}

NTSTATUS
P4vfsCloseReparsePoint(
	_In_ HANDLE hFile,
	_In_ PFILE_OBJECT pFileObject
	)
{
	PAGED_CODE();

	if (hFile != NULL)
	{
		FltClose(hFile);
	}

	if (pFileObject != NULL)
	{
		ObDereferenceObject(pFileObject);
	}

	return STATUS_SUCCESS;
}

BOOLEAN
P4vfsIsCurrentProcessElevated(
	)
{
	BOOLEAN result = FALSE;
	NTSTATUS status = STATUS_SUCCESS;
	PEPROCESS pProcessObject = NULL;
	PACCESS_TOKEN pAccessToken = NULL;
	PTOKEN_ELEVATION_TYPE pElevationType = NULL;

	PAGED_CODE();

	pProcessObject = PsGetCurrentProcess();
	if (pProcessObject == NULL)
	{
		P4vfsTraceError(Core, L"P4vfsIsCurrentProcessElevated: PsGetCurrentProcess is NULL");
		goto CLEANUP;
	}

	pAccessToken = PsReferencePrimaryToken(pProcessObject);
	if (pAccessToken == NULL)
	{
		P4vfsTraceError(Core, L"P4vfsIsCurrentProcessElevated: PsReferencePrimaryToken is NULL");
		goto CLEANUP;
	}

	status = SeQueryInformationToken(pAccessToken,
									 TokenElevationType,
									 &pElevationType);

	if (!NT_SUCCESS(status) || pElevationType == NULL) 
	{
		P4vfsTraceError(Core, L"P4vfsIsCurrentProcessElevated: Failed to query TokenElevationType");
		goto CLEANUP;
	}

	if (*pElevationType == TokenElevationTypeFull)
	{
		result = TRUE;
	}

	P4vfsTraceInfo(Core, L"P4vfsIsCurrentProcessElevated: Result [%d] pElevationType [%d]", (LONG)result, (LONG)(*pElevationType));

CLEANUP:
	if (pElevationType != NULL) 
	{
		ExFreePool(pElevationType);
	}

	if (pAccessToken != NULL)
	{
		PsDereferencePrimaryToken(pAccessToken);
	}

	return result;
}

