// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "DepotClient.h"
#pragma managed(push, off)

namespace Microsoft {
namespace P4VFS {
namespace P4 {

	class P4VFS_CORE_API DepotClientCache
	{
	public:
		DepotClientCache();
		~DepotClientCache();

		DepotClient Alloc(const DepotConfig& config, FileContext& fileContext);
		void Free(const DepotConfig& config, DepotClient client);
		void Clear();
		void GarbageCollect(int64_t timeoutSeconds);
		
		size_t GetFreeCount() const;

	private:
		static DepotString CreateKey(const DepotConfig& config);

	private:
		typedef UnorderedMultiMap<DepotString, DepotClient, StringInfo::Hash, StringInfo::EqualInsensitive> FreeMapType;

		const int64_t m_KeepAliveSeconds;
		CriticalSection m_FreeMapLock;
		FreeMapType* m_FreeMap;
	};

}}}

#pragma managed(pop)
