// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "ExtensionsServiceHost.h"

namespace Microsoft {
namespace P4VFS {

class ServiceHost : public ExtensionsInterop::ServiceHost
{
public:

	static ServiceHost&
	StaticInstance(
		);

	static VOID WINAPI 
	StaticSrvMain(
		DWORD dwNumServicesArgs,
		LPWSTR* lpServiceArgVectors
		);

	static VOID WINAPI StaticSrvCtrlHandler(
		DWORD dwCtrl 
		);

	ServiceHost(
		);

	void 
	Start(
		);

	void 
	SrvMain(
		DWORD dwNumServicesArgs,
		LPWSTR* lpServiceArgVectors
		);

	void 
	SrvCtrlHandler(
		DWORD dwCtrl 
		);

	void 
	SrvReportStatus(
		DWORD dwCurrentState,
		DWORD dwWin32ExitCode,
		DWORD dwWaitHint
		);

	void
	SrvBeginTickThread(
		);

	void
	SrvEndTickThread(
		);

	static DWORD
	SrvTickThreadEntry(
		void* data
		);

	virtual bool 
	IsDriverConnected(
		) override;

	void
	NotifyLastRequestTime(
		);
		
	virtual FILETIME
	GetLastRequestTime(
		) override;
		
	virtual FILETIME
	GetLastModifiedTime(
		) override;

	virtual bool
	GarbageCollect(
		int64_t timeout
		) override;

private:
	static bool
	HasArgument(
		DWORD dwNumServicesArgs,
		LPWSTR* lpServiceArgVectors,
		LPCWSTR lpArgName
		);

private:
	static const WCHAR*				SERVICE_NAME;
	static ServiceHost*				m_Instance;

	SERVICE_STATUS					m_SrvStatus;
	SERVICE_STATUS_HANDLE			m_SrvStatusHandle; 
	HANDLE							m_SrvStopEvent;
	class ServiceListener*			m_SrvListener;
	std::atomic<FILETIME>			m_SrvLastRequestTime;
	HANDLE							m_SrvTickThread;
};

}}
