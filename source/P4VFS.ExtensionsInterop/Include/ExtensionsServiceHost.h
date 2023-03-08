// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#pragma managed(push, off)

namespace Microsoft {
namespace P4VFS {
namespace ExtensionsInterop {

class ServiceHost
{
public:
	virtual bool 
	IsDriverConnected(
		) = 0;

	virtual FILETIME
	GetLastRequestTime(
		) = 0;
		
	virtual FILETIME
	GetLastModifiedTime(
		) = 0;

	virtual bool
	GarbageCollect(
		int64_t timeout
		) = 0;
};
	
}}}

#pragma managed(pop)
