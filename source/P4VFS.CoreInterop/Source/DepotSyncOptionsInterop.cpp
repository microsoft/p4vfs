// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "DepotSyncOptionsInterop.h"
#include "CoreMarshal.h"

namespace Microsoft {
namespace P4VFS {
namespace CoreInterop {

DepotSyncOptions::DepotSyncOptions() :
	Files(nullptr),
	Revision(nullptr),
	SyncType(DepotSyncType::Normal),
	SyncMethod(DepotSyncMethod::Virtual),
	FlushType(DepotFlushType::Atomic),
	Context(nullptr)
{
}

P4::FDepotSyncOptions DepotSyncOptions::ToNative()
{
	P4::FDepotSyncOptions dst;
	dst.m_Revision = P4::FDepotRevision::FromString(marshal_as_astring(Revision));
	dst.m_SyncType = safe_cast<P4::DepotSyncType::Enum>(SyncType);
	dst.m_FlushType = safe_cast<P4::DepotFlushType::Enum>(FlushType);
	dst.m_SyncMethod = safe_cast<P4::DepotSyncMethod::Enum>(SyncMethod);
	dst.m_SyncResident = marshal_as_astring(SyncResident);
	dst.m_Context = marshal_as_user_context(Context).Get();

	if (Files != nullptr)
	{
		dst.m_Files.reserve(Files->Length);
		for each (System::String^ file in Files)
		{
			dst.m_Files.push_back(marshal_as_astring(file));
		}
	}
	return dst;
}

}}}

