// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "FileOperations.h"
#include "FileCore.h"
#include "FileAssert.h"
#include "SettingManager.h"

namespace Microsoft {
namespace P4VFS {
namespace FileOperations {

class AutoFltFileHandle : FileCore::NonCopyable<AutoFltFileHandle>
{
public:
	AutoFltFileHandle(const P4VFS_FLT_FILE_HANDLE& handle) : 
		m_hHandle(handle) 
	{}

	~AutoFltFileHandle() 
	{ 
		Close(); 
	}

	HANDLE Handle() const 
	{ 
		return m_hHandle.fileHandle;
	}

	P4VFS_FLT_FILE_HANDLE FileHandle() const 
	{ 
		return m_hHandle;
	}

	P4VFS_FLT_FILE_HANDLE* FileHandlePtr() 
	{ 
		return &m_hHandle;
	}
	
	bool IsValid() const
	{
		return m_hHandle.fileHandle != INVALID_HANDLE_VALUE && m_hHandle.fileHandle != NULL;
	}

	HRESULT Close()
	{
		HRESULT hr = FileOperations::CloseReparsePointFile(m_hHandle);
		m_hHandle = {0};
		return hr;
	}

private:
	P4VFS_FLT_FILE_HANDLE m_hHandle;
};

HRESULT 
RestrictFileTimeChange(
	HANDLE hFile
	)
{
	// From MSDN: To prevent file operations using the given handle from modifying the 
	// last access time and or last modified time, call SetFileTime immediately after 
	// opening the file handle and pass a FILETIME structure whose dwLowDateTime and 
	// dwHighDateTime members are both set to 0xFFFFFFFF.
	FILETIME changeRestrictedFileTime = { 0xFFFFFFFF,  0xFFFFFFFF };
	if (!SetFileTime(hFile, NULL, &changeRestrictedFileTime, &changeRestrictedFileTime))
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	return S_OK;
}

bool 
SleepAndVerifyFileExists(
	const WCHAR* filePath,
	int32_t totalMilisecondsToWait
	) 
{
	const UINT WAIT_TIME = 16;
	const UINT DEFAULT_WAIT_FOR_FILE_RESOLUTION = 2000;

	UINT iterationTime = totalMilisecondsToWait >= 0 ? UINT(totalMilisecondsToWait) : DEFAULT_WAIT_FOR_FILE_RESOLUTION;
	UINT iterationCount = iterationTime / WAIT_TIME;
	bool fileVerified = false;
	for (UINT i = 0; i < iterationCount && !fileVerified; ++i)
	{
		if (!FileCore::FileInfo::IsRegular(filePath))
		{
			Sleep(WAIT_TIME);
		}
		else
		{
			fileVerified = true;
		}
	}
	return fileVerified;
}

HRESULT 
GetReparseDataSize( 
	_In_ HANDLE handle,
	_Out_ DWORD* dwReparseBufferSize
	) 
{
	if (dwReparseBufferSize == nullptr)
	{
		return E_POINTER;
	}

	*dwReparseBufferSize = 0;

	// Get the reparse header that has the ID we want to remove
	DWORD dwUnusedBytesReturned = 0;
	REPARSE_GUID_DATA_BUFFER reparseHeader = {0};   // get just the header using this

	BOOL ioControlResult = DeviceIoControl(
								handle,                
								FSCTL_GET_REPARSE_POINT,    
								NULL,                       
								0,                          
								&reparseHeader,         
								sizeof(reparseHeader),
								&dwUnusedBytesReturned,           
								NULL                     
								);

	// We expect an error - the error should be failed to copy all the reparse data
	if (!ioControlResult)
	{
		HRESULT expectedError = HRESULT_FROM_WIN32(GetLastError());
		if (expectedError != S_OK && expectedError != HRESULT_FROM_WIN32(ERROR_MORE_DATA))
		{
			return expectedError;
		}
	}

	*dwReparseBufferSize = reparseHeader.ReparseDataLength;

	return S_OK;
}

HRESULT
GetFileReparseData(
	const REPARSE_GUID_DATA_BUFFER* pReparsePoint,
	FileCore::GAllocPtr<P4VFS_REPARSE_DATA_2>& pReparseData
	)
{
	if (pReparsePoint == nullptr)
	{
		return E_POINTER;
	}

	const P4VFS_REPARSE_DATA_HEADER* reparseHeader = reinterpret_cast<const P4VFS_REPARSE_DATA_HEADER*>(pReparsePoint->GenericReparseBuffer.DataBuffer);
	if (reparseHeader == nullptr)
	{
		return E_POINTER;
	}

	switch (reparseHeader->dataVersion)
	{
		case P4VFS_REPARSE_DATA_VERSION_1:
		{
			const P4VFS_REPARSE_DATA_1* srcReparseData = reinterpret_cast<const P4VFS_REPARSE_DATA_1*>(reparseHeader);
			const size_t dstReparseDataSize = sizeof(P4VFS_REPARSE_DATA_2) + sizeof(P4VFS_REPARSE_DATA_1);
			P4VFS_REPARSE_DATA_2* dstReparseData2 = reinterpret_cast<P4VFS_REPARSE_DATA_2*>(FileCore::GAlloc(dstReparseDataSize));
			P4VFS_REPARSE_DATA_1* dstReparseData1 = reinterpret_cast<P4VFS_REPARSE_DATA_1*>(dstReparseData2+1);

			memset(dstReparseData2, 0, sizeof(P4VFS_REPARSE_DATA_2));
			memcpy(dstReparseData1, srcReparseData, sizeof(P4VFS_REPARSE_DATA_1));

			dstReparseData2->header = dstReparseData1->header;
			dstReparseData2->dataSize = dstReparseDataSize;
			dstReparseData2->majorVersion = dstReparseData1->majorVersion;
			dstReparseData2->minorVersion = dstReparseData1->minorVersion;
			dstReparseData2->buildVersion = dstReparseData1->buildVersion;
			dstReparseData2->residencyPolicy = dstReparseData1->residencyPolicy;
			dstReparseData2->populatePolicy = dstReparseData1->populatePolicy;
			dstReparseData2->fileRevision = dstReparseData1->fileRevision;

			AssignUnicodeStringReference(&dstReparseData2->depotPath, dstReparseData1->depotPath, sizeof(dstReparseData1->depotPath));
			AssignUnicodeStringReference(&dstReparseData2->depotServer, dstReparseData1->depotServer, sizeof(dstReparseData1->depotServer));
			AssignUnicodeStringReference(&dstReparseData2->depotClient, dstReparseData1->depotClient, sizeof(dstReparseData1->depotClient));
			AssignUnicodeStringReference(&dstReparseData2->depotUser, dstReparseData1->depotUser, sizeof(dstReparseData1->depotUser));

			pReparseData = FileCore::GAllocPtr<P4VFS_REPARSE_DATA_2>(dstReparseData2);
			return S_OK;
		}
		case P4VFS_REPARSE_DATA_VERSION_2:
		{
			const P4VFS_REPARSE_DATA_2* srcReparseData = reinterpret_cast<const P4VFS_REPARSE_DATA_2*>(reparseHeader);
			if (srcReparseData->dataSize < sizeof(P4VFS_REPARSE_DATA_2))
			{
				return E_INVALIDARG;
			}
			
			P4VFS_REPARSE_DATA_2* dstReparseData = reinterpret_cast<P4VFS_REPARSE_DATA_2*>(FileCore::GAlloc(srcReparseData->dataSize));
			memcpy(dstReparseData, srcReparseData, srcReparseData->dataSize);
			
			pReparseData = FileCore::GAllocPtr<P4VFS_REPARSE_DATA_2>(dstReparseData);
			return S_OK;
		}
	}		

	return E_NOTIMPL;
}

HRESULT
GetFileReparseData(
	HANDLE hFile,
	FileCore::GAllocPtr<P4VFS_REPARSE_DATA_2>& pReparseData
	)
{
	BY_HANDLE_FILE_INFORMATION fileInfo = {0};
	if (GetFileInformationByHandle(hFile, &fileInfo) == FALSE)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	DWORD dwReparseDataSize = 0;
	HRESULT hr = GetReparseDataSize(hFile, &dwReparseDataSize);
	if (FAILED(hr))
	{
		return hr;
	}

	const DWORD dwReparsePointSize = REPARSE_GUID_DATA_BUFFER_HEADER_SIZE + dwReparseDataSize;
	FileCore::GAllocPtr<BYTE> reparsePointBuffer((BYTE*)FileCore::GAlloc(dwReparsePointSize));
	memset(reparsePointBuffer.get(), 0, dwReparsePointSize);

	REPARSE_GUID_DATA_BUFFER* pReparsePoint = reinterpret_cast<REPARSE_GUID_DATA_BUFFER*>(reparsePointBuffer.get());
	pReparsePoint->ReparseTag = P4VFS_REPARSE_TAG;
	pReparsePoint->ReparseGuid = P4VFS_REPARSE_GUID;
	pReparsePoint->ReparseDataLength = (WORD)dwReparseDataSize;

	hr = GetReparsePointOfHandle(hFile, pReparsePoint, dwReparsePointSize);
	if (FAILED(hr))
	{
		return hr;
	}

	hr = GetFileReparseData(pReparsePoint, pReparseData);
	return hr;
}

HRESULT
GetFileReparseData(
	const WCHAR* fileName,
	FileCore::GAllocPtr<P4VFS_REPARSE_DATA_2>& pReparseData
	)
{
	const FileCore::String filePath = FileCore::FileInfo::FullPath(fileName);
	if (filePath.empty())
	{
		return HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
	}

	AutoFltFileHandle hFile = OpenReparsePointFile(
									filePath.c_str(),
									GENERIC_READ,
									FILE_SHARE_READ);

	if (hFile.IsValid() == false)
	{
		return HRESULT_FROM_WIN32(ERROR_OPEN_FAILED);
	}

	HRESULT hr = GetFileReparseData(hFile.Handle(), pReparseData);
	return hr;
}


HRESULT
GetReparsePointOfHandle(
	HANDLE handle,
	REPARSE_GUID_DATA_BUFFER* pReparsePoint,
	DWORD dwReparsePointSize
	)
{
	if (pReparsePoint == nullptr)
	{
		return E_POINTER;
	}

	DWORD bytesRead = 0;
	BOOL ioControlResult = DeviceIoControl(
								handle,						// the file	to set reparse on
								FSCTL_GET_REPARSE_POINT,	// setting a reparse
								NULL,						// reparse data
								0,							// reparse data	size
								pReparsePoint,				// output buffer
								dwReparsePointSize,			// output buffer size
								&bytesRead,					// bytes returned, unused
								NULL						// overlapped structure	for	async IO
								);

	if (!ioControlResult)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}
	
