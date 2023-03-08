// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "TestFactory.h"

using namespace Microsoft::P4VFS::FileCore;
using namespace Microsoft::P4VFS::TestCore;


void TestStringInfoHash(const TestContext& context)
{
	{ 
		uint8_t data[] = "some example text"; 
		Assert(StringInfo::HashMd5(data, sizeof(data)) > 0); 
	}
	{ 
		uint8_t data[] = ""; 
		Assert(StringInfo::HashMd5(data, sizeof(data)) > 0); 
	}
	{ 
		uint8_t data[] = ""; 
		Assert(StringInfo::HashMd5(data, 0) == 0); 
	}
	{ 
		uint8_t data[] = "foobar"; 
		Assert(StringInfo::HashMd5(data, sizeof(data)) > 0); 
	}
	{ 
		wchar_t data0[] = L"foobar"; 
		uint64_t h0 = StringInfo::HashMd5(data0, sizeof(data0)); 
		Assert(h0 > 0); 
		char data1[] = "foobar"; 
		uint64_t h1 = StringInfo::HashMd5(data1, sizeof(data1)); 
		Assert(h1 > 0);
		Assert(h1 != h0); 
	}
	{ 
		Assert(StringInfo::HashMd5(nullptr, 0) == 0); 
	}
}

void TestStringInfoToFromWide(const TestContext& context)
{
	#define AssertToFromWide(str) do { const AString s(str); Assert(s == StringInfo::ToAnsi(StringInfo::ToWide(s.c_str()).c_str()).c_str()); } while (0)
	
	AssertToFromWide("");
	AssertToFromWide("some example text");
	AssertToFromWide("foobar\n\t\rstar");
	AssertToFromWide("X");

	const size_t fastFixedBufferCharCount = 256;
	AssertToFromWide(AString(fastFixedBufferCharCount-1, 'A'));
	AssertToFromWide(AString(fastFixedBufferCharCount, 'B'));
	AssertToFromWide(AString(fastFixedBufferCharCount+1, 'C'));
	
	#undef AssertToFromWide
}

void TestStringInfoContainsToken(const TestContext& context)
{
	typedef StringInfo::SearchCase SearchCase;
	struct LocalAssert
	{
		static void ContainsToken(const char delim, const char* searchIn, const char* token, SearchCase::Enum cmp, bool result)
		{
			Assert(StringInfo::ContainsToken(delim, searchIn, token, cmp) == result);
			Assert(StringInfo::ContainsToken(StringInfo::ToWide(delim), StringInfo::ToWide(searchIn).c_str(), StringInfo::ToWide(token).c_str(), cmp) == result);
		}
	};
	LocalAssert::ContainsToken(';', "this;is;some;text", "text", SearchCase::Sensitive, true);
	LocalAssert::ContainsToken(';', "this;is;some;text", "TeXt", SearchCase::Sensitive, false);
	LocalAssert::ContainsToken(';', "this;is;some;text", "TeXt", SearchCase::Insensitive, true);
	LocalAssert::ContainsToken(';', "this;is;some;text", "some", SearchCase::Sensitive, true);
	LocalAssert::ContainsToken(';', "this;is;some;text", "soMe", SearchCase::Sensitive, false);
	LocalAssert::ContainsToken(';', "this;is;some;text", "soMe", SearchCase::Insensitive, true);
	LocalAssert::ContainsToken(';', "this;is;some;text", "this", SearchCase::Sensitive, true);
	LocalAssert::ContainsToken(';', "this;is;some;text", "s", SearchCase::Sensitive, false);
	LocalAssert::ContainsToken(';', "this;is;some;text", "", SearchCase::Sensitive, false);
	LocalAssert::ContainsToken(';', "this;;is;nothing", "is", SearchCase::Sensitive, true);
	LocalAssert::ContainsToken(';', "thIS;;is;nothing", "IS", SearchCase::Sensitive, false);
	LocalAssert::ContainsToken(';', "thIS;;is;nothing", "IS", SearchCase::Insensitive, true);
	LocalAssert::ContainsToken(';', "this;;is;nothing", "this", SearchCase::Sensitive, true);
	LocalAssert::ContainsToken(';', "this;;is;nothing", "his", SearchCase::Sensitive, false);
	LocalAssert::ContainsToken(';', "this;;is;nothing", "s", SearchCase::Sensitive, false);
	LocalAssert::ContainsToken(';', "this;;is;nothing", "not", SearchCase::Sensitive, false);
	LocalAssert::ContainsToken(';', "this;;is;nothing", "", SearchCase::Sensitive, false);
	LocalAssert::ContainsToken(';', ";;stuff;", "", SearchCase::Sensitive, false);
	LocalAssert::ContainsToken(';', ";;stuff;", "stuf", SearchCase::Sensitive, false);
	LocalAssert::ContainsToken(';', ";;stuff;", "STUF", SearchCase::Sensitive, false);
	LocalAssert::ContainsToken(';', ";;stuff;", "STUF", SearchCase::Insensitive, false);
	LocalAssert::ContainsToken(';', ";;stuff;", "stuff", SearchCase::Sensitive, true);
	LocalAssert::ContainsToken(';', ";;STUFF;", "stuff", SearchCase::Sensitive, false);
	LocalAssert::ContainsToken(';', ";;STUFF;", "stuff", SearchCase::Insensitive, true);
	LocalAssert::ContainsToken(';', ";;", "", SearchCase::Sensitive, false);
	LocalAssert::ContainsToken(';', ";;", "stuff", SearchCase::Sensitive, false);
	LocalAssert::ContainsToken(';', "", "", SearchCase::Sensitive, false);
	LocalAssert::ContainsToken(';', "", "s", SearchCase::Sensitive, false);
	LocalAssert::ContainsToken(';', "", "stuff", SearchCase::Sensitive, false);
}
