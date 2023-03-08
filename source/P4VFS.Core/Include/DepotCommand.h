// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "DepotResult.h"
#pragma managed(push, off)

namespace Microsoft {
namespace P4VFS {
namespace P4 {

	struct DepotCommand
	{
		struct Flags { enum Enum {
			None		= 0,
			UnTagged	= 1<<0,
		};};

		DepotCommand() :
			m_Flags(Flags::None)
		{}

		DepotCommand(const DepotString& name, const DepotStringArray& args = DepotStringArray(), Flags::Enum flags = Flags::None) :
			m_Name(name),
			m_Args(args),
			m_Flags(flags)
		{}

		DepotString m_Name;
		DepotStringArray m_Args;
		DepotString m_Input;
		DepotString m_Prompt;
		Flags::Enum m_Flags;
	};

	struct IDepotClientCommand
	{
		virtual void SetOutputEncoding(const DepotString& fileType) = 0;
	};

	DEFINE_ENUM_FLAG_OPERATORS(DepotCommand::Flags::Enum);

}}}

#pragma managed(pop)
