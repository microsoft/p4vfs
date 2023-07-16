// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "SettingManagerInterop.h"
#include "CoreMarshal.h"

using namespace msclr::interop;

namespace Microsoft {
namespace P4VFS {
namespace CoreInterop {

SettingNode::SettingNode(
	)
{
}

SettingNode::SettingNode(
	System::String^ data
	)
{
	m_Data = data;
}

SettingNode::SettingNode(
	System::String^ data,
	System::String^ node
	)
{
	m_Data = data;
	m_Nodes = gcnew array<SettingNode^>(1){ gcnew SettingNode(node) };
}

SettingNode::SettingNode(
	System::String^ data,
	array<SettingNode^>^ nodes
	)
{
	m_Data = data;
	m_Nodes = nodes;
}

System::String^
SettingNode::ToString(
	)
{
	return m_Data != nullptr ? m_Data : nullptr;
}

System::String^
SettingNode::ToString(
	System::String^ defaultValue
	)
{
	return m_Data != nullptr ? m_Data : defaultValue;
}

System::Int32
SettingNode::ToInt32(
	)
{
	return ToInt32(0);
}

System::Int32
SettingNode::ToInt32(
	System::Int32 defaultValue
	)
{
	return ToNative().GetInt32(defaultValue);
}

System::Boolean
SettingNode::ToBool(
	)
{
	return ToBool(false);
}

System::Boolean
SettingNode::ToBool(
	System::Boolean defaultValue
	)
{
	return ToNative().GetBool(defaultValue);
}

FileCore::SettingNode
SettingNode::ToNative(
	)
{
	FileCore::SettingNode node;
	node.SetString(marshal_as_wstring(m_Data));
	if (m_Nodes != nullptr)
	{
		node.Nodes().resize(m_Nodes->Length);
		for (int32_t i = 0; i < m_Nodes->Length; ++i)
		{
			node.Nodes()[i] = m_Nodes[i] != nullptr ? m_Nodes[i]->ToNative() : FileCore::SettingNode();
		}
	}
	return node;
}
	
SettingNode^
SettingNode::FromString(
	System::String^ value
	)
{
	FileCore::SettingNode node;
	node.SetString(marshal_as_wstring(value));
	return FromNative(node);
}

SettingNode^
SettingNode::FromInt32(
	System::Int32 value
	)
{
	FileCore::SettingNode node;
	node.SetInt32(value);
	return FromNative(node);
}

SettingNode^
SettingNode::FromBool(
	bool value
	)
{
	FileCore::SettingNode node;
	node.SetBool(value);
	return FromNative(node);
}

SettingNode^ 
SettingNode::FromNative(
	const FileCore::SettingNode& srcNode
	)
{
	SettingNode^ node = gcnew SettingNode();
	node->m_Data = Marshal::FromNativeWide(srcNode.GetString().c_str());
	node->m_Nodes = gcnew array<SettingNode^>(int32_t(srcNode.Nodes().size()));
	for (uint32_t i = 0; i < srcNode.Nodes().size(); ++i)
	{
		node->m_Nodes[i] = FromNative(srcNode.Nodes()[i]);
	}
	return node;
}

void
SettingManager::Reset(
	)
{
	FileCore::SettingManager::StaticInstance().Reset();
}

SettingNode^ 
SettingManager::GetProperty(
	System::String^ name
	)
{
	FileCore::SettingNode node;
	if (FileCore::SettingManager::StaticInstance().GetProperty(marshal_as_wstring(name), node))
	{
		return SettingNode::FromNative(node);
	}
	return nullptr;
}

bool
SettingManager::SetProperty(
	System::String^ name,
	SettingNode^ value
	)
{
	const marshal_as_wstring cname(name);
	if (FileCore::SettingManager::StaticInstance().HasProperty(cname))
	{
		FileCore::SettingManager::StaticInstance().SetProperty(cname, value ? value->ToNative() : FileCore::SettingNode());
		return true;
	}
	return false;
}

SettingNodeMap^
SettingManager::GetProperties(
	)
{
	FileCore::SettingManager::PropertyMap propertyMap;
	FileCore::SettingManager::StaticInstance().GetProperties(propertyMap);
	
	SettingNodeMap^ nodeMap = gcnew SettingNodeMap();
	for (FileCore::SettingManager::PropertyMap::const_iterator propertyIt = propertyMap.begin(); propertyIt != propertyMap.end(); ++propertyIt)
	{
		nodeMap[Marshal::FromNativeWide(propertyIt->first.c_str())] = SettingNode::FromNative(propertyIt->second);
	}
	return nodeMap;
}

bool
SettingManager::SetProperties(
	SettingNodeMap^ nodeMap
	)
{
	if (nodeMap == nullptr)
	{
		return false;
	}
	if (nodeMap->Count > 0)
	{
		FileCore::SettingManager::PropertyMap propertyMap;
		for each (System::Collections::Generic::KeyValuePair<System::String^, SettingNode^>^ keyValue in nodeMap)
		{
			if (keyValue != nullptr)
			{
				propertyMap[marshal_as_wstring(keyValue->Key)] = keyValue->Value ? keyValue->Value->ToNative() : FileCore::SettingNode();
			}
		}
		FileCore::SettingManager::StaticInstance().SetProperties(propertyMap);
	}
	return true;
}

}}}

