// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "DepotResult.h"
#include "DepotConfig.h"
#pragma managed(push, off)

namespace Microsoft {
namespace P4VFS {
namespace P4 {

	struct FDepotResultClientOption
	{
		struct Name
		{
			static constexpr const char* AllWrite	= "allwrite";
			static constexpr const char* NoAllWrite = "noallwrite";
			static constexpr const char* Clobber	= "clobber";
			static constexpr const char* NoClobber	= "noclobber";
			static constexpr const char* Compress	= "compress";
			static constexpr const char* NoCompress = "nocompress";
			static constexpr const char* Locked		= "locked";
			static constexpr const char* UnLocked	= "unlocked";
			static constexpr const char* ModTime	= "modtime";
			static constexpr const char* NoModTime	= "nomodtime";
			static constexpr const char* RmDir		= "rmdir";
			static constexpr const char* NoRmDir	= "normdir";
		};

		enum Enum
		{
			None		= 0,
			AllWrite	= 1<<0,
			Clobber		= 1<<1,
			Compress	= 1<<2,
			Locked		= 1<<3,
			ModTime		= 1<<4,
			RmDir		= 1<<5,
		};

		static DepotString ToString(FDepotResultClientOption::Enum value);
		static Enum FromString(const DepotString& text);
	};

	struct FDepotResultLineEnd
	{
		static constexpr const char* Local	= "local";
		static constexpr const char* Unix	= "unix";
		static constexpr const char* Mac	= "mac";
		static constexpr const char* Win	= "win";
		static constexpr const char* Share	= "share";
	};

	struct FDepotResultClientField
	{
		struct Name
		{
			static constexpr const char* Client				= "Client";
			static constexpr const char* Update				= "Update";
			static constexpr const char* Access				= "Access";
			static constexpr const char* Owner				= "Owner";
			static constexpr const char* Options			= "Options";
			static constexpr const char* SubmitOptions		= "SubmitOptions";
			static constexpr const char* LineEnd			= "LineEnd";
			static constexpr const char* Root				= "Root";
			static constexpr const char* Host				= "Host";
			static constexpr const char* Description		= "Description";
			static constexpr const char* Stream				= "Stream";
			static constexpr const char* StreamAtChange		= "StreamAtChange";
			static constexpr const char* ServerID			= "ServerID";
		};
	};

	struct FDepotResultClient : FDepotResult
	{
		const DepotString& Client() const					{ return GetTagValue(FDepotResultClientField::Name::Client); }
		const DepotString& Update() const					{ return GetTagValue(FDepotResultClientField::Name::Update); }
		const DepotString& Access() const					{ return GetTagValue(FDepotResultClientField::Name::Access); }
		const DepotString& Owner() const					{ return GetTagValue(FDepotResultClientField::Name::Owner); }
		const DepotString& Options() const					{ return GetTagValue(FDepotResultClientField::Name::Options); }
		const DepotString& SubmitOptions() const			{ return GetTagValue(FDepotResultClientField::Name::SubmitOptions); }
		const DepotString& LineEnd() const					{ return GetTagValue(FDepotResultClientField::Name::LineEnd); }
		const DepotString& Root() const						{ return GetTagValue(FDepotResultClientField::Name::Root); }
		const DepotString& Host() const						{ return GetTagValue(FDepotResultClientField::Name::Host); }
		const DepotString& Description() const				{ return GetTagValue(FDepotResultClientField::Name::Description); }
		const DepotString& Stream() const					{ return GetTagValue(FDepotResultClientField::Name::Stream); }
		const DepotString& StreamAtChange() const			{ return GetTagValue(FDepotResultClientField::Name::StreamAtChange); }
		const DepotString& ServerID() const					{ return GetTagValue(FDepotResultClientField::Name::ServerID); }
		FDepotResultClientOption::Enum OptionFlags() const	{ return FDepotResultClientOption::FromString(Options()); }

		DepotConfig Config() const;
		time_t UpdateTime() const;
		time_t AccessTime() const;
	};

	typedef std::shared_ptr<FDepotResultClient> DepotResultClient;

	DEFINE_ENUM_FLAG_OPERATORS(FDepotResultClientOption::Enum);
}}}

#pragma managed(pop)
