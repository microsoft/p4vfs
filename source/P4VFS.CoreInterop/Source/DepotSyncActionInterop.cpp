// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "DepotSyncActionInterop.h"
#include "CoreMarshal.h"

namespace Microsoft {
namespace P4VFS {
namespace CoreInterop {

bool FDepotSyncActionType::IsError(DepotSyncActionType value)
{
	return P4::DepotSyncActionType::IsError(safe_cast<P4::DepotSyncActionType::Enum>(value));
}

bool FDepotSyncActionType::IsChanged(DepotSyncActionType value)
{
	return P4::DepotSyncActionType::IsChanged(safe_cast<P4::DepotSyncActionType::Enum>(value));
}

bool FDepotSyncActionType::IsLocalChanged(DepotSyncActionType value)
{
	return P4::DepotSyncActionType::IsLocalChanged(safe_cast<P4::DepotSyncActionType::Enum>(value));
}

DepotSyncActionInfo::DepotSyncActionInfo() :
	DepotFile(nullptr),
	ClientFile(nullptr),
	Revision(nullptr),
	SyncActionType(DepotSyncActionType::None)
{
}

P4::DepotSyncActionInfo DepotSyncActionInfo::ToNative()
{
	P4::DepotSyncActionInfo dst = std::make_shared<P4::FDepotSyncActionInfo>();
	dst->m_DepotFile = marshal_as_astring(DepotFile);
	dst->m_ClientFile = marshal_as_astring(ClientFile);
	dst->m_Revision = P4::FDepotRevision::FromString(marshal_as_astring(Revision));
	dst->m_SyncActionType = safe_cast<P4::DepotSyncActionType::Enum>(SyncActionType);
	dst->m_Message = marshal_as_astring(Message);
	return dst;
}

DepotSyncActionInfo^ DepotSyncActionInfo::FromNative(const P4::DepotSyncActionInfo& src)
{
	DepotSyncActionInfo^ dst = nullptr;
	if (src.get() != nullptr)
	{
		dst = gcnew DepotSyncActionInfo();
		dst->DepotFile = gcnew System::String(src->m_DepotFile.c_str());
		dst->ClientFile = gcnew System::String(src->m_ClientFile.c_str());
		dst->Revision = gcnew System::String(P4::FDepotRevision::ToString(src->m_Revision).c_str());
		dst->SyncActionType = safe_cast<DepotSyncActionType>(src->m_SyncActionType);
		dst->Message = gcnew System::String(src->m_Message.c_str());
	}
	return dst;
}

DepotSyncResult::DepotSyncResult() :
	Status(DepotSyncStatus::Success),
	Modifications(nullptr)
{
}

P4::DepotSyncResult DepotSyncResult::ToNative()
{
	P4::DepotSyncResult dst = std::make_shared<P4::FDepotSyncResult>();
	dst->m_Status = safe_cast<P4::DepotSyncStatus::Enum>(Status);
	if (Modifications != nullptr)
	{
		const size_t resultCount = size_t(std::max(0, Modifications->Length));
		dst->m_Modifications = std::make_shared<P4::DepotSyncActionInfoArray::element_type>();
		dst->m_Modifications->resize(resultCount);
		for (size_t resultIndex = 0; resultIndex < resultCount; ++resultIndex)
		{
			DepotSyncActionInfo^ modification = Modifications[int32_t(resultIndex)];
			dst->m_Modifications->at(resultIndex) = (modification != nullptr ? modification->ToNative() : nullptr);
		}
	}
	return dst;
}

DepotSyncResult^ DepotSyncResult::FromNative(const P4::DepotSyncResult& src)
{
	DepotSyncResult^ dst = nullptr;
	if (src.get() != nullptr)
	{
		dst = gcnew DepotSyncResult();
		dst->Status = safe_cast<DepotSyncStatus>(src->m_Status);
		if (src->m_Modifications.get() != nullptr)
		{
			const size_t resultCount = src->m_Modifications->size();
			dst->Modifications = gcnew array<DepotSyncActionInfo^>(int32_t(resultCount));
			for (size_t resultIndex = 0; resultIndex < resultCount; ++resultIndex)
			{
				dst->Modifications[int32_t(resultIndex)] = DepotSyncActionInfo::FromNative(src->m_Modifications->at(resultIndex));
			}
		}
	}
	return dst;
}

}}}