	if (pReparsePoint->ReparseTag != P4VFS_REPARSE_TAG)
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT
RemoveReparsePointOnHandle(
	HANDLE handle
	)
{
	HRESULT hr = S_OK;

	// Get the reparse header that has the ID we want to remove
	DWORD dwUnusedBytesReturned = 0;
	REPARSE_GUID_DATA_BUFFER reparseDataHeader = {0};

	BOOL ioControlResult = DeviceIoControl(
								handle,
								FSCTL_GET_REPARSE_POINT,
								NULL,
								0,
								&reparseDataHeader,
								sizeof(reparseDataHeader),
								&dwUnusedBytesReturned,
								NULL
								);

	// We expect an error that failed to copy all the reparse data
	if (ioControlResult == FALSE)
	{
		HRESULT expectedError = HRESULT_FROM_WIN32(GetLastError());
		if (expectedError != S_OK && expectedError != HRESULT_FROM_WIN32(ERROR_MORE_DATA))
		{
			hr = expectedError;
			return hr;
		}
	}

	reparseDataHeader.ReparseDataLength = 0;

	// Now that we have the header and ID, remove the reparse point
	ioControlResult = DeviceIoControl(
							handle,
							FSCTL_DELETE_REPARSE_POINT,
							&reparseDataHeader,
							REPARSE_GUID_DATA_BUFFER_HEADER_SIZE,
							NULL,
							0,
							&dwUnusedBytesReturned,
							NULL
							);

	if (ioControlResult == FALSE)
	{
		// An error of ERROR_NOT_A_REPARSE_POINT means that there wasn't a 
		// reparse point to begin with - that shouldn't cause an error.
		DWORD lastError = GetLastError();
		if (lastError != ERROR_NOT_A_REPARSE_POINT)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			return hr;
		}
	}

