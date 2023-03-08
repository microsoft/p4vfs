// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "FileAssert.h"
#include "FileCore.h"

namespace Microsoft {
namespace P4VFS {
namespace FileCore {

	void AssertImpl(const wchar_t* expr, const wchar_t* file, int line, const wchar_t* fmt, ...)
	{
		String msg = TEXT("Assertion Failed!\n");
		if (fmt != nullptr)
		{
			va_list va;
			va_start(va, fmt);
			msg += StringInfo::Formatv(fmt, va);
			msg += TEXT("\n");
			va_end(va);
		}

		msg += StringInfo::Format(TEXT("%s(%d): [%s]\n"), file, line, expr);
		if (IsDebuggerPresent())
			OutputDebugString(msg.c_str());

		AssertDebugBreak();
		throw new std::exception(StringInfo::WtoA(msg));
	}

	void AssertDebugBreakImpl()
	{
		if (IsDebuggerPresent()) 
			__debugbreak(); 
	}

}}}
