// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "DepotResult.h"
#pragma managed(push, off)

namespace Microsoft {
namespace P4VFS {
namespace P4 {

	typedef std::shared_ptr<struct FDepotRevision> DepotRevision;

	enum class DepotRevisionType
	{
		Empty,
		Range,
		Date,
		Number,
		Now,
		None,
		Changelist,
		Have,
		Head,
		Label,
	};

	#define DEPOTREVISION_BODY(type) \
		static const DepotRevisionType RevisionType = DepotRevisionType::##type; \
		virtual DepotRevisionType GetType() const { return RevisionType; } \
		P4VFS_CORE_API static DepotRevision FromString(const DepotString& text); \
		P4VFS_CORE_API virtual DepotString ToString() const; \

	struct FDepotRevision
	{
		DEPOTREVISION_BODY(Empty);

		P4VFS_CORE_API FDepotRevision();
		P4VFS_CORE_API virtual ~FDepotRevision();
		
		template <typename ToType, typename FromType, 
			typename std::enable_if<std::is_pointer_v<ToType>>::type* = nullptr, 
			typename std::enable_if<std::is_pointer_v<FromType>>::type* = nullptr>
		static ToType Cast(FromType revision)
		{
			return revision && std::remove_pointer<ToType>::type::RevisionType == revision->GetType() ? static_cast<ToType>(revision) : nullptr;
		}

		template <typename RevType, typename... ArgTypes>
		static DepotRevision New(ArgTypes&&... args)
		{
			return std::make_shared<RevType>(std::forward<ArgTypes>(args)...);
		}

		P4VFS_CORE_API static DepotString ToString(const DepotRevision& revision);

		bool IsHeadRevision() const;
		bool IsHaveRevision() const;
		bool IsNoneRevision() const;
		bool IsRevisionString(const DepotString& text) const;
	};

	struct FDepotRevisionNone : FDepotRevision
	{
		DEPOTREVISION_BODY(None);
	};

	struct FDepotRevisionHave : FDepotRevision
	{
		DEPOTREVISION_BODY(Have);
	};

	struct FDepotRevisionHead : FDepotRevision
	{
		DEPOTREVISION_BODY(Head);
	};

	struct FDepotRevisionNumber : FDepotRevision
	{
		DEPOTREVISION_BODY(Number);

		FDepotRevisionNumber(int32_t value = 0) : m_Value(value) {}
		int32_t m_Value;
	};

	struct FDepotRevisionLabel : FDepotRevision
	{
		DEPOTREVISION_BODY(Label);

		FDepotRevisionLabel() {}
		FDepotRevisionLabel(const DepotString& value) : m_Value(value) {}
		DepotString m_Value;
	};

	struct FDepotRevisionRange : FDepotRevision
	{
		DEPOTREVISION_BODY(Range);

		FDepotRevisionRange() {}
		FDepotRevisionRange(const DepotRevision& start, const DepotRevision& end) : m_Start(start), m_End(end) {}
		DepotRevision m_Start;
		DepotRevision m_End;
	};

	struct FDepotRevisionChangelist : FDepotRevision
	{
		DEPOTREVISION_BODY(Changelist);

		FDepotRevisionChangelist(int32_t value = 0) : m_Value(value) {}
		int32_t m_Value = 0;
	};

	struct FDepotRevisionNow : FDepotRevision
	{
		DEPOTREVISION_BODY(Now);
	};

	struct FDepotRevisionDate : FDepotRevision
	{
		DEPOTREVISION_BODY(Date);

		FDepotRevisionDate(time_t value = 0) : m_Value(value) {}
		time_t m_Value = 0;
	};
}}}

#pragma managed(pop)
