/*++
Copyright (c) Microsoft Corporation.
Licensed under the MIT license.

Module Name:
	DriverFilter.c

Abstract:
	This is the main entry module of the P4VFS minifilter driver.

Environment:
	Kernel mode

--*/
#include <fltKernel.h>
#include "DriverCore.h"
#include "DriverFilter.h"
#include "DriverFilter.tmh"

P4VFS_FLT_CONTEXT g_FltContext = {0};

DRIVER_INITIALIZE DriverEntry;

NTSTATUS
DriverEntry(
	_In_ DRIVER_OBJECT* pDriverObject,
	_In_ UNICODE_STRING* pRegistryPath
	);

NTSTATUS
P4vfsInstanceSetup(
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_In_ FLT_INSTANCE_SETUP_FLAGS dwFlags,
	_In_ DEVICE_TYPE dwVolumeDeviceType,
	_In_ FLT_FILESYSTEM_TYPE dwVolumeFilesystemType
	);

VOID
P4vfsInstanceTeardownStart(
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS dwFlags
	);

VOID
P4vfsInstanceTeardownComplete(
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS dwFlags
	);

NTSTATUS
P4vfsUnload(
	_In_ FLT_FILTER_UNLOAD_FLAGS dwFlags
	);

NTSTATUS
P4vfsInstanceQueryTeardown(
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS dwFlags
	);

NTSTATUS
P4vfsServicePortConnect(
	_In_ PFLT_PORT pClientPort,
	_In_opt_ VOID* pServerPortCookie,
	_In_reads_bytes_opt_(dwSizeOfContext) VOID* pConnectionContext,
	_In_ ULONG dwSizeOfContext,
	_Outptr_result_maybenull_ VOID** ppConnectionCookie
	);

VOID
P4vfsServicePortDisconnect(
	_In_opt_ VOID* pConnectionCookie
	);

NTSTATUS
P4vfsControlPortConnect(
	_In_ PFLT_PORT pClientPort,
	_In_opt_ VOID* pServerPortCookie,
	_In_reads_bytes_opt_(dwSizeOfContext) VOID* pConnectionContext,
	_In_ ULONG dwSizeOfContext,
	_Outptr_result_maybenull_ VOID** ppConnectionCookie
	);

VOID
P4vfsControlPortDisconnect(
	_In_opt_ VOID* pConnectionCookie
	);

NTSTATUS
P4vfsControlPortMessage(
	_In_opt_ PVOID pPortCookie,
	_In_reads_bytes_opt_(InputBufferLength)	PVOID pInputBuffer,
	_In_ ULONG dwInputBufferLength,
	_Out_writes_bytes_to_opt_(OutputBufferLength, *ReturnOutputBufferLength) PVOID pOutputBuffer,
	_In_ ULONG dwOutputBufferLength,
	_Out_ PULONG pReturnOutputBufferLength
	);

FLT_PREOP_CALLBACK_STATUS
P4vfsPreCreate(
	_Inout_ PFLT_CALLBACK_DATA pData,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* ppCompletionContext
	);

FLT_POSTOP_CALLBACK_STATUS
P4vfsPostCreate(
	_Inout_ PFLT_CALLBACK_DATA pData,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_In_opt_ PVOID pCompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS dwFlags
	);

FLT_PREOP_CALLBACK_STATUS
P4vfsPreQueryInformation(
	_Inout_ PFLT_CALLBACK_DATA pData,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* ppCompletionContext
	);

FLT_POSTOP_CALLBACK_STATUS
P4vfsPostQueryInformation(
	_Inout_ PFLT_CALLBACK_DATA pData,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_In_opt_ PVOID pCompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS dwFlags
	);

FLT_PREOP_CALLBACK_STATUS
P4vfsPreQueryOpen(
	_Inout_ PFLT_CALLBACK_DATA pData,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* ppCompletionContext
	);

FLT_POSTOP_CALLBACK_STATUS
P4vfsPostQueryOpen(
	_Inout_ PFLT_CALLBACK_DATA pData,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_In_opt_ PVOID pCompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS dwFlags
	);

FLT_PREOP_CALLBACK_STATUS
P4vfsPreNetworkQueryOpen(
	_Inout_ PFLT_CALLBACK_DATA pData,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* ppCompletionContext
	);

FLT_POSTOP_CALLBACK_STATUS
P4vfsPostNetworkQueryOpen(
	_Inout_ PFLT_CALLBACK_DATA pData,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_In_opt_ PVOID pCompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS dwFlags
	);

FLT_PREOP_CALLBACK_STATUS
P4vfsPreFileSystemControl(
	_Inout_ PFLT_CALLBACK_DATA pData,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* ppCompletionContext
	);

FLT_POSTOP_CALLBACK_STATUS
P4vfsPostFileSystemControl(
	_Inout_ PFLT_CALLBACK_DATA pData,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_In_opt_ PVOID ppCompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS dwFlags
	);

//
// Assign text sections for each routine
//
#ifdef ALLOC_PRAGMA
	#pragma alloc_text(INIT, DriverEntry)
	#pragma alloc_text(PAGE, P4vfsInstanceSetup)
	#pragma alloc_text(PAGE, P4vfsUnload)
	#pragma alloc_text(PAGE, P4vfsInstanceQueryTeardown)
	#pragma alloc_text(PAGE, P4vfsInstanceTeardownStart)
	#pragma alloc_text(PAGE, P4vfsInstanceTeardownComplete)
	#pragma alloc_text(PAGE, P4vfsInstanceTeardownComplete)
	#pragma alloc_text(PAGE, P4vfsServicePortConnect)
	#pragma alloc_text(PAGE, P4vfsServicePortDisconnect)
	#pragma alloc_text(PAGE, P4vfsControlPortConnect)
	#pragma alloc_text(PAGE, P4vfsControlPortDisconnect)
	#pragma alloc_text(PAGE, P4vfsControlPortMessage)
	#pragma alloc_text(PAGE, P4vfsPreCreate)
	#pragma alloc_text(PAGE, P4vfsPostCreate)
	#pragma alloc_text(PAGE, P4vfsPreQueryInformation)
	#pragma alloc_text(PAGE, P4vfsPostQueryInformation)
	#pragma alloc_text(PAGE, P4vfsPreQueryOpen)
	#pragma alloc_text(PAGE, P4vfsPostQueryOpen)
	#pragma alloc_text(PAGE, P4vfsPreNetworkQueryOpen)
	#pragma alloc_text(PAGE, P4vfsPostNetworkQueryOpen)
	#pragma alloc_text(PAGE, P4vfsPreFileSystemControl)
	#pragma alloc_text(PAGE, P4vfsPostFileSystemControl)
