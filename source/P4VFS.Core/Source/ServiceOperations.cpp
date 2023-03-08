// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "ServiceOperations.h"

namespace Microsoft {
namespace P4VFS {
namespace ServiceOperations {

static HRESULT
QueryWaitForServiceStatusFlag(
	SC_HANDLE hSCService, 
	DWORD dwWaitStatusFlag,
	DWORD dwTimeout = DefaultServiceTimeout
	)
{
	SERVICE_STATUS serviceStatus = {0};
	ULONGLONG startTickCount = GetTickCount64();
	DWORD waitTime = 0;
	while ((GetTickCount64() - startTickCount) <= dwTimeout)
	{
		if (waitTime > 0)
			Sleep(waitTime);
			
		if (QueryServiceStatus(hSCService, &serviceStatus) == FALSE)
			return HRESULT_FROM_WIN32(GetLastError());

		if ((1<<serviceStatus.dwCurrentState) & dwWaitStatusFlag)
			return S_OK;

		waitTime = std::min<DWORD>(500, std::max<DWORD>(serviceStatus.dwWaitHint / 10, 5000));
	}
	return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
}

HRESULT
InstallLocalService(
	const WCHAR* pathToServiceBinary,
	const WCHAR* serviceName,
	const WCHAR* friendlyServiceName,
	const WCHAR* serviceDescription
	)
{
	FileCore::AutoServiceHandle hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);   
	if (hSCManager.IsValid() == false)
		return E_FAIL;

	FileCore::AutoServiceHandle hSCService = CreateService(
		hSCManager.Handle(),		// SCM database
		serviceName,				// name of service
		friendlyServiceName,		// service name to display
		SERVICE_ALL_ACCESS,			// desired access
		SERVICE_WIN32_OWN_PROCESS,	// service type
		SERVICE_AUTO_START,			// start type
		SERVICE_ERROR_NORMAL,		// error control type
		pathToServiceBinary,		// path to service's binary
		NULL,						// no load ordering group
		NULL,						// no tag identifier
		NULL,						// no dependencies
		NULL,						// LocalSystem account
		NULL);						// no password
 
	if (hSCService.IsValid() == false)
	{
		if (GetLastError() == ERROR_SERVICE_EXISTS)
		{
			hSCService.Reset(OpenService(hSCManager.Handle(), serviceName, SERVICE_ALL_ACCESS));
			if (hSCService.IsValid() == false)
				return HRESULT_FROM_WIN32(GetLastError());
		}
		else
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}
	}

	SC_ACTION failActions[3];
	memset(failActions, 0, sizeof(failActions));
	failActions[0].Type = SC_ACTION_RESTART;
	failActions[0].Delay = 500;
	failActions[1].Type = SC_ACTION_RESTART;
	failActions[1].Delay = 500;
	failActions[2].Type = SC_ACTION_RESTART;
	failActions[2].Delay = 500;

	SERVICE_DESCRIPTION description;
	memset(&description, 0, sizeof(description));
	description.lpDescription = serviceDescription ? serviceDescription : TEXT("");

	SERVICE_FAILURE_ACTIONS onFailureActions;
	memset(&onFailureActions, 0, sizeof(onFailureActions));
	onFailureActions.dwResetPeriod = 60 * 60 * 24; // Reset failures after a day
	onFailureActions.cActions = ARRAYSIZE(failActions);
	onFailureActions.lpsaActions = failActions;
	onFailureActions.lpCommand = NULL;
	onFailureActions.lpRebootMsg = NULL;

	if (ChangeServiceConfig2(hSCService.Handle(), SERVICE_CONFIG_DESCRIPTION, &description) == FALSE)
		return HRESULT_FROM_WIN32(GetLastError());
	
	if (ChangeServiceConfig2(hSCService.Handle(), SERVICE_CONFIG_FAILURE_ACTIONS, &onFailureActions) == FALSE)
		return HRESULT_FROM_WIN32(GetLastError());

	if (StartService(hSCService.Handle(), 0, NULL) == FALSE)
		return HRESULT_FROM_WIN32(GetLastError());

	return S_OK;
}

HRESULT
UninstallLocalService(
	const WCHAR* serviceName,
	UninstallFlags::Enum flags
	)
{
	FileCore::AutoServiceHandle hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hSCManager.IsValid() == false)
		return E_FAIL;

