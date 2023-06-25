// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "TestFactory.h"
#include "DepotClientCache.h"
#include "SettingManager.h"

using namespace Microsoft::P4VFS::FileCore;
using namespace Microsoft::P4VFS::TestCore;
using namespace Microsoft::P4VFS::P4;

void TestDepotClientCacheCommon(const TestContext& context)
{
	TestUtilities::WorkspaceReset(context);

	Assert(context.m_FileContext != nullptr);
	FileContext& fileContext = *context.m_FileContext;
	Assert(fileContext.m_DepotClientCache != nullptr);
	fileContext.m_DepotClientCache->Clear();

	DepotConfig config[3];
	config[0].m_Port = context.GetDepotConfig().m_Port;
	config[0].m_Client = "p4vfstest-depot";
	config[0].m_User = "p4vfstest";
	config[1].m_Port = context.GetDepotConfig().m_Port;
	config[1].m_Client = "quebec-pc-depot";
	config[1].m_User = "northamerica\\quebec";
	config[2].m_Port = context.GetDepotConfig().m_Port;
	config[2].m_Client = "montreal-pc-main";
	config[2].m_User = "northamerica\\montreal";

	auto allocateMultipleSerial = [&config = std::as_const(config), &fileContext](const size_t runCount) -> void
	{
		for (size_t runIndex = 0; runIndex < runCount; ++runIndex)
		{
			for (size_t configIndex = 0; configIndex < _countof(config); ++configIndex)
			{
				DepotClient client = fileContext.m_DepotClientCache->Alloc(config[configIndex], fileContext);
				Assert(client.get() != nullptr);
				Assert(client->Info()->HasError() == false);
				fileContext.m_DepotClientCache->Free(config[configIndex], client);
			}
		}
	};

	auto allocateMultipleParallel = [&config = std::as_const(config), &fileContext](const size_t clientCount) -> void
	{
		for (size_t configIndex = 0; configIndex < _countof(config); ++configIndex)
		{
			Array<DepotClient> clients;
			clients.resize(clientCount);
			for (size_t runIndex = 0; runIndex < clientCount; ++runIndex)
			{
				clients[runIndex] = fileContext.m_DepotClientCache->Alloc(config[configIndex], fileContext);
				Assert(clients[runIndex].get() != nullptr);
				Assert(clients[runIndex]->Info()->HasError() == false);
			}
			for (size_t runIndex = 0; runIndex < clientCount; ++runIndex)
			{
				 fileContext.m_DepotClientCache->Free(config[configIndex], clients[runIndex]);
			}
		}
	};

	// Allocate multiple clients many times and re-use
	allocateMultipleSerial(3);

	// There should only be one client for each unique config
	Assert(fileContext.m_DepotClientCache->GetFreeCount() == _countof(config));

	// Allocate multiple clients at once, which will re-use but also increase the cache size
	allocateMultipleParallel(2);

	// The cache size should peek as expected
	Assert(fileContext.m_DepotClientCache->GetFreeCount() == _countof(config)*2);

	// Purposely invalidate a few clients in the cache... these should be automatically removed during next allocate
	for (size_t configIndex = 0; configIndex < _countof(config); ++configIndex)
	{
		DepotClient client = fileContext.m_DepotClientCache->Alloc(config[configIndex], fileContext);
		client->Disconnect();
		fileContext.m_DepotClientCache->Free(config[configIndex], client);
	}

	// The cache size should remain the same
	Assert(fileContext.m_DepotClientCache->GetFreeCount() == _countof(config)*2);

	// Allocate multiple clients at once, which will re-use and remove the invalid entries
	allocateMultipleParallel(2);

	// The cache size should remain the same
	Assert(fileContext.m_DepotClientCache->GetFreeCount() == _countof(config)*2);

	// Allocation of various invalid configurations
	{
		SettingPropertyScope<bool> unattended(SettingManager::StaticInstance().Unattended, true);

		// A valid user & port with invalid clientspec will allocate connected and logged in, with uncreated client
		DepotConfig brokenConfig = config[0];
		brokenConfig.m_Client = "__non_existing_clientspec__";
		DepotClient client = fileContext.m_DepotClientCache->Alloc(brokenConfig, fileContext);
		Assert(client.get() != nullptr && client->IsConnectedClient() == false);
		Assert(client->Config().m_User == brokenConfig.m_User);
		Assert(client->Config().m_Client == brokenConfig.m_Client);

		// An invalid user with valid clientspec will allocate connected with specified clientspec owner as user
		brokenConfig = config[0];
		brokenConfig.m_User = "__non_existing_user__";
		client = fileContext.m_DepotClientCache->Alloc(brokenConfig, fileContext);
		Assert(client.get() != nullptr && client->IsConnectedClient());
		Assert(client->Config().m_User == client->Client()->Owner());
		Assert(client->Config().m_Client == brokenConfig.m_Client);

		// An invalid user & clientspec will allocate connected with any possible ticketed user, with uncreated client
		brokenConfig = config[0];
		brokenConfig.m_Client = "__non_existing_clientspec__";
		brokenConfig.m_User = "__non_existing_user__";
		client = fileContext.m_DepotClientCache->Alloc(brokenConfig, fileContext);
		Assert(client.get() != nullptr && client->IsConnectedClient() == false);
		Assert(client->Config().m_User != brokenConfig.m_User);
		Assert(client->Config().m_Client == brokenConfig.m_Client);

		// An invalid prot will not allocate connected
		brokenConfig = config[0];
		brokenConfig.m_Port = "__non_existing_p4vfstest_port__:1666";
		client = fileContext.m_DepotClientCache->Alloc(brokenConfig, fileContext);
		Assert(client.get() == nullptr);
	}

	// All original configs should be free 
	Assert(fileContext.m_DepotClientCache->GetFreeCount() == _countof(config)*2);

	// Clear the cache
	fileContext.m_DepotClientCache->Clear();
	Assert(fileContext.m_DepotClientCache->GetFreeCount() == 0);
}

