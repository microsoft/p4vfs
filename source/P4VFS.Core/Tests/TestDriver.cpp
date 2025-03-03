// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "TestFactory.h"
#include "minwindef.h"
#include "intsafe.h"

typedef	HANDLE DRIVER_OBJECT;
typedef	VOID* PFLT_FILTER;
typedef	VOID* PFLT_PORT;
typedef	INT	FAST_MUTEX;
typedef	FAST_MUTEX*	PFAST_MUTEX;
typedef	VOID* PETHREAD;
typedef	ULONG FLT_CALLBACK_DATA_FLAGS;
typedef	VOID* PFLT_INSTANCE;
typedef	VOID* PFLT_VOLUME;
typedef	VOID* PKTRANSACTION;
typedef	INT	FLT_FILE_NAME_PARSED_FLAGS;
typedef	INT	FLT_FILE_NAME_OPTIONS;
typedef	VOID* PACCESS_STATE;
typedef INT FILE_INFORMATION_CLASS;
typedef HANDLE PEPROCESS;
typedef VOID* PESILO;

typedef	struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} UNICODE_STRING;
typedef	UNICODE_STRING *PUNICODE_STRING;
typedef	const UNICODE_STRING *PCUNICODE_STRING;

typedef	struct _FILE_OBJECT	{
	PVOID Unused;
	UNICODE_STRING FileName;
} FILE_OBJECT;
typedef	struct _FILE_OBJECT	*PFILE_OBJECT; 

typedef	struct _IO_SECURITY_CONTEXT	
{
	PSECURITY_QUALITY_OF_SERVICE SecurityQos;
	PACCESS_STATE AccessState;
	ACCESS_MASK	DesiredAccess;
	ULONG FullCreateOptions;
} IO_SECURITY_CONTEXT, *PIO_SECURITY_CONTEXT;

typedef	struct _IO_STATUS_BLOCK	
{
	union 
	{
		NTSTATUS Status;
		PVOID Pointer;
	} DUMMYUNIONNAME;
	ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef	struct _IO_DRIVER_CREATE_CONTEXT 
{
	SHORT Size;
	PVOID ExtraCreateParameter;
	PVOID DeviceObjectHint;
	PVOID TxnParameters;
#if	(NTDDI_VERSION >= NTDDI_WIN10_RS1)
	PESILO SiloContext;
#endif
} IO_DRIVER_CREATE_CONTEXT,	*PIO_DRIVER_CREATE_CONTEXT;

#define IoInitializeDriverCreateContext(DriverContext) {	\
	ZeroMemory(DriverContext, sizeof(*DriverContext));		\
	}

typedef	struct _OBJECT_ATTRIBUTES 
{
	ULONG Length;
	HANDLE RootDirectory;
	PUNICODE_STRING	ObjectName;
	ULONG Attributes;
	PVOID SecurityDescriptor;
	PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES;

typedef OBJECT_ATTRIBUTES *POBJECT_ATTRIBUTES;
typedef CONST OBJECT_ATTRIBUTES *PCOBJECT_ATTRIBUTES;

#define	InitializeObjectAttributes(	p, n, a, r,	s )	{	\
	(p)->Length	= sizeof( OBJECT_ATTRIBUTES	);			\
	(p)->RootDirectory = r;								\
	(p)->Attributes	= a;								\
	(p)->ObjectName	= n;								\
	(p)->SecurityDescriptor	= s;						\
	(p)->SecurityQualityOfService =	NULL;				\
	}

typedef	union _FLT_PARAMETERS
{
	struct 
	{
		PIO_SECURITY_CONTEXT SecurityContext;
		ULONG Options;
		USHORT FileAttributes;
		USHORT ShareAccess;
		ULONG EaLength;
		PVOID EaBuffer;
		LARGE_INTEGER AllocationSize;
	} Create;
	struct 
	{
		PVOID Argument1;
		PVOID Argument2;
		PVOID Argument3;
		PVOID Argument4;
		PVOID Argument5;
		LARGE_INTEGER Argument6;
	} Others;
} FLT_PARAMETERS, *PFLT_PARAMETERS;

