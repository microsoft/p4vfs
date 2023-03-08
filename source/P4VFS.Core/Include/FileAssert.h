// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#pragma managed(push, off)
#include "FileCore.h"

namespace Microsoft {
namespace P4VFS {
namespace FileCore {

	P4VFS_CORE_API void AssertImpl(const wchar_t* expr, const wchar_t* file, int line, const wchar_t* fmt, ...);
	P4VFS_CORE_API void AssertDebugBreakImpl();

	#if 1
		#define	Assert(expression)					do { if (!(expression)) Microsoft::P4VFS::FileCore::AssertImpl(TEXT(#expression), TEXT(__FILE__), __LINE__, nullptr); } while (0)
		#define	AssertMsg(expression, fmt, ...)		do { if (!(expression)) Microsoft::P4VFS::FileCore::AssertImpl(TEXT(#expression), TEXT(__FILE__), __LINE__, fmt, __VA_ARGS__); } while (0)
		#define AssertDebugBreak()					AssertDebugBreakImpl()
	#else
		#define Assert(expression)					__noop
		#define	AssertMsg(expression, fmt, ...)		__noop
		#define AssertDebugBreak()					__noop
	#endif

	#if defined(_DEBUG)
		#define AssertSlow(expression)				Assert(expression)
	#else
		#define AssertSlow(expression)				__noop
	#endif

	#define StaticAssert(expression)				static_assert(expression, "StaticAssert: " #expression)
	#define Ensure(expression)						((expression) != 0)
	#define Verify(expression)						do { if (!(expression)) { AssertMsg(false, TEXT("%s"), TEXT(#expression)); } } while (0)
	
}}}

#pragma managed(pop)


