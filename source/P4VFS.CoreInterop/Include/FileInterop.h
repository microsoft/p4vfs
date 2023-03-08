// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "FileSystem.h"

namespace Microsoft {
namespace P4VFS {
namespace CoreInterop {

public enum class FilePopulateMethod : System::Byte
{
	Copy	= FileSystem::FilePopulateMethod::Copy,
	Move	= FileSystem::FilePopulateMethod::Move,
	Stream	= FileSystem::FilePopulateMethod::Stream,
};

public enum class FilePopulatePolicy : System::Byte
{
	Undefined   = FileSystem::FilePopulatePolicy::Undefined,
	Depot		= FileSystem::FilePopulatePolicy::Depot,
	Share		= FileSystem::FilePopulatePolicy::Share,
};

public enum class FileResidencyPolicy : System::Byte
{
    Undefined   = FileSystem::FileResidencyPolicy::Undefined,
    Resident    = FileSystem::FileResidencyPolicy::Resident,
    Symlink     = FileSystem::FileResidencyPolicy::Symlink,
    RemoveFile  = FileSystem::FileResidencyPolicy::RemoveFile,
};

public ref class FilePopulateInfo
{
public:
	FileResidencyPolicy ResidencyPolicy;
	FilePopulatePolicy PopulatePolicy;
	System::UInt32 FileRevision;
	System::String^ DepotPath;
	System::String^ DepotServer;
	System::String^ DepotClient;
	System::String^ DepotUser;

	FilePopulateInfo() :
		ResidencyPolicy(FileResidencyPolicy::Undefined),
		PopulatePolicy(FilePopulatePolicy::Undefined),
		FileRevision(0),
		DepotPath(nullptr),
		DepotServer(nullptr),
		DepotClient(nullptr),
		DepotUser(nullptr)
	{
	}
};

}}}

