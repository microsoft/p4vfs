// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "DepotResult.h"
#include "DepotConfig.h"
#pragma managed(push, off)

namespace Microsoft {
namespace P4VFS {
namespace P4 {

	struct FDepotResultWhereField
	{
		struct Name
		{
			static constexpr const char* LocalPath		= "path";
			static constexpr const char* DepotPath		= "depotFile";
			static constexpr const char* WorkspacePath	= "clientFile";
		};
	};

	struct FDepotResultWhereNode : FDepotResultNode
	{
		const DepotString& LocalPath()		{ return GetTagValue(FDepotResultWhereField::Name::LocalPath); }
		const DepotString& DepotPath()		{ return GetTagValue(FDepotResultWhereField::Name::DepotPath); }
		const DepotString& WorkspacePath()	{ return GetTagValue(FDepotResultWhereField::Name::WorkspacePath); }
	};

	typedef FDepotResultNodeProvider<FDepotResultWhereNode> FDepotResultWhere;
	typedef std::shared_ptr<FDepotResultWhere> DepotResultWhere;

}}}

#pragma managed(pop)
