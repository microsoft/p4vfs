// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "DepotResult.h"
#pragma managed(push, off)

namespace Microsoft {
namespace P4VFS {
namespace P4 {

	struct FDepotResultDiff2Field
	{
		struct Name
		{
			static constexpr const char* Status 		= "status";
			static constexpr const char* DepotFile 		= "depotFile";
			static constexpr const char* Rev			= "rev";
			static constexpr const char* Type 			= "type";
			static constexpr const char* DepotFile2		= "depotFile2";
			static constexpr const char* Rev2			= "rev2";
			static constexpr const char* Type2 			= "type2";
		};
	};

	struct FDepotResultDiff2Node : FDepotResultNode
	{
		const DepotString& Status() 			{ return GetTagValue(FDepotResultDiff2Field::Name::Status); }
		const DepotString& DepotFile() 			{ return GetTagValue(FDepotResultDiff2Field::Name::DepotFile); }
		int32_t Rev() 							{ return Tag().GetValueInt32(FDepotResultDiff2Field::Name::Rev); }
		const DepotString& Type() 				{ return GetTagValue(FDepotResultDiff2Field::Name::Type); }
		const DepotString& DepotFile2()			{ return GetTagValue(FDepotResultDiff2Field::Name::DepotFile2); }
		int32_t Rev2()							{ return Tag().GetValueInt32(FDepotResultDiff2Field::Name::Rev2); }
		const DepotString& Type2() 				{ return GetTagValue(FDepotResultDiff2Field::Name::Type2); }
	};

	typedef FDepotResultNodeProvider<FDepotResultDiff2Node> FDepotResultDiff2;
	typedef std::shared_ptr<FDepotResultDiff2> DepotResultDiff2;

}}}

#pragma managed(pop)
