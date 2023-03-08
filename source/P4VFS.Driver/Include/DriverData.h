// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#ifdef __cplusplus_cli 
#pragma managed(push, off)
#endif

#pragma warning(push)
#pragma warning(disable : 4201) // nonstandard extension used : nameless struct/union

// {3CA7BDAC-A3DC-4AB8-93CA-2C815E5EC15A}
#define P4VFS_REPARSE_GUID						{ 0x3ca7bdac, 0xa3dc, 0x4ab8, { 0x93, 0xca, 0x2c, 0x81, 0x5e, 0x5e, 0xc1, 0x5a } }
#define P4VFS_REPARSE_TAG						0xBAC

#define P4VFS_NTFS_PATH_MAX						0x8000

#define P4VFS_RESIDENCY_POLICY_UNDEFINED		0x00
#define P4VFS_RESIDENCY_POLICY_RESIDENT			0x01
#define P4VFS_RESIDENCY_POLICY_SYMLINK			0x02
#define P4VFS_RESIDENCY_POLICY_REMOVE_FILE		0x03

#define P4VFS_POPULATE_POLICY_UNDEFINED			0x00
#define P4VFS_POPULATE_POLICY_DEPOT				0x01
#define P4VFS_POPULATE_POLICY_SHARE				0x02

#define P4VFS_POPULATE_METHOD_COPY				0x00
#define P4VFS_POPULATE_METHOD_MOVE				0x01
#define P4VFS_POPULATE_METHOD_STREAM			0x02

#define P4VFS_SERVICE_RESOLVE_FILE				0x01
#define P4VFS_SERVICE_LOG_WRITE					0x02
#define P4VFS_SERVICE_PORT_NAME					L"\\P4VFS_SERVICE_PORT_NAME"

#define P4VFS_OPERATION_SET_TRACE_ENABLED		0x01
#define P4VFS_OPERATION_GET_IS_CONNECTED		0x02
#define P4VFS_OPERATION_GET_VERSION				0x03
#define P4VFS_OPERATION_SET_FLAG				0x04
#define P4VFS_OPERATION_OPEN_REPARSE_POINT		0x05
#define P4VFS_OPERATION_CLOSE_REPARSE_POINT		0x06

#define P4VFS_CONTROL_PORT_NAME					L"\\P4VFS_CONTROL_PORT_NAME"
#define P4VFS_CONTROL_FLAG_LENGTH				32

typedef struct _P4VFS_UNICODE_STRING
{
	ULONG sizeBytes;
	LONG offsetBytes;

#define P4VFS_UNICODE_STRING_CSTR(str)	((str).sizeBytes ? (const WCHAR*)((CHAR*)(&(str))+((str).offsetBytes)) : L"")
#ifdef __cplusplus
	const WCHAR* c_str() const { return P4VFS_UNICODE_STRING_CSTR(*this); }
#endif
} P4VFS_UNICODE_STRING;

typedef struct _P4VFS_REPARSE_DATA_HEADER
{
	ULONG dataVersion;
} P4VFS_REPARSE_DATA_HEADER;

#define P4VFS_MAX_PATH_LENGTH_1 320
#define P4VFS_MAX_NAME_LENGTH_1 128

typedef struct _P4VFS_REPARSE_DATA_1
{
	P4VFS_REPARSE_DATA_HEADER header;
	USHORT majorVersion;
	USHORT minorVersion;
	USHORT buildVersion;
	UCHAR residencyPolicy;
	UCHAR populatePolicy;
	USHORT fileRevision;
	WCHAR depotPath[P4VFS_MAX_PATH_LENGTH_1];
	WCHAR depotServer[P4VFS_MAX_NAME_LENGTH_1];
	WCHAR depotClient[P4VFS_MAX_NAME_LENGTH_1];
	WCHAR depotUser[P4VFS_MAX_NAME_LENGTH_1];
} P4VFS_REPARSE_DATA_1;

