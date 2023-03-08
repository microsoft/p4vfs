// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "CoreInterop.h"

namespace Microsoft {
namespace P4VFS {
namespace CoreInterop {

public ref class TestContextInterop
{
public:
	System::Collections::Generic::Dictionary<System::String^, System::String^>^ m_Environment;
	System::Func<System::String^, bool>^ m_ReconcilePreviewAny;
	System::Action^ m_WorkspaceReset;
	System::Func<System::String^, bool>^ m_IsPlaceholderFile;
	System::Func<System::DateTime>^ m_ServiceLastRequestTime;
};

public ref class TestFactoryInterop abstract
{
public:

	static void
	Run(
		array<System::String^>^ args,
		TestContextInterop^ context
		);
};

}}}
