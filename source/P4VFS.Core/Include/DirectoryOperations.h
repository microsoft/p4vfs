// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "FileCore.h"
#include "FileContext.h"
#include "DriverData.h"
#pragma managed(push, off)

namespace Microsoft {
namespace P4VFS {
namespace DirectoryOperations {

	struct IterateDirectoryFlags { enum Enum
	{
		None			= 0,
		DepthFirst		= 1<<0,
		BreadthFirst	= 1<<1,
	};};
	
	DEFINE_ENUM_FLAG_OPERATORS(IterateDirectoryFlags::Enum);

	typedef std::function<bool(const FileCore::String&, DWORD dwAttributes)> IterateDirectoryVisitor;

	HRESULT IterateDirectoryParallel(
		const WCHAR* folderPath, 
		IterateDirectoryVisitor visitor, 
		int32_t numThreads = -1,
		IterateDirectoryFlags::Enum flags = IterateDirectoryFlags::None
		);

}}}
#pragma managed(pop)