	FileCore::AutoServiceHandle hSCService = OpenService(hSCManager.Handle(), serviceName, SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE);
	if (hSCService.IsValid() == false)
		return S_OK;

	// Wait or timeout until not SERVICE_STOP_PENDING
	HRESULT hr = QueryWaitForServiceStatusFlag(hSCService.Handle(), ~(1U<<SERVICE_STOP_PENDING));
	if (FAILED(hr))
		return hr;

	// Stop the service if needed
	if (FAILED(QueryWaitForServiceStatusFlag(hSCService.Handle(), 1U<<SERVICE_STOPPED, 0)))
	{
		SERVICE_STATUS serviceStatus = {0};
		if (ControlService(hSCService.Handle(), SERVICE_CONTROL_STOP, &serviceStatus) == 0)
			return HRESULT_FROM_WIN32(GetLastError());

		// Wait or timeout until SERVICE_STOPPED
		hr = QueryWaitForServiceStatusFlag(hSCService.Handle(), 1U<<SERVICE_STOPPED);
		if (FAILED(hr))
			return hr;
	}
	
	if ((flags & UninstallFlags::NoDelete) == 0)
	{
		if (DeleteService(hSCService.Handle()) == 0)
			return HRESULT_FROM_WIN32(GetLastError());
	}

	return S_OK;
}

HRESULT
StartLocalService(
	const WCHAR* serviceName,
	DWORD dwTimeout
	)
{
	FileCore::AutoServiceHandle hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (hSCManager.IsValid() == false)
		return HRESULT_FROM_WIN32(GetLastError());

	FileCore::AutoServiceHandle hSCService = OpenService(hSCManager.Handle(), serviceName, SERVICE_START | SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG);
	if (hSCService.IsValid() == false)
		return HRESULT_FROM_WIN32(GetLastError());

	// If the service is already running there is nothing to do.
	SERVICE_STATUS serviceStatus = {0};
	if (QueryServiceStatus(hSCService.Handle(), &serviceStatus) && SERVICE_RUNNING == serviceStatus.dwCurrentState)
		return S_OK;

	if (StartService(hSCService.Handle(), 0, NULL) == FALSE)
		return HRESULT_FROM_WIN32(GetLastError());

	if (dwTimeout == 0)
		return E_PENDING;

	HRESULT hr = QueryWaitForServiceStatusFlag(hSCService.Handle(), 1<<SERVICE_RUNNING, dwTimeout);
	if (FAILED(hr))
		return hr;

	return S_OK;
}

HRESULT
StopLocalService(
	const WCHAR* serviceName,
	DWORD dwTimeout
	)
{
	FileCore::AutoServiceHandle hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (hSCManager.IsValid() == false)
		return HRESULT_FROM_WIN32(GetLastError());

	FileCore::AutoServiceHandle hSCService = OpenService(hSCManager.Handle(), serviceName, SERVICE_STOP | SERVICE_QUERY_STATUS);
	if (hSCService.IsValid() == false)
		return HRESULT_FROM_WIN32(GetLastError());

	// If the service is already running there is nothing to do.
	SERVICE_STATUS serviceStatus = {0};
	if (QueryServiceStatus(hSCService.Handle(), &serviceStatus) && SERVICE_STOPPED == serviceStatus.dwCurrentState)
		return S_OK;

	if (ControlService(hSCService.Handle(), SERVICE_CONTROL_STOP, &serviceStatus) == FALSE)
		return HRESULT_FROM_WIN32(GetLastError());

	if (serviceStatus.dwCurrentState == SERVICE_STOPPED)
		return S_OK;

	if (dwTimeout == 0)
		return E_PENDING;

	HRESULT hr = QueryWaitForServiceStatusFlag(hSCService.Handle(), 1<<SERVICE_STOPPED, dwTimeout);
	if (FAILED(hr))
		return hr;

	return S_OK;
}

DWORD
GetLocalServiceState(
	const WCHAR* serviceName
	)
{
	FileCore::AutoServiceHandle hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (hSCManager.IsValid() == false)
		return 0;

	FileCore::AutoServiceHandle hSCService = OpenService(hSCManager.Handle(), serviceName, SERVICE_QUERY_STATUS);
	if (hSCService.IsValid() == false)
		return 0;

	SERVICE_STATUS serviceStatus = {0};
	if (QueryServiceStatus(hSCService.Handle(), &serviceStatus) == FALSE)
		return 0;

	return serviceStatus.dwCurrentState;
}

}}}
