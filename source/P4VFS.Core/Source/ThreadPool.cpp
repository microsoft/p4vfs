// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "ThreadPool.h"
#include "SettingManager.h"

namespace Microsoft {
namespace P4VFS {
namespace ThreadPool {

DWORD
GetPoolMaxNumberOfThreads(
	)
{
	static const DWORD count = ([]() -> DWORD {
		SYSTEM_INFO systemInfo = {0};
		GetSystemInfo(&systemInfo);
		return std::max<DWORD>(1, systemInfo.dwNumberOfProcessors);
	})();
	return count;
}

DWORD
GetPoolDefaultNumberOfThreads(
	)
{
	int32_t settingNumThreads = FileCore::SettingManager::StaticInstance().PoolDefaultNumberOfThreads.GetValue();
	DWORD numThreads = std::max<DWORD>(1, std::min<DWORD>(settingNumThreads, GetPoolMaxNumberOfThreads()));
	return numThreads;
}

}}}
