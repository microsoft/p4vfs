// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "DriverData.h"
#include "DriverVersion.h"
#include "DriverTrace.h"

typedef struct _P4VFS_REPARSE_ACTION
{
	struct _P4VFS_REPARSE_ACTION*	pNext;
	UNICODE_STRING					fileKey;
	LONG							nRefCount;
} P4VFS_REPARSE_ACTION;

typedef struct _P4VFS_FLT_CONTEXT
{
	DRIVER_OBJECT*					pDriverObject;				// The object that IDs the driver
	PFLT_FILTER						pFilter;					// Handle to this filter, result from FltRegisterFilter
	PFLT_PORT						pServiceServerPort;			// Filter service listening port
	PFLT_PORT						pServiceClientPort;			// Latest active service port
	PFLT_PORT						pControlServerPort;			// Filter control listening port
	LONG							nRequestCount;				// Number of requests processed
	BOOLEAN							bSanitizeAttributes;		// Enable stripping reparse and sparse file attributes
	BOOLEAN							bShareModeDuringHydration;	// Force file handle creation during hydration have share mode (legacy requirement)
	P4VFS_REPARSE_ACTION*			pReparseActionList;
	FAST_MUTEX						hReparseActionLock;
} P4VFS_FLT_CONTEXT;

extern P4VFS_FLT_CONTEXT g_FltContext;

typedef struct _P4VFS_SERVICE_PORT_CONNECTION_HANDLE
{
	PFLT_PORT	pClientPort;
	HANDLE		hUserProcess;
} P4VFS_SERVICE_PORT_CONNECTION_HANDLE;

typedef struct _P4VFS_CONTROL_PORT_CONNECTION_HANDLE
{
	PFLT_PORT	pClientPort;
} P4VFS_CONTROL_PORT_CONNECTION_HANDLE;