	return S_OK;
}

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
	)
{
	FileCore::Array<BYTE> reparseDataBuffer;
	reparseDataBuffer.resize(REPARSE_GUID_DATA_BUFFER_HEADER_SIZE + sizeof(P4VFS_REPARSE_DATA_2), 0);
	const uint32_t reparseDataOffset = REPARSE_GUID_DATA_BUFFER_HEADER_SIZE;

	if (depotPath)
	{
		AppendUnicodeStringReference(
			reparseDataBuffer, 
			reparseDataOffset+FIELD_OFFSET(P4VFS_REPARSE_DATA_2, depotPath), 
			depotPath);
	}
	if (depotServer)
	{
		AppendUnicodeStringReference(
			reparseDataBuffer, 
			reparseDataOffset+FIELD_OFFSET(P4VFS_REPARSE_DATA_2, depotServer), 
			depotServer);
	}
	if (depotClient)
	{
		AppendUnicodeStringReference(
			reparseDataBuffer, 
			reparseDataOffset+FIELD_OFFSET(P4VFS_REPARSE_DATA_2, depotClient), 
			depotClient);
	}
	if (depotUser)
	{
		AppendUnicodeStringReference(
			reparseDataBuffer, 
			reparseDataOffset+FIELD_OFFSET(P4VFS_REPARSE_DATA_2, depotUser), 
			depotUser);
	}

	REPARSE_GUID_DATA_BUFFER* reparsePoint = reinterpret_cast<REPARSE_GUID_DATA_BUFFER*>(reparseDataBuffer.data());
	reparsePoint->ReparseTag = P4VFS_REPARSE_TAG;
	reparsePoint->ReparseDataLength = WORD(reparseDataBuffer.size() - REPARSE_GUID_DATA_BUFFER_HEADER_SIZE);
	reparsePoint->ReparseGuid = P4VFS_REPARSE_GUID;

	P4VFS_REPARSE_DATA_2* pReparseData = reinterpret_cast<P4VFS_REPARSE_DATA_2*>(reparsePoint->GenericReparseBuffer.DataBuffer);
	pReparseData->header.dataVersion = P4VFS_REPARSE_DATA_VERSION_2;
	pReparseData->dataSize = reparsePoint->ReparseDataLength;
	pReparseData->majorVersion = majorVersion;
	pReparseData->minorVersion = minorVersion;
	pReparseData->buildVersion = buildversion;
	pReparseData->fileRevision = fileRevision;
	pReparseData->residencyPolicy = residencyPolicy;
	pReparseData->populatePolicy = populatePolicy;

	DWORD dwSetReparseBytesReturned = 0;
	if (DeviceIoControl(handle, FSCTL_SET_REPARSE_POINT, reparseDataBuffer.data(), DWORD(reparseDataBuffer.size()), NULL, 0, &dwSetReparseBytesReturned, NULL) == FALSE)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}
	return S_OK;
}

HRESULT
SetSparseFileSizeOnHandle(
	HANDLE handle,
	INT64 fileSize
	)
{
	if (fileSize <= 0)
	{
		return S_OK;
	}

	DWORD dwSetSparseBytesReturned = 0;
	if (DeviceIoControl(handle, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &dwSetSparseBytesReturned, NULL) == FALSE)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	LARGE_INTEGER fileEnd;
	fileEnd.QuadPart = fileSize;

	DWORD dwSetZeroBytesReturned = 0;
	FILE_ZERO_DATA_INFORMATION zeroInfo = {0};
	zeroInfo.BeyondFinalZero = fileEnd;
	if (DeviceIoControl(handle, FSCTL_SET_ZERO_DATA, &zeroInfo, sizeof(zeroInfo), NULL, 0, &dwSetZeroBytesReturned, NULL) == FALSE)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}
	
	if (SetFilePointer(handle, fileEnd.LowPart, &fileEnd.HighPart, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	if (SetEndOfFile(handle) == FALSE)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}
	return S_OK;
}

HRESULT
RemoveSparseFileSizeOnHandle(
	HANDLE handle
	)
{
	DWORD dwSetSparseBytesReturned = 0;
	FILE_SET_SPARSE_BUFFER sparseBuffer = {0};
	if (DeviceIoControl(handle, FSCTL_SET_SPARSE, &sparseBuffer, sizeof(sparseBuffer), NULL, 0, &dwSetSparseBytesReturned, NULL) == FALSE)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}
	return S_OK;
}

HRESULT 
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
	)
{
	HRESULT hr = S_OK;

	if (!(residencyPolicy == P4VFS_RESIDENCY_POLICY_UNDEFINED || 
		  residencyPolicy == P4VFS_RESIDENCY_POLICY_RESIDENT || 
		  residencyPolicy == P4VFS_RESIDENCY_POLICY_SYMLINK || 
		  residencyPolicy == P4VFS_RESIDENCY_POLICY_REMOVE_FILE))
	{
		return E_INVALIDARG;
	}

	const FileCore::String filePath = FileCore::FileInfo::FullPath(fileName);
	if (filePath.empty())
	{
		hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
		return hr;
	}

	if (FileCore::FileInfo::CreateFileDirectory(filePath.c_str()) == false)
	{
		hr = HRESULT_FROM_WIN32(ERROR_DIRECTORY);
		return hr;
	}

	DWORD creationDisposition = OPEN_ALWAYS;
	DWORD existingAttributes = FileCore::FileInfo::FileAttributes(filePath.c_str());
	if (existingAttributes != INVALID_FILE_ATTRIBUTES)
	{
		// Since the file exists, ensure that it is normal writable and we open to truncate it
		creationDisposition = TRUNCATE_EXISTING;
		if (!SetFileAttributes(filePath.c_str(), FILE_ATTRIBUTE_NORMAL))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			return hr;
		}
	}

	static const int32_t retryCount = std::min(std::max(1, FileCore::SettingManager::StaticInstance().m_CreateFileRetryCount.GetValue()), 20);
	static const int32_t retryWaitMs = std::min(std::max(0, FileCore::SettingManager::StaticInstance().m_CreateFileRetryWaitMs.GetValue()), 5000);

	// Open the file for exclusive write, retrying as needed.
	FileCore::AutoHandle hFile;
	for (int32_t retryIndex = 0; retryIndex < retryCount; ++retryIndex)
	{
		if (retryIndex > 0)
		{
			::Sleep(retryWaitMs);
		}

		HANDLE hWrite = CreateFile(
							filePath.c_str(), 
							GENERIC_WRITE, 
							0, 
							NULL, 
							creationDisposition, 
							FILE_FLAG_OPEN_REPARSE_POINT, 
							NULL);
		
		if (hWrite != INVALID_HANDLE_VALUE)
		{
			hFile.Reset(hWrite);
			break;
		}
	}

	if (hFile.IsValid() == false)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		if (existingAttributes != INVALID_FILE_ATTRIBUTES)
		{
			SetFileAttributes(filePath.c_str(), existingAttributes);
		}
		return hr;
	}

	hr = RestrictFileTimeChange(hFile.Handle());
	if (FAILED(hr))
	{
		hFile.Close();
		DeleteFile(filePath.c_str());
		return hr;
	}

	// Setup reparse point
	hr = SetReparsePointOnHandle(
				hFile.Handle(), 
				majorVersion,
				minorVersion,
				buildversion,
				residencyPolicy, 
				P4VFS_POPULATE_POLICY_DEPOT, 
				fileRevision,
				fileSize,
				depotPath,
				depotServer,
				depotClient,
				depotUser);

	if (FAILED(hr))
	{
		hFile.Close();
		DeleteFile(filePath.c_str());
		return hr;
	}

	// Setup sparse file size
	hr = SetSparseFileSizeOnHandle(
				hFile.Handle(),
				fileSize);

	if (FAILED(hr))
	{
		hFile.Close();
		DeleteFile(filePath.c_str());
		return hr;
	}

	hr = hFile.Close();
	if (FAILED(hr))
	{
		DeleteFile(filePath.c_str());
		return hr;
	}

	if (!SetFileAttributes(filePath.c_str(), fileAttributes | FILE_ATTRIBUTE_OFFLINE))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		DeleteFile(filePath.c_str());
		return hr;
	}

	return S_OK;
}

