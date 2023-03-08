// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "DepotConstants.h"

namespace Microsoft {
namespace P4VFS {
namespace CoreInterop {

public ref class DepotConstants abstract sealed
{
public:
	static const System::String^ P4CHARSET		= gcnew System::String(P4::DepotConstants::P4CHARSET);
	static const System::String^ P4CLIENT		= gcnew System::String(P4::DepotConstants::P4CLIENT);
	static const System::String^ P4CONFIG		= gcnew System::String(P4::DepotConstants::P4CONFIG);
	static const System::String^ P4DIFF			= gcnew System::String(P4::DepotConstants::P4DIFF);
	static const System::String^ P4EDITOR		= gcnew System::String(P4::DepotConstants::P4EDITOR);
	static const System::String^ P4HOST			= gcnew System::String(P4::DepotConstants::P4HOST);
	static const System::String^ P4JOURNAL		= gcnew System::String(P4::DepotConstants::P4JOURNAL);
	static const System::String^ P4LANGUAGE		= gcnew System::String(P4::DepotConstants::P4LANGUAGE);
	static const System::String^ P4LOG			= gcnew System::String(P4::DepotConstants::P4LOG);
	static const System::String^ P4LOGINSSO		= gcnew System::String(P4::DepotConstants::P4LOGINSSO);
	static const System::String^ P4MERGE		= gcnew System::String(P4::DepotConstants::P4MERGE);
	static const System::String^ P4PAGER		= gcnew System::String(P4::DepotConstants::P4PAGER);
	static const System::String^ P4PASSWD		= gcnew System::String(P4::DepotConstants::P4PASSWD);
	static const System::String^ P4PORT			= gcnew System::String(P4::DepotConstants::P4PORT);
	static const System::String^ P4ROOT			= gcnew System::String(P4::DepotConstants::P4ROOT);
	static const System::String^ P4TICKETS		= gcnew System::String(P4::DepotConstants::P4TICKETS);
	static const System::String^ P4TRUST		= gcnew System::String(P4::DepotConstants::P4TRUST);
	static const System::String^ P4USER			= gcnew System::String(P4::DepotConstants::P4USER);
};

}}}

