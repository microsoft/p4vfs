// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "FileCore.h"
#include "LogDevice.h"
#pragma managed(push, off)

namespace Microsoft {
namespace P4VFS {
namespace ThreadPool {

	DWORD
	GetPoolMaxNumberOfThreads(
		);

	DWORD
	GetPoolDefaultNumberOfThreads(
		);

	struct ForEach
	{
		template <typename ItemType>
		struct Params
		{
			Params() :
				m_MaxThreads(0),
				m_ItemCount(0),
				m_Items(nullptr),
				m_CancelationEvent(NULL),
				m_Log(nullptr),
				m_Predicate(),
				m_Initialize(),
				m_Shutdown()
			{}

			size_t m_MaxThreads;
			size_t m_ItemCount;
			ItemType* m_Items;
			HANDLE m_CancelationEvent;
			FileCore::LogDevice* m_Log;
			std::function<void(ItemType&)> m_Predicate;
			std::function<bool()> m_Initialize;
			std::function<bool()> m_Shutdown;
		};

		template <typename ItemType>
		static size_t Execute(
			const Params<ItemType>& params
			)
		{
			if (params.m_Items == nullptr)
				return 0;
			if (params.m_Predicate == nullptr)
				return 0;

			size_t numThreads = std::min(params.m_MaxThreads, params.m_ItemCount);
			if (numThreads == 0)
				return 0;

			struct FThreadData
			{
				FThreadData(const Params<ItemType>* p) :
					m_ItemIndex(0),
					m_ItemsComplete(0),
					m_Params(p)
				{}

				std::atomic<size_t> m_ItemIndex;
				std::atomic<size_t> m_ItemsComplete;
				const Params<ItemType>* m_Params;
			};

			FThreadData threadData(&params);

			struct FThreadEntry
			{
				static DWORD Execute(void* data)
				{
					FThreadData& threadData = *reinterpret_cast<FThreadData*>(data);
					if (threadData.m_Params->m_Initialize != nullptr && threadData.m_Params->m_Initialize() == false)
					{
						if (threadData.m_Params->m_Log)
						{
							threadData.m_Params->m_Log->Error("ForEach failed to initialize action");
						}
						return 0;
					}

					while (true)
					{
						if (threadData.m_Params->m_CancelationEvent != NULL && WaitForSingleObject(threadData.m_Params->m_CancelationEvent, 0) == WAIT_OBJECT_0)
						{
							if (threadData.m_Params->m_Log)
							{
								threadData.m_Params->m_Log->Info("ForEach cancel requested from event");
							}
							break;
						}

						size_t index = threadData.m_ItemIndex++;
						if (index >= threadData.m_Params->m_ItemCount)
						{
							break;
						}

						threadData.m_Params->m_Predicate(threadData.m_Params->m_Items[index]);
						threadData.m_ItemsComplete++;
					}

					if (threadData.m_Params->m_Shutdown != nullptr && threadData.m_Params->m_Shutdown() == false)
					{
						if (threadData.m_Params->m_Log)
						{
							threadData.m_Params->m_Log->Error("ForEach failed to shutdown action");
						}
						return 0;
					}
					return 0;
				}
			};

			std::vector<HANDLE> threads;
			threads.resize(numThreads, NULL);
			for (size_t threadIndex = 0; threadIndex < numThreads; ++threadIndex)
			{
				threads[threadIndex] = CreateThread(NULL, 0, FThreadEntry::Execute, &threadData, 0, NULL);
			}

			WaitForMultipleObjects(DWORD(numThreads), threads.data(), TRUE, INFINITE);
			for (size_t threadIndex = 0; threadIndex < numThreads; ++threadIndex)
			{
				CloseHandle(threads[threadIndex]);
			}
			return threadData.m_ItemsComplete;
		}

		template <typename ItemType, typename PredicateType>
		static size_t Execute(
			size_t maxThreads, 
			ItemType* items, 
			size_t itemCount, 
			HANDLE cancelationEvent, 
			PredicateType predicate
			)
		{
			Params<ItemType> params;
			params.m_MaxThreads = maxThreads; 
			params.m_Items = items;
			params.m_ItemCount = itemCount;
			params.m_CancelationEvent = cancelationEvent; 
			params.m_Predicate = predicate;
			return Execute(params);
		}

		template <typename ItemType, typename PredicateType, typename InitializeType, typename ShutdownType>
		static size_t Execute(
			size_t maxThreads, 
			ItemType* items, 
			size_t itemCount, 
			HANDLE cancelationEvent, 
			PredicateType predicate,
			InitializeType initialize,
			ShutdownType shutdown
			)
		{
			Params<ItemType> params;
			params.m_MaxThreads = maxThreads; 
			params.m_Items = items;
			params.m_ItemCount = itemCount;
			params.m_CancelationEvent = cancelationEvent; 
			params.m_Predicate = predicate;
			params.m_Initialize = initialize;
			params.m_Shutdown = shutdown;
			return Execute(params);
		}

		template <typename ItemType, typename PredicateType>
		static size_t ExecuteImpersonated(
			size_t maxThreads, 
			ItemType* items, 
			size_t itemCount, 
			HANDLE cancelationEvent, 
			const FileCore::UserContext* context,
			PredicateType predicate
			)
		{
			Params<ItemType> params;
			params.m_MaxThreads = maxThreads; 
			params.m_Items = items;
			params.m_ItemCount = itemCount;
			params.m_CancelationEvent = cancelationEvent; 
			params.m_Predicate = predicate;
			params.m_Initialize = [context]() -> bool
			{
				return SUCCEEDED(FileOperations::ImpersonateLoggedOnUser(context));
			};
			params.m_Shutdown = [context]() -> bool
			{
				return !!RevertToSelf();
			};
			return Execute(params);
		}
	};
}}}
#pragma managed(pop)