HRESULT
CreateSymlinkFile(
	const WCHAR* symlinkFileName,
	const WCHAR* targetFileName
)
{
	HRESULT hr = S_OK;

	const FileCore::String symlinkFilePath = FileCore::FileInfo::FullPath(symlinkFileName);
	if (symlinkFilePath.empty())
	{
		hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
		return hr;
	}

	if (FileCore::FileInfo::Exists(symlinkFilePath.c_str()) && DeleteFile(symlinkFilePath.c_str()) == FALSE)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}

	if (FileCore::FileInfo::CreateFileDirectory(symlinkFilePath.c_str()) == false)
	{
		hr = HRESULT_FROM_WIN32(ERROR_DIRECTORY);
		return hr;
	}

	FileCore::String targetPath = FileCore::StringInfo::Replace(targetFileName, TEXT("/"), TEXT("\\"));
	if (CreateSymbolicLink(symlinkFilePath.c_str(), targetPath.c_str(), SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE) == FALSE)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}

	return S_OK;
}

HRESULT
RemoveReparsePoint(
	const WCHAR* fileName
	)
{
	HRESULT hr = S_OK;

	const FileCore::String filePath = FileCore::FileInfo::FullPath(fileName);
	if (filePath.empty())
	{
		hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
		return hr;
	}

	DWORD originalFileAttributes = FileCore::FileInfo::FileAttributes(filePath.c_str());
	if (originalFileAttributes == INVALID_FILE_ATTRIBUTES)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}

	if (!SetFileAttributes(filePath.c_str(), FILE_ATTRIBUTE_NORMAL))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}

	HANDLE handle = CreateFile(
						filePath.c_str(),
						GENERIC_READ | GENERIC_WRITE,
						FILE_SHARE_READ|FILE_SHARE_WRITE,
						NULL,
						OPEN_EXISTING,
						FILE_FLAG_OPEN_REPARSE_POINT,
						NULL);

	if (handle == INVALID_HANDLE_VALUE)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		SetFileAttributes(filePath.c_str(), originalFileAttributes);
		return hr;
	}

	hr = RestrictFileTimeChange(handle);
	if (FAILED(hr))
	{
		CloseHandle(handle);
		return hr;
	}

	hr = RemoveReparsePointOnHandle(handle);
	if (FAILED(hr))
	{
		CloseHandle(handle);
		return hr;
	}

	CloseHandle(handle);

	if (!SetFileAttributes(filePath.c_str(), originalFileAttributes))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}

	return S_OK;
}

P4VFS_FLT_FILE_HANDLE
OpenReparsePointFile(
	const WCHAR* fileName,
	DWORD desiredAccess,
	DWORD shareMode
	)
{
	HRESULT hr = S_OK;
	P4VFS_FLT_FILE_HANDLE handle = {0};
	P4VFS_CONTROL_REPLY reply = {0};

	FileCore::Array<uint8_t> messageBuffer;
	messageBuffer.resize(sizeof(P4VFS_CONTROL_MSG), 0);
	{
		P4VFS_CONTROL_MSG* message = reinterpret_cast<P4VFS_CONTROL_MSG*>(messageBuffer.data());
		message->operation = P4VFS_OPERATION_OPEN_REPARSE_POINT;
		message->data.OPEN_REPARSE_POINT.accessRead = !!(desiredAccess & (GENERIC_READ|GENERIC_ALL));
		message->data.OPEN_REPARSE_POINT.accessWrite = !!(desiredAccess & (GENERIC_WRITE|GENERIC_ALL));
		message->data.OPEN_REPARSE_POINT.accessDelete = !!(desiredAccess & (DELETE|GENERIC_ALL));
		message->data.OPEN_REPARSE_POINT.shareRead = !!(shareMode & FILE_SHARE_READ);
		message->data.OPEN_REPARSE_POINT.shareWrite = !!(shareMode & FILE_SHARE_WRITE);
		message->data.OPEN_REPARSE_POINT.shareDelete = !!(shareMode & FILE_SHARE_DELETE);
	
		hr = AppendUnicodeStringReference(
				messageBuffer, 
				FIELD_OFFSET(P4VFS_CONTROL_MSG, data.OPEN_REPARSE_POINT.filePath), 
				FileCore::StringInfo::Format(L"\\??\\%s", FileCore::FileInfo::UnextendedPath(fileName).c_str()).c_str());

		if (FAILED(hr))
		{
			return handle;
		}
	}

	hr = SendDriverControlMessage(
			reinterpret_cast<P4VFS_CONTROL_MSG*>(messageBuffer.data()),
			DWORD(messageBuffer.size()),
			&reply,
			DWORD(sizeof(P4VFS_CONTROL_REPLY)));

	if (FAILED(hr))
	{
		return handle;
	}

	handle = reply.data.OPEN_REPARSE_POINT.handle;
	return handle;
}

HRESULT
CloseReparsePointFile(
	const P4VFS_FLT_FILE_HANDLE& handle
	)
{
	if (handle.fileHandle == NULL || handle.fileHandle == INVALID_HANDLE_VALUE)
	{
		return S_OK;
	}

	P4VFS_CONTROL_MSG message = {0};
	P4VFS_CONTROL_REPLY reply = {0};

	message.operation = P4VFS_OPERATION_CLOSE_REPARSE_POINT;
	message.data.CLOSE_REPARSE_POINT.handle = handle;
	
	HRESULT hr = SendDriverControlMessage(message, reply);
	if (FAILED(hr))
	{
		return hr;
	}

	return S_OK;
}

HRESULT 
PopulateFileByCopy(
	const WCHAR* dstFileName,
	const WCHAR* srcFileName
	)
{
	HRESULT hr = S_OK;

	const FileCore::String fileToPopulate = FileCore::FileInfo::FullPath(dstFileName);
	if (fileToPopulate.empty())
	{
		return HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
	}

	const FileCore::String fileToPopulateFrom = FileCore::FileInfo::FullPath(srcFileName);
	if (fileToPopulateFrom.empty())
	{
		return HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
	}

	DWORD dstFileAttributes = FileCore::FileInfo::FileAttributes(fileToPopulate.c_str());
	if (dstFileAttributes == INVALID_FILE_ATTRIBUTES)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}

	if (SetFileAttributes(fileToPopulate.c_str(), dstFileAttributes & ~FILE_ATTRIBUTE_READONLY) == FALSE)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}

	// Wait for up to 2 seconds to see if the remote file can be accessed
	if (SleepAndVerifyFileExists(fileToPopulateFrom.c_str()) == FALSE)
	{
		return E_FAIL;
	}

	// Open the file to be copied FROM
	FileCore::AutoHandle srcFile = CreateFile(
										fileToPopulateFrom.c_str(),
										GENERIC_READ,
										FILE_SHARE_READ,
										NULL,
										OPEN_EXISTING,
										FILE_ATTRIBUTE_READONLY | FILE_FLAG_SEQUENTIAL_SCAN,
										NULL);

	if (srcFile.IsValid() == false)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}

	// Open the file to copy TO
	AutoFltFileHandle dstFile = OpenReparsePointFile(
									fileToPopulate.c_str(),
									GENERIC_WRITE,
									FILE_SHARE_READ|FILE_SHARE_WRITE);

	if (dstFile.IsValid() == false)
	{
		hr = HRESULT_FROM_WIN32(ERROR_CANTOPEN);
		return hr;
	}

	hr = RestrictFileTimeChange(dstFile.Handle());
	if (FAILED(hr))
	{
		return hr;
	}

	static const UINT BYTES_TO_READ = 4096;
	BYTE buffer[BYTES_TO_READ] = {0};
	DWORD nBytesRead = 0;
	DWORD nBytesWritten = 0;

	do
	{    
		if (ReadFile(srcFile.Handle(), buffer, BYTES_TO_READ, &nBytesRead, NULL) == FALSE)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			return hr;
		}

		if (WriteFile(dstFile.Handle(), buffer, nBytesRead, &nBytesWritten, NULL) == FALSE)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			return hr;
		}

	} while (nBytesRead != 0);

	// In case the file already contained data, set this as the new end of the file
	if (!SetEndOfFile(dstFile.Handle()))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}
	
	hr = RemoveReparsePointOnHandle(dstFile.Handle());
	if (FAILED(hr))
	{
		return hr;
	}

	hr = RemoveSparseFileSizeOnHandle(dstFile.Handle());
	if (FAILED(hr))
	{
		return hr;
	}

	srcFile.Close();
	dstFile.Close();

	if (!SetFileAttributes(fileToPopulate.c_str(), dstFileAttributes & ~(FILE_ATTRIBUTE_OFFLINE)))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}

	return S_OK;
}

