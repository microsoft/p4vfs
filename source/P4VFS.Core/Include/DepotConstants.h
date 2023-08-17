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

	// https://www.perforce.com/manuals/p4sag/Content/P4SAG/protocol-levels.html
	#define DEPOT_PROTOCOL_LEVELS(_N) \
		_N( 2023_1, 56, 94 ) \
		_N( 2022_2, 55, 93 ) \
		_N( 2022_1, 54, 92 ) \
		_N( 2021_2, 53, 91 ) \
		_N( 2021_1, 52, 90 ) \
		_N( 2020_2, 51, 89 ) \
		_N( 2020_1, 50, 88 ) \
		_N( 2019_2, 49, 87 ) \
		_N( 2019_1, 47, 86 ) \
		_N( 2018_2, 46, 85 ) \
		_N( 2018_1, 45, 84 ) \
		_N( 2017_2, 44, 83 ) \
		_N( 2017_1, 43, 82 ) \
		_N( 2016_2, 42, 81 ) \
		_N( 2016_1, 41, 80 ) \
		_N( 2015_2, 40, 79 ) \
		_N( 2015_1, 39, 78 ) \
		_N( 2014_2, 38, 77 ) \
		_N( 2014_1, 37, 76 ) \
		_N( 2013_3, 36, 75 ) \
		_N( 2013_2, 35, 74 ) \
		_N( 2013_1, 34, 73 ) \
		_N( 2012_2, 33, 72 ) \
		_N( 2012_1, 32, 71 ) \
		_N( 2011_1, 31, 70 ) \
		_N( 2010_2, 30, 68 ) \
		_N( 2010_1, 29, 67 ) \
		_N( 2009_2, 28, 66 ) \
		_N( 2009_1, 27, 65 ) \
		_N( 2008_2, 26, 64 ) \
		_N( 2008_1, 25, 63 ) \
		_N( 2007_3, 24, 62 ) \
		_N( 2007_2, 23, 61 ) \
		_N( 2006_2, 22, 60 ) \
		_N( 2006_1, 21, 59 ) \
		_N( 2005_2, 20, 58 ) \
		_N( 2003_2, 17, 56 ) \
		_N( 2002_2, 14, 55 ) \
		_N( 2002_1, 13, 54 ) \
		_N( 2001_2, 12, 52 ) \
		_N( 2001_1, 11, 51 ) \

	struct DepotProtocol
	{
		#define DEPOT_PROTOCOL_LEVEL_INFO(name, server, client) \
			static constexpr int32_t SERVER_##name = server; \
			static constexpr int32_t CLIENT_##name = client; \
			static constexpr char* SERVER_##name##_STRING = #server; \
			static constexpr char* CLIENT_##name##_STRING = #client;
		DEPOT_PROTOCOL_LEVELS(DEPOT_PROTOCOL_LEVEL_INFO)
		#undef DEPOT_PROTOCOL_LEVEL_INFO

		static constexpr int32_t SERVER_ALT_SYNC = SERVER_2023_1;
		static constexpr int32_t SERVER_SIZES_C = SERVER_2023_1;
	};
	
}}}

#pragma managed(pop)
