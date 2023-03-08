// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "DepotResultFStat.h"

namespace Microsoft {
namespace P4VFS {
namespace P4 {

DepotStringArray FDepotResultFStatField::ToNames(Enum types)
{
	DepotStringArray names;
	if (types & FDepotResultFStatField::DepotFile)
		names.push_back(FDepotResultFStatField::Name::DepotFile);
	if (types & FDepotResultFStatField::ClientFile)
		names.push_back(FDepotResultFStatField::Name::ClientFile);
	if (types & FDepotResultFStatField::FileSize)
		names.push_back(FDepotResultFStatField::Name::FileSize);
	if (types & FDepotResultFStatField::HaveRev)
		names.push_back(FDepotResultFStatField::Name::HaveRev);
	if (types & FDepotResultFStatField::HeadRev)
		names.push_back(FDepotResultFStatField::Name::HeadRev);
	if (types & FDepotResultFStatField::HeadType)
		names.push_back(FDepotResultFStatField::Name::HeadType);
	return names;
}

}}}

