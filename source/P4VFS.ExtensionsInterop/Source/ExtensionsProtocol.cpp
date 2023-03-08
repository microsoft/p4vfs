// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "ExtensionsProtocol.h"
#include "ExtensionsServiceHost.h"
#include "DriverData.h"
#include <vcclr.h>
#include <msclr\marshal_cppstd.h>

#using <System.Core.dll>
#using <P4VFS.Extensions.dll>

using namespace System;
using namespace System::IO;
using namespace Microsoft::P4VFS::Extensions;
using namespace Microsoft::P4VFS::ExtensionsProtocol;
using namespace msclr::interop;

namespace Microsoft {
namespace P4VFS {
namespace ExtensionsProtocol {

inline System::DateTime
marshal_as_datetime(const FILETIME& src)
{
	return System::DateTime::FromFileTime((uint64_t(src.dwHighDateTime)<<32)|src.dwLowDateTime);
}

public ref class ManagedServiceHost : Microsoft::P4VFS::Extensions::ServiceHost
{
public:
	ManagedServiceHost(Microsoft::P4VFS::ExtensionsInterop::ServiceHost* srvHost) :
		m_SrvHost(srvHost)
	{
	}

	virtual bool 
	IsDriverConnected(
		)
	{
		return m_SrvHost ? m_SrvHost->IsDriverConnected() : false;
	}

	virtual System::DateTime
	GetLastRequestTime(
		)
	{
		return m_SrvHost ? marshal_as_datetime(m_SrvHost->GetLastRequestTime()) : DateTime::MinValue;
	}
	
	virtual System::DateTime
	GetLastModifiedTime(
		)
	{
		return m_SrvHost ? marshal_as_datetime(m_SrvHost->GetLastModifiedTime()) : DateTime::MinValue;
	}

	virtual bool 
	GarbageCollect(
		System::Int64 timeout
		)
	{
		return m_SrvHost ? m_SrvHost->GarbageCollect(timeout) : false;
	}

private:
	Microsoft::P4VFS::ExtensionsInterop::ServiceHost* m_SrvHost;
};

}}}

long
Microsoft::P4VFS::ExtensionsProtocol::InitializeServiceHost(
	ExtensionsInterop::ServiceHost* srvHost
	)
{
	try
	{
		if (VirtualFileSystem::InitializeServiceHost(gcnew ManagedServiceHost(srvHost)))
		{
			return S_OK;
		}
	}
	catch (...)
	{
	}
	return E_FAIL;
}

long
Microsoft::P4VFS::ExtensionsProtocol::ShutdownServiceHost(
	)
{
	try
	{
		if (VirtualFileSystem::ShutdownServiceHost())
		{
			return S_OK;
		}
	}
	catch (...)
	{
	}
	return E_FAIL;
}

void
Microsoft::P4VFS::ExtensionsProtocol::LaunchAttachDebugger(
	)
{
	System::Diagnostics::Debugger::Launch();
}
