// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "DirectoryOperations.h"
#include "FileCore.h"
#include "ThreadPool.h"

namespace Microsoft {
namespace P4VFS {
namespace DirectoryOperations {

struct IDP
{
	enum class ItemType
	{
		None,
		File,
		Directory,
	};

	struct Item
	{
		ItemType m_Type;
		FileCore::String m_Path;
		DWORD m_Attributes;

		Item() :
			m_Type(ItemType::None),
			m_Attributes(INVALID_FILE_ATTRIBUTES)
		{}
	};

	typedef std::list<std::shared_ptr<Item>> ItemQueue;

	struct SharedContext : FileCore::NonCopyable<SharedContext>
	{
		ItemQueue m_Queue;
		int32_t m_QueueInFlight;
		HANDLE m_QueueLock;
		HANDLE m_QueueSemaphore;
		HANDLE m_CancelationEvent;
		IterateDirectoryVisitor m_Visitor;
		IterateDirectoryFlags::Enum m_Flags;

		SharedContext() :
			m_QueueInFlight(0),
			m_QueueLock(NULL),
			m_QueueSemaphore(NULL),
			m_CancelationEvent(NULL),
			m_Flags(IterateDirectoryFlags::None)
		{}
	};

	struct ThreadContext
	{
		SharedContext* m_Shared;
		int32_t m_ThreadIndex;
		HANDLE m_ThreadHandle;

		ThreadContext() :
			m_Shared(NULL),
			m_ThreadIndex(-1),
			m_ThreadHandle(NULL)
		{}
	};

	static bool IsCancelRequested(SharedContext* shared)
	{
		return WaitForSingleObject(shared->m_CancelationEvent, 0) == WAIT_OBJECT_0;
	}

	static void AddItem(SharedContext* shared, std::shared_ptr<Item> item)
	{
		if (WaitForSingleObject(shared->m_QueueLock, INFINITE) == WAIT_OBJECT_0)
		{
			if (shared->m_Flags & IterateDirectoryFlags::BreadthFirst)
			{
				shared->m_Queue.push_front(item);
			}
			else
			{
				shared->m_Queue.push_back(item);
			}
			ReleaseMutex(shared->m_QueueLock);
			ReleaseSemaphore(shared->m_QueueSemaphore, 1, NULL);
		}
	}

	static void AddItem(SharedContext* shared, ItemType type, const FileCore::String& path, DWORD dwAttributes)
	{
		std::shared_ptr<Item> item = std::make_shared<Item>();
		item->m_Type = type;
		item->m_Path = path;
		item->m_Attributes = dwAttributes;
		AddItem(shared, item);
	}

	static bool TryBeginItem(SharedContext* shared, std::shared_ptr<Item>& outItem)
	{
		bool result = false;
		if (WaitForSingleObject(shared->m_QueueLock, INFINITE) == WAIT_OBJECT_0)
		{
			if (shared->m_Queue.empty() == false)
			{
				outItem = shared->m_Queue.back();
				shared->m_Queue.pop_back();
				shared->m_QueueInFlight++;
				result = true;
			}
			ReleaseMutex(shared->m_QueueLock);
		}
		return result;
	}

	static void EndItem(SharedContext* shared, const std::shared_ptr<Item>& item)
	{
		if (WaitForSingleObject(shared->m_QueueLock, INFINITE) == WAIT_OBJECT_0)
		{
			shared->m_QueueInFlight--;
			if (shared->m_Queue.empty() && shared->m_QueueInFlight <= 0)
			{
				// The queue is empty and nothing in flight. Iteration is finished.
				SetEvent(shared->m_CancelationEvent);
			}
			ReleaseMutex(shared->m_QueueLock);
		}
	}