typedef	struct _FLT_IO_PARAMETER_BLOCK 
{
	ULONG IrpFlags;
	UCHAR MajorFunction;
	UCHAR MinorFunction;
	UCHAR OperationFlags;
	UCHAR Reserved;
	PFILE_OBJECT TargetFileObject;
	PFLT_INSTANCE TargetInstance;
	FLT_PARAMETERS Parameters;
} FLT_IO_PARAMETER_BLOCK, *PFLT_IO_PARAMETER_BLOCK;

typedef	struct _FLT_TAG_DATA_BUFFER	
{
	ULONG FileTag;
	USHORT TagDataLength;
	USHORT UnparsedNameLength;
	union 
	{
		struct 
		{
			USHORT SubstituteNameOffset;
			USHORT SubstituteNameLength;
			USHORT PrintNameOffset;
			USHORT PrintNameLength;
			ULONG  Flags;
			WCHAR  PathBuffer[1];
		} SymbolicLinkReparseBuffer;

		struct 
		{
			USHORT SubstituteNameOffset;
			USHORT SubstituteNameLength;
			USHORT PrintNameOffset;
			USHORT PrintNameLength;
			WCHAR PathBuffer[1];
		} MountPointReparseBuffer;

		struct 
		{
			UCHAR DataBuffer[1];
		} GenericReparseBuffer;
		
		struct 
		{
			GUID TagGuid;
			UCHAR DataBuffer[1];
		} GenericGUIDReparseBuffer;
	};
} FLT_TAG_DATA_BUFFER, *PFLT_TAG_DATA_BUFFER;

typedef	struct _FLT_RELATED_OBJECTS	
{
	USHORT CONST Size;
	USHORT CONST TransactionContext;
	PFLT_FILTER	CONST Filter;
	PFLT_VOLUME	CONST Volume;
	PFLT_INSTANCE CONST	Instance;
	PFILE_OBJECT CONST FileObject;
	PKTRANSACTION CONST	Transaction;
} FLT_RELATED_OBJECTS, *PFLT_RELATED_OBJECTS;

typedef	CONST struct _FLT_RELATED_OBJECTS *PCFLT_RELATED_OBJECTS;

typedef	struct _FLT_CALLBACK_DATA 
{
	FLT_CALLBACK_DATA_FLAGS	Flags;
	PETHREAD CONST Thread;
	PFLT_IO_PARAMETER_BLOCK	CONST Iopb;
	IO_STATUS_BLOCK	IoStatus;
	struct _FLT_TAG_DATA_BUFFER	*TagData;
} FLT_CALLBACK_DATA, *PFLT_CALLBACK_DATA;

typedef	struct _FLT_FILE_NAME_INFORMATION 
{
	USHORT Size;
	FLT_FILE_NAME_PARSED_FLAGS NamesParsed;
	FLT_FILE_NAME_OPTIONS Format;
	UNICODE_STRING Name;
	UNICODE_STRING Volume;
	UNICODE_STRING Extension;
	UNICODE_STRING Stream;
	UNICODE_STRING FinalComponent;
	UNICODE_STRING ParentDir;
} FLT_FILE_NAME_INFORMATION, *PFLT_FILE_NAME_INFORMATION;