typedef struct _P4VFS_REPARSE_DATA_2
{
	P4VFS_REPARSE_DATA_HEADER header;
	ULONG dataSize;
	USHORT majorVersion;
	USHORT minorVersion;
	USHORT buildVersion;
	UCHAR residencyPolicy;
	UCHAR populatePolicy;
	ULONG fileRevision;
	P4VFS_UNICODE_STRING depotPath;
	P4VFS_UNICODE_STRING depotServer;
	P4VFS_UNICODE_STRING depotClient;
	P4VFS_UNICODE_STRING depotUser;
} P4VFS_REPARSE_DATA_2;

#define P4VFS_REPARSE_DATA_VERSION_1	1
#define P4VFS_REPARSE_DATA_VERSION_2	2

typedef struct _P4VFS_SERVICE_RESOLVE_FILE_MSG
{
	ULONG sessionId;
	P4VFS_UNICODE_STRING volumeName;
	P4VFS_UNICODE_STRING dataName;
	ULONG processId;
	ULONG threadId;
} P4VFS_SERVICE_RESOLVE_FILE_MSG;

typedef struct _P4VFS_SERVICE_LOG_WRITE_MSG
{
	P4VFS_UNICODE_STRING text;
} P4VFS_SERVICE_LOG_WRITE_MSG;

typedef struct _P4VFS_SERVICE_MSG
{
	ULONG size;
	ULONG requestID;
	ULONG operation;
	union
	{
		P4VFS_SERVICE_RESOLVE_FILE_MSG resolveFile;
		P4VFS_SERVICE_LOG_WRITE_MSG logWrite;
	};
} P4VFS_SERVICE_MSG;

typedef struct _P4VFS_SERVICE_REQ_BUFFER
{
	CHAR data[sizeof(P4VFS_SERVICE_MSG)+P4VFS_NTFS_PATH_MAX*sizeof(WCHAR)];
} P4VFS_SERVICE_REQ_BUFFER;

typedef struct _P4VFS_SERVICE_REPLY
{
	ULONG requestID;
	ULONG requestResult;
} P4VFS_SERVICE_REPLY;

#ifndef P4VFS_KERNEL_MODE
typedef struct _FILE_OBJECT* PFILE_OBJECT;
#endif

typedef struct _P4VFS_FLT_FILE_HANDLE
{
	HANDLE fileHandle;
	PFILE_OBJECT fileObject;
} P4VFS_FLT_FILE_HANDLE;

typedef struct _P4VFS_CONTROL_MSG
{
	ULONG operation;
	union
	{
		struct
		{
			ULONG channels;
		} SET_TRACE_ENABLED;

		struct
		{
			WCHAR name[P4VFS_CONTROL_FLAG_LENGTH];
			ULONG value;
		} SET_FLAG;

		struct
		{
			P4VFS_UNICODE_STRING filePath;
			UCHAR accessRead;
			UCHAR accessWrite;
			UCHAR accessDelete;
			UCHAR shareRead;
			UCHAR shareWrite;
			UCHAR shareDelete;
		} OPEN_REPARSE_POINT;

		struct
		{
			P4VFS_FLT_FILE_HANDLE handle;
		} CLOSE_REPARSE_POINT;

	} data;

} P4VFS_CONTROL_MSG;

typedef struct _P4VFS_CONTROL_REPLY
{
	ULONG operation;
	union
	{
		struct
		{
			ULONG connected;
		} GET_IS_CONNECTED;

		struct
		{
			USHORT major;
			USHORT minor;
			USHORT build;
			USHORT revision;
		} GET_VERSION;

		struct
		{
			P4VFS_FLT_FILE_HANDLE handle;
			LONG ntstatus;
		} OPEN_REPARSE_POINT;

		struct
		{
			LONG ntstatus;
		} CLOSE_REPARSE_POINT;

	} data;

} P4VFS_CONTROL_REPLY;

#pragma warning(pop)
#ifdef __cplusplus_cli 
#pragma managed(pop)
#endif
