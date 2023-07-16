// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "TestFactoryInterop.h"
#include "TestFactory.h"
#include "FileOperations.h"
#include "CoreInterop.h"
#include "CoreMarshal.h"
#include "DepotClientCache.h"

namespace Microsoft {
namespace P4VFS {
namespace CoreInterop {

void
TestFactoryInterop::Run(
	array<System::String^>^ args,
	TestContextInterop^ context
	)
{
	try
	{
		Assert(context != nullptr);
		TestCore::TestContext factoryContext;
		gcroot<TestContextInterop^> gccontext = context;

		P4::DepotClientCache depotClientCache;

		FileCore::FileContext fileContext;
		fileContext.m_LogDevice = &FileCore::LogSystem::StaticInstance();
		fileContext.m_DepotClientCache = &depotClientCache;
		factoryContext.m_FileContext = &fileContext;

		factoryContext.m_ReconcilePreviewAny = [&gccontext](const FileCore::String& s) -> bool 
		{ 
			return gccontext->m_ReconcilePreviewAny(Marshal::FromNativeWide(s.c_str())); 
		};

		factoryContext.m_WorkspaceReset = [&gccontext]() -> void 
		{ 
			gccontext->m_WorkspaceReset(); 
		};

		factoryContext.m_IsPlaceholderFile = [&gccontext](const FileCore::String& s) -> bool 
		{ 
			return gccontext->m_IsPlaceholderFile(Marshal::FromNativeWide(s.c_str())); 
		};

		factoryContext.m_ServiceLastRequestTime = [&gccontext]() -> FILETIME 
		{
			return marshal_as_filetime(gccontext->m_ServiceLastRequestTime());
		};

		Assert(context->m_Environment != nullptr);
		for each (System::Collections::Generic::KeyValuePair<System::String^, System::String^>^ item in context->m_Environment)
		{
			factoryContext.m_Environment.insert(TestCore::TestContext::EnvironmentMap::value_type(marshal_as_wstring(item->Key), marshal_as_wstring(item->Value)));
		}

		TestCore::TestFactory factory;
		FileCore::StringArray factoryArgs = Marshal::ToNativeWide(args);

		factory.Run(factoryArgs, factoryContext);
	}
	catch (std::exception e)
	{
		throw gcnew System::Exception(Marshal::FromNativeAnsi(e.what()));
	}
	catch (...)
	{
		throw gcnew System::Exception("TestFactoryInterop::Run unhandled exception");
	}
}

}}}

