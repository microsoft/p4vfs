// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "FileCore.h"
#include "LogDevice.h"
#pragma managed(push, off)

namespace Microsoft {
namespace P4VFS {
namespace P4 {
	class DepotClientCache;
}}};

namespace Microsoft {
namespace P4VFS {
namespace FileCore {

	struct UserContext
	{
		ULONG m_SessionId;
		ULONG m_ProcessId;
		ULONG m_ThreadId;

		UserContext() : 
			m_SessionId(0), 
			m_ProcessId(0),
			m_ThreadId(0)
		{}
	};

	struct FileContext
	{
		LogDevice* m_LogDevice;
		UserContext* m_UserContext;
		P4::DepotClientCache* m_DepotClientCache;

		FileContext() :
			m_LogDevice(nullptr),
			m_UserContext(nullptr),
			m_DepotClientCache(nullptr)
		{}

		ULONG SessionId() const { return m_UserContext ? m_UserContext->m_SessionId : 0; }
		ULONG ProcessId() const { return m_UserContext ? m_UserContext->m_ProcessId : 0; }
		ULONG ThreadId() const { return m_UserContext ? m_UserContext->m_ThreadId : 0; }
	};
}}}
#pragma managed(pop)
