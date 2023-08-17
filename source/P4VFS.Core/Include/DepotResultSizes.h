// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "DepotResult.h"
#pragma managed(push, off)

namespace Microsoft {
namespace P4VFS {
namespace P4 {

	struct FDepotResultSizesField
	{
		struct Name
		{
			static constexpr const char* DepotFile 		= "depotFile";
			static constexpr const char* FileSize 		= "fileSize";
			static constexpr const char* FileCount		= "fileCount";
			static constexpr const char* Path			= "path";
			static constexpr const char* Rev			= "rev";
		};
	};

	struct FDepotResultSizesNode : FDepotResultNode
	{
		const DepotString& DepotFile() 			{ return GetTagValue(FDepotResultSizesField::Name::DepotFile); }
		int64_t FileSize() 						{ return Tag().GetValueInt64(FDepotResultSizesField::Name::FileSize); }
		int64_t FileCount() 					{ return Tag().GetValueInt64(FDepotResultSizesField::Name::FileCount); }
		const DepotString& Path() 				{ return GetTagValue(FDepotResultSizesField::Name::Path); }
		int32_t Rev()							{ return Tag().GetValueInt32(FDepotResultSizesField::Name::Rev); }
	};

	typedef FDepotResultNodeProvider<FDepotResultSizesNode> FDepotResultSizes;
	typedef std::shared_ptr<FDepotResultSizes> DepotResultSizes;

}}}

#pragma managed(pop)