#endif

// 
// These are the the IRPs we are registering for
//
CONST FLT_OPERATION_REGISTRATION kDriverCallbacks[] = {
	{ 
		IRP_MJ_CREATE,
		0,
		P4vfsPreCreate,
		P4vfsPostCreate
	},
	{ 
		IRP_MJ_QUERY_INFORMATION,
		0,
		P4vfsPreQueryInformation,
		P4vfsPostQueryInformation
	},
	{ 
		IRP_MJ_QUERY_OPEN,
		0,
		P4vfsPreQueryOpen,
		P4vfsPostQueryOpen 
	},
	{ 
		IRP_MJ_NETWORK_QUERY_OPEN,
		0,
		P4vfsPreNetworkQueryOpen,
		P4vfsPostNetworkQueryOpen 
	},
	{
		IRP_MJ_FILE_SYSTEM_CONTROL,
		0,
		P4vfsPreFileSystemControl,
		P4vfsPostFileSystemControl
	},
	{ 
		IRP_MJ_OPERATION_END
	}
};

//
//  This defines what we want to filter with FltMgr
//
CONST FLT_REGISTRATION kDriverFilterRegistration = {
	sizeof(FLT_REGISTRATION),			// Size
	FLT_REGISTRATION_VERSION,			// Version
	0,									// Flags
	NULL,								// Context
	kDriverCallbacks,					// Operation callbacks
	P4vfsUnload,						// MiniFilterUnload
	P4vfsInstanceSetup,					// InstanceSetup
	P4vfsInstanceQueryTeardown,			// InstanceQueryTeardown
	P4vfsInstanceTeardownStart,			// InstanceTeardownStart
	P4vfsInstanceTeardownComplete,		// InstanceTeardownComplete
	NULL,								// GenerateFileName
	NULL,								// GenerateDestinationFileName
	NULL								// NormalizeNameComponent
};

NTSTATUS
DriverEntry(
	_In_ DRIVER_OBJECT* pDriverObject,
	_In_ UNICODE_STRING* pRegistryPath
	)
{
	NTSTATUS				status = STATUS_SUCCESS;
	UNICODE_STRING			servicePortString;
	UNICODE_STRING			controlPortString;
	PSECURITY_DESCRIPTOR	serviceSecurityDescriptor =	NULL;
	PSECURITY_DESCRIPTOR	controlSecurityDescriptor =	NULL;
	OBJECT_ATTRIBUTES		serviceObjectAttributes;
	OBJECT_ATTRIBUTES		controlObjectAttributes;

	UNREFERENCED_PARAMETER(pRegistryPath);

	RtlZeroMemory(&g_FltContext, sizeof(g_FltContext));
	g_FltContext.pDriverObject = pDriverObject;
	g_FltContext.bSanitizeAttributes = FALSE;
	g_FltContext.bShareModeDuringHydration = FALSE;

	WPP_INIT_TRACING(pDriverObject, pRegistryPath);
	P4vfsTraceInfo(Init, L"DriverEntry: Entered");

	if (!pDriverObject)
	{
		status = STATUS_INVALID_PARAMETER;
		P4vfsTraceError(Filter, L"DriverEntry: Driver object is NULL");
		goto CLEANUP;
	}

	// Register with FltMgr to tell it our callback routines
	status = FltRegisterFilter(pDriverObject, &kDriverFilterRegistration, &g_FltContext.pFilter);
	if (!NT_SUCCESS(status)) 
	{ 
		P4vfsTraceError(Filter, L"DriverEntry: Register Filter Failed [%!STATUS!]", status); 
		goto CLEANUP; 
	}

	// Create the security descriptor used for the service communication port
	status = FltBuildDefaultSecurityDescriptor(&serviceSecurityDescriptor, FLT_PORT_ALL_ACCESS);
	if (!NT_SUCCESS(status)) 
	{ 
		P4vfsTraceError(Filter, L"DriverEntry: Failed To Build Service Port Security Descriptor [%!STATUS!]", status); 
		goto CLEANUP; 
	}

	// Create the driver service communication port
	RtlInitUnicodeString(&servicePortString, P4VFS_SERVICE_PORT_NAME);

	InitializeObjectAttributes(
		&serviceObjectAttributes,
		&servicePortString,
		OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
		NULL,
		serviceSecurityDescriptor);

	status = FltCreateCommunicationPort(
					g_FltContext.pFilter, 
					&g_FltContext.pServiceServerPort,
					&serviceObjectAttributes,
					NULL,
					P4vfsServicePortConnect,
					P4vfsServicePortDisconnect,
					NULL,
					1);

	if (!NT_SUCCESS(status)) 
	{ 
		P4vfsTraceError(Filter, L"DriverEntry: Failed To Create Service Communication Port [%!STATUS!]", status); 
		goto CLEANUP; 
	}

	// Create the security descriptor used for the control communication port
	status = FltBuildDefaultSecurityDescriptor(&controlSecurityDescriptor, FLT_PORT_ALL_ACCESS);
	if (!NT_SUCCESS(status)) 
	{ 
		P4vfsTraceError(Filter, L"DriverEntry: Failed To Build Control Port Security Descriptor [%!STATUS!]", status); 
		goto CLEANUP; 
	}

	RtlSetDaclSecurityDescriptor(controlSecurityDescriptor, TRUE, NULL, FALSE);
	if (!NT_SUCCESS(status)) 
	{ 
		P4vfsTraceError(Filter, L"DriverEntry: Failed To set access for Control Port Security Descriptor [%!STATUS!]", status); 
		goto CLEANUP; 
	}

	// Create the driver control communication port
	RtlInitUnicodeString(&controlPortString, P4VFS_CONTROL_PORT_NAME);

	InitializeObjectAttributes(
		&controlObjectAttributes,
		&controlPortString,
		OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
		NULL,
		controlSecurityDescriptor);

	status = FltCreateCommunicationPort(
					g_FltContext.pFilter, 
					&g_FltContext.pControlServerPort,
					&controlObjectAttributes,
					NULL,
					P4vfsControlPortConnect,
					P4vfsControlPortDisconnect,
					P4vfsControlPortMessage,
					256);

	if (!NT_SUCCESS(status)) 
	{ 
		P4vfsTraceError(Filter, L"DriverEntry: Failed To Create Control Communication Port [%!STATUS!]", status); 
		goto CLEANUP; 
	}

	// Initialize the reparse action mutex
	ExInitializeFastMutex(&g_FltContext.hReparseActionLock);

	// After we have created everything we needed, actually start filtering 
	status = FltStartFiltering(g_FltContext.pFilter);
	if (!NT_SUCCESS(status)) 
	{ 
		P4vfsTraceError(Filter, L"DriverEntry: Failed to Start Filtering [%!STATUS!]", status); 
		goto CLEANUP; 
	}

CLEANUP:
	if (serviceSecurityDescriptor)
	{
		FltFreeSecurityDescriptor(serviceSecurityDescriptor);
	}
	if (controlSecurityDescriptor)
	{
		FltFreeSecurityDescriptor(controlSecurityDescriptor);
	}

	if (!NT_SUCCESS(status))
	{
		P4vfsTraceInfo(Filter, L"DriverEntry: Clean Up After Failure [%!STATUS!]", status);

		if (g_FltContext.pServiceServerPort)
		{
			FltCloseCommunicationPort(g_FltContext.pServiceServerPort); 
		}

		if (g_FltContext.pControlServerPort)
		{
			FltCloseCommunicationPort(g_FltContext.pControlServerPort); 
		}

		if (g_FltContext.pFilter)
		{
			FltUnregisterFilter(g_FltContext.pFilter);
		}

		WPP_CLEANUP(pDriverObject);
	}

	return status;
}

