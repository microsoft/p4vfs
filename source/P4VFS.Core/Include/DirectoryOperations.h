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

	typedef std::function<bool(const FileCore::String&, DWORD dwAttributes)> IterateDirectoryVisitor;

	void IterateDirectoryParallel(
		const WCHAR* folderPath, 
		IterateDirectoryVisitor* visitor, 
		int32_t numThreads = -1
		);

}}}
#pragma managed(pop)