HRESULT 
PopulateFileByMove(
	const WCHAR* dstFileName,
	const WCHAR* srcFileName
	)
{
	HRESULT hr = S_OK;

	const FileCore::String fileToPopulate = FileCore::FileInfo::FullPath(dstFileName);
	if (fileToPopulate.empty())
	{
		return HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
	}

	const FileCore::String fileToPopulateFrom = FileCore::FileInfo::FullPath(srcFileName);
	if (fileToPopulateFrom.empty())
	{
		return HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
	}

	DWORD dstFileAttributes = FileCore::FileInfo::FileAttributes(fileToPopulate.c_str());
	if (dstFileAttributes == INVALID_FILE_ATTRIBUTES)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}

	if (SetFileAttributes(fileToPopulate.c_str(), dstFileAttributes & ~FILE_ATTRIBUTE_READONLY) == FALSE)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}

	// Wait for up to 2 seconds to see if the remote file can be accessed
	if (SleepAndVerifyFileExists(fileToPopulateFrom.c_str()) == FALSE)
	{
		return E_FAIL;
	}

	// Make sure the fileToPopulateFrom has compatible attributes that we desire on destination fileToPopulate
	if (SetFileAttributes(fileToPopulateFrom.c_str(), dstFileAttributes & ~(FILE_ATTRIBUTE_OFFLINE | FILE_ATTRIBUTE_READONLY)) == FALSE)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}

	// Get the destination fileToPopulate file access and write times.
	// These must remain constant as they will be carried over with the MoveFileEx
	FileCore::AutoHandle dstFile = CreateFile(
										fileToPopulate.c_str(), 
										GENERIC_READ, 
										FILE_SHARE_READ, 
										NULL, 
										OPEN_EXISTING, 
										FILE_FLAG_OPEN_REPARSE_POINT, 
										NULL);
	
	if (dstFile.Handle() == INVALID_HANDLE_VALUE)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}
	
	FILETIME dstCreationTime;
	FILETIME dstAccessTime;
	FILETIME dstWriteTime;
	if (GetFileTime(dstFile.Handle(), &dstCreationTime, &dstAccessTime, &dstWriteTime) == FALSE)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}

	dstFile.Close();

	// Open the source fileToPopulateFrom file and set the file times to equal the destination fileToPopulate
	FileCore::AutoHandle srcFile = CreateFile(
										fileToPopulateFrom.c_str(), 
										GENERIC_WRITE | GENERIC_READ, 
										FILE_SHARE_READ, 
										NULL, 
										OPEN_EXISTING, 
										FILE_FLAG_OPEN_REPARSE_POINT, 
										NULL);

	if (srcFile.Handle() == INVALID_HANDLE_VALUE)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}

	if (SetFileTime(srcFile.Handle(), &dstCreationTime, &dstAccessTime, &dstWriteTime) == FALSE)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}

	srcFile.Close();

	// Move the fileToPopulateFrom over the placeholder.
	if (MoveFileEx(fileToPopulateFrom.c_str(), fileToPopulate.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == FALSE)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}

	// Make sure the fileToPopulateFrom has the final attributes we desire on destination fileToPopulate
	if (SetFileAttributes(fileToPopulate.c_str(), dstFileAttributes & ~(FILE_ATTRIBUTE_OFFLINE)) == FALSE)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}

	return S_OK;
}

HRESULT 
PopulateFileByStream(
	const WCHAR* dstFileName,
	FileCore::FileStream* srcFileStream
	)
{
	HRESULT hr = S_OK;

	const FileCore::String fileToPopulate = FileCore::FileInfo::FullPath(dstFileName);
	if (fileToPopulate.empty())
	{
		return HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
	}

	DWORD dstFileAttributes = FileCore::FileInfo::FileAttributes(fileToPopulate.c_str());
	if (dstFileAttributes == INVALID_FILE_ATTRIBUTES)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}

	if (SetFileAttributes(fileToPopulate.c_str(), dstFileAttributes & ~FILE_ATTRIBUTE_READONLY) == FALSE)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}

	FileCore::FileStream* fileToPopulateFrom = srcFileStream;
	if (fileToPopulateFrom == nullptr || fileToPopulateFrom->CanRead() == false)
	{
		hr = HRESULT_FROM_WIN32(ERROR_CANTREAD);
		return hr;
	}

	// Open the file to copy TO
	AutoFltFileHandle dstFile = OpenReparsePointFile(
									fileToPopulate.c_str(),
									GENERIC_WRITE,
									FILE_SHARE_READ|FILE_SHARE_WRITE);

	if (dstFile.IsValid() == false)
	{
		hr = HRESULT_FROM_WIN32(ERROR_CANTOPEN);
		return hr;
	}

	hr = RestrictFileTimeChange(dstFile.Handle());
	if (FAILED(hr))
	{
		return hr;
	}

	UINT64 nBytesWritten = 0;
	hr = fileToPopulateFrom->Read(dstFile.Handle(), &nBytesWritten);
	if (FAILED(hr))
	{
		return hr;
	}

	// In case the file already contained data, set this as the new end of the file
	if (!SetEndOfFile(dstFile.Handle()))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}
	
	hr = RemoveReparsePointOnHandle(dstFile.Handle());
	if (FAILED(hr))
	{
		return hr;
	}

	hr = RemoveSparseFileSizeOnHandle(dstFile.Handle());
	if (FAILED(hr))
	{
		return hr;		
	}

	dstFile.Close();

	if (!SetFileAttributes(fileToPopulate.c_str(), dstFileAttributes & ~(FILE_ATTRIBUTE_OFFLINE)))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}

	return S_OK;
}

