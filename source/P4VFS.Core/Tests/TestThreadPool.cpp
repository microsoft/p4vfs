// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "TestFactory.h"
#include "ThreadPool.h"
#include "FileOperations.h"

using namespace Microsoft::P4VFS::FileCore;
using namespace Microsoft::P4VFS::TestCore;
using namespace Microsoft::P4VFS::ThreadPool;


void TestThreadPool(const TestContext& context)
{
	int32_t itemSum = 0;
	Array<int32_t> items;
	for (int i = 0; i < 50; ++i)
	{
		items.push_back(i*7);
		itemSum += items.back();
	}
	{
		std::atomic<int32_t> threadItemSum = 0;
		std::atomic<int32_t> threadItemCount = 0;
		ForEach::Execute(4, items.data(), items.size(), NULL, [&](int32_t value) -> void
		{
			uint32_t id = GetCurrentThreadId();
			Sleep(id % 500);
			context.Log()->Verbose(StringInfo::Format("[0] ForEach %u [%d]", id, value));
			threadItemSum += value;
			threadItemCount += 1;
		});
		Assert(threadItemSum == itemSum);
		Assert(threadItemCount == int32_t(items.size()));
	}
	{
		std::atomic<int32_t> threadItemSum = 0;
		std::atomic<int32_t> threadItemCount = 0;
		ForEach::Execute(4, items.data(), items.size(), NULL, [&](int32_t value) -> void
		{
			uint32_t id = GetCurrentThreadId();
			Sleep(id % 500);
			context.Log()->Verbose(StringInfo::Format("[1] ForEach %u [%d]", id, value));
			threadItemSum += value;
			threadItemCount += 1;
		}, [&]() -> bool
		{
			uint32_t id = GetCurrentThreadId();
			context.Log()->Verbose(StringInfo::Format("[1] ForEach Initialize %u", id));
			return true;
		}, [&]() -> bool
		{
			uint32_t id = GetCurrentThreadId();
			context.Log()->Verbose(StringInfo::Format("[1] ForEach Shutdown %u", id));
			return true;
		});
		Assert(threadItemSum == itemSum);
		Assert(threadItemCount == int32_t(items.size()));
	}
	{
		std::atomic<int32_t> threadItemSum = 0;
		std::atomic<int32_t> threadItemCount = 0;
		Assert(context.m_FileContext != nullptr);
		ForEach::ExecuteImpersonated(4, items.data(), items.size(), NULL, context.m_FileContext->m_UserContext, [&](int32_t value) -> void
		{
			uint32_t id = GetCurrentThreadId();
			Sleep(id % 500);
			context.Log()->Verbose(StringInfo::Format("[0] ForEach %u [%d]", id, value));
			threadItemSum += value;
			threadItemCount += 1;
		});
		Assert(threadItemSum == itemSum);
		Assert(threadItemCount == int32_t(items.size()));
	}
}

