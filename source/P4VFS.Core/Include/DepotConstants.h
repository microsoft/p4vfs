// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "DepotResult.h"
#pragma managed(push, off)

namespace Microsoft {
namespace P4VFS {
namespace P4 {

	struct DepotConstants
	{
		static constexpr char* P4CHARSET	= "P4CHARSET";
		static constexpr char* P4CLIENT		= "P4CLIENT";
		static constexpr char* P4CONFIG		= "P4CONFIG";
		static constexpr char* P4DIFF		= "P4DIFF";
		static constexpr char* P4EDITOR		= "P4EDITOR";
		static constexpr char* P4HOST		= "P4HOST";
		static constexpr char* P4JOURNAL	= "P4JOURNAL";
		static constexpr char* P4LANGUAGE	= "P4LANGUAGE";
		static constexpr char* P4LOG		= "P4LOG";
		static constexpr char* P4LOGINSSO	= "P4LOGINSSO";
		static constexpr char* P4MERGE		= "P4MERGE";
		static constexpr char* P4PAGER		= "P4PAGER";
		static constexpr char* P4PASSWD		= "P4PASSWD";
		static constexpr char* P4PORT		= "P4PORT";
		static constexpr char* P4ROOT		= "P4ROOT";
		static constexpr char* P4TICKETS	= "P4TICKETS";
		static constexpr char* P4TRUST		= "P4TRUST";
		static constexpr char* P4USER		= "P4USER";

		static constexpr char* COMPUTERNAME	= "COMPUTERNAME";
		static constexpr char* USERPROFILE	= "USERPROFILE";
	};

}}}

#pragma managed(pop)
