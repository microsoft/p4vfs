// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "SettingManager.h"
#include "DepotSyncAction.h"
#include "FileSystem.h"
#include "CoreMarshal.h"

namespace Microsoft {
namespace P4VFS {
namespace CoreInterop {

public ref class SettingNode
{
public:

	SettingNode(
		);

	SettingNode(
		System::String^ data
		);

	SettingNode(
		System::String^ data,
		System::String^ node
		);

	SettingNode(
		System::String^ data,
		array<SettingNode^>^ nodes
		);

	System::String^
	ToString(
		) override;

	System::String^
	ToString(
		System::String^ defaultValue
		);

	System::Int32
	ToInt32(
		);

	System::Int32
	ToInt32(
		System::Int32 defaultValue
		);

	System::Boolean
	ToBool(
		);

	System::Boolean
	ToBool(
		System::Boolean defaultValue
		);

	FileCore::SettingNode
	ToNative(
		);
	
	static SettingNode^
	FromString(
		System::String^ value
		);

	static SettingNode^
	FromInt32(
		System::Int32 value
		);

	static SettingNode^
	FromBool(
		System::Boolean value
		);

	static SettingNode^ 
	FromNative(
		const FileCore::SettingNode& srcNode
		);

public:

	System::String^ m_Data;
	array<SettingNode^>^ m_Nodes;
};

public ref class SettingNodeMap : System::Collections::Generic::SortedDictionary<System::String^, SettingNode^>
{
};

namespace SettingTraits
{
	template <typename T> struct Value
	{
	};

	template <> struct Value<bool>
	{ 
		typedef System::Boolean ManagedType;
		
		static ManagedType 
		ToManagedType(
			SettingNode^ n, 
			bool defaultValue
			) 
		{ 
			return n != nullptr ? n->ToBool() : defaultValue; 
		}

		static SettingNode^ 
		FromManagedType(
			ManagedType v
			) 
		{ 
			return SettingNode::FromBool(v); 
		}
	};

	template <> struct Value<int32_t>
	{ 
		typedef System::Int32 ManagedType; 

		static ManagedType 
		ToManagedType(
			SettingNode^ n, 
			int32_t defaultValue
			) 
		{ 
			return n != nullptr ? n->ToInt32() : defaultValue; 
		}

		static SettingNode^ 
		FromManagedType(
			ManagedType v
			) 
		{ 
			return SettingNode::FromInt32(v); 
		}
	};

	template <> struct Value<FileCore::String>
	{ 
		typedef System::String^ ManagedType; 
		
		static ManagedType 
		ToManagedType(
			SettingNode^ n, 
			const wchar_t* defaultValue
			) 
		{ 
			return n != nullptr ? n->ToString() : Marshal::FromNativeWide(defaultValue); 
		}

		static SettingNode^ 
		FromManagedType(
			ManagedType v
			) 
		{ 
			return SettingNode::FromString(v); 
		}
	};
};

public ref class SettingManager abstract sealed
{
public:

	static void
	Reset(
		);

	static SettingNode^ 
	GetProperty(
		System::String^ name
		);

	static bool
	SetProperty(
		System::String^ name,
		SettingNode^ value
		);

	static SettingNodeMap^
	GetProperties(
		);

	static bool
	SetProperties(
		SettingNodeMap^ nodeMap
		);

	typedef FileCore::String String;

	#define SETTING_MANAGER_DECLARE_PROP(type, name, value) \
	static property SettingTraits::Value<type>::ManagedType name \
	{ \
		SettingTraits::Value<type>::ManagedType get() \
		{ \
			return SettingTraits::Value<type>::ToManagedType(GetProperty(#name), value); \
		} \
		void set(SettingTraits::Value<type>::ManagedType v) \
		{ \
			SetProperty(#name, SettingTraits::Value<type>::FromManagedType(v)); \
		} \
	};
	SETTING_MANAGER_PROPERTIES(SETTING_MANAGER_DECLARE_PROP)
	#undef SETTING_MANAGER_DECLARE_PROP
};

}}}

