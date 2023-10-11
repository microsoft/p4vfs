// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "TestFactory.h"

// TestFileInfo
P4VFS_REGISTER_TEST( TestFileInfoPath,							10000 )
P4VFS_REGISTER_TEST( TestFileInfoDirectory,						10001 )
P4VFS_REGISTER_TEST( TestFileInfoLongPathSupport,				10002 )

// TestDepotClient
P4VFS_REGISTER_TEST( TestDepotClientPrintToFile,				10100 )
P4VFS_REGISTER_TEST( TestDepotClientConnectFromClientOwner,		10101 )
P4VFS_REGISTER_TEST( TestDepotClientIsWritableFileType,			10102 )
P4VFS_REGISTER_TEST( TestDepotClientIsSymlinkFileType,			10103 )

// TestFileSystem
P4VFS_REGISTER_TEST( TestResolveFileResidency,					10200 )
P4VFS_REGISTER_TEST( TestRequireFilterOpLock,					10201 )
P4VFS_REGISTER_TEST( TestFileAlternateStream,					10202 )

// TestStringInfo
P4VFS_REGISTER_TEST( TestStringInfoHash,						10300 )
P4VFS_REGISTER_TEST( TestStringInfoToFromWide,					10301 )
P4VFS_REGISTER_TEST( TestStringInfoContainsToken,				10302 )

// TestDepotRevision
P4VFS_REGISTER_TEST( TestDepotRevisionChangelist,				10400 )
P4VFS_REGISTER_TEST( TestDepotRevisionNumber,					10401 )
P4VFS_REGISTER_TEST( TestDepotNoneRevision,						10402 )
P4VFS_REGISTER_TEST( TestDepotHaveRevision,						10403 )
P4VFS_REGISTER_TEST( TestDepotHeadRevision,						10404 )
P4VFS_REGISTER_TEST( TestDepotNowRevision,						10405 )
P4VFS_REGISTER_TEST( TestDepotRevisionRange,					10406 )
P4VFS_REGISTER_TEST( TestDepotRevisionLabel,					10407 )
P4VFS_REGISTER_TEST( TestDepotRevisionDate,						10408 )
P4VFS_REGISTER_TEST( TestDepotRevisionDateShort,				10409 )

// TestDepotClientCache
P4VFS_REGISTER_TEST( TestDepotClientCacheCommon,				10500 )

// TestDepotOperations
P4VFS_REGISTER_TEST( TestDepotOperationsSync,					10600 )
P4VFS_REGISTER_TEST( TestDepotOperationsToString,				10601 )
P4VFS_REGISTER_TEST( TestDepotOperationsCreateFileSpec,			10602 )
P4VFS_REGISTER_TEST( TestDepotOperationsToDisplayString,		10603 )

// TestServiceOperations
P4VFS_REGISTER_TEST( TestServiceOperationsStartStop,			10700 )

// TestThreadPool
P4VFS_REGISTER_TEST( TestThreadPool,							10800 )

// TestDriver
P4VFS_REGISTER_TEST( TestDriverUnicodeString,					10900 )

// TestFileOperations
P4VFS_REGISTER_TEST( TestFileOperationsUnicodeString,			11000 )
P4VFS_REGISTER_TEST( TestFileOperationsOpenReparsePointFile,	11001 )
P4VFS_REGISTER_TEST( TestFileOperationsAccess,					11002 )
P4VFS_REGISTER_TEST( TestFileOperationsAccessElevated,			11003, TestFlags::Explicit )
P4VFS_REGISTER_TEST( TestFileOperationsAccessUnelevated,		11004, TestFlags::Explicit )

// TestDirectoryOperations
P4VFS_REGISTER_TEST( TestIterateDirectoryParallel,				12000 )

