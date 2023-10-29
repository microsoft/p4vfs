// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "FileCore.h"
#include "FileContext.h"
#include "FileAssert.h"
#include "DepotConfig.h"
#pragma managed(push, off)

namespace Microsoft {
namespace P4VFS {
namespace TestCore {

	using FileCore::String;
	using FileCore::StringInfo;
	using FileCore::StringArray;

	struct TestContext
	{
		P4VFS_CORE_API TestContext();

		String GetEnvironment(const wchar_t* key) const;
		P4::DepotConfig GetDepotConfig() const;
		FileCore::LogDevice* Log() const;

		typedef FileCore::Map<String, String, StringInfo::LessInsensitive> EnvironmentMap;
		EnvironmentMap m_Environment;
		FileCore::FileContext* m_FileContext;

		std::function<bool(String)> m_ReconcilePreviewAny;
		std::function<void()> m_WorkspaceReset;
		std::function<bool(String)> m_IsPlaceholderFile;
		std::function<FILETIME()> m_ServiceLastRequestTime;
	};

	typedef void (*TestDelegate)(const TestContext& context);

	struct TestFlags 
	{
		enum Enum
		{
			None		= 0,
			Explicit	= 1<<0,
		};
	};

	DEFINE_ENUM_FLAG_OPERATORS(TestFlags::Enum);

	struct TestObject
	{	
		TestObject(const wchar_t* name, const wchar_t* filename, int32_t priority, TestDelegate exec, TestFlags::Enum flags = TestFlags::None);

		String m_Name;
		String m_Filename;
		int32_t m_Priority;
		TestDelegate m_Exec;
		TestFlags::Enum m_Flags;
	};

	typedef TestObject (*TestObjectCreator)();
	typedef FileCore::Array<TestObjectCreator> TestObjectCreatorList;
	typedef FileCore::Array<TestObject> TestObjectList;

	class TestFactory
	{
	public:
		P4VFS_CORE_API TestFactory();
		P4VFS_CORE_API void Run(const StringArray& args, const TestContext& context);

	private:
		bool IsRequested(const StringArray& args, const TestObject& test) const;
		
		friend struct TestRegistrator;
		static TestObjectCreatorList& Registry();

	private:
		TestObjectList m_Tests;
	};

	struct TestRegistrator
	{
		TestRegistrator(TestObjectCreator Creator) { TestFactory::Registry().push_back(Creator); }
		~TestRegistrator() {}
	};

	struct P4VFS_CORE_API TestUtilities
	{
		static void WorkspaceReset(const TestContext& context);
		static DWORD ExecuteWait(const FileCore::String& cmd, FileCore::String* stdOutput = nullptr);
		static DWORD ExecuteLogWait(const FileCore::String& cmd, const TestContext& context, const wchar_t* logLinePrefix = nullptr);
	};
}}}

#define P4VFS_REGISTER_TEST(name, priority, ...) \
	__declspec(dllexport) Microsoft::P4VFS::TestCore::TestObject TestFactoryCreateTestObject##name() \
	{ \
		using namespace Microsoft::P4VFS::TestCore; \
		extern void name(const Microsoft::P4VFS::TestCore::TestContext&); \
		return Microsoft::P4VFS::TestCore::TestObject(TEXT(#name), TEXT(__FILE__), priority, &name, __VA_ARGS__); \
	} \
	static Microsoft::P4VFS::TestCore::TestRegistrator TestRegistrator##name(&TestFactoryCreateTestObject##name); \

#define P4VFS_FIND_TEST(name) \
	([]() -> Microsoft::P4VFS::TestCore::TestObject \
	{ \
		extern Microsoft::P4VFS::TestCore::TestObject TestFactoryCreateTestObject##name(); \
		return TestFactoryCreateTestObject##name(); \
	})()

#pragma managed(pop)
