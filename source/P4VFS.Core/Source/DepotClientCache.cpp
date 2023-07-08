// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "DepotClientCache.h"
#include "SettingManager.h"

namespace Microsoft {
namespace P4VFS {
namespace P4 {

DepotClientCache::DepotClientCache() :
	m_FreeMap(new FreeMapType)
{
}

DepotClientCache::~DepotClientCache()
{
	SafeDeletePointer(m_FreeMap);
}

DepotClient DepotClientCache::Alloc(const DepotConfig& config, FileContext& fileContext)
{
	AutoCriticalSection lock(m_FreeMapLock);

	const DepotString key = CreateKey(config);
	while (true)
	{
		FreeMapType::iterator clientIt = m_FreeMap->find(key);
		if (clientIt == m_FreeMap->end())
		{
			break;
		}

		DepotClient client = clientIt->second;
		m_FreeMap->erase(clientIt);
		client->SetContext(&fileContext);

		if (client->HasError())
		{
			if (fileContext.m_LogDevice)
			{
				fileContext.m_LogDevice->Info(StringInfo::Format(L"Existing client in error state, discarding [%s]", CSTR_ATOW(key)));
			}
			// Discard this client
			continue;
		}

		if (client->IsConnected() == false)
		{
			if (fileContext.m_LogDevice)
			{
				fileContext.m_LogDevice->Info(StringInfo::Format(L"Existing client disconnected, discarding [%s]", CSTR_ATOW(key)));
			}
			// Discard this client
			continue;
		}

		if (client->GetAccessTimeSpan() >= GetIdleTimeoutSeconds())
		{
			if (fileContext.m_LogDevice)
			{
				fileContext.m_LogDevice->Info(StringInfo::Format(L"Existing client keepalive expired, discarding [%s]", CSTR_ATOW(key)));
			}
			// Discard this client
			continue;
		}

		return client;
	}

	if (fileContext.m_LogDevice)
	{
		fileContext.m_LogDevice->Info(StringInfo::Format(L"Creating new client for [%s]", CSTR_ATOW(key)));
	}

	DepotClient client = FDepotClient::New(&fileContext);
	if (client->Connect(config))
	{
		if (fileContext.m_LogDevice)
		{
			fileContext.m_LogDevice->Info(StringInfo::Format(L"Successfully created new client [%s]", CSTR_ATOW(key)));
		}
		return client;
	}

	if (fileContext.m_LogDevice)
	{
		fileContext.m_LogDevice->Error(StringInfo::Format(L"Failed to created new client [%s] %s", CSTR_ATOW(key), CSTR_ATOW(client->GetErrorText())));
	}
	return nullptr;
}

void DepotClientCache::Free(const DepotConfig& config, DepotClient client)
{
	if (client.get())
	{
		AutoCriticalSection lock(m_FreeMapLock);
		client->SetContext(nullptr);
		m_FreeMap->insert(FreeMapType::value_type(CreateKey(config), client));
	}
}

void DepotClientCache::Clear()
{
	AutoCriticalSection lock(m_FreeMapLock);
	m_FreeMap->clear();
}

void DepotClientCache::GarbageCollect(int64_t timeoutSeconds)
{
	if (timeoutSeconds >= 0)
	{
		AutoCriticalSection lock(m_FreeMapLock);
		for (FreeMapType::iterator clientIt = m_FreeMap->begin(); clientIt != m_FreeMap->end();)
		{
			DepotClient client = clientIt->second;
			if (client->GetAccessTimeSpan() >= timeoutSeconds)
			{
				clientIt = m_FreeMap->erase(clientIt);
			}
			else
			{
				++clientIt;
			}
		}
	}
}

size_t DepotClientCache::GetFreeCount() const 
{ 
	return m_FreeMap->size(); 
}

time_t DepotClientCache::GetIdleTimeoutSeconds()
{
	return std::max<time_t>(0, FileCore::SettingManager::StaticInstance().DepotClientCacheIdleTimeoutMs.GetValue()/1000);
}

DepotString DepotClientCache::CreateKey(const DepotConfig& config)
{
	return StringInfo::Format("%s,%s,%s", config.m_Port.c_str(), config.m_User.c_str(), config.m_Client.c_str());
}

}}}

