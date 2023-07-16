// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "DepotConstants.h"
#include "CoreMarshal.h"

namespace Microsoft {
namespace P4VFS {
namespace CoreInterop {

public ref class DepotConstants abstract sealed
{
public:
	static initonly System::String^ P4CHARSET		= Marshal::FromNativeAnsi(P4::DepotConstants::P4CHARSET);
	static initonly System::String^ P4CLIENT		= Marshal::FromNativeAnsi(P4::DepotConstants::P4CLIENT);
	static initonly System::String^ P4CONFIG		= Marshal::FromNativeAnsi(P4::DepotConstants::P4CONFIG);
	static initonly System::String^ P4DIFF			= Marshal::FromNativeAnsi(P4::DepotConstants::P4DIFF);
	static initonly System::String^ P4EDITOR		= Marshal::FromNativeAnsi(P4::DepotConstants::P4EDITOR);
	static initonly System::String^ P4HOST			= Marshal::FromNativeAnsi(P4::DepotConstants::P4HOST);
	static initonly System::String^ P4JOURNAL		= Marshal::FromNativeAnsi(P4::DepotConstants::P4JOURNAL);
	static initonly System::String^ P4LANGUAGE		= Marshal::FromNativeAnsi(P4::DepotConstants::P4LANGUAGE);
	static initonly System::String^ P4LOG			= Marshal::FromNativeAnsi(P4::DepotConstants::P4LOG);
	static initonly System::String^ P4LOGINSSO		= Marshal::FromNativeAnsi(P4::DepotConstants::P4LOGINSSO);
	static initonly System::String^ P4MERGE			= Marshal::FromNativeAnsi(P4::DepotConstants::P4MERGE);
	static initonly System::String^ P4PAGER			= Marshal::FromNativeAnsi(P4::DepotConstants::P4PAGER);
	static initonly System::String^ P4PASSWD		= Marshal::FromNativeAnsi(P4::DepotConstants::P4PASSWD);
	static initonly System::String^ P4PORT			= Marshal::FromNativeAnsi(P4::DepotConstants::P4PORT);
	static initonly System::String^ P4ROOT			= Marshal::FromNativeAnsi(P4::DepotConstants::P4ROOT);
	static initonly System::String^ P4TICKETS		= Marshal::FromNativeAnsi(P4::DepotConstants::P4TICKETS);
	static initonly System::String^ P4TRUST			= Marshal::FromNativeAnsi(P4::DepotConstants::P4TRUST);
	static initonly System::String^ P4USER			= Marshal::FromNativeAnsi(P4::DepotConstants::P4USER);
};

}}}