	static void VisitItem(SharedContext* shared, const std::shared_ptr<Item>& item)
	{
		if (item.get())
		{
			if (shared->m_Visitor)
			{
				// Execute the visitor predicate and skip deeper directory iteration if false. 
				if (shared->m_Visitor(item->m_Path, item->m_Attributes) == false)
				{
					return;
				}
			}

			if (item->m_Type == ItemType::Directory)
			{
				FileCore::String root = item->m_Path;
				if (root.size() > 0 && root[root.size()-1] != TEXT('\\'))
				{
					root += TEXT("\\");
				}
	
				WIN32_FIND_DATA findFileData = {0};
				HANDLE hFind = ::FindFirstFile(FileCore::StringInfo::Format(TEXT("%s*"), root.c_str()).c_str(), &findFileData);
				if (hFind != INVALID_HANDLE_VALUE)
				{
					do
					{
						if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
						{
							if (FileCore::StringInfo::Strcmp(findFileData.cFileName, TEXT(".")) != 0 && FileCore::StringInfo::Strcmp(findFileData.cFileName, TEXT("..")) != 0)
							{
								// Queue asyncronous visitation for this directory
								AddItem(shared, ItemType::Directory, root + findFileData.cFileName, findFileData.dwFileAttributes);
							}
						}
						else
						{
							// Queue asyncronous visitation for this file
							AddItem(shared, ItemType::File, root + findFileData.cFileName, findFileData.dwFileAttributes);
						}
					}
					while (::FindNextFile(hFind, &findFileData));
					::FindClose(hFind);
				}
			}
		}
	}

	static DWORD ThreadEntry(void* data)
	{
		ThreadContext* thread = reinterpret_cast<ThreadContext*>(data);
		while (IsCancelRequested(thread->m_Shared) == false)
		{
			// Wait for a chance to take an item from the queue
			const HANDLE handles[] = { thread->m_Shared->m_QueueSemaphore, thread->m_Shared->m_CancelationEvent };
			if (WaitForMultipleObjects(_countof(handles), handles, FALSE, INFINITE) != WAIT_OBJECT_0)
			{
				continue;
			}

			// Try to consume as many items from the queue as possible
			while (IsCancelRequested(thread->m_Shared) == false)
			{				
				std::shared_ptr<Item> item;
				if (TryBeginItem(thread->m_Shared, item) == false)
				{
					//LogInfo("Starved: Thread=%d", thread->m_ThreadIndex);
					break;
				}

				VisitItem(thread->m_Shared, item);
				EndItem(thread->m_Shared, item);
			}
		}
		return 0;
	}
};

HRESULT IterateDirectoryParallel(const wchar_t* folderPath, IterateDirectoryVisitor visitor, int32_t numThreads, IterateDirectoryFlags::Enum flags)
{
	FileCore::AutoHandle queueLock = CreateMutex(NULL, FALSE, NULL);
	FileCore::AutoHandle queueSemaphore = CreateSemaphore(NULL, 0, LONG_MAX, NULL);
	FileCore::AutoHandle cancelationEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (queueLock.IsValid() == false || queueSemaphore.IsValid() == false || cancelationEvent.IsValid() == false)
	{
		return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
	}

	IDP::SharedContext shared;
	shared.m_QueueLock = queueLock.Handle();
	shared.m_QueueSemaphore = queueSemaphore.Handle();
	shared.m_CancelationEvent = cancelationEvent.Handle();
	shared.m_Visitor = visitor;
	shared.m_Flags = flags;

	const int32_t maxThreads = ThreadPool::GetPoolMaxNumberOfThreads();
	const int32_t numRequestThreads = numThreads <= 0 ? ThreadPool::GetPoolDefaultNumberOfThreads() : numThreads;
	const int32_t numWorkerThreads = std::max(0, std::min(numRequestThreads-1, maxThreads));

	std::vector<IDP::ThreadContext> threads;
	threads.resize(static_cast<size_t>(numWorkerThreads));

	for (IDP::ThreadContext& thread : threads)
	{
		thread.m_Shared = &shared;
		thread.m_ThreadIndex = static_cast<int32_t>(&thread - threads.data());
		thread.m_ThreadHandle = CreateThread(NULL, 0, &IDP::ThreadEntry, &thread, 0, NULL);
	}

	IDP::AddItem(&shared, IDP::ItemType::Directory, FileCore::FileInfo::FullPath(folderPath), FILE_ATTRIBUTE_DIRECTORY);

	IDP::ThreadContext mainThread;
	mainThread.m_Shared = &shared;
	IDP::ThreadEntry(&mainThread);

	for (IDP::ThreadContext& thread : threads)
	{
		WaitForSingleObject(thread.m_ThreadHandle, INFINITE);
		CloseHandle(thread.m_ThreadHandle);
	}

	return S_OK;
}

}}}
