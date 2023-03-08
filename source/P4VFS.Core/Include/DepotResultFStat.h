// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "DepotResult.h"
#pragma managed(push, off)

namespace Microsoft {
namespace P4VFS {
namespace P4 {

	struct FDepotResultFStatField
	{
		struct Name
		{
			static constexpr const char* ClientFile		= "clientFile";
			static constexpr const char* DepotFile		= "depotFile";
			static constexpr const char* MovedFile		= "movedFile";
			static constexpr const char* Path			= "path";
			static constexpr const char* IsMapped		= "isMapped";
			static constexpr const char* Shelved		= "shelved";
			static constexpr const char* HeadAction		= "headAction";
			static constexpr const char* HeadChange		= "headChange";
			static constexpr const char* HeadRev		= "headRev";
			static constexpr const char* HeadType		= "headType";
			static constexpr const char* HeadTime		= "headTime";
			static constexpr const char* HeadModTime	= "headModTime";
			static constexpr const char* MovedRev		= "movedRev";
			static constexpr const char* HaveRev		= "haveRev";
			static constexpr const char* Desc			= "desc";
			static constexpr const char* Digest			= "digest";
			static constexpr const char* FileSize		= "fileSize";
			static constexpr const char* Action			= "action";
			static constexpr const char* Type			= "type";
			static constexpr const char* ActionOwner	= "actionOwner";
			static constexpr const char* Change			= "change";
			static constexpr const char* Resolved		= "resolved";
			static constexpr const char* Unresolved		= "unresolved";
			static constexpr const char* OtherOpen		= "otherOpen";
			static constexpr const char* OtherLock		= "otherLock";
			static constexpr const char* OurLock		= "ourLock";
		};

		enum Enum
		{
			Default			= 0,
			DepotFile		= 1<<0,
			ClientFile		= 1<<1,
			FileSize		= 1<<2,
			HaveRev			= 1<<3,
			HeadRev			= 1<<4,
			HeadType		= 1<<5,
		};

		static DepotStringArray ToNames(Enum types);
	};

	struct FDepotResultFStatNode : FDepotResultNode
	{
		bool InDepot() const					{ return DepotFile().empty() == false && HeadAction() != "delete"; }
		const DepotString& ClientFile() const	{ return GetTagValue(FDepotResultFStatField::Name::ClientFile); }
		const DepotString& DepotFile() const	{ return GetTagValue(FDepotResultFStatField::Name::DepotFile); }
		const DepotString& MovedFile() const	{ return GetTagValue(FDepotResultFStatField::Name::MovedFile); }
		const DepotString& Path() const			{ return GetTagValue(FDepotResultFStatField::Name::Path); }
		bool IsMapped() const					{ return ContainsTagKey(FDepotResultFStatField::Name::IsMapped); }
		bool Shelved() const					{ return ContainsTagKey(FDepotResultFStatField::Name::Shelved); }
		const DepotString& HeadAction() const	{ return GetTagValue(FDepotResultFStatField::Name::HeadAction); }
		int32_t HeadChange() const				{ return GetTagValueInt32(FDepotResultFStatField::Name::HeadChange); }
		int32_t HeadRev() const					{ return GetTagValueInt32(FDepotResultFStatField::Name::HeadRev); }
		const DepotString& HeadType() const		{ return GetTagValue(FDepotResultFStatField::Name::HeadType); }
		int32_t HeadTime() const				{ return GetTagValueInt32(FDepotResultFStatField::Name::HeadTime); }
		int32_t HeadModTime() const				{ return GetTagValueInt32(FDepotResultFStatField::Name::HeadModTime); }
		int32_t MovedRev() const				{ return GetTagValueInt32(FDepotResultFStatField::Name::MovedRev); }
		int32_t HaveRev() const					{ return GetTagValueInt32(FDepotResultFStatField::Name::HaveRev); }
		const DepotString& Desc() const			{ return GetTagValue(FDepotResultFStatField::Name::Desc); }
		const DepotString& Digest() const		{ return GetTagValue(FDepotResultFStatField::Name::Digest); }
		int64_t FileSize() const				{ return GetTagValueInt64(FDepotResultFStatField::Name::FileSize); }
		const DepotString& Action() const		{ return GetTagValue(FDepotResultFStatField::Name::Action); }
		const DepotString& Type() const			{ return GetTagValue(FDepotResultFStatField::Name::Type); }
		const DepotString& ActionOwner() const	{ return GetTagValue(FDepotResultFStatField::Name::ActionOwner); }
		int32_t Change() const					{ return GetTagValueInt32(FDepotResultFStatField::Name::Change); }
		const DepotString& Resolved() const		{ return GetTagValue(FDepotResultFStatField::Name::Resolved); }
		const DepotString& Unresolved() const	{ return GetTagValue(FDepotResultFStatField::Name::Unresolved); }
		bool OtherOpen() const					{ return ContainsTagKey(FDepotResultFStatField::Name::OtherOpen); }
		bool OtherLock() const					{ return ContainsTagKey(FDepotResultFStatField::Name::OtherLock); }
		bool OurLock() const					{ return ContainsTagKey(FDepotResultFStatField::Name::OurLock); }
	};

	typedef FDepotResultNodeProvider<FDepotResultFStatNode> FDepotResultFStat;
	typedef std::shared_ptr<FDepotResultFStat> DepotResultFStat;

	DEFINE_ENUM_FLAG_OPERATORS(FDepotResultFStatField::Enum);
}}}

#pragma managed(pop)
