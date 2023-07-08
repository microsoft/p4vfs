// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "FileCore.h"
#pragma managed(push, off)

namespace Microsoft {
namespace P4VFS {
namespace FileCore {

	#define SETTING_MANAGER_PROPERTIES(_N) \
		_N( bool,     AllowSymlinkResidencyPolicy,     false ) \
		_N( bool,     ConsoleImmediateLogging,         true ) \
		_N( bool,     ConsoleRemoteLogging,            false ) \
		_N( String,   DefaultFlushType,                CSTR_ATOW(P4::DepotFlushType::ToString(P4::DepotFlushType::Atomic)) ) \
		_N( String,   DepotServerConfig,               L"" ) \
		_N( String,   FileLoggerLocalDirectory,        L"%PUBLIC%\\Public Logs\\P4VFS" ) \
		_N( String,   FileLoggerRemoteDirectory,       L"" ) \
		_N( bool,     ImmediateLogging,                false ) \
		_N( int32_t,  MaxSyncConnections,              8 ) \
		_N( String,   PopulateMethod,                  CSTR_ATOW(FileSystem::FilePopulateMethod::ToString(FileSystem::FilePopulateMethod::Stream)) ) \
		_N( bool,     RemoteLogging,                   false ) \
		_N( bool,     ReportUsageExternally,           false ) \
		_N( bool,     SyncDefaultQuiet,                false ) \
		_N( String,   SyncResidentPattern,             L"" ) \
		_N( bool,     Unattended,                      false ) \
		_N( String,   Verbosity,                       CSTR_ATOW(FileCore::LogChannel::ToString(FileCore::LogChannel::Info)) ) \
		_N( String,   ExcludedProcessNames,            L"" ) \
		_N( int32_t,  CreateFileRetryCount,            8 ) \
		_N( int32_t,  CreateFileRetryWaitMs,           250 ) \
		_N( int32_t,  PoolDefaultNumberOfThreads,      8 ) \
		_N( int32_t,  GarbageCollectPeriodMs,          5*60*1000 ) \
		_N( int32_t,  DepotClientCacheIdleTimeoutMs,   5*60*1000 ) \


	class SettingManager;
	typedef Array<class SettingNode> SettingNodeArray;

	struct SettingTraits
	{
		template <typename T> struct Value
		{
		};
		template <> struct Value<bool>
		{ 
			typedef bool DataType; 
			static const DataType& Default() { static const DataType v = false; return v; }
		};
		template <> struct Value<int32_t>
		{ 
			typedef int32_t DataType; 
			static const DataType& Default() { static const DataType v = 0; return v; }
		};
		template <> struct Value<String>
		{ 
			typedef String DataType; 
			static const DataType& Default() { return StringInfo::EmptyW(); }
		};
	};

	class SettingNode
	{
	public:
		SettingNode() {}
		SettingNode(const SettingNode& rhs) { *this = rhs; }

		P4VFS_CORE_API SettingNode& operator=(const SettingNode& rhs);

		const String& Data() const { return m_Data; }
		const SettingNodeArray& Nodes() const { return m_Nodes; }
		SettingNodeArray& Nodes() { return m_Nodes; }

		P4VFS_CORE_API SettingNode FirstNode(const String& data) const;
		P4VFS_CORE_API SettingNode FirstNode() const;
		P4VFS_CORE_API String GetAttribute(const String& name) const;

		void Set(bool value) { SetBool(value); }
		void Set(int32_t value) { SetInt32(value); }
		void Set(const String& value) { SetString(value); }

		P4VFS_CORE_API void SetBool(bool value);
		P4VFS_CORE_API void SetInt32(int32_t value);
		P4VFS_CORE_API void SetString(const String& value);

		template <typename T> T Get(const T& defaultValue) const = 0;
		template <> bool Get<bool>(const bool& defaultValue) const { return GetBool(defaultValue); }
		template <> int32_t Get<int32_t>(const int32_t& defaultValue) const { return GetInt32(defaultValue); }
		template <> String Get<String>(const String& defaultValue) const { return GetString(defaultValue); }

		P4VFS_CORE_API bool GetBool(bool defaultValue = SettingTraits::Value<bool>::Default()) const;
		P4VFS_CORE_API int32_t GetInt32(int32_t defaultValue = SettingTraits::Value<int32_t>::Default()) const;
		P4VFS_CORE_API String GetString(const String& defaultValue = SettingTraits::Value<String>::Default()) const;

	private:
		String m_Data;
		SettingNodeArray m_Nodes;
	};

	class SettingPropertyBase
	{
	protected:
		SettingPropertyBase();

	public:
		P4VFS_CORE_API SettingNode		GetNode() const;
		P4VFS_CORE_API void				SetNode(const SettingNode& node);

	protected:
		String m_Name;
		SettingManager* m_Manager;
	};
	
	template <typename ValueType>
	class SettingProperty : public SettingPropertyBase
	{
	public:
		P4VFS_CORE_API void				Create(SettingManager* manager, const String& name, const ValueType& value);
		P4VFS_CORE_API const String&	GetName() const;
		P4VFS_CORE_API ValueType		GetValue(const ValueType& defaultValue = SettingTraits::Value<ValueType>::Default()) const;
		P4VFS_CORE_API void				SetValue(const ValueType& value);
	};

	template <typename ValueType>
	class SettingPropertyScope : public FileCore::NonCopyable<SettingPropertyScope<ValueType>>
	{
	public:
		P4VFS_CORE_API SettingPropertyScope(SettingProperty<ValueType>& prop, const ValueType& value);
		P4VFS_CORE_API ~SettingPropertyScope();

	private:
		SettingProperty<ValueType>* m_Property;
		ValueType m_PreviousValue;
	};

	class SettingManager
	{
	private:
		SettingManager();
		~SettingManager();

	public:
		P4VFS_CORE_API static SettingManager& StaticInstance();

		P4VFS_CORE_API void Reset();
		P4VFS_CORE_API bool HasProperty(const String& propertyName);
		P4VFS_CORE_API void SetProperty(const String& propertyName, const SettingNode& propertyValue);
		P4VFS_CORE_API bool GetProperty(const String& propertyName, SettingNode& propertyValue) const;

		typedef Map<String, SettingNode, StringInfo::LessInsensitive> PropertyMap;
		P4VFS_CORE_API void SetProperties(const PropertyMap& propertyMap);
		P4VFS_CORE_API bool GetProperties(PropertyMap& propertyMap) const;

		#define SETTING_MANAGER_DECLARE_PROP(type, name, value)  SettingProperty<type> name;
		SETTING_MANAGER_PROPERTIES(SETTING_MANAGER_DECLARE_PROP)
		#undef SETTING_MANAGER_DECLARE_PROP

	private:
		PropertyMap m_PropertMap;
		HANDLE m_PropertMapMutex;
	};

}}}
#pragma managed(pop)
