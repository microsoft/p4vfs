// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "CoreInterop.h"
#include "DepotConfig.h"

namespace Microsoft {
namespace P4VFS {
namespace CoreInterop {

public ref class DepotConfig
{
public:
	virtual System::String^ ToString() override;

	P4::DepotConfig ToNative();
	static DepotConfig^ FromNative(const P4::DepotConfig& src);

	property System::String^ Host;
	property System::String^ Port;
	property System::String^ Client;
	property System::String^ User;
	property System::String^ Passwd;
	property System::String^ Ignore;
	property System::String^ Directory;
};

}}}

