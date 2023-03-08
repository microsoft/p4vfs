// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "TestFactory.h"
#include <regex>

namespace Microsoft {
namespace P4VFS {
namespace TestCore {

	TestContext::TestContext() :
		m_FileContext(nullptr)
	{
	}

	String TestContext::GetEnvironment(const wchar_t* key) const
	{
		if (StringInfo::IsNullOrEmpty(key) == false)
		{
			EnvironmentMap::const_iterator it = m_Environment.find(key);
			if (it != m_Environment.end())
				return it->second;
		}
		return String();
	}

	P4::DepotConfig TestContext::GetDepotConfig() const
	{
		P4::DepotConfig config;
		config.m_Client = StringInfo::ToAnsi(GetEnvironment(L"P4CLIENT"));
		config.m_User = StringInfo::ToAnsi(GetEnvironment(L"P4USER"));
		config.m_Host = StringInfo::ToAnsi(GetEnvironment(L"P4HOST"));
		config.m_Port = StringInfo::ToAnsi(GetEnvironment(L"P4PORT"));
		config.m_Passwd = StringInfo::ToAnsi(GetEnvironment(L"P4PASSWD"));
		return config;
	}

	FileCore::LogDevice* TestContext::Log() const
	{
		Assert(m_FileContext && m_FileContext->m_LogDevice);
		return m_FileContext->m_LogDevice;
	}

	TestObject::TestObject(const wchar_t* name, const wchar_t* filename, int32_t priority) :
		m_Name(name),
		m_Filename(filename),
		m_Priority(priority),
		m_Exec(nullptr)
	{
	}

	TestObject::TestObject(const wchar_t* name, const wchar_t* filename, int32_t priority, TestDelegate exec) :
		TestObject(name, filename, priority)
	{
		m_Exec = exec;
	}

	TestFactory::TestFactory()
	{
		for (const TestObjectCreator& creator : Registry())
		{
			m_Tests.push_back((*creator)());
		}
	}

	void TestFactory::Run(const StringArray& args, const TestContext& context)
	{
		TestObjectList executingTests;
		for (const TestObject& test : m_Tests)
		{
			if (IsRequested(args, test))
				executingTests.push_back(test);
		}

		WCHAR previousTitle[1024];
		DWORD previousTitleLen = GetConsoleTitle(previousTitle, _countof(previousTitle));

		std::sort(executingTests.begin(), executingTests.end(), [](const TestObject& a, const TestObject& b) -> bool { return a.m_Priority < b.m_Priority; });
		for (const TestObject& test : executingTests)
		{
			SetConsoleTitle(StringInfo::Format(TEXT("P4VFS UnitTest [%d] TestCore.%s"), test.m_Priority, test.m_Name.c_str()).c_str());
			context.Log()->Info(StringInfo::Format(TEXT("Running native unit test: %s"), test.m_Name.c_str()));
			if (test.m_Exec != nullptr)
				(*test.m_Exec)(context);
		}

		if (previousTitleLen)
			SetConsoleTitle(previousTitle);
	}

	bool TestFactory::IsRequested(const StringArray& args, const TestObject& test) const
	{
		if (args.size() == 0)
			return true;
		for (const String& arg : args)
		{
			if (StringInfo::Stricmp(arg.c_str(), test.m_Name.c_str()) == 0)
				return true;
			if (arg == StringInfo::ToString(test.m_Priority))
				return true;
			std::match_results<const wchar_t*> match;
			if (std::regex_search(arg.c_str(), match, std::wregex(L"^\\s*\\[?(\\d*):(\\d*)\\]?\\s*$")))
			{
				String strBegin = match.str(1);
				String strEnd = match.str(2);
				int32_t begin = strBegin.empty() ? 0 : _wtoi(strBegin.c_str());
				int32_t end = strEnd.empty() ? std::numeric_limits<int32_t>::max() : _wtoi(strEnd.c_str());
				if (test.m_Priority >= begin && test.m_Priority <= end)
					return true;
			}
		}
		return false;
	}

	TestObjectCreatorList& TestFactory::Registry()
	{
		static TestObjectCreatorList registry;
		return registry;
	}

	void TestUtilities::WorkspaceReset(const TestContext& context)
	{
		context.m_WorkspaceReset();
	}

	DWORD TestUtilities::ExecuteWait(const FileCore::String& fileName, const FileCore::String& args, FileCore::String* stdOutput)
	{
		using namespace FileCore;
		if (fileName.empty())
			return DWORD(-1);

		Process::ExecuteFlags::Enum flags = Process::ExecuteFlags::HideWindow | Process::ExecuteFlags::WaitForExit;
		if (stdOutput != nullptr)
			flags |= Process::ExecuteFlags::StdOut;

		String cmd = StringInfo::Format(L"\"%s\"", fileName.c_str());
		if (args.empty() == false)
		{
			cmd += L" ";
			cmd += args;
		}

		Process::ExecuteResult result = Process::Execute(cmd.c_str(), nullptr, flags);
		if (stdOutput != nullptr)
			stdOutput->assign(StringInfo::ToWide(result.m_StdOut));

		return result.m_ExitCode;
	}
}}}

