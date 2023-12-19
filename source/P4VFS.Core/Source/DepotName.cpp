// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "DepotName.h"

namespace Microsoft {
namespace P4VFS {
namespace P4 {

FDepotNameTable::FDepotNameTable()
{
	InitializeSRWLock(&m_Lock);
}

FDepotNameTable::~FDepotNameTable()
{
}

size_t FDepotNameTable::GetIndex(const char* name)
{
	if (name == nullptr)
	{
		return IndexNone;
	}

	AcquireSRWLockShared(&m_Lock);
	NameMapType::const_iterator entry = m_NameMap.find(name);
	size_t index = entry != m_NameMap.end() ? entry->second : IndexNone;
	ReleaseSRWLockShared(&m_Lock);

	if (index == IndexNone)
	{
		AcquireSRWLockExclusive(&m_Lock);
		const char* entryName = AllocEntry(name);
		index = m_NameArray.size();
		m_NameArray.push_back(entryName);
		m_NameMap.insert(NameMapType::value_type(entryName, index));
		ReleaseSRWLockExclusive(&m_Lock);
	}
	return index;
}

DepotString FDepotNameTable::GetString(size_t index)
{
	AcquireSRWLockShared(&m_Lock);
	DepotString name = index < m_NameArray.size() ? m_NameArray[index] : DepotString();
	ReleaseSRWLockShared(&m_Lock);
	return name;
}

char* FDepotNameTable::AllocEntry(const char* name)
{
	size_t entrySize = StringInfo::Strlen(name) + 1;
	char* entryName = (char*)GAlloc(entrySize);
	memcpy(entryName, name, entrySize);
	return entryName;
}

void FDepotNameTable::FreeEntry(char* name)
{
	GFree(name);
}

FDepotNameTable& FDepotNameTable::Instance()
{
	static FDepotNameTable* table = nullptr;
	if (table == nullptr)
	{
		static char tableBuffer[sizeof(FDepotNameTable)];
		table = new(tableBuffer) FDepotNameTable();
	}
	return *table;
}

FDepotName::FDepotName() :
	m_Index(FDepotNameTable::IndexNone)
{
}

FDepotName::FDepotName(const char* name)
{
	m_Index = FDepotNameTable::Instance().GetIndex(name);
}

FDepotName::FDepotName(const DepotString& name) : 
	FDepotName(name.c_str())
{
}

FDepotName::FDepotName(const FDepotName& name)
{
	*this = name;
}

FDepotName& FDepotName::operator=(const FDepotName& rhs)
{
	m_Index = rhs.m_Index;
	return *this;
}

bool FDepotName::operator<(const FDepotName& rhs) const
{
	return m_Index < rhs.m_Index;
}

bool FDepotName::IsNone() const
{
	return m_Index == FDepotNameTable::IndexNone;
}

DepotString FDepotName::ToString() const
{
	return FDepotNameTable::Instance().GetString(m_Index);
}

}}}

