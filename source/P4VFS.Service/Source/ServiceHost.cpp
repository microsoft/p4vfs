// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "ServiceHost.h"
#include "ServiceListener.h"
#include "ServiceTask.h"
#include "ServiceContext.h"
#include "ServiceLog.h"
#include "FileOperations.h"
#include "FileCore.h"
#include "FileAssert.h"
#include "SettingManager.h"

using namespace Microsoft::P4VFS::ExtensionsInterop;
using namespace Microsoft::P4VFS::FileCore;

namespace Microsoft {
namespace P4VFS {

const WCHAR* ServiceHost::SERVICE_NAME = TEXT("P4VFS.Service");
ServiceHost* ServiceHost::m_Instance = nullptr;

ServiceHost&
ServiceHost::StaticInstance(
	)
{
	Assert(m_Instance != nullptr);
	return *m_Instance;
}

VOID WINAPI 
ServiceHost::StaticSrvMain(
	DWORD dwNumServicesArgs,
	LPWSTR* lpServiceArgVectors
	)
{
	StaticInstance().SrvMain(dwNumServicesArgs, lpServiceArgVectors);
}

VOID WINAPI 
ServiceHost::StaticSrvCtrlHandler(
	DWORD dwCtrl 
	)
{
	StaticInstance().SrvCtrlHandler(dwCtrl);
}

ServiceHost::ServiceHost() :
	m_SrvStatus{0},
	m_SrvStatusHandle(NULL),
	m_SrvStopEvent(NULL),
	m_SrvListener(NULL),
	m_SrvLastRequestTime({0}),
	m_SrvTickThread(NULL)
{
	Assert(m_Instance == nullptr);
	m_Instance = this;
}

void 
ServiceHost::Start(
	)
{
	SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);

	const SERVICE_TABLE_ENTRY dispatchTable[] = { 
		{ (LPWSTR)ServiceHost::SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)StaticSrvMain }, 
		{ NULL, NULL } 
	}; 
 
	if (StartServiceCtrlDispatcher(dispatchTable) == FALSE) 
	{ 
		ServiceLog::Error(StringInfo::Format(TEXT("ServiceHost::Start Failed to StartServiceCtrlDispatcher [%s]"), StringInfo::ToString(HRESULT_FROM_WIN32(GetLastError())).c_str()).c_str());
	} 
}

void 
ServiceHost::SrvMain(
	DWORD dwNumServicesArgs,
	LPWSTR* lpServiceArgVectors
	)
{
	// Development option to attach debugger on launch
	if (HasArgument(dwNumServicesArgs, lpServiceArgVectors, TEXT("--break")))
	{
		ExtensionsInterop::LaunchAttachDebugger();
	}

	// Note that this handle does not have to be closed
	m_SrvStatusHandle = RegisterServiceCtrlHandler(ServiceHost::SERVICE_NAME, StaticSrvCtrlHandler);
	if (m_SrvStatusHandle == NULL)
	{ 
		ServiceLog::Error(StringInfo::Format(TEXT("ServiceHost::SrvMain Failed to RegisterServiceCtrlHandler [%s]"), StringInfo::ToString(HRESULT_FROM_WIN32(GetLastError())).c_str()).c_str());
		return; 
	} 
	
	m_SrvStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS; 
	m_SrvStatus.dwServiceSpecificExitCode = 0;    

	// Report initial status to the SCM
	SrvReportStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

	// Create an event to signal when the service is requested to stop
	m_SrvStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (m_SrvStopEvent == NULL)
	{
		SrvReportStatus(SERVICE_STOPPED, NO_ERROR, 0);
		return;
	}

	LogSystem::StaticInstance().Initialize();
	ExtensionsInterop::InitializeServiceHost(this);
	
	SrvBeginTickThread();
	ServiceLog::Info(TEXT("ServiceHost::SrvMain Begin"));

	SrvReportStatus(SERVICE_RUNNING, NO_ERROR, 0);
	{
		m_SrvListener = new ServiceListener();
		m_SrvListener->UpdateService(m_SrvStopEvent);
		SafeDeletePointer(m_SrvListener);
	}
	
	ServiceLog::Info(TEXT("ServiceHost::SrvMain End"));
	SrvEndTickThread();
	LogSystem::StaticInstance().Shutdown(0);
	ExtensionsInterop::ShutdownServiceHost();

	CloseHandle(m_SrvStopEvent);
	SrvReportStatus(SERVICE_STOPPED, NO_ERROR, 0);
}

