// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "DriverData.h"
#include "ServiceListener.h"
#include "FileCore.h"

namespace Microsoft {
namespace P4VFS {

class ServiceTaskManager
{
public:
	ServiceTaskManager(
		);

	~ServiceTaskManager(
		);

	HRESULT 
	Initialize(
		DWORD threadCount
		);

	HRESULT 
	Shutdown(
		);

	HRESULT
	Submit(
		HANDLE driverPort,
		const std::shared_ptr<const P4VFS_SERVICE_MSG_USER_MODE>& message
		);

private:
	struct ServiceTask
	{
		ServiceTask() :
			m_DriverPort(NULL),
			m_Message()
		{}

		HANDLE m_DriverPort;
		std::shared_ptr<const P4VFS_SERVICE_MSG_USER_MODE> m_Message;
	};

	struct ServiceThread
	{
		ServiceThread() :
			m_Handle(NULL),
			m_Index(0),
			m_Manager(nullptr)
		{}

		HANDLE m_Handle;
		DWORD m_Index;
		ServiceTaskManager* m_Manager;
	};

	typedef Microsoft::P4VFS::FileCore::Array<ServiceThread*> ServiceThreadArray;
	typedef Microsoft::P4VFS::FileCore::Array<ServiceTask*> ServiceTaskArray;

	struct ServiceReply
	{
		ServiceReply(HRESULT requestResult = E_FAIL, NTSTATUS successStatus = STATUS_SUCCESS, NTSTATUS failureStatus = STATUS_UNSUCCESSFUL) :
			m_RequestResult(requestResult),
			m_SuccessStatus(successStatus),
			m_FailureStatus(failureStatus)
		{}

		HRESULT m_RequestResult;
		NTSTATUS m_SuccessStatus;
		NTSTATUS m_FailureStatus;
	};

private:	
	static DWORD
	ServiceTaskThreadEntry(
		void* data
	);

	DWORD
	UpdateServiceTasks(
		);

	bool
	IsCancelRequested(
		) const;

	ServiceTask*
	BeginTask(
		);
	
	HRESULT
	EndTask(
		ServiceTask* task
		);

	bool 
	IsTaskReady(
		const ServiceTask* task
		) const;

	ServiceReply
	HandleResolveFileRequest(
		const P4VFS_SERVICE_RESOLVE_FILE_MSG& message
		);

	ServiceReply
	HandleLogWriteRequest(
		const P4VFS_SERVICE_LOG_WRITE_MSG& message
		);

	static HRESULT 
	ReplyToDriver(
		HANDLE driverPort,
		const P4VFS_SERVICE_MSG_USER_MODE& message, 
		const ServiceReply& reply
		);

	static HRESULT
	ResolvePathFromMessage(
		const P4VFS_SERVICE_RESOLVE_FILE_MSG& message,
		FileCore::String& resolvedDataName
		);

private:
	HANDLE m_CancelationEvent;
	HANDLE m_TaskNotifySemaphore;
	HANDLE m_TaskArrayMutex;
	ServiceThreadArray m_Threads;
	ServiceTaskArray m_TasksPending;
	ServiceTaskArray m_TasksActive;
};

}}