NTSTATUS
P4vfsInstanceSetup(
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_In_ FLT_INSTANCE_SETUP_FLAGS dwFlags,
	_In_ DEVICE_TYPE dwVolumeDeviceType,
	_In_ FLT_FILESYSTEM_TYPE dwVolumeFilesystemType
	)
{
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(dwVolumeFilesystemType);
	UNREFERENCED_PARAMETER(pFltObjects);

	PAGED_CODE();

	P4vfsTraceInfo(Filter, L"P4vfsInstanceSetup: Entered");

	if (pFltObjects == NULL)
	{
		P4vfsTraceInfo(Filter, L"P4vfsInstanceSetup: Filter Object is NULL");
		return STATUS_FLT_DO_NOT_ATTACH;
	}

	if (dwVolumeDeviceType != FILE_DEVICE_DISK_FILE_SYSTEM && dwVolumeFilesystemType != FLT_FSTYPE_NTFS)
	{
		return STATUS_FLT_DO_NOT_ATTACH;
	}

	return STATUS_SUCCESS;

}

NTSTATUS
P4vfsInstanceQueryTeardown(
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS dwFlags
	)
{
	UNREFERENCED_PARAMETER(pFltObjects);
	UNREFERENCED_PARAMETER(dwFlags);

	PAGED_CODE();

	P4vfsTraceInfo(Filter, L"P4vfsInstanceQueryTeardown: Entered");

	return STATUS_SUCCESS;
}

VOID
P4vfsInstanceTeardownStart(
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS dwFlags
	)
{
	UNREFERENCED_PARAMETER(pFltObjects);
	UNREFERENCED_PARAMETER(dwFlags);

	PAGED_CODE();

	P4vfsTraceInfo(Shutdown, L"P4vfsInstanceTeardownStart: Entered");
}
 
VOID
P4vfsInstanceTeardownComplete(
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS dwFlags
	)
{
	UNREFERENCED_PARAMETER(pFltObjects);
	UNREFERENCED_PARAMETER(dwFlags);

	PAGED_CODE();

	P4vfsTraceInfo(Shutdown, L"P4vfsInstanceTeardownComplete: Entered");
}

NTSTATUS
P4vfsUnload(
	_In_ FLT_FILTER_UNLOAD_FLAGS Flags
	)
{
	UNREFERENCED_PARAMETER( Flags );

	PAGED_CODE();

	P4vfsTraceInfo(Shutdown, L"P4vfsUnload: Entered");

	if (g_FltContext.pServiceServerPort)
	{
		FltCloseCommunicationPort(g_FltContext.pServiceServerPort); 
	}

	if (g_FltContext.pControlServerPort)
	{
		FltCloseCommunicationPort(g_FltContext.pControlServerPort); 
	}

	if (g_FltContext.pFilter)
	{
		FltUnregisterFilter(g_FltContext.pFilter);
	}
	
	WPP_CLEANUP(g_FltContext.pDriverObject);
	return STATUS_SUCCESS;
}

NTSTATUS 
P4vfsServicePortConnect( 
	_In_ PFLT_PORT pClientPort,
	_In_opt_ VOID* pServerPortCookie,
	_In_reads_bytes_opt_(dwSizeOfContext) VOID* pConnectionContext,
	_In_ ULONG dwSizeOfContext,
	_Outptr_result_maybenull_ VOID** ppConnectionCookie
	)
{
	NTSTATUS status	= STATUS_SUCCESS;
	P4VFS_SERVICE_PORT_CONNECTION_HANDLE* pConnectionHandle = NULL;

	UNREFERENCED_PARAMETER(pServerPortCookie);
	UNREFERENCED_PARAMETER(pConnectionContext);
	UNREFERENCED_PARAMETER(dwSizeOfContext);

	PAGED_CODE();

	P4vfsTraceInfo(Filter, L"P4vfsServicePortConnect: Entered");

	if (ppConnectionCookie == NULL)
	{
		status = STATUS_INVALID_PARAMETER;
		P4vfsTraceError(Filter, L"P4vfsServicePortConnect: ppConnectionCookie is invalid"); 
		goto CLEANUP;
	}

	pConnectionHandle = (P4VFS_SERVICE_PORT_CONNECTION_HANDLE*)ExAllocatePool2( 
																	POOL_FLAG_NON_PAGED,
																	sizeof(P4VFS_SERVICE_PORT_CONNECTION_HANDLE),
																	P4VFS_SERVICE_PORT_HANDLE_ALLOC_TAG);

	if (pConnectionHandle == NULL) 
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		P4vfsTraceError(Filter, L"P4vfsServicePortConnect: Failed to Allocate P4VFS_SERVICE_PORT_CONNECTION_HANDLE size [%d]", sizeof(P4VFS_SERVICE_PORT_CONNECTION_HANDLE)); 
		goto CLEANUP;
	}

	*ppConnectionCookie = pConnectionHandle;
	RtlZeroMemory(pConnectionHandle, sizeof(*pConnectionHandle));
	pConnectionHandle->pClientPort = pClientPort;

	status = ObOpenObjectByPointer(
				PsGetCurrentProcess(), 
				OBJ_KERNEL_HANDLE, 
				NULL, 
				0, 
				NULL, 
				KernelMode, 
				&pConnectionHandle->hUserProcess);
	
	if (!NT_SUCCESS(status)) 
	{ 
		P4vfsTraceError(Filter, L"P4vfsServicePortConnect: Failed to get connected user mode process handle [%!STATUS!]", status); 
		goto CLEANUP; 
	}

	// This new exclusive active connection has now been established for the driver
	P4vfsTraceInfo(Filter, L"P4vfsServicePortConnect: Opened active connection [%p]", pClientPort);
	g_FltContext.pServiceClientPort = pClientPort;

