// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "ServiceListener.h"
#include "ServiceTask.h"
#include "ServiceContext.h"
#include "ServiceLog.h"
#include "ServiceHost.h"
#include "FileOperations.h"
#include "FileSystem.h"

using namespace Microsoft::P4VFS::ExtensionsInterop;
using namespace Microsoft::P4VFS::FileCore;

namespace Microsoft {
namespace P4VFS {

ServiceTaskManager::ServiceTaskManager() : 
	m_CancelationEvent(NULL),
	m_TaskNotifySemaphore(NULL),
	m_TaskArrayMutex(NULL)
{
}

ServiceTaskManager::~ServiceTaskManager()
{
}

HRESULT 
ServiceTaskManager::Initialize(
	DWORD threadCount
	)
{
	if (threadCount <= 0)
		return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

	m_CancelationEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_TaskNotifySemaphore = CreateSemaphore(NULL, 0, LONG_MAX, NULL);
	m_TaskArrayMutex = CreateMutex(NULL, FALSE, NULL);

	m_Threads.resize(threadCount);
	for (size_t threadIndex = 0; threadIndex < m_Threads.size(); ++threadIndex)
	{
		ServiceThread* thread = new ServiceThread();
		thread->m_Index = DWORD(threadIndex);
		thread->m_Manager = this;
		thread->m_Handle = CreateThread(NULL, 0, &ServiceTaskThreadEntry, thread, 0, NULL);
		m_Threads[threadIndex] = thread;
	}
	return S_OK;
}

HRESULT 
ServiceTaskManager::Shutdown()
{
	Array<HANDLE> hThreads;
	for (size_t threadIndex = 0; threadIndex < m_Threads.size(); ++threadIndex)
		hThreads.push_back(m_Threads[threadIndex]->m_Handle);

	SetEvent(m_CancelationEvent);
	WaitForMultipleObjects(DWORD(hThreads.size()), hThreads.data(), TRUE, INFINITE);

	for (size_t threadIndex = 0; threadIndex < m_Threads.size(); ++threadIndex)
		CloseHandle(m_Threads[threadIndex]->m_Handle);

	Algo::ClearDelete(m_Threads);
	Algo::ClearDelete(m_TasksPending);
	Algo::ClearDelete(m_TasksActive);

	SafeCloseHandle(m_CancelationEvent);
	SafeCloseHandle(m_TaskNotifySemaphore);
	SafeCloseHandle(m_TaskArrayMutex);
	return S_OK;
}

HRESULT 
ServiceTaskManager::Submit(
	HANDLE driverPort, 
	const std::shared_ptr<const P4VFS_SERVICE_MSG_USER_MODE>& message
	)
{
	HRESULT hr = E_FAIL;
	if (message.get() != nullptr)
	{
		ServiceTask* task = new ServiceTask();
		task->m_DriverPort = driverPort;
		task->m_Message = message;
	
		if (WaitForSingleObject(m_TaskArrayMutex, INFINITE) == WAIT_OBJECT_0)
		{
			m_TasksPending.push_back(task);
			ReleaseMutex(m_TaskArrayMutex);
			ReleaseSemaphore(m_TaskNotifySemaphore, 1, NULL);
			hr = S_OK;
		}
	}
	return hr;
}

DWORD 
ServiceTaskManager::ServiceTaskThreadEntry(
	void* data
	)
{
	ServiceThread* thread = reinterpret_cast<ServiceThread*>(data);
	return thread->m_Manager->UpdateServiceTasks();
}

bool
ServiceTaskManager::IsCancelRequested(
	) const
{
	return WaitForSingleObject(m_CancelationEvent, 0) == WAIT_OBJECT_0;
}

ServiceTaskManager::ServiceTask*
ServiceTaskManager::BeginTask(
	)
{
	ServiceTask* result = nullptr;
	if (WaitForSingleObject(m_TaskArrayMutex, INFINITE) == WAIT_OBJECT_0)
	{
		for (ServiceTaskArray::iterator pendingIt = m_TasksPending.begin(); pendingIt != m_TasksPending.end(); ++pendingIt)
		{
			ServiceTask* pendingTask = *pendingIt;
			if (IsTaskReady(pendingTask))
			{
				m_TasksPending.erase(pendingIt);
				m_TasksActive.push_back(pendingTask);
				result = pendingTask;
				break;
			}
		}
		ReleaseMutex(m_TaskArrayMutex);
	}
	return result;
}
	
HRESULT
ServiceTaskManager::EndTask(
	ServiceTask* task
	)
{
	HRESULT result = E_FAIL;
	if (WaitForSingleObject(m_TaskArrayMutex, INFINITE) == WAIT_OBJECT_0)
	{
		Algo::Remove(m_TasksActive, task);
		delete task;
		ReleaseMutex(m_TaskArrayMutex);
		result = S_OK;
	}
	return result;
}

bool 
ServiceTaskManager::IsTaskReady(
	const ServiceTask* task
	) const
{
	if (task->m_Message->m_driverRequest.operation == P4VFS_SERVICE_RESOLVE_FILE)
	{
		const wchar_t* taskDataName = task->m_Message->m_driverRequest.resolveFile.dataName.c_str();
		for (const ServiceTask* taskActive : m_TasksActive)
		{
			// The task is not ready if it's file is currently being processed by another task!
			if (StringInfo::Stricmp(taskDataName, taskActive->m_Message->m_driverRequest.resolveFile.dataName.c_str()) == 0)
				return false;
		}
	}
	return true;
}

DWORD
ServiceTaskManager::UpdateServiceTasks(
	)
{
	while (IsCancelRequested() == false)
	{
		// Wait for a chance for this thread to take a task from the queue
		const HANDLE handles[] = { m_TaskNotifySemaphore, m_CancelationEvent };
		if (WaitForMultipleObjects(_countof(handles), handles, FALSE, INFINITE) != WAIT_OBJECT_0)
			continue;

		// Try to process as many tasks from the queue as possible before waiting again
		while (IsCancelRequested() == false)
		{				
			ServiceTask* task = BeginTask();
			if (task == nullptr)
				break;

			ServiceReply reply;
			switch (task->m_Message->m_driverRequest.operation)
			{
				case P4VFS_SERVICE_RESOLVE_FILE:
					reply = HandleResolveFileRequest(task->m_Message->m_driverRequest.resolveFile);
					break;
				case P4VFS_SERVICE_LOG_WRITE:
					reply = HandleLogWriteRequest(task->m_Message->m_driverRequest.logWrite);
					break;
			}

			ReplyToDriver(task->m_DriverPort, *task->m_Message, reply);
			EndTask(task);
		}
	}
	return 0;
}

ServiceTaskManager::ServiceReply
ServiceTaskManager::HandleResolveFileRequest(
	const P4VFS_SERVICE_RESOLVE_FILE_MSG& message
	)
{
	if (FileSystem::IsExcludedProcessId(message.processId))
	{
		ServiceLog::Verbose(StringInfo::Format(TEXT("HandleResolveFileRequest Ignoring '%s' process [%d.%d]"), message.dataName.c_str(), message.processId, message.threadId).c_str());
		return ServiceReply(E_FAIL, STATUS_ACCESS_DENIED);
	}

	ServiceLog::Verbose(StringInfo::Format(TEXT("HandleResolveFileRequest Start '%s' process [%d.%d]"), message.dataName.c_str(), message.processId, message.threadId).c_str());
	ServiceHost::GetInstance()->NotifyLastRequestTime();

	String localFileToMakeResident;
	HRESULT hr = ResolvePathFromMessage(message, localFileToMakeResident);
	if (FAILED(hr))
	{
		ServiceLog::Error(StringInfo::Format(TEXT("HandleResolveFileRequest Failed to resolve file '%s' process [%d.%d] error [%s]"), localFileToMakeResident.c_str(), message.processId, message.threadId, StringInfo::ToString(hr).c_str()).c_str());
		return ServiceReply(hr);
	}

	FileCore::UserContext userContext;
	userContext.m_SessionId = message.sessionId;
	userContext.m_ProcessId = message.processId;
	userContext.m_ThreadId = message.threadId;

	hr = FileOperations::ImpersonateLoggedOnUser(&userContext);
	if (FAILED(hr))
	{
		ServiceLog::Error(StringInfo::Format(TEXT("HandleResolveFileRequest Failed to impersonate user for session [%d] process [%d.%d] file '%s' error [%s]"), message.sessionId, message.processId, message.threadId, localFileToMakeResident.c_str(), StringInfo::ToString(hr).c_str()).c_str());
		return ServiceReply(hr);
	}

	ServiceContext serviceContext(m_CancelationEvent);
	serviceContext.m_UserContext = &userContext;
	BYTE fileResidencyPolicy = P4VFS_RESIDENCY_POLICY_UNDEFINED;

	hr = FileSystem::ResolveFileResidency(serviceContext, localFileToMakeResident.c_str(), &fileResidencyPolicy);
	if (FAILED(hr))
	{
		ServiceLog::Error(StringInfo::Format(TEXT("HandleResolveFileRequest Failed to resolve file '%s' process [%d.%d] error [%s]"), localFileToMakeResident.c_str(), message.processId, message.threadId, StringInfo::ToString(hr).c_str()).c_str());
	}

	if (RevertToSelf() == FALSE)
	{
		ServiceLog::Error(StringInfo::Format(TEXT("HandleResolveFileRequest Failed to revert back to self for file '%s' process [%d.%d] error [%s]"), localFileToMakeResident.c_str(), message.processId, message.threadId, StringInfo::ToString(HRESULT_FROM_WIN32(GetLastError())).c_str()).c_str());
	}

	NTSTATUS successStatus = STATUS_SUCCESS;
	if (fileResidencyPolicy == P4VFS_RESIDENCY_POLICY_SYMLINK)
		successStatus = STATUS_RETRY;
	else if (fileResidencyPolicy == P4VFS_RESIDENCY_POLICY_UNDEFINED)
		successStatus = STATUS_UNSUCCESSFUL;

	ServiceLog::Verbose(StringInfo::Format(TEXT("HandleResolveFileRequest End %s"), message.dataName.c_str()).c_str());
	return ServiceReply(hr, successStatus);
}

ServiceTaskManager::ServiceReply
ServiceTaskManager::HandleLogWriteRequest(
	const P4VFS_SERVICE_LOG_WRITE_MSG& message
	)
{
	ServiceLog::Info(StringInfo::Format(TEXT("[Driver] %s"), message.text).c_str());
	return ServiceReply(S_OK, STATUS_SUCCESS);
}

HRESULT 
ServiceTaskManager::ReplyToDriver(
	HANDLE driverPort,
	const P4VFS_SERVICE_MSG_USER_MODE& message,
	const ServiceReply& reply
	)
{
	HRESULT hr = S_OK;

	P4VFS_SERVICE_REPLY_USER_MODE replyMessage  = {0};
	replyMessage.m_messageHeader.MessageId              = message.m_messageHeader.MessageId;
	replyMessage.m_requestReply.requestID               = message.m_driverRequest.requestID;
	replyMessage.m_messageHeader.Status                 = STATUS_SUCCESS;
	replyMessage.m_requestReply.requestResult           = SUCCEEDED(reply.m_RequestResult) ? reply.m_SuccessStatus : reply.m_FailureStatus;

	hr = FilterReplyMessage(
			driverPort,
			&replyMessage.m_messageHeader,
			sizeof(replyMessage.m_messageHeader) + sizeof(replyMessage.m_requestReply)
			);

	if (FAILED(hr))
	{
		// Something really bad happened. We cant tell the driver success or failure.
		ServiceLog::Error(StringInfo::Format(TEXT("ReplyToDriver Failed to reply to driver [%s]"), StringInfo::ToString(hr).c_str()).c_str());
	}
	return hr;
}

HRESULT 
ServiceTaskManager::ResolvePathFromMessage(
	const P4VFS_SERVICE_RESOLVE_FILE_MSG& message,
	FileCore::String& resolvedDataName
	)
{
	HRESULT hr = S_OK;

	// translate from driver volume name to dos friendly name
	const DWORD dwVolumeNameBufferSize = MAX_PATH*2;
	WCHAR volumeDosName[dwVolumeNameBufferSize];

	hr = FilterGetDosName(
				message.volumeName.c_str(), 
				volumeDosName, 
				dwVolumeNameBufferSize
				);

	if (FAILED(hr))
	{
		return hr;
	}

	resolvedDataName = message.dataName.c_str();
	resolvedDataName.replace(0, StringInfo::Strlen(message.volumeName.c_str()), volumeDosName);
	return S_OK;
}

}}
