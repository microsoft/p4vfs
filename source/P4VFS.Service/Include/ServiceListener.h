// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "DriverData.h"

namespace Microsoft {
namespace P4VFS {

struct P4VFS_SERVICE_MSG_USER_MODE
{
	FILTER_MESSAGE_HEADER			m_messageHeader;
	union
	{
		P4VFS_SERVICE_MSG			m_driverRequest;
		P4VFS_SERVICE_REQ_BUFFER	m_buffer;
	};
	OVERLAPPED						m_overlapped;
};

struct P4VFS_SERVICE_REPLY_USER_MODE 
{
	FILTER_REPLY_HEADER		m_messageHeader;
	P4VFS_SERVICE_REPLY		m_requestReply;
};

class ServiceListener
{
public:
	ServiceListener(
		);

	~ServiceListener(
		);

	void 
	UpdateService(
		HANDLE hCancelationToken
		);

	bool
	IsDriverConnected(
		) const;

	FILETIME
	GetLastRequestTime(
		) const;

private:
	HRESULT 
	SrvConnectToDriver(
		);

private:
	HANDLE		m_DriverPort;
	DWORD		m_ServiceTaskCount;
	DWORD		m_MaxUpdatePeriod;
	DWORD		m_DriverConnectRetryCount;
};

}}
