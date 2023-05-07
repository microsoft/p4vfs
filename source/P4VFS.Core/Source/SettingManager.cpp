// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "SettingManager.h"
#include "LogDevice.h"
#include "DepotSyncAction.h"
#include "DepotSyncOptions.h"
#include "FileSystem.h"
#include "FileAssert.h"
#include <shlwapi.h>

namespace Microsoft {
namespace P4VFS {
namespace FileCore {

SettingNode& SettingNode::operator=(const SettingNode& rhs)
{
	m_Data = rhs.m_Data;
	m_Nodes = rhs.m_Nodes;
	return *this;
}

SettingNode SettingNode::FirstNode(const String& data) const
{
	for (const SettingNode& node : m_Nodes)
	{
		if (StringInfo::Stricmp(node.Data().c_str(), data.c_str()) == 0)
		{
			return node;
		}
	}
	return SettingNode();
}

SettingNode SettingNode::FirstNode() const
{
	return m_Nodes.size() ? m_Nodes[0] : SettingNode();
}

String SettingNode::GetAttribute(const String& name) const
{
	return FirstNode(name).FirstNode().Data();
}

void SettingNode::SetBool(bool value)
{
	m_Data = value ? TEXT("True") : TEXT("False");
}

void SettingNode::SetInt32(int32_t value)
{
	m_Data = StringInfo::Format(TEXT("%d"), value);
}

void SettingNode::SetString(const String& value)
{
	m_Data = value;
}

bool SettingNode::GetBool(bool defaultValue) const
{
	if (StringInfo::Stricmp(m_Data.c_str(), TEXT("True")) == 0)
	{
		return true;
	}
	if (StringInfo::Stricmp(m_Data.c_str(), TEXT("False")) == 0)
	{
		return false;
	}
	int32_t value = 0;
	if (StrToIntExW(m_Data.c_str(), STIF_DEFAULT, &value))
	{
		return value;
	}
	return defaultValue;
}

int32_t SettingNode::GetInt32(int32_t defaultValue) const
{
	int32_t value = 0;
	if (StrToIntExW(m_Data.c_str(), STIF_DEFAULT, &value))
	{
		return value;
	}
	return defaultValue;
}

String SettingNode::GetString(const String& defaultValue) const
{
	return m_Data;
}

SettingPropertyBase::SettingPropertyBase() :
	m_Manager(nullptr)
{
}

SettingNode SettingPropertyBase::GetNode() const
{
	SettingNode node;
	m_Manager->GetProperty(m_Name, node);
	return node;
}

void SettingPropertyBase::SetNode(const SettingNode& node)
{
	m_Manager->SetProperty(m_Name, node);
}

template <typename ValueType>
void SettingProperty<ValueType>::Create(SettingManager* manager, const String& name, const ValueType& value)
{
	Assert(manager);
	m_Manager = manager;
	m_Name = name;
	SetValue(value);
}

template <typename ValueType>
const String& SettingProperty<ValueType>::GetName() const
{
	return m_Name;
}
		
template <typename ValueType>
ValueType SettingProperty<ValueType>::GetValue(const ValueType& defaultValue) const
{
	return GetNode().Get<ValueType>(defaultValue);
}

template <typename ValueType>
void SettingProperty<ValueType>::SetValue(const ValueType& value)
{
	SettingNode node;
	node.Set(value);
	SetNode(node);
}

template SettingProperty<bool>;
template SettingProperty<int32_t>;
template SettingProperty<String>;
	
template <typename ValueType>
SettingPropertyScope<ValueType>::SettingPropertyScope(SettingProperty<ValueType>& prop, const ValueType& value)
{
	m_Property = &prop;
	m_PreviousValue = m_Property->GetValue();
	m_Property->SetValue(value);
}

template <typename ValueType>
SettingPropertyScope<ValueType>::~SettingPropertyScope()
{
	m_Property->SetValue(m_PreviousValue);
	m_Property = nullptr;
}

template SettingPropertyScope<bool>;
template SettingPropertyScope<int32_t>;
template SettingPropertyScope<String>;

SettingManager::SettingManager()
{
	m_PropertMapMutex = CreateMutex(NULL, FALSE, NULL);
	Reset();
}

SettingManager::~SettingManager()
{
	SafeCloseHandle(m_PropertMapMutex);
}

SettingManager& SettingManager::StaticInstance()
{
	static SettingManager instance;
	return instance;
}

void SettingManager::Reset()
{
	#define SETTING_MANAGER_DEFINE_PROP(type, name, value) name##.Create(this, TEXT(#name), value);
	SETTING_MANAGER_PROPERTIES(SETTING_MANAGER_DEFINE_PROP)
	#undef SETTING_MANAGER_DEFINE_PROP
}

bool SettingManager::HasProperty(const String& propertyName)
{
	AutoMutex lock(m_PropertMapMutex);
	return m_PropertMap.find(propertyName) != m_PropertMap.end();
}

void SettingManager::SetProperty(const String& propertyName, const SettingNode& propertyValue)
{
	AutoMutex lock(m_PropertMapMutex);
	m_PropertMap[propertyName] = propertyValue;
}

bool SettingManager::GetProperty(const String& propertyName, SettingNode& propertyValue) const
{
	AutoMutex lock(m_PropertMapMutex);
	PropertyMap::const_iterator propIt = m_PropertMap.find(propertyName);
	if (propIt != m_PropertMap.end())
	{
		propertyValue = propIt->second;
		return true;
	}
	return false;
}

void SettingManager::SetProperties(const PropertyMap& propertyMap)
{
	AutoMutex lock(m_PropertMapMutex);
	for (PropertyMap::const_iterator propertyIt = propertyMap.begin(); propertyIt != propertyMap.end(); ++propertyIt)
	{
		m_PropertMap[propertyIt->first] = propertyIt->second;
	}
}

bool SettingManager::GetProperties(PropertyMap& propertyMap) const
{
	AutoMutex lock(m_PropertMapMutex);
	propertyMap.clear();
	for (PropertyMap::const_iterator propertyIt = m_PropertMap.begin(); propertyIt != m_PropertMap.end(); ++propertyIt)
	{
		propertyMap[propertyIt->first] = propertyIt->second;
	}
	return true;
}

}}}