HRESULT
PopulateFile(
	const WCHAR* dstFileName,
	const WCHAR* srcFileName,
	BYTE populateMethod
	)
{
	if (populateMethod == P4VFS_POPULATE_METHOD_COPY)
	{
		return PopulateFileByCopy(dstFileName, srcFileName);
	}

	if (populateMethod == P4VFS_POPULATE_METHOD_MOVE)
	{
		return PopulateFileByMove(dstFileName, srcFileName);
	}
	return E_FAIL;
}

HRESULT 
PopulateFile(
	const WCHAR* dstFileName,
	FileCore::FileStream* srcFileStream
	)
{
	return PopulateFileByStream(dstFileName, srcFileStream); 
}

BOOL
IsSessionIdToken(
	HANDLE hToken
	)
{
	DWORD sessionId = 0;
	DWORD resultSize = 0;
	if (GetTokenInformation(hToken, TokenSessionId, &sessionId, sizeof(sessionId), &resultSize) && resultSize == sizeof(sessionId))
	{
		return TRUE;
	}
	return FALSE;
}

HRESULT
ImpersonateFileAppend(
	const WCHAR* fileName,
	const WCHAR* text,
	const FileCore::UserContext* context
	)
{
	HRESULT hr = ImpersonateLoggedOnUser(context);
	if (FAILED(hr))
	{
		return hr;
	}

	hr = FileAppend(fileName, text);

	RevertToSelf();
	return hr;
}

HRESULT
FileAppend(
	const WCHAR* fileName,
	const WCHAR* text
	)
{
	FileCore::FileInfo::CreateFileDirectory(fileName);

	HANDLE hFile = CreateFile(
						fileName, 
						FILE_APPEND_DATA, 
						FILE_SHARE_READ|FILE_SHARE_WRITE, 
						NULL, OPEN_ALWAYS, 
						FILE_FLAG_WRITE_THROUGH, 
						NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	DWORD dwBytesWritten = 0;
	FileCore::AString ansiText = FileCore::StringInfo::ToAnsi(text);
	WriteFile(hFile, ansiText.c_str(), DWORD(ansiText.length()), &dwBytesWritten, NULL);
	
	CloseHandle(hFile);
	return S_OK;
}

HRESULT
ImpersonateLoggedOnUser(
	const FileCore::UserContext* context
	)
{
	FileCore::AutoHandle hToken = GetLoggedOnUserToken(context);
	if (hToken.IsValid() == false)
	{
		return HRESULT_FROM_WIN32(ERROR_INVALID_TOKEN);
	}

	if (::ImpersonateLoggedOnUser(hToken.Handle()) == FALSE)
	{
		return HRESULT_FROM_WIN32(ERROR_CANNOT_IMPERSONATE);
	}

	return S_OK;
}

FileCore::String
GetImpersonatedUserName(
	const FileCore::UserContext* context,
	HRESULT* pHR
	)
{
	HRESULT tmpHR = S_OK;
	HRESULT& hr = pHR ? *pHR : tmpHR;

	hr = ImpersonateLoggedOnUser(context);
	if (FAILED(hr))
	{
		return FileCore::String();
	}

	FileCore::String userName = FileOperations::GetUserName(&hr);

	RevertToSelf();
	return userName;
}

FileCore::String
GetUserName(
	HRESULT* pHR
	)
{
	HRESULT tmpHR = S_OK;
	HRESULT& hr = pHR ? *pHR : tmpHR;

	wchar_t userName[512] = {0};
	DWORD userNameSize = _countof(userName);
	if (::GetUserName(userName, &userNameSize) == FALSE)
	{
		hr = HRESULT_FROM_WIN32(ERROR_BAD_USERNAME);
	}

	return FileCore::String(userName);
}

FileCore::String
GetSessionUserName(
	DWORD sessionId,
	HRESULT* pHR
	)
{
	HRESULT tmpHR = S_OK;
	HRESULT& hr = pHR ? *pHR : tmpHR;

	WCHAR* wtsUserName = NULL;
	DWORD wtsUserNameLen = 0;
	if (WTSQuerySessionInformation(NULL, sessionId, WTSUserName, &wtsUserName, &wtsUserNameLen) == FALSE || wtsUserName == NULL)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		return FileCore::String();
	}
	
	FileCore::String dstUserName(wtsUserName);
	WTSFreeMemory(wtsUserName);
	return dstUserName;
}

HRESULT
GetImpersonatedEnvironmentStrings(
	const WCHAR* srcText,
	WCHAR* dstText,
	DWORD dstTextSize,
	const FileCore::UserContext* context
	)
{
	if (IsCurrentProcessUserContext(context))
	{
		return GetExpandedEnvironmentStrings(srcText, dstText, dstTextSize);
	}

	FileCore::AutoHandle hToken = GetLoggedOnUserToken(context);
	if (hToken.IsValid() == false)
	{
		return HRESULT_FROM_WIN32(ERROR_INVALID_TOKEN);
	}

	if (ExpandEnvironmentStringsForUser(hToken.Handle(), srcText, dstText, dstTextSize) == FALSE)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	if (FileCore::StringInfo::Contains(dstText, L'%'))
	{
		return HRESULT_FROM_WIN32(ERROR_NOT_SUBSTED);
	}

	return S_OK;
}

FileCore::String
GetImpersonatedEnvironmentStrings(
	const WCHAR* srcText,
	const FileCore::UserContext* context,
	HRESULT* pHR
	)
{
	HRESULT tmpHR = S_OK;
	HRESULT& hr = pHR ? *pHR : tmpHR;

	wchar_t dstText[2048] = {0};
	hr = GetImpersonatedEnvironmentStrings(srcText, dstText, _countof(dstText), context);
	if (FAILED(hr))
	{
		return FileCore::String();
	}

	return FileCore::String(dstText);
}

bool
IsSystemUserContext(
	const FileCore::UserContext* context,
	HRESULT* pHR
	)
{
	HRESULT tmpHR = S_OK;
	HRESULT& hr = pHR ? *pHR : tmpHR;

	FileCore::AutoHandle hUserToken = GetLoggedOnUserToken(context);
	if (hUserToken.IsValid() == false)
	{
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_TOKEN);
		return false;
	}

	DWORD dwTokenBytes = 0;
	if (GetTokenInformation(hUserToken.Handle(), TOKEN_INFORMATION_CLASS::TokenUser, NULL, 0, &dwTokenBytes) == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		FileCore::GAllocPtr<TOKEN_USER> tokenUser((TOKEN_USER*)FileCore::GAlloc(dwTokenBytes));
		if (GetTokenInformation(hUserToken.Handle(), TOKEN_INFORMATION_CLASS::TokenUser, tokenUser.get(), dwTokenBytes, &dwTokenBytes))
		{
			if (IsWellKnownSid(tokenUser->User.Sid, WELL_KNOWN_SID_TYPE::WinLocalSystemSid) ||
				IsWellKnownSid(tokenUser->User.Sid, WELL_KNOWN_SID_TYPE::WinLocalServiceSid) ||
				IsWellKnownSid(tokenUser->User.Sid, WELL_KNOWN_SID_TYPE::WinNetworkServiceSid))
			{
				return true;
			}
		}
	}
	return false;
}