typedef	struct _FILE_BASIC_INFORMATION
{
	LARGE_INTEGER CreationTime;
	LARGE_INTEGER LastAccessTime;
	LARGE_INTEGER LastWriteTime;
	LARGE_INTEGER ChangeTime;
	ULONG FileAttributes;
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;

typedef	struct _FILE_STANDARD_INFORMATION_EX {
	LARGE_INTEGER AllocationSize;
	LARGE_INTEGER EndOfFile;
	ULONG NumberOfLinks;
	BOOLEAN	DeletePending;
	BOOLEAN	Directory;
	BOOLEAN	AlternateStream;
	BOOLEAN	MetadataAttribute;
} FILE_STANDARD_INFORMATION_EX,	*PFILE_STANDARD_INFORMATION_EX;

typedef struct _FILE_ID_INFORMATION {
    ULONGLONG VolumeSerialNumber;
    FILE_ID_128 FileId;
} FILE_ID_INFORMATION, *PFILE_ID_INFORMATION;

typedef enum _POOL_TYPE {
	NonPagedPoolNx = 512,
} POOL_TYPE;

#define	P4vfsTraceError(...)			__noop
#define	P4vfsTraceWarning(...)			__noop
#define	P4vfsTraceInfo(...)				__noop
#define	PAGED_CODE()					__noop
#define	FlagOn(_F,_SF)					((_F) &	(_SF))
#define ClearFlag(_F,_SF)				((_F) &= ~(_SF))
#define	ObDereferenceObject(f)			__noop
#define PsGetCurrentProcess()			GetCurrentProcess()
#define	PsGetCurrentProcessId()			GetCurrentProcessId()
#define	PsGetCurrentThreadId()			GetCurrentThreadId()
#define PsGetHostSilo()					NULL
#define min(a,b)						(((a) < (b)) ? (a) : (b))
#define max(a,b)						(((a) > (b)) ? (a) : (b))
#define RtlUShortAdd					UShortAdd

#define NT_SUCCESS(Status)				(((NTSTATUS)(Status)) >= 0)
#define	STATUS_SUCCESS					((NTSTATUS)0x00000000L)
#define	STATUS_UNSUCCESSFUL				((NTSTATUS)0xC0000001L)
#define	STATUS_BUFFER_OVERFLOW			((NTSTATUS)0x80000005L)
#define STATUS_BUFFER_TOO_SMALL			((NTSTATUS)0xC0000023L)
#define	STATUS_PORT_DISCONNECTED		((NTSTATUS)0xC0000037L)
#define	STATUS_DATA_ERROR				((NTSTATUS)0xC000003EL)
#define STATUS_INVALID_PORT_HANDLE		((NTSTATUS)0xC0000042L)
#define	STATUS_INSUFFICIENT_RESOURCES	((NTSTATUS)0xC000009AL)
#define	STATUS_MEMORY_NOT_ALLOCATED		((NTSTATUS)0xC00000A0L)
#define STATUS_NOT_ALL_ASSIGNED			((NTSTATUS)0x00000106L)
#define STATUS_INVALID_BUFFER_SIZE		((NTSTATUS)0xC0000206L)

#define	IO_IGNORE_SHARE_ACCESS_CHECK			0x0800
#define	FILE_SHARE_VALID_FLAGS					0x00000007
#define	FLT_FILE_NAME_NORMALIZED				0x01
#define	FLT_FILE_NAME_QUERY_DEFAULT				0x0100
#define	FLT_FILE_NAME_ALLOW_QUERY_ON_REPARSE	0x04000000
#define OBJ_CASE_INSENSITIVE					0x00000040L
#define OBJ_KERNEL_HANDLE						0x00000200L
#define POOL_FLAG_NON_PAGED						0x0000000000000040UI64
#define	FileBasicInformation					4
#define FileStandardInformation					5
#define	FileInternalInformation					6
#define FileIdInformation						59

#define	FILE_OPEN								0x00000001
#define	FILE_SYNCHRONOUS_IO_NONALERT			0x00000020
#define	FILE_NON_DIRECTORY_FILE					0x00000040
#define	FILE_NO_INTERMEDIATE_BUFFERING			0x00000008
#define	FILE_OPEN_REPARSE_POINT					0x00200000
#define	FILE_OPEN_BY_FILE_ID					0x00002000

#define P4VFS_DEFAULT_ACTION()							[](...) -> VOID { }
#define P4VFS_DEFAULT_FUNCTION(retType, retValue)		[](...) -> retType { return retValue; }
#define P4VFS_DEFAULT_FUNCTION_NTSTATUS()				P4VFS_DEFAULT_FUNCTION(NTSTATUS, STATUS_UNSUCCESSFUL)

std::function<NTSTATUS(PFLT_CALLBACK_DATA, FLT_FILE_NAME_OPTIONS, PFLT_FILE_NAME_INFORMATION*)> FltGetFileNameInformation;
std::function<NTSTATUS(PFLT_FILE_NAME_INFORMATION)> FltParseFileNameInformation;
std::function<VOID(PFLT_FILE_NAME_INFORMATION)> FltReleaseFileNameInformation;
std::function<NTSTATUS(PFLT_CALLBACK_DATA, PULONG)> FltGetRequestorSessionId;
std::function<NTSTATUS(PFLT_FILTER, PFLT_PORT, PVOID, ULONG, PVOID, PULONG, PLARGE_INTEGER)> FltSendMessage;
std::function<NTSTATUS(PFLT_INSTANCE, PFILE_OBJECT, ULONG, PVOID, ULONG, PVOID, ULONG, PULONG)> FltFsControlFile;
std::function<NTSTATUS(PFLT_FILTER, PFLT_INSTANCE, PHANDLE, PFILE_OBJECT*, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, PLARGE_INTEGER, ULONG, ULONG, ULONG, ULONG, PVOID, ULONG, ULONG)> FltCreateFileEx;
std::function<NTSTATUS(PFLT_FILTER, PFLT_INSTANCE, PHANDLE, PFILE_OBJECT*, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, PLARGE_INTEGER, ULONG, ULONG, ULONG, ULONG, PVOID, ULONG, ULONG, PIO_DRIVER_CREATE_CONTEXT)> FltCreateFileEx2;
std::function<VOID(PVOID)> FltObjectDereference;
std::function<NTSTATUS(HANDLE)> FltClose;
std::function<NTSTATUS(PFLT_INSTANCE, PFILE_OBJECT, PVOID, ULONG, FILE_INFORMATION_CLASS, PULONG)> FltQueryInformationFile;
std::function<NTSTATUS(PFLT_INSTANCE, PFILE_OBJECT, PVOID, ULONG, FILE_INFORMATION_CLASS)> FltSetInformationFile;
std::function<NTSTATUS(PFLT_INSTANCE, PFLT_VOLUME*)> FltGetVolumeFromInstance;
std::function<NTSTATUS(PFLT_VOLUME, PUNICODE_STRING, PULONG)> FltGetVolumeName;
std::function<NTSTATUS(PFLT_FILTER, PFILE_OBJECT, PFLT_VOLUME*)> FltGetVolumeFromFileObject;
std::function<NTSTATUS(PFLT_FILTER, PFLT_VOLUME, PCUNICODE_STRING, PFLT_INSTANCE*)> FltGetVolumeInstanceFromName;

std::function<PVOID(POOL_TYPE, SIZE_T, ULONG)> ExAllocatePoolZero;
std::function<VOID(PVOID, ULONG)> ExFreePoolWithTag;
std::function<VOID(PFAST_MUTEX)> ExAcquireFastMutex;
std::function<VOID(PFAST_MUTEX)> ExReleaseFastMutex;

NTSTATUS RtlAppendUnicodeToString(PUNICODE_STRING dst, PCWSTR src)
{
	if (!src || !dst || !dst->Buffer)
	{
		return STATUS_UNSUCCESSFUL;
	}
	const size_t srcLength = Microsoft::P4VFS::FileCore::StringInfo::Strlen(src)*sizeof(WCHAR);
	const size_t requiredLength = dst->Length + srcLength + sizeof(WCHAR);
	if (dst->MaximumLength < requiredLength)
	{
		return STATUS_UNSUCCESSFUL;
	}
	memcpy(((char*)dst->Buffer)+dst->Length, src, srcLength);
	dst->Length += USHORT(srcLength);
	memset(((char*)dst->Buffer)+dst->Length, 0, sizeof(WCHAR));
	Assert(dst->Length+sizeof(WCHAR) == requiredLength);
	return STATUS_SUCCESS;
}

NTSTATUS RtlAppendUnicodeStringToString(PUNICODE_STRING dst, PCUNICODE_STRING src)
{
	if (!src || !dst || !src->Buffer || !dst->Buffer)
	{
		return STATUS_UNSUCCESSFUL;
	}
	const size_t requiredLength = dst->Length + src->Length + sizeof(WCHAR);
	if (dst->MaximumLength < requiredLength)
	{
		return STATUS_UNSUCCESSFUL;
	}
	memcpy(((char*)dst->Buffer)+dst->Length, src, src->Length);
	dst->Length += src->Length;
	memset(((char*)dst->Buffer)+dst->Length, 0, sizeof(WCHAR));
	Assert(dst->Length+sizeof(WCHAR) == requiredLength);
	return STATUS_SUCCESS;
}

NTSTATUS RtlDowncaseUnicodeString(PUNICODE_STRING dst, PCUNICODE_STRING src, BOOLEAN allocate)
{
	if (!src || !dst || !src->Length || !src->Buffer)
	{
		return STATUS_UNSUCCESSFUL;
	}
	const size_t requiredLength = src->Length+sizeof(WCHAR);
	if (allocate)
	{
		dst->MaximumLength = USHORT(requiredLength);
		dst->Buffer = (PWSTR)Microsoft::P4VFS::FileCore::GAlloc(dst->MaximumLength);
	}
	else if (dst->MaximumLength < requiredLength || !dst->Buffer)
	{
		return STATUS_UNSUCCESSFUL;
	}
	dst->Length = src->Length;
	for (int i = 0; i < requiredLength/sizeof(WCHAR); ++i)
	{
		dst->Buffer[i] = Microsoft::P4VFS::FileCore::StringInfo::ToLower(src->Buffer[i]);
	}
	return STATUS_SUCCESS;
}

VOID RtlFreeUnicodeString(PUNICODE_STRING ustr)
{
	if (ustr)
	{
		Microsoft::P4VFS::FileCore::GFree(ustr->Buffer);
		memset(ustr, 0, sizeof(*ustr));
	}
}

VOID RtlInitUnicodeString(PUNICODE_STRING dst, PCWSTR src)
{
	RtlFreeUnicodeString(dst);
	if (src)
	{
		dst->Length = USHORT(Microsoft::P4VFS::FileCore::StringInfo::Strlen(src)*sizeof(WCHAR));
		dst->MaximumLength = dst->Length + sizeof(WCHAR);
		dst->Buffer = (PWSTR)Microsoft::P4VFS::FileCore::GAlloc(dst->MaximumLength);
		memcpy(dst->Buffer, src, dst->MaximumLength);
	}
}

PACCESS_TOKEN PsReferencePrimaryToken(PEPROCESS process)
{
	HANDLE hToken = INVALID_HANDLE_VALUE;
	return OpenProcessToken(process, TOKEN_ALL_ACCESS, &hToken) ? hToken : INVALID_HANDLE_VALUE;
}

VOID PsDereferencePrimaryToken(PACCESS_TOKEN token)
{
	CloseHandle(token);
}

VOID ExFreePool(VOID* ptr)
{
	Microsoft::P4VFS::FileCore::GFree(ptr);
}

NTSTATUS SeQueryInformationToken(PACCESS_TOKEN token, TOKEN_INFORMATION_CLASS tokenInformationClass, PVOID outTokenInformation)
{
	if (outTokenInformation && tokenInformationClass == TokenElevationType)
	{
		DWORD dwTokenInformationWritten = 0;
		LPVOID tokenInformation = Microsoft::P4VFS::FileCore::GAlloc(sizeof(TOKEN_ELEVATION_TYPE));
		if (GetTokenInformation(token, tokenInformationClass, tokenInformation, sizeof(TOKEN_ELEVATION_TYPE), &dwTokenInformationWritten) && 
		    dwTokenInformationWritten == sizeof(TOKEN_ELEVATION_TYPE))
		{
			*(PVOID*)outTokenInformation = tokenInformation;
			return STATUS_SUCCESS;	
		}
		Microsoft::P4VFS::FileCore::GFree(tokenInformation);
	}
	return STATUS_UNSUCCESSFUL;
}

#include "DriverCore.h"
#include "DriverCore.c"

P4VFS_FLT_CONTEXT g_FltContext = {0};

using namespace Microsoft::P4VFS::FileCore;
using namespace Microsoft::P4VFS::TestCore;

static void InternalTestDriverReset(const TestContext& context)
{
	ZeroMemory(&g_FltContext, sizeof(g_FltContext));
	g_FltContext.pDriverObject	= (DRIVER_OBJECT*)&g_FltContext;
	g_FltContext.pFilter = &g_FltContext;
	g_FltContext.pServiceServerPort = &g_FltContext;	
	g_FltContext.pServiceClientPort = &g_FltContext;
	g_FltContext.pControlServerPort = &g_FltContext;

	FltGetFileNameInformation = P4VFS_DEFAULT_FUNCTION_NTSTATUS();
	FltParseFileNameInformation = P4VFS_DEFAULT_FUNCTION_NTSTATUS();
	FltReleaseFileNameInformation = P4VFS_DEFAULT_ACTION();
	FltGetRequestorSessionId = [](PFLT_CALLBACK_DATA, PULONG session) -> NTSTATUS { *session = 1; return STATUS_SUCCESS; };
	FltSendMessage = P4VFS_DEFAULT_FUNCTION_NTSTATUS();
	FltFsControlFile = P4VFS_DEFAULT_FUNCTION_NTSTATUS();
	FltCreateFileEx = P4VFS_DEFAULT_FUNCTION_NTSTATUS();
	FltCreateFileEx2 = P4VFS_DEFAULT_FUNCTION_NTSTATUS();
	FltObjectDereference = P4VFS_DEFAULT_FUNCTION_NTSTATUS();
	FltClose = P4VFS_DEFAULT_FUNCTION_NTSTATUS();
	FltQueryInformationFile = P4VFS_DEFAULT_FUNCTION_NTSTATUS();
	FltSetInformationFile = P4VFS_DEFAULT_FUNCTION_NTSTATUS();
	FltGetVolumeFromInstance = P4VFS_DEFAULT_FUNCTION_NTSTATUS();
	FltGetVolumeName = P4VFS_DEFAULT_FUNCTION_NTSTATUS();
	FltGetVolumeFromFileObject = P4VFS_DEFAULT_FUNCTION_NTSTATUS();
	FltGetVolumeInstanceFromName = P4VFS_DEFAULT_FUNCTION_NTSTATUS();

	ExAllocatePoolZero = [](POOL_TYPE, SIZE_T s, ULONG) -> PVOID { return GAlloc(s); };
	ExFreePoolWithTag = [](PVOID p, ULONG) -> VOID { GFree(p); };
	ExAcquireFastMutex = P4VFS_DEFAULT_ACTION();
	ExReleaseFastMutex = P4VFS_DEFAULT_ACTION();
};

static UNICODE_STRING CStrToUnicodeString(const WCHAR* text)
{
	UNICODE_STRING s = {0};
	if (text != nullptr)
	{
		s.Buffer = const_cast<PWSTR>(text);
		s.Length = static_cast<USHORT>(StringInfo::Strlen(text)*sizeof(WCHAR));
		s.MaximumLength = s.Length+sizeof(WCHAR);
	}
	return s;
}

void TestDriverUnicodeString(const TestContext& context)
{
	const UCHAR sentinalByte = 0xCD;
	const WCHAR* shortFilePath = L"C:\\memory.dmp";
	const WCHAR* typicalFilePath = L"C:\\depot\\tools\\dev\\source\\Hammer\\Hammer.Interfaces\\BaseClasses\\Launcher\\ApplicationDescription.cs";
	const WCHAR* veryLongFilePath = L"C:\\depot\\tools\\dev\\external\\packages\\thirdparty\\microsoft\\Windows Kits\\10\\References\\10.0.17134.0\\Windows.ApplicationModel.Activation.WebUISearchActivatedEventsContract\\1.0.0.0\\zh-hans\\2019\\Enterprise\\VSSDK\\VisualStudioIntegration\\Common\\Source\\CPP\\VSL\\VSLArcitecture_files\\Microsoft Azure Tools\\Visual Studio 16.0\\2.9\\RemoteDebuggerConnector\\Connector\\MSBuild\\Microsoft\\Microsoft.NET.Build.Extensions\\net471\\Windows.ApplicationModel.Activation.WebUISearchActivatedEventsContract\\References\\Windows.ApplicationModel.CommunicationBlocking.CommunicationBlockingContract\\2.0.0.0\\Windows.ApplicationModel.CommunicationBlocking.CommunicationBlockingContract.WinMD\\Windows.ApplicationModel.Background.BackgroundAlarmApplicationContract.xml";

	auto AssertCopyAssignUnicodeString = [&](const WCHAR* srcText, const ULONG dstPadding = 0, const LONG srcLength = -1) -> void 
	{
		InternalTestDriverReset(context);
		const LONG srcTextLength = srcLength < 0 ? LONG(StringInfo::Strlen(srcText)) : srcLength;
		const ULONG srcTextSizeBytes = ULONG(srcTextLength*sizeof(WCHAR));
		const ULONG dstTextSizeBytes = srcTextSizeBytes+sizeof(WCHAR);
		Array<UCHAR> dstBuffer(sizeof(P4VFS_UNICODE_STRING)+dstPadding+dstTextSizeBytes+sizeof(sentinalByte), 0);
		dstBuffer[dstBuffer.size()-sizeof(sentinalByte)] = sentinalByte;
		P4VFS_UNICODE_STRING* dstString = (P4VFS_UNICODE_STRING*)dstBuffer.data();

		Assert(P4vfsCopyAssignUnicodeString(dstString, dstBuffer.data()+sizeof(P4VFS_UNICODE_STRING)+dstPadding, dstTextSizeBytes, srcText, srcTextSizeBytes) == STATUS_SUCCESS);

		Assert(StringInfo::Strncmp(dstString->c_str(), srcText, srcTextLength) == 0);
		Assert(StringInfo::Strlen(dstString->c_str()) == srcTextLength);
		Assert(dstString->c_str()[srcTextLength] == L'\0');
		Assert(dstString->sizeBytes == dstTextSizeBytes);
		Assert(dstBuffer[dstBuffer.size()-1] == sentinalByte);
	};

	AssertCopyAssignUnicodeString(typicalFilePath);
	AssertCopyAssignUnicodeString(typicalFilePath, 39);
	AssertCopyAssignUnicodeString(typicalFilePath, 15);
	AssertCopyAssignUnicodeString(veryLongFilePath);
	AssertCopyAssignUnicodeString(veryLongFilePath, 51);
	AssertCopyAssignUnicodeString(veryLongFilePath);
	AssertCopyAssignUnicodeString(veryLongFilePath, 51, 25);
	AssertCopyAssignUnicodeString(shortFilePath);
	AssertCopyAssignUnicodeString(shortFilePath, 17);
	AssertCopyAssignUnicodeString(L"");
	AssertCopyAssignUnicodeString(L"", 19);
	AssertCopyAssignUnicodeString(L"", 19, 0);

	auto AssertUserModeResolveFile = [&](const WCHAR* srcName, const WCHAR* srcVolume) -> void 
	{
		InternalTestDriverReset(context);
		FLT_CALLBACK_DATA fltCallbackData = {0};
		FLT_RELATED_OBJECTS fltObjects = {0};
		FLT_FILE_NAME_INFORMATION fltFileNameInfo = {0};
		fltFileNameInfo.Name = CStrToUnicodeString(srcName);
		fltFileNameInfo.Volume = CStrToUnicodeString(srcVolume);
		
		FltSendMessage = [&](PFLT_FILTER, PFLT_PORT, PVOID requestMsgData, ULONG requestMsgSize, PVOID replyMsgData, PULONG replyMsgSize, PLARGE_INTEGER) -> NTSTATUS
		{
			P4VFS_SERVICE_MSG* requestMsg = (P4VFS_SERVICE_MSG*)requestMsgData;
			Assert(requestMsg != nullptr);
			Assert(requestMsgSize >= sizeof(P4VFS_SERVICE_MSG));
			Assert(requestMsgSize <= sizeof(P4VFS_SERVICE_REQ_BUFFER));
			Assert(requestMsg->size == requestMsgSize);
			Assert(requestMsg->operation == P4VFS_SERVICE_RESOLVE_FILE);
			Assert(StringInfo::Strcmp(requestMsg->resolveFile.dataName.c_str(), srcName) == 0);
			Assert(StringInfo::Strcmp(requestMsg->resolveFile.volumeName.c_str(), srcVolume) == 0);

			P4VFS_SERVICE_REPLY* replyMsg = (P4VFS_SERVICE_REPLY*)replyMsgData;
			Assert(replyMsg != nullptr);
			Assert(replyMsgSize != nullptr);
			Assert(*replyMsgSize == sizeof(P4VFS_SERVICE_REPLY));
			replyMsg->requestResult = STATUS_SUCCESS;
			return STATUS_SUCCESS;
		};

		Assert(P4vfsUserModeResolveFile(&fltCallbackData, &fltObjects, &fltFileNameInfo) == STATUS_SUCCESS);
	};

	AssertUserModeResolveFile(shortFilePath, L"\\Device\\HarddiskVolume1\\");
	AssertUserModeResolveFile(typicalFilePath, L"\\Device\\HarddiskVolume1\\");
	AssertUserModeResolveFile(veryLongFilePath, L"C:\\");
}