void 
ServiceHost::SrvCtrlHandler(
	DWORD dwCtrl
	)
{
	switch (dwCtrl) 
	{  
		case SERVICE_CONTROL_STOP: 
		{
			SrvReportStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
			SetEvent(m_SrvStopEvent);
			SrvReportStatus(m_SrvStatus.dwCurrentState, NO_ERROR, 0);
			break;
		}
		case SERVICE_CONTROL_INTERROGATE: 
		{
			break; 
		}
		default: 
		{
			break;
		}
	}
}

void 
ServiceHost::SrvReportStatus(
	DWORD dwCurrentState,
	DWORD dwWin32ExitCode,
	DWORD dwWaitHint
	)
{
	static DWORD dwCheckPoint = 0;

	m_SrvStatus.dwCurrentState = dwCurrentState;
	m_SrvStatus.dwWin32ExitCode = dwWin32ExitCode;
	m_SrvStatus.dwWaitHint = dwWaitHint;

	if (dwCurrentState == SERVICE_START_PENDING)
	{
		m_SrvStatus.dwControlsAccepted = 0;
	}
	else 
	{
		m_SrvStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	}

	if ((dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED))
	{
		m_SrvStatus.dwCheckPoint = 0;
	}
	else 
	{
		m_SrvStatus.dwCheckPoint = ++dwCheckPoint;
	}

	SetServiceStatus(m_SrvStatusHandle, &m_SrvStatus);
}

void
ServiceHost::SrvBeginTickThread(
	)
{
	if (m_SrvTickThread != NULL)
	{
		ServiceLog::Error(TEXT("ServiceHost::SrvBeginTickThread already created"));
	}

	m_SrvTickThread = CreateThread(NULL, 0, &SrvTickThreadEntry, this, 0, NULL);
	if (m_SrvTickThread == NULL)
	{
		ServiceLog::Error(StringInfo::Format(TEXT("ServiceHost::SrvBeginTickThread Failed to CreateThread [%s]"), StringInfo::ToString(HRESULT_FROM_WIN32(GetLastError())).c_str()).c_str());
	}
}

void
ServiceHost::SrvEndTickThread(
	)
{
	if (m_SrvTickThread != NULL)
	{
		WaitForSingleObject(m_SrvTickThread, INFINITE);
		SafeCloseHandle(m_SrvTickThread);
	}
}

DWORD
ServiceHost::SrvTickThreadEntry(
	void* data
	)
{
	ServiceHost* pSrvHost = reinterpret_cast<ServiceHost*>(data);
	Assert(pSrvHost != nullptr);
	
	auto tickPeriodMs = []() -> DWORD 
	{ 
		return static_cast<DWORD>(std::max<int32_t>(1000, FileCore::SettingManager::StaticInstance().GarbageCollectPeriodMs.GetValue())); 
	};

	while (WaitForSingleObject(pSrvHost->m_SrvStopEvent, tickPeriodMs()) != WAIT_OBJECT_0)
	{
		pSrvHost->GarbageCollect(P4::DepotClientCache::GetIdleTimeoutSeconds());
	}

	return 0;
}

bool 
ServiceHost::IsDriverConnected(
	)
{
	return m_SrvListener != nullptr ? m_SrvListener->IsDriverConnected() : false;
}

void
ServiceHost::NotifyLastRequestTime(
	)
{
	m_SrvLastRequestTime = TimeInfo::GetUtcFileTime();
}

FILETIME
ServiceHost::GetLastRequestTime(
	)
{
	return m_SrvLastRequestTime;
}
	
FILETIME
ServiceHost::GetLastModifiedTime(
	)
{
	return LogSystem::StaticInstance().GetLastWriteTime();
}

bool
ServiceHost::GarbageCollect(
	int64_t timeout
	)
{
	ServiceContext::m_StaticDepotClientCache.GarbageCollect(timeout);
	return true;
}

bool
ServiceHost::HasArgument(
	DWORD dwNumServicesArgs,
	LPWSTR* lpServiceArgVectors,
	LPCWSTR lpArgName
	)
{
	if (lpServiceArgVectors != nullptr)
	{
		for (DWORD argIndex = 0; argIndex < dwNumServicesArgs; ++argIndex)
		{
			if (StringInfo::Stricmp(lpArgName, lpServiceArgVectors[argIndex]) == 0)
			{
				return true;
			}
		}
	}
	return false;
}

}}