bool
IsCurrentProcessUserContext(
	const FileCore::UserContext* context
	)
{
	if (context != nullptr && context->m_SessionId == 0 && context->m_ProcessId == GetCurrentProcessId())
	{
		return true;
	}
	return false;
}

HRESULT
GetExpandedEnvironmentStrings(
	const WCHAR* srcText,
	WCHAR* dstText,
	DWORD dstTextSize
	)
{
	if (ExpandEnvironmentStringsW(srcText, dstText, dstTextSize) == 0)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	if (FileCore::StringInfo::Contains(dstText, L'%'))
	{
		return HRESULT_FROM_WIN32(ERROR_NOT_SUBSTED);
	}
	
	return S_OK;
}

FileCore::String
GetExpandedEnvironmentStrings(
	const WCHAR* srcText,
	HRESULT* pHR
	)
{
	HRESULT tmpHR = S_OK;
	HRESULT& hr = pHR ? *pHR : tmpHR;

	wchar_t dstText[2048] = {0};
	hr = GetExpandedEnvironmentStrings(srcText, dstText, _countof(dstText));
	if (FAILED(hr))
	{
		return FileCore::String();
	}

	return FileCore::String(dstText);
}

HANDLE
GetLoggedOnUserToken(
	const FileCore::UserContext* context
	)
{
	if (context != nullptr)
	{
		if (context->m_SessionId != 0)
		{
			HANDLE hUserToken = INVALID_HANDLE_VALUE;
			if (WTSQueryUserToken(context->m_SessionId, &hUserToken))
			{
				return hUserToken;
			}
		}

		if (context->m_ProcessId != 0)
		{
			FileCore::AutoHandle hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, context->m_ProcessId);
			if (hProcess.IsValid())
			{
				HANDLE hProcessToken = INVALID_HANDLE_VALUE;
				if (OpenProcessToken(hProcess.Handle(), MAXIMUM_ALLOWED, &hProcessToken))
				{
					return hProcessToken;
				}
			}
		}
	}
	return GetPreferredLoggedOnUserToken();
}

HANDLE
GetPreferredLoggedOnUserToken(
	)
{
	HANDLE hToken = INVALID_HANDLE_VALUE;

	DWORD dwConsoleSessionId = WTSGetActiveConsoleSessionId();
	if (GetSessionUserName(dwConsoleSessionId).empty() == false)
	{
		if (WTSQueryUserToken(dwConsoleSessionId, &hToken))
			return hToken; 
	}

	struct UserSession
	{
		DWORD Priority;
		DWORD SessionId;
		enum
		{
			FlagUnlocked		= (1<<0),
			FlagDisconnected	= (1<<1),
			FlagConnected		= (1<<2),
			FlagActive			= (1<<3),
		};
	};

	FileCore::Array<UserSession> users;
	WTS_SESSION_INFO_1* pSessionInfo = nullptr;
	DWORD dwSessionInfoCount = 0;
	DWORD dwLevel = 1;

#pragma warning(push)
#pragma warning(disable : 6387)
	if (WTSEnumerateSessionsEx(WTS_CURRENT_SERVER_HANDLE, &dwLevel, 0, &pSessionInfo, &dwSessionInfoCount) && pSessionInfo != nullptr)
#pragma warning(pop)
	{
		for (DWORD dwSessionInfoIndex = 0; dwSessionInfoIndex < dwSessionInfoCount; ++dwSessionInfoIndex)
		{
			const WTS_SESSION_INFO_1& si = pSessionInfo[dwSessionInfoIndex];
			if (si.pUserName == nullptr)
			{
				continue;
			}
			if (si.SessionId == 0)
			{
				continue;
			}

			FileCore::AutoHandle hSessionUserToken;
			if (WTSQueryUserToken(si.SessionId, hSessionUserToken.HandlePtr()) == FALSE)
			{
				continue;
			}

			UserSession user;
			user.Priority = 0;
			user.SessionId = si.SessionId;

			WTSINFOEX* pInfoEx = nullptr;
			DWORD dwInfoExSize = 0;
			if (WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, si.SessionId, WTSSessionInfoEx, (LPWSTR*)&pInfoEx, &dwInfoExSize) && pInfoEx != nullptr)
			{
				if (dwInfoExSize == sizeof(WTSINFOEX))
				{
					if (pInfoEx->Data.WTSInfoExLevel1.SessionFlags & LONG(WTS_SESSIONSTATE_UNLOCK))
					{
						user.Priority |= UserSession::FlagUnlocked;
					}
				}
				WTSFreeMemory(pInfoEx);
			}

			switch (si.State)
			{
				case WTSActive:			user.Priority |= UserSession::FlagActive;			break;
				case WTSConnected:		user.Priority |= UserSession::FlagConnected;		break;
				case WTSDisconnected:	user.Priority |= UserSession::FlagDisconnected;		break;
			}

			users.push_back(user);
		}

		WTSFreeMemoryEx(WTSTypeSessionInfoLevel1, pSessionInfo, dwSessionInfoCount);
	}

	if (users.empty() == false)
	{
		// Sort the logged-on user sessions so that highest Priority is first
		std::stable_sort(users.begin(), users.end(), [](const UserSession& a, const UserSession& b) -> bool { return a.Priority > b.Priority; });
		for (size_t userIndex = 0; userIndex < users.size(); ++userIndex)
		{
			if (WTSQueryUserToken(users[userIndex].SessionId, &hToken))
			{
				return hToken;
			}
		}
	}

	// If we've failed this far, then this thread is likely already impersonating a logged on user
	if (OpenThreadToken(GetCurrentThread(), TOKEN_ALL_ACCESS, TRUE, &hToken))
	{
		if (IsSessionIdToken(hToken))
		{
			return hToken;
		}

		CloseHandle(hToken);
		hToken = INVALID_HANDLE_VALUE;
	}

	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken))
	{
		if (IsSessionIdToken(hToken))
		{
			return hToken;
		}

		CloseHandle(hToken);
		hToken = INVALID_HANDLE_VALUE;
	}
	return INVALID_HANDLE_VALUE;
}

