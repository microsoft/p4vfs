// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "ServiceListener.h"
#include "ServiceHost.h"
#include "ServiceTask.h"
#include "ServiceLog.h"
#include "FileOperations.h"
#include "DriverOperations.h"
#include "DriverVersion.h"
#include "FileCore.h"

using namespace Microsoft::P4VFS::ExtensionsInterop;
using namespace Microsoft::P4VFS::FileCore;

namespace Microsoft {
namespace P4VFS {

ServiceListener::ServiceListener() : 
	m_DriverPort(NULL),
	m_ServiceTaskCount(8),
	m_MaxUpdatePeriod(1000/60),
	m_DriverConnectRetryCount(0)
{
}

ServiceListener::~ServiceListener()
{
	SafeCloseHandle(m_DriverPort);
}

void 
ServiceListener::UpdateService(
	HANDLE hCancelationEvent
	)
{
	ServiceTaskManager taskManager;
	if (taskManager.Initialize(m_ServiceTaskCount) != S_OK)
	{
		ServiceLog::Error(TEXT("ServiceListener::UpdateService Failed to Initialize ServiceTaskManager"));
		return;
	}

	while (WaitForSingleObject(hCancelationEvent, 0) != WAIT_OBJECT_0)
	{
		HRESULT hr = S_OK;

		// If we are not connected to the driver connect
		if (m_DriverPort == NULL)
		{
			hr = SrvConnectToDriver();
			if (FAILED(hr) || m_DriverPort == NULL)
			{
				WaitForSingleObject(hCancelationEvent, m_MaxUpdatePeriod);
				continue;
			}
		}
	
		std::shared_ptr<P4VFS_SERVICE_MSG_USER_MODE> message = std::make_shared<P4VFS_SERVICE_MSG_USER_MODE>();
		ZeroMemory(message.get(), sizeof(P4VFS_SERVICE_MSG_USER_MODE));

		// The event is freed when the message is freed
		message->m_overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (message->m_overlapped.hEvent == NULL)
		{
			ServiceLog::Error(StringInfo::Format(TEXT("ServiceListener::UpdateService Failed to CreateRequestMessage [%s]"), StringInfo::ToString(HRESULT_FROM_WIN32(GetLastError())).c_str()).c_str());
			WaitForSingleObject(hCancelationEvent, m_MaxUpdatePeriod);
			continue;
		}

		hr = FilterGetMessage(m_DriverPort, &message->m_messageHeader, FIELD_OFFSET(P4VFS_SERVICE_MSG_USER_MODE, m_overlapped), &message->m_overlapped);
		if (hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING))
		{
			// We expect ERROR_IO_PENDING to be returned
			hr = S_OK;
		}
		else if (FAILED(hr))
		{
			if (hr == E_HANDLE)
			{
				// Driver port handle is invalid now (driver was probably unloaded), we need to reinitialize the driver communication
				SafeCloseHandle(m_DriverPort);
			}

			SafeCloseHandle(message->m_overlapped.hEvent);
			ServiceLog::Error(StringInfo::Format(TEXT("ServiceListener::UpdateService Failed to FilterGetMessage [%s]"), StringInfo::ToString(HRESULT_FROM_WIN32(GetLastError())).c_str()).c_str());
			WaitForSingleObject(hCancelationEvent, m_MaxUpdatePeriod);
			continue;
		}

		// Wait for a message to be recieved, or cancelation or error
		const HANDLE handles[] = { message->m_overlapped.hEvent, hCancelationEvent };
		bool messageRecieved = WaitForMultipleObjects(_countof(handles), handles, FALSE, INFINITE) == WAIT_OBJECT_0;
		if (messageRecieved)
		{
			// Handle the mesage
			taskManager.Submit(m_DriverPort, message);
			SafeCloseHandle(message->m_overlapped.hEvent);
		}
		else
		{
			// Interupted by a cancelation or error... close the port to kill any pending IO
			ServiceLog::Error(StringInfo::Format(TEXT("ServiceListener::UpdateService message receival interupted [%s]"), StringInfo::ToString(HRESULT_FROM_WIN32(GetLastError())).c_str()).c_str());
			SafeCloseHandle(message->m_overlapped.hEvent);
			SafeCloseHandle(m_DriverPort);
		}
	}

	taskManager.Shutdown();
}

bool 
ServiceListener::IsDriverConnected(
	) const
{
	return m_DriverPort != NULL;
}

HRESULT 
ServiceListener::SrvConnectToDriver(
	)
{
	HRESULT hr = FilterConnectCommunicationPort(P4VFS_SERVICE_PORT_NAME, 0, NULL, 0, NULL, &m_DriverPort);
	if (FAILED(hr))
	{
		m_DriverPort = NULL;
		if (m_DriverConnectRetryCount < 10)
		{
			m_DriverConnectRetryCount++;
			ServiceLog::Error(StringInfo::Format(TEXT("ServiceListener::SrvConnectToDriver Failed to SrvConnectToDriver [%s] Retry [%d]"), StringInfo::ToString(HRESULT_FROM_WIN32(GetLastError())).c_str(), m_DriverConnectRetryCount).c_str());
		}

		// Attempt to Load the driver just in case it isn't already loaded
		DriverOperations::LoadFilter(TEXT(P4VFS_DRIVER_TITLE));

		// Purposely pause here to try to connect to the driver only once every second
		Sleep(1000);
	}
	else
	{
		m_DriverConnectRetryCount = 0;
		ServiceLog::Info(TEXT("ServiceListener::SrvConnectToDriver Connected to Driver!"));
	}
	return hr;
}

}}
