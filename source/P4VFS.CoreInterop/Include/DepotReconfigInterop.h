// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "CoreInterop.h"
#include "DepotReconfig.h"

namespace Microsoft {
namespace P4VFS {
namespace CoreInterop {

[System::FlagsAttribute]
public enum class DepotReconfigFlags : System::Int32
{
	None		= P4::DepotReconfigFlags::None,
	Preview		= P4::DepotReconfigFlags::Preview,
	P4Port		= P4::DepotReconfigFlags::P4Port,
	P4Client	= P4::DepotReconfigFlags::P4Client,
	P4User		= P4::DepotReconfigFlags::P4User,
};

public ref class DepotReconfigOptions
{
public:
	DepotReconfigOptions();

	P4::FDepotReconfigOptions ToNative();

	array<System::String^>^ Files;
	DepotReconfigFlags Flags;
};

}}}