HRESULT
CreateProcessImpersonated(
	const WCHAR* commandLine,
	const WCHAR* currentDirectory,
	BOOL waitForExit,
	FileCore::String* stdOutput,
	const FileCore::UserContext* context
	)
{
	if (FileCore::StringInfo::IsNullOrEmpty(commandLine))
	{
		return HRESULT_FROM_WIN32(ERROR_BAD_COMMAND);
	}

	FileCore::AutoHandle hUserToken = GetLoggedOnUserToken(context);
	if (hUserToken.IsValid() == false)
	{
		return HRESULT_FROM_WIN32(ERROR_INVALID_TOKEN);
	}

	FileCore::Process::ExecuteFlags::Enum flags = FileCore::Process::ExecuteFlags::HideWindow;
	if (waitForExit)
	{
		flags |= FileCore::Process::ExecuteFlags::WaitForExit;
	}
	if (stdOutput != nullptr)
	{
		flags |= FileCore::Process::ExecuteFlags::StdOut;
	}

	FileCore::Process::ExecuteResult result = FileCore::Process::Execute(commandLine, currentDirectory, flags, hUserToken.Handle());
	if (stdOutput != nullptr)
	{
		stdOutput->assign(FileCore::StringInfo::ToWide(result.m_StdOut));
	}
	return result.m_HR;
}

HRESULT
SendDriverControlMessage(
		P4VFS_CONTROL_MSG& message,
		P4VFS_CONTROL_REPLY& reply
	)
{
	return SendDriverControlMessage(
		&message,
		sizeof(message),
		&reply,
		sizeof(reply));
}

HRESULT
SendDriverControlMessage(
	P4VFS_CONTROL_MSG* message,
	DWORD messageSize,
	P4VFS_CONTROL_REPLY* reply,
	DWORD replySize,
	DWORD* replySizeWritten
	)
{
	if (message == nullptr || reply == nullptr)
	{
		return E_POINTER;
	}

	if (messageSize < sizeof(P4VFS_CONTROL_MSG) || replySize < sizeof(P4VFS_CONTROL_REPLY))
	{
		return E_INVALIDARG;
	}

	FileCore::AutoHandle hDriverPort;

	HRESULT hr = FilterConnectCommunicationPort(
					P4VFS_CONTROL_PORT_NAME,            // port name
					0,                                  // unused, pass 0
					NULL,                               // context
					0,                                  // size of context
					NULL,                               // security context
					hDriverPort.HandlePtr()             // connected port
					);

	if (FAILED(hr))
	{
		return hr;
	}

	DWORD dwReplyWrittenTmp = 0;
	if (replySizeWritten == nullptr)
	{
		replySizeWritten = &dwReplyWrittenTmp;
	}

	hr = FilterSendMessage(
			hDriverPort.Handle(),
			message,
			messageSize,
			reply,
			replySize,
			replySizeWritten
			);

	if (FAILED(hr))
	{
		return hr;
	}
	return S_OK;
}

HRESULT
SetDriverTraceEnabled(
	DWORD channels
	)
{
	P4VFS_CONTROL_MSG message = {0};
	P4VFS_CONTROL_REPLY reply = {0};
	
	message.operation = P4VFS_OPERATION_SET_TRACE_ENABLED;
	message.data.SET_TRACE_ENABLED.channels = channels;
	HRESULT hr = SendDriverControlMessage(message, reply);
	if (FAILED(hr))
	{
		return hr;
	}
	return S_OK;
}

HRESULT
SetDriverFlag(
	const wchar_t* flagName,
	ULONG flagValue
	)
{
	if (FileCore::StringInfo::IsNullOrEmpty(flagName))
	{
		return E_INVALIDARG;
	}

	if (FileCore::StringInfo::Strlen(flagName) >= P4VFS_CONTROL_FLAG_LENGTH)
	{
		return E_BOUNDS;
	}

	P4VFS_CONTROL_MSG message = {0};
	P4VFS_CONTROL_REPLY reply = {0};
	
	message.operation = P4VFS_OPERATION_SET_FLAG;
	StaticAssert(_countof(message.data.SET_FLAG.name) == P4VFS_CONTROL_FLAG_LENGTH);
	FileCore::StringInfo::Strncpy(message.data.SET_FLAG.name, flagName, P4VFS_CONTROL_FLAG_LENGTH);
	message.data.SET_FLAG.value = flagValue;

	HRESULT hr = SendDriverControlMessage(message, reply);
	if (FAILED(hr))
	{
		return hr;
	}
	return S_OK;
}

HRESULT
GetDriverIsConnected(
	bool& connected
	)
{
	P4VFS_CONTROL_MSG message = {0};
	P4VFS_CONTROL_REPLY reply = {0};

	message.operation = P4VFS_OPERATION_GET_IS_CONNECTED;
	HRESULT hr = SendDriverControlMessage(message, reply);
	if (FAILED(hr))
	{
		return hr;
	}

	connected = !!reply.data.GET_IS_CONNECTED.connected;
	return S_OK;
}

HRESULT
GetDriverVersion(
	USHORT& major,
	USHORT& minor,
	USHORT& build,
	USHORT& revision
	)
{
	P4VFS_CONTROL_MSG message = {0};
	P4VFS_CONTROL_REPLY reply = {0};

	message.operation = P4VFS_OPERATION_GET_VERSION;
	HRESULT hr = SendDriverControlMessage(message, reply);
	if (FAILED(hr))
	{
		return hr;
	}

	major = reply.data.GET_VERSION.major;
	minor = reply.data.GET_VERSION.minor;
	build = reply.data.GET_VERSION.build;
	revision = reply.data.GET_VERSION.revision;
	return S_OK;
}

HRESULT
AssignUnicodeStringReference(
	P4VFS_UNICODE_STRING* dstString,
	const WCHAR* srcReference,
	ULONG srcReferenceBytes
	)
{
	if (dstString == nullptr || srcReference == nullptr)
	{
		return E_POINTER;
	}

	const std::ptrdiff_t ptrOffset = reinterpret_cast<const BYTE*>(srcReference) - reinterpret_cast<BYTE*>(dstString);
	const LONG longOffset = static_cast<LONG>(ptrOffset);
	if (std::ptrdiff_t(longOffset) != ptrOffset)
	{
		return E_BOUNDS;
	}

	dstString->sizeBytes = srcReferenceBytes;
	dstString->offsetBytes = longOffset;
	return S_OK;
}

HRESULT
AppendUnicodeStringReference(
	FileCore::Array<BYTE>& dstDataBuffer,
	size_t dstUnicodeStringOffset,
	const WCHAR* srcString
	)
{
	if (srcString == nullptr)
	{
		return E_POINTER;
	}

	const size_t minDataBufferSize = dstUnicodeStringOffset + sizeof(P4VFS_UNICODE_STRING);
	if (minDataBufferSize > dstDataBuffer.size())
	{
		dstDataBuffer.resize(minDataBufferSize);
	}

	const size_t strBytes = (FileCore::StringInfo::Strlen(srcString)+1)*sizeof(WCHAR);
	const size_t strStart = dstDataBuffer.size();
	FileCore::Algo::Append(dstDataBuffer, reinterpret_cast<const BYTE*>(srcString), strBytes);
	P4VFS_UNICODE_STRING* dstUnicodeString = reinterpret_cast<P4VFS_UNICODE_STRING*>(dstDataBuffer.data()+dstUnicodeStringOffset);
	return AssignUnicodeStringReference(dstUnicodeString, reinterpret_cast<WCHAR*>(dstDataBuffer.data()+strStart), ULONG(strBytes));
}

}}}
