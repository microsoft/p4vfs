// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "DepotReconfig.h"

namespace Microsoft {
namespace P4VFS {
namespace P4 {

DepotString DepotReconfigFlags::ToString(DepotReconfigFlags::Enum value)
{
	DepotString result;
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotReconfigFlags, None);
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotReconfigFlags, Preview);
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotReconfigFlags, Quiet);
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotReconfigFlags, P4Port);
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotReconfigFlags, P4Client);
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotReconfigFlags, P4User);
	return result;
}

FDepotReconfigOptions::FDepotReconfigOptions() :
	m_Flags(DepotReconfigFlags::None)
{
}

FDepotReconfigOptions::~FDepotReconfigOptions()
{
}

}}}

