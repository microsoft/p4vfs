// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "FileCore.h"
#pragma managed(push, off)

namespace Microsoft {
namespace P4VFS {
namespace P4 {
	
	using namespace FileCore;
	typedef AString DepotString;
	typedef Array<DepotString> DepotStringArray;

	class FDepotNameTable
	{
	public:
		FDepotNameTable();
		~FDepotNameTable();

	private:
		size_t GetIndex(const char* name);
		DepotString GetString(size_t index);

		char* AllocEntry(const char* name);
		void FreeEntry(char* name);

		static FDepotNameTable& Instance();

	private:
		friend class FDepotName;
		static constexpr size_t IndexNone = size_t(-1);

		typedef Map<const char*, size_t, StringInfo::LessInsensitive> NameMapType;
		typedef Array<const char*> NameArrayType;
		NameMapType m_NameMap;
		NameArrayType m_NameArray;
		SRWLOCK m_Lock;
	};

	class FDepotName
	{
	public:
		FDepotName();
		FDepotName(const char* name);
		FDepotName(const DepotString& name);
		FDepotName(const FDepotName& name);

		FDepotName& operator=(const FDepotName& rhs);
		bool operator<(const FDepotName& rhs) const;

		P4VFS_CORE_API bool IsNone() const;
		P4VFS_CORE_API DepotString ToString() const;

	private:
		size_t m_Index;
	};

}}}

#pragma managed(pop)