CLEANUP:
	return status;
}

VOID
P4vfsServicePortDisconnect(
	_In_opt_ VOID* pConnectionCookie 
	)
{
	P4VFS_SERVICE_PORT_CONNECTION_HANDLE* pConnectionHandle = NULL;

	PAGED_CODE();

	P4vfsTraceInfo(Filter, L"P4vfsServicePortDisconnect: Entered");
	
	if (pConnectionCookie == NULL)
	{
		P4vfsTraceError(Filter, L"P4vfsServicePortDisconnect: pConnectionCookie is invalid"); 
		goto CLEANUP; 
	}

	pConnectionHandle = (P4VFS_SERVICE_PORT_CONNECTION_HANDLE*)pConnectionCookie;
	if (pConnectionHandle->pClientPort != NULL)
	{
		// If this connection handle is our exclusive active pServiceClientPort, clear this value as we close the handle
		if (pConnectionHandle->pClientPort == g_FltContext.pServiceClientPort)
		{
			P4vfsTraceInfo(Filter, L"P4vfsServicePortDisconnect: Closed active connection [%p]", g_FltContext.pServiceClientPort);
			g_FltContext.pServiceClientPort = NULL;
		}

		FltCloseClientPort(g_FltContext.pFilter, &pConnectionHandle->pClientPort);
	}

	if (pConnectionHandle->hUserProcess != NULL)
	{
		ZwClose(pConnectionHandle->hUserProcess);
	}

	ExFreePoolWithTag(pConnectionHandle, P4VFS_SERVICE_PORT_HANDLE_ALLOC_TAG);
	
CLEANUP:
	return;
}

NTSTATUS
P4vfsControlPortConnect(
	_In_ PFLT_PORT pClientPort,
	_In_opt_ VOID* pServerPortCookie,
	_In_reads_bytes_opt_(dwSizeOfContext) VOID* pConnectionContext,
	_In_ ULONG dwSizeOfContext,
	_Outptr_result_maybenull_ VOID** ppConnectionCookie
	)
{
	NTSTATUS status	= STATUS_SUCCESS;
	P4VFS_CONTROL_PORT_CONNECTION_HANDLE* pConnectionHandle = NULL;

	UNREFERENCED_PARAMETER(pClientPort);
	UNREFERENCED_PARAMETER(pServerPortCookie);
	UNREFERENCED_PARAMETER(pConnectionContext);
	UNREFERENCED_PARAMETER(dwSizeOfContext);

	PAGED_CODE();

	P4vfsTraceInfo(Filter, L"P4vfsControlPortConnect: Entered");

	if (ppConnectionCookie == NULL)
	{
		status = STATUS_INVALID_PARAMETER;
		P4vfsTraceError(Filter, L"P4vfsControlPortConnect: ppConnectionCookie is invalid"); 
		goto CLEANUP;
	}

	pConnectionHandle = (P4VFS_CONTROL_PORT_CONNECTION_HANDLE*)ExAllocatePool2( 
																	POOL_FLAG_NON_PAGED,
																	sizeof(P4VFS_CONTROL_PORT_CONNECTION_HANDLE),
																	P4VFS_CONTROL_PORT_HANDLE_ALLOC_TAG);

	if (pConnectionHandle == NULL) 
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		P4vfsTraceError(Filter, L"P4vfsControlPortConnect: Failed to Allocate P4VFS_CONTROL_PORT_CONNECTION_HANDLE size [%d]", sizeof(P4VFS_CONTROL_PORT_CONNECTION_HANDLE)); 
		goto CLEANUP;
	}

	*ppConnectionCookie = pConnectionHandle;
	RtlZeroMemory(pConnectionHandle, sizeof(*pConnectionHandle));
	pConnectionHandle->pClientPort = pClientPort;

CLEANUP:
	return status;
}

VOID 
P4vfsControlPortDisconnect( 
	_In_opt_ VOID* pConnectionCookie 
	)
{
	P4VFS_CONTROL_PORT_CONNECTION_HANDLE* pConnectionHandle = NULL;

	PAGED_CODE();

	P4vfsTraceInfo(Filter, L"P4vfsControlPortDisconnect: Entered");
	
	if (pConnectionCookie == NULL)
	{
		P4vfsTraceError(Filter, L"P4vfsControlPortDisconnect: pConnectionCookie is invalid"); 
		goto CLEANUP; 
	}

	pConnectionHandle = (P4VFS_CONTROL_PORT_CONNECTION_HANDLE*)pConnectionCookie;
	if (pConnectionHandle->pClientPort != NULL)
	{
		FltCloseClientPort(g_FltContext.pFilter, &pConnectionHandle->pClientPort);
	}

	ExFreePoolWithTag(pConnectionHandle, P4VFS_CONTROL_PORT_HANDLE_ALLOC_TAG);
	
CLEANUP:
	return;
}

