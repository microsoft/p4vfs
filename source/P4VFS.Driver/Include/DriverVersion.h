// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once

#define P4VFS_VER_MAJOR					1			// Increment this number almost never
#define P4VFS_VER_MINOR					28			// Increment this number whenever the driver changes
#define P4VFS_VER_BUILD					4			// Increment this number when a major user mode change has been made
#define P4VFS_VER_REVISION				6			// Increment this number when we rebuild with any change

#define P4VFS_VER_STRINGIZE_EX(v)		L#v
#define P4VFS_VER_STRINGIZE(v)			P4VFS_VER_STRINGIZE_EX(v)

#define P4VFS_VER_FILE_VERSION			P4VFS_VER_MAJOR, P4VFS_VER_MINOR, P4VFS_VER_BUILD, P4VFS_VER_REVISION

#define P4VFS_VER_VERSION_STRING		P4VFS_VER_STRINGIZE(P4VFS_VER_MAJOR) L"." \
										P4VFS_VER_STRINGIZE(P4VFS_VER_MINOR) L"." \
										P4VFS_VER_STRINGIZE(P4VFS_VER_BUILD) L"." \
										P4VFS_VER_STRINGIZE(P4VFS_VER_REVISION)

#define P4VFS_DRIVER_TITLE				"p4vfsflt"
#define P4VFS_SERVICE_TITLE				"P4VFS.Service"
#define P4VFS_MONITOR_TITLE				"P4VFS.Monitor"