NTSTATUS
P4vfsControlPortMessage(
	_In_opt_ PVOID pPortCookie,
	_In_reads_bytes_opt_(InputBufferLength) PVOID pInputBuffer,
	_In_ ULONG dwInputBufferLength,
	_Out_writes_bytes_to_opt_(OutputBufferLength, *ReturnOutputBufferLength) PVOID pOutputBuffer,
	_In_ ULONG dwOutputBufferLength,
	_Out_ PULONG pReturnOutputBufferLength
	)
{
	NTSTATUS status = STATUS_SUCCESS;
	P4VFS_CONTROL_MSG* input = NULL;
	P4VFS_CONTROL_REPLY* output = NULL;

	UNREFERENCED_PARAMETER(pPortCookie);

	PAGED_CODE();

	if (pInputBuffer == NULL || dwInputBufferLength < sizeof(P4VFS_CONTROL_MSG))
	{
		status = STATUS_INVALID_PARAMETER;
		P4vfsTraceError(Filter, L"P4vfsControlPortMessage: pInputBuffer is invalid [%ld]", dwInputBufferLength); 
		goto CLEANUP;
	}

	if (pOutputBuffer == NULL || dwOutputBufferLength < sizeof(P4VFS_CONTROL_REPLY))
	{
		status = STATUS_INVALID_PARAMETER;
		P4vfsTraceError(Filter, L"P4vfsControlPortMessage: pOutputBuffer is invalid [%ld]", dwOutputBufferLength); 
		goto CLEANUP;
	}

	if (pReturnOutputBufferLength == NULL)
	{
		status = STATUS_INVALID_PARAMETER;
		P4vfsTraceError(Filter, L"P4vfsControlPortMessage: pReturnOutputBufferLength is NULL"); 
		goto CLEANUP;
	}

	input = (P4VFS_CONTROL_MSG*)pInputBuffer;
	output = (P4VFS_CONTROL_REPLY*)pOutputBuffer;
	
	RtlZeroMemory(output, sizeof(P4VFS_CONTROL_REPLY));
	output->operation = input->operation;
	*pReturnOutputBufferLength = sizeof(P4VFS_CONTROL_REPLY);

	switch (input->operation) 
	{
		case P4VFS_OPERATION_GET_IS_CONNECTED:
		{
			output->data.GET_IS_CONNECTED.connected = g_FltContext.pServiceClientPort ? 1 : 0;
			P4vfsTraceInfo(Filter, L"P4vfsControlPortMessage: P4VFS_CONTROL_GET_IS_CONNECTED [%d]", output->data.GET_IS_CONNECTED.connected);
			break;
		}
		case P4VFS_OPERATION_GET_VERSION:
		{
			output->data.GET_VERSION.major = P4VFS_VER_MAJOR;
			output->data.GET_VERSION.minor = P4VFS_VER_MINOR;
			output->data.GET_VERSION.build = P4VFS_VER_BUILD;
			output->data.GET_VERSION.revision = P4VFS_VER_REVISION;
			P4vfsTraceInfo(Filter, L"P4vfsControlPortMessage: P4VFS_CONTROL_GET_VERSION [%ls]", P4VFS_VER_VERSION_STRING);
			break;
		}
		case P4VFS_OPERATION_SET_FLAG:
		{
			const WCHAR strSanitizeAttributes[] = L"SanitizeAttributes";
			if (RtlCompareMemory(strSanitizeAttributes, input->data.SET_FLAG.name, sizeof(strSanitizeAttributes)) == sizeof(strSanitizeAttributes))
			{
				g_FltContext.bSanitizeAttributes = !!input->data.SET_FLAG.value;
				P4vfsTraceInfo(Filter, L"P4vfsControlPortMessage: P4VFS_CONTROL_SET_FLAG SanitizeAttributes = [0x%08d]", (LONG)g_FltContext.bSanitizeAttributes);
				break;
			}

			const WCHAR strShareModeDuringHydration[] = L"ShareModeDuringHydration";
			if (RtlCompareMemory(strShareModeDuringHydration, input->data.SET_FLAG.name, sizeof(strShareModeDuringHydration)) == sizeof(strShareModeDuringHydration))
			{
				g_FltContext.bShareModeDuringHydration = !!input->data.SET_FLAG.value;
				P4vfsTraceInfo(Filter, L"P4vfsControlPortMessage: P4VFS_CONTROL_SET_FLAG ShareModeDuringHydration = [0x%08d]", (LONG)g_FltContext.bShareModeDuringHydration);
				break;
			}

			status = STATUS_INVALID_PARAMETER;
			break;
		}
		case P4VFS_OPERATION_OPEN_REPARSE_POINT:
		{
			UNICODE_STRING unicodeFilePath = {0};
			status = P4vfsToUnicodeString(&input->data.OPEN_REPARSE_POINT.filePath, &unicodeFilePath);
			if (!NT_SUCCESS(status))
			{
				P4vfsTraceInfo(Filter, L"P4vfsControlPortMessage: P4VFS_OPERATION_OPEN_REPARSE_POINT P4vfsToUnicodeString failed [%!STATUS!]", status);
				break;
			}

			ULONG objectAttributes = 0;
			ACCESS_MASK desiredAccess = 0;
			desiredAccess |= input->data.OPEN_REPARSE_POINT.accessRead ? FILE_GENERIC_READ : 0;
			desiredAccess |= input->data.OPEN_REPARSE_POINT.accessWrite ? FILE_GENERIC_WRITE : 0;
			desiredAccess |= input->data.OPEN_REPARSE_POINT.accessDelete ? DELETE : 0;
			
			status = P4vfsOpenReparsePoint(NULL,
										   &unicodeFilePath,
										   desiredAccess,
										   objectAttributes,
										   &output->data.OPEN_REPARSE_POINT.handle.fileHandle,
										   &output->data.OPEN_REPARSE_POINT.handle.fileObject);

			output->data.OPEN_REPARSE_POINT.ntstatus = status;
			break;
		}
		case P4VFS_OPERATION_CLOSE_REPARSE_POINT:
		{
			status = P4vfsCloseReparsePoint(input->data.CLOSE_REPARSE_POINT.handle.fileHandle,
											input->data.CLOSE_REPARSE_POINT.handle.fileObject);

			output->data.CLOSE_REPARSE_POINT.ntstatus = status;
			break;
		}
		default:
		{
			P4vfsTraceInfo(Filter, L"P4vfsControlPortMessage: UNKNOWN 0x%08x", input->operation);
			status = STATUS_INVALID_PARAMETER;
			break;
		}
	}

CLEANUP:
	return status;
}

FLT_PREOP_CALLBACK_STATUS 
P4vfsPreCreate(	
	_Inout_ PFLT_CALLBACK_DATA pData,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* ppCompletionContext
	)
{
	UNREFERENCED_PARAMETER(pFltObjects);
	UNREFERENCED_PARAMETER(ppCompletionContext);

	PAGED_CODE();

	if (!pData)
	{
		P4vfsTraceInfo(Filter, L"P4vfsPreCreate: FLT_CALLBACK_DATA is NULL");
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	if (!pData->Iopb)
	{
		P4vfsTraceInfo(Filter, L"P4vfsPreCreate: FLT_CALLBACK_DATA::Iopb is NULL");
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	if (!pData->Iopb->TargetFileObject)
	{
		P4vfsTraceInfo(Filter, L"P4vfsPreCreate: FLT_CALLBACK_DATA::Iopb::TargetFileObject is NULL");
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	if (FlagOn(pData->Iopb->OperationFlags, SL_OPEN_PAGING_FILE) ||
		FlagOn(pData->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY) ||
		FlagOn(pData->Iopb->TargetFileObject->Flags, FO_VOLUME_OPEN) ||
		FlagOn(pData->Iopb->Parameters.Create.Options, FILE_OPEN_BY_FILE_ID) ||
		FlagOn(pData->Iopb->Parameters.Create.Options, FILE_DIRECTORY_FILE))
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	if (g_FltContext.bShareModeDuringHydration)
	{
		if (P4vfsQueryReparseActionInProgress(pData))
		{
			SetFlag(pData->Iopb->Parameters.Create.ShareAccess, pData->Iopb->Parameters.Create.ShareAccess|FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE);
			FltSetCallbackDataDirty(pData);
		}
	}

	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS
P4vfsPostCreate(
	_Inout_ PFLT_CALLBACK_DATA pData,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_In_opt_ PVOID pCompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS dwFlags
	)
{
	NTSTATUS status	= STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(pCompletionContext);
	UNREFERENCED_PARAMETER(dwFlags);

	PAGED_CODE();

	//P4vfsTraceInfo(Filter, L"P4vfsPostCreate: Entered");

	if (!pData)
	{
		P4vfsTraceError(Filter, L"P4vfsPostCreate: FLT_CALLBACK_DATA is NULL"); 
		goto CLEANUP;
	}

	if (!pFltObjects)
	{
		P4vfsTraceError(Filter, L"P4vfsPostCreate: PCFLT_RELATED_OBJECTS is NULL"); 
		goto CLEANUP;
	}

	if (pData->IoStatus.Status == STATUS_REPARSE && !pData->TagData)
	{
		P4vfsTraceError(Filter, L"P4vfsPostCreate: FLT_CALLBACK_DATA::TagData is NULL"); 
		goto CLEANUP;
	}

	if (pData->IoStatus.Status == STATUS_REPARSE && !pData->Iopb)
	{
		P4vfsTraceError(Filter, L"P4vfsPostCreate: FLT_CALLBACK_DATA::Iopb is NULL"); 
		goto CLEANUP;
	}

	// Determine if we our hitting our reparse point and should hydrate
	if (pData->IoStatus.Status == STATUS_REPARSE &&
		pData->TagData->FileTag == P4VFS_REPARSE_TAG &&
		FlagOn(pData->Iopb->Parameters.Create.Options, FILE_OPEN_REPARSE_POINT) == 0 &&
		P4vfsIsReparseGuid(&pData->TagData->GenericGUIDReparseBuffer.TagGuid))
	{
		// Since this is our reparse determine if all the 
		// other conditions are met to make the file resident
		if (pData->RequestorMode == UserMode &&
			FlagOn(pData->Flags, FLTFL_CALLBACK_DATA_POST_OPERATION) &&
			FlagOn(pData->Iopb->IrpFlags, IRP_CREATE_OPERATION) &&
			P4vfsIsRequestingReadOrWriteAccessToFile(pData))
		{
			P4vfsTraceInfo(Filter, "P4vfsPostCreate: ExecuteReparseAction [%wZ]", (pFltObjects->FileObject ? &pFltObjects->FileObject->FileName : NULL));
			status = P4vfsExecuteReparseAction(pData, pFltObjects, P4VFS_RESOLVE_FILE_FLAG_NONE);

			if (!NT_SUCCESS(status))
			{
				P4vfsTraceInfo(Filter, L"P4vfsPostCreate: Failed ExecuteReparseAction [%wZ] [%!STATUS!]", (pFltObjects->FileObject ? &pFltObjects->FileObject->FileName : NULL), status);
				FltCancelFileOpen(pFltObjects->Instance, pFltObjects->FileObject);
				goto CLEANUP; 
			}
		}
		else
		{
			// If the file did not meet our requirements to make it resident
			// reissue the IRP, with ignore reparse flag turned on
			SetFlag(pData->Iopb->Parameters.Create.Options, FILE_OPEN_REPARSE_POINT);
			FltSetCallbackDataDirty(pData);
		}

		// Reissue the IRP so the driver stack below can see it
		FltReissueSynchronousIo(pFltObjects->Instance, pData);
	}

CLEANUP:
	if (!NT_SUCCESS(status))
	{
		if (pData != NULL)
		{
			pData->IoStatus.Status = STATUS_UNSUCCESSFUL;
			pData->IoStatus.Information = 0;
			FltSetCallbackDataDirty(pData);
		}

		P4vfsTraceInfo(Filter, L"P4vfsPostCreate: Cleaned Up After Failure.");
	}

	return FLT_POSTOP_FINISHED_PROCESSING;
}

FLT_PREOP_CALLBACK_STATUS
P4vfsPreQueryInformation(
	_Inout_ PFLT_CALLBACK_DATA pData,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* ppCompletionContext
	)
{
	FILE_INFORMATION_CLASS fileInformationClass;

	UNREFERENCED_PARAMETER(pFltObjects);
	UNREFERENCED_PARAMETER(ppCompletionContext);

	PAGED_CODE();

	//P4vfsTraceInfo(Filter, L"P4vfsPreQueryInformation: Entered");

	if (g_FltContext.bSanitizeAttributes == FALSE)
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	if (!pData)
	{
		P4vfsTraceInfo(Filter, L"P4vfsPreQueryInformation: FLT_CALLBACK_DATA is NULL");
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	if (!pData->Iopb)
	{
		P4vfsTraceInfo(Filter, L"P4vfsPreQueryInformation: FLT_CALLBACK_DATA::Iopb is NULL");
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	fileInformationClass = pData->Iopb->Parameters.QueryFileInformation.FileInformationClass;
	if (fileInformationClass != FileAllInformation &&
		fileInformationClass != FileBasicInformation &&
		fileInformationClass != FileNetworkOpenInformation &&
		fileInformationClass != FileAttributeTagInformation)
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS
P4vfsPostQueryInformation(
	_Inout_ PFLT_CALLBACK_DATA pData,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_In_opt_ PVOID pCompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS dwFlags
	)
{
	FLT_POSTOP_CALLBACK_STATUS callbackStatus = FLT_POSTOP_FINISHED_PROCESSING;
	PULONG pFileAttributes;
	PULONG pReparseTag;

	UNREFERENCED_PARAMETER(pFltObjects);
	UNREFERENCED_PARAMETER(pCompletionContext);
	UNREFERENCED_PARAMETER(dwFlags);

	PAGED_CODE();

	//P4vfsTraceInfo(Filter, L"P4vfsPostQueryInformation: Entered");

	if (!pData)
	{
		P4vfsTraceError(Filter, L"P4vfsPostQueryInformation: FLT_CALLBACK_DATA is NULL"); 
		goto CLEANUP;
	}

	if (!pData->Iopb)
	{
		P4vfsTraceError(Filter, L"P4vfsPostQueryInformation: FLT_CALLBACK_DATA::Iopb is NULL"); 
		goto CLEANUP;
	}

	if (!pData->Iopb->TargetFileObject)
	{
		P4vfsTraceError(Filter, L"P4vfsPostQueryInformation: FLT_CALLBACK_DATA::Iopb::TargetFileObject is NULL"); 
		goto CLEANUP;
	}

	if (!NT_SUCCESS(pData->IoStatus.Status) && pData->IoStatus.Status != STATUS_BUFFER_OVERFLOW)
	{
		P4vfsTraceError(Filter, L"P4vfsPostQueryInformation: FLT_CALLBACK_DATA::IoStatus failed");
		goto CLEANUP;
	}

	pFileAttributes = NULL;
	pReparseTag = NULL;
	switch (pData->Iopb->Parameters.QueryFileInformation.FileInformationClass) 
	{
		case FileAllInformation:
		{
			pFileAttributes = &((FILE_ALL_INFORMATION*)pData->Iopb->Parameters.QueryFileInformation.InfoBuffer)->BasicInformation.FileAttributes;
			break;
		}
		case FileBasicInformation:
		{
			pFileAttributes = &((FILE_BASIC_INFORMATION*)pData->Iopb->Parameters.QueryFileInformation.InfoBuffer)->FileAttributes;
			break;
		}
		case FileNetworkOpenInformation:
		{
			pFileAttributes = &((FILE_NETWORK_OPEN_INFORMATION*)pData->Iopb->Parameters.QueryFileInformation.InfoBuffer)->FileAttributes;
			break;
		}
		case FileAttributeTagInformation:
		{
			pFileAttributes = &((FILE_ATTRIBUTE_TAG_INFORMATION*)pData->Iopb->Parameters.QueryFileInformation.InfoBuffer)->FileAttributes;
			pReparseTag = &((FILE_ATTRIBUTE_TAG_INFORMATION*)pData->Iopb->Parameters.QueryFileInformation.InfoBuffer)->ReparseTag;
			break;
		}
	}

	if (pFileAttributes != NULL && FlagOn(*pFileAttributes, (FILE_ATTRIBUTE_OFFLINE|FILE_ATTRIBUTE_SPARSE_FILE|FILE_ATTRIBUTE_REPARSE_POINT)))
	{
		P4vfsTraceInfo(Filter, L"P4vfsPostQueryInformation: Testing P4vfsIsPlaceholderFile [0x%08x]", (ULONG)*pFileAttributes);
		if (P4vfsIsPlaceholderFile(pFltObjects->Instance, pData->Iopb->TargetFileObject))
		{
			*pFileAttributes = P4vfsSanitizeFileAttributes(*pFileAttributes);
			if (pReparseTag != NULL)
			{
				*pReparseTag = IO_REPARSE_TAG_RESERVED_ZERO;
			}
			P4vfsTraceInfo(Filter, L"P4vfsPostQueryInformation: Modified P4vfsIsPlaceholderFile [0x%08x]", (ULONG)*pFileAttributes);
		}
	}

CLEANUP:
	return callbackStatus;
}

FLT_PREOP_CALLBACK_STATUS
P4vfsPreQueryOpen(
	_Inout_ PFLT_CALLBACK_DATA pData,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* ppCompletionContext
	)
{
	UNREFERENCED_PARAMETER(pFltObjects);
	UNREFERENCED_PARAMETER(ppCompletionContext);

	PAGED_CODE();

	if (g_FltContext.bSanitizeAttributes == FALSE)
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	if (!pData)
	{
		P4vfsTraceInfo(Filter, L"P4vfsPreQueryOpen: FLT_CALLBACK_DATA is NULL");
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	if (!pData->Iopb)
	{
		P4vfsTraceInfo(Filter, L"P4vfsPreQueryOpen: FLT_CALLBACK_DATA::Iopb is NULL");
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	// If the query info class doesn't contain enough information for us to determine whether it is
	// on our reparse point, we have to force the query down the slow path.

	if (pData->Iopb->Parameters.QueryOpen.FileInformationClass != FileStatInformation &&
		pData->Iopb->Parameters.QueryOpen.FileInformationClass != FileStatLxInformation) 
	{
		P4vfsTraceInfo(Filter, L"P4vfsPreQueryOpen: FileInformationClass ignored");
		return FLT_PREOP_DISALLOW_FSFILTER_IO;
	}

	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS
P4vfsPostQueryOpen(
	_Inout_ PFLT_CALLBACK_DATA pData,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_In_opt_ PVOID pCompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS dwFlags
	)
{
	FLT_POSTOP_CALLBACK_STATUS callbackStatus = FLT_POSTOP_FINISHED_PROCESSING;
	PULONG pFileAttributes;

	UNREFERENCED_PARAMETER(pFltObjects);
	UNREFERENCED_PARAMETER(pCompletionContext);

	PAGED_CODE();

	if (FlagOn(dwFlags, FLTFL_POST_OPERATION_DRAINING)) 
	{
		P4vfsTraceInfo(Filter, L"P4vfsPostQueryOpen: POST_OPERATION_DRAINING ignored");
		goto CLEANUP;
	}

	if (!pData)
	{
		P4vfsTraceError(Filter, L"P4vfsPostQueryOpen: FLT_CALLBACK_DATA is NULL"); 
		goto CLEANUP;
	}

	if (!pData->Iopb)
	{
		P4vfsTraceError(Filter, L"P4vfsPostQueryOpen: FLT_CALLBACK_DATA::Iopb is NULL"); 
		goto CLEANUP;
	}

	if (!NT_SUCCESS(pData->IoStatus.Status))
	{
		P4vfsTraceInfo(Filter, L"P4vfsPostQueryOpen: status ignored [%!STATUS!]", (ULONG)pData->IoStatus.Status);
		callbackStatus = FLT_POSTOP_DISALLOW_FSFILTER_IO;
		goto CLEANUP;
	}

	pFileAttributes = NULL;
	switch (pData->Iopb->Parameters.QueryOpen.FileInformationClass) 
	{
		case FileStatInformation:
		{
			pFileAttributes = &((FILE_STAT_INFORMATION*)pData->Iopb->Parameters.QueryOpen.FileInformation)->FileAttributes;
			break;
		}
		case FileStatLxInformation:
		{
			pFileAttributes = &((FILE_STAT_LX_INFORMATION*)pData->Iopb->Parameters.QueryOpen.FileInformation)->FileAttributes;
			break;
		}
	}

	if (pFileAttributes != NULL && FlagOn(*pFileAttributes, (FILE_ATTRIBUTE_OFFLINE|FILE_ATTRIBUTE_SPARSE_FILE|FILE_ATTRIBUTE_REPARSE_POINT)))
	{
		P4vfsTraceInfo(Filter, L"P4vfsPostQueryOpen: Testing P4vfsIsPlaceholderFile [0x%08x]", (ULONG)*pFileAttributes);
		if (P4vfsIsPlaceholderFile(pFltObjects->Instance, pData->Iopb->TargetFileObject))
		{
			*pFileAttributes = P4vfsSanitizeFileAttributes(*pFileAttributes);
			P4vfsTraceInfo(Filter, L"P4vfsPostQueryOpen: Modified P4vfsIsPlaceholderFile [0x%08x]", (ULONG)*pFileAttributes);
		}
	}
	else
	{
		P4vfsTraceInfo(Filter, L"P4vfsPostQueryOpen: Ignored [0x%08x]", pFileAttributes ? *pFileAttributes : -1);
	}

CLEANUP:
	return callbackStatus;
}

FLT_PREOP_CALLBACK_STATUS
P4vfsPreNetworkQueryOpen(
	_Inout_ PFLT_CALLBACK_DATA pData,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* ppCompletionContext
	)
{
	UNREFERENCED_PARAMETER(pData);
	UNREFERENCED_PARAMETER(pFltObjects);
	UNREFERENCED_PARAMETER(ppCompletionContext);

	PAGED_CODE();

	if (g_FltContext.bSanitizeAttributes == FALSE)
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	return FLT_PREOP_DISALLOW_FASTIO;
}

FLT_POSTOP_CALLBACK_STATUS
P4vfsPostNetworkQueryOpen(
	_Inout_ PFLT_CALLBACK_DATA pData,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_In_opt_ PVOID pCompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS dwFlags
	)
{
	UNREFERENCED_PARAMETER(pData);
	UNREFERENCED_PARAMETER(pFltObjects);
	UNREFERENCED_PARAMETER(pCompletionContext);
    UNREFERENCED_PARAMETER(dwFlags);

	PAGED_CODE();

    return FLT_POSTOP_FINISHED_PROCESSING;
}

FLT_PREOP_CALLBACK_STATUS
P4vfsPreFileSystemControl(
	_Inout_ PFLT_CALLBACK_DATA pData,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* ppCompletionContext
	)
{
	NTSTATUS status	= STATUS_SUCCESS;
	FLT_PREOP_CALLBACK_STATUS callbackStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
	ULONG ctrlCode = 0;

	UNREFERENCED_PARAMETER(ppCompletionContext);

	PAGED_CODE();

	if (!pData)
	{
		P4vfsTraceError(Filter,	L"P4vfsPreFileSystemControl: FLT_CALLBACK_DATA is NULL"); 
		goto CLEANUP;
	}

	if (!pData->Iopb)
	{
		P4vfsTraceError(Filter,	L"P4vfsPreFileSystemControl: FLT_CALLBACK_DATA::Iopb is	NULL");	
		goto CLEANUP;
	}

	if (!pFltObjects)
	{
		P4vfsTraceError(Core, L"P4vfsPreFileSystemControl: PCFLT_RELATED_OBJECTS is NULL"); 
		goto CLEANUP;
	}

	if (!pFltObjects->FileObject)
	{
		P4vfsTraceError(Core, L"P4vfsPreFileSystemControl: PCFLT_RELATED_OBJECTS::FileObject is NULL"); 
		goto CLEANUP;
	}

	ctrlCode = pData->Iopb->Parameters.FileSystemControl.Common.FsControlCode;
	switch (ctrlCode)
	{
		case FSCTL_REQUEST_BATCH_OPLOCK:
		case FSCTL_REQUEST_FILTER_OPLOCK:
		case FSCTL_REQUEST_OPLOCK:
		case FSCTL_REQUEST_OPLOCK_LEVEL_1:
		case FSCTL_REQUEST_OPLOCK_LEVEL_2:
		{
			//
			//  This description is quoted from GVFLT, and this OPLOCK handling fix is likewise inspired.
			//
			//  Before granting an oplock on the primary stream of a file recall the data. Otherwise when
			//  we recall on first read we write to the file which can trigger a break that the lock holder
			//  would not be expecting. We saw this deadlock on a lock holder who wasn't set up to acknowledge
			//  a break in the context of a read.
			//
			//  As an optimization, we suppress this recall if the oplock request is on a handle that couldn't
			//  be used to modify or access the file data (by a user-mode caller anyway).  Even though such
			//  a handle wouldn't cause sharing violations, it still is a handle, so it will interfere with
			//  attempts to rename an ancestor directory.  A *-Handle oplock allows the caller to get out of
			//  the way in that case, just as it does in the sharing violation case.  Such a handle would be
			//  opened for attribute access only, and might be used by a caller trying to determine whether
			//  the file is a candidate for scanning.  It won't be used to read the file, so it won't trigger
			//  a recall.
			// 

			if (pData->RequestorMode == UserMode &&
				P4vfsIsPlaceholderFile(pFltObjects->Instance, pData->Iopb->TargetFileObject))
			{
				P4vfsTraceInfo(Filter, "P4vfsPreFileSystemControl: ExecuteReparseAction [%wZ] OpLock [0x%x]", &pFltObjects->FileObject->FileName, ctrlCode);
				status = P4vfsExecuteReparseAction(pData, pFltObjects, P4VFS_RESOLVE_FILE_FLAG_IGNORE_TAG);

				if (!NT_SUCCESS(status))
				{
					P4vfsTraceInfo(Filter, L"P4vfsPreFileSystemControl: Failed ExecuteReparseAction [%wZ] OpLock [0x%x] [%!STATUS!]", &pFltObjects->FileObject->FileName, ctrlCode, status);
					goto CLEANUP; 
				}
			}
			break;
		}
	}

CLEANUP:
	if (!NT_SUCCESS(status))
	{
		if (pData)
		{
			pData->IoStatus.Status = status;
			pData->IoStatus.Information	= 0;
		}
	}
	return callbackStatus;
}

FLT_POSTOP_CALLBACK_STATUS
P4vfsPostFileSystemControl(
	_Inout_ PFLT_CALLBACK_DATA pData,
	_In_ PCFLT_RELATED_OBJECTS pFltObjects,
	_In_opt_ PVOID ppCompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS dwFlags
	)
{
	UNREFERENCED_PARAMETER(pData);
	UNREFERENCED_PARAMETER(pFltObjects);
	UNREFERENCED_PARAMETER(ppCompletionContext);
	UNREFERENCED_PARAMETER(dwFlags);

	PAGED_CODE();

	return FLT_POSTOP_FINISHED_PROCESSING;
}

