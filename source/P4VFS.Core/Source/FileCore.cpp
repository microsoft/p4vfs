// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "FileCore.h"
#include <shlwapi.h>
#include <shellapi.h>
#include <fstream>

namespace Microsoft {
namespace P4VFS {
namespace FileCore {

void* GAlloc(size_t size, size_t align)
{
	return ::_aligned_malloc(size, align);
}

void* GRealloc(void* ptr, size_t size, size_t align)
{
	return ::_aligned_realloc(ptr, size, align);
}

void GFree(void* ptr)
{
	::_aligned_free(ptr);
}

struct StringInfoInternal
{
	template <typename CharType>
	static typename const StringInfo::Traits::Type<CharType>::TString& Empty()
	{
		using TString = StringInfo::Traits::Type<CharType>::TString;
		static const TString* s = ([]() -> TString* { static uint8_t mem[sizeof(TString)]; return new(mem) TString(); })();
		return *s;
	}

	template <typename CharType>
	static bool IsNullOrEmpty(const CharType* s)
	{
		return s == nullptr || *s == LITERAL(CharType, '\0');
	}

	template <typename CharType>
	static bool IsNullOrWhiteSpace(const CharType* s)
	{
		if (s != nullptr)
		{
			for (; *s != LITERAL(CharType, '\0'); ++s)
			{
				if (IsWhite(*s) == false)
					return false;
			}
		}
		return true;
	}

	template <typename CharType>
	static bool IsDigit(const CharType c)
	{
		return (c >= LITERAL(CharType, '0') && c <= LITERAL(CharType, '9'));
	}

	template <typename CharType>
	static bool IsAlpha(const CharType c)
	{
		return (c >= LITERAL(CharType, 'a') && c <= LITERAL(CharType, 'z')) || (c >= LITERAL(CharType, 'A') && c <= LITERAL(CharType, 'Z'));
	}

	template <typename CharType>
	static bool IsAlphaNum(const CharType c)
	{
		return IsDigit(c) || IsAlpha(c);
	}

	template <typename CharType>
	static bool IsWhite(const CharType c)
	{
		return (StringInfo::Strchr(LITERAL(CharType, " \t\r\n"), c) != nullptr || c == LITERAL(CharType, '\0'));
	}

	template <typename CharType>
	static CharType ToLower(const CharType c)
	{
		return (c >= LITERAL(CharType, 'A') && c <= LITERAL(CharType, 'Z') ? c + (LITERAL(CharType, 'a') - LITERAL(CharType, 'A')) : c);
	}

	template <typename CharType>
	static typename StringInfo::Traits::Type<CharType>::TString ToLower(const CharType* s)
	{
		StringInfo::Traits::Type<CharType>::TString r;
		if (IsNullOrEmpty(s) == false)
		{
			r = s;
			for (StringInfo::Traits::Type<CharType>::TString::iterator i = r.begin(); i != r.end(); ++i)
				*i = ToLower(*i);
		}
		return r;
	}

	template <typename CharType>
	static CharType ToUpper(const CharType c)
	{
		return (c >= LITERAL(CharType, 'a') && c <= LITERAL(CharType, 'z') ? c - (LITERAL(CharType, 'a') - LITERAL(CharType, 'A')) : c);
	}

	template <typename CharType>
	static typename StringInfo::Traits::Type<CharType>::TString ToUpper(const CharType* s)
	{
		StringInfo::Traits::Type<CharType>::TString r;
		if (IsNullOrEmpty(s) == false)
		{
			r = s;
			for (StringInfo::Traits::Type<CharType>::TString::iterator i = r.begin(); i != r.end(); ++i)
				*i = ToUpper(*i);
		}
		return r;
	}

	template <typename CharType>
	static typename StringInfo::Traits::Type<CharType>::TString Capitalize(const CharType* s)
	{
		StringInfo::Traits::Type<CharType>::TString r;
		if (IsNullOrEmpty(s) == false)
		{
			r = s;
			r[0] = ToUpper(s[0]);
		}
		return r;
	}

	template <typename CharType>
	static typename StringInfo::Traits::Type<CharType>::TString Trim(const CharType* s, const CharType* trimChrs)
	{
		StringInfo::Traits::Type<CharType>::TString r;
		if (IsNullOrEmpty(s) == false)
		{
			r = TrimLeft(s, trimChrs);
			r = TrimRight(r.c_str(), trimChrs);
		}
		return r;
	}

	template <typename CharType>
	static typename StringInfo::Traits::Type<CharType>::TString TrimRight(const CharType* s, const CharType* trimChrs)
	{
		StringInfo::Traits::Type<CharType>::TString r;
		if (IsNullOrEmpty(s) == false)
		{
			size_t len = StringInfo::Strlen(s);
			if (trimChrs != nullptr)
			{
				for (; len > 0 && StringInfo::Strchr(trimChrs, s[len-1]) != nullptr; --len);
			}
			else
			{
				for (; len > 0 && IsWhite(s[len-1]); --len);
			}
			r.assign(s, len);
		}
		return r;
	}

	template <typename CharType>
	static typename StringInfo::Traits::Type<CharType>::TString TrimLeft(const CharType* s, const CharType* trimChrs)
	{
		StringInfo::Traits::Type<CharType>::TString r;
		if (IsNullOrEmpty(s) == false)
		{
			if (trimChrs != nullptr)
			{
				for (; *s != LITERAL(CharType, '\0') && StringInfo::Strchr(trimChrs, *s) != nullptr; ++s);
			}
			else
			{
				for (; *s != LITERAL(CharType, '\0') && IsWhite(*s); ++s);
			}
			r = s;
		}
		return r;
	}

	template <typename CharType>
	static bool StartsWith(const CharType* searchIn, const CharType searchFor, StringInfo::SearchCase::Enum cmp)
	{
		if (StringInfo::IsNullOrEmpty(searchIn) == false && searchFor != LITERAL(CharType, '\0'))
		{
			if (cmp == StringInfo::SearchCase::Sensitive)
			{
				if (searchIn[0] == searchFor)
					return true;
			}
			else
			{
				if (ToLower(searchIn[0]) == ToLower(searchFor))
					return true;
			}
		}
		return false;
	}

	template <typename CharType>
	static bool StartsWith(const CharType* searchIn, const CharType* searchFor, StringInfo::SearchCase::Enum cmp)
	{
		if (searchIn != nullptr && searchFor != nullptr)
		{
			size_t searchLen = StringInfo::Strlen(searchFor);
			if (cmp == StringInfo::SearchCase::Sensitive)
				return StringInfo::Strncmp(searchIn, searchFor, searchLen) == 0;
			else
				return StringInfo::Strnicmp(searchIn, searchFor, searchLen) == 0;
		}
		return false;
	}

	template <typename CharType>
	static bool EndsWith(const CharType* searchIn, const CharType* searchFor, StringInfo::SearchCase::Enum cmp)
	{
		if (searchIn != nullptr && searchFor != nullptr)
		{
			size_t searchForLen = StringInfo::Strlen(searchFor);
			size_t searchInLen = StringInfo::Strlen(searchIn);
			if (searchInLen >= searchForLen)
			{
				const CharType* searchStart = searchIn+searchInLen-searchForLen;
				if (cmp == StringInfo::SearchCase::Sensitive)
					return StringInfo::Strcmp(searchStart, searchFor) == 0;
				else
					return StringInfo::Stricmp(searchStart, searchFor) == 0;
			}
		}
		return false;
	}

	template <typename CharType>
	static bool EndsWith(const CharType* searchIn, const CharType searchFor, StringInfo::SearchCase::Enum cmp)
	{
		if (StringInfo::IsNullOrEmpty(searchIn) == false && searchFor != LITERAL(CharType, '\0'))
		{
			if (cmp == StringInfo::SearchCase::Sensitive)
			{
				if (searchIn[StringInfo::Strlen(searchIn)-1] == searchFor)
					return true;
			}
			else
			{
				if (ToLower(searchIn[StringInfo::Strlen(searchIn)-1]) == ToLower(searchFor))
					return true;
			}
		}
		return false;
	}

	template <typename CharType>
	static typename StringInfo::Traits::Type<CharType>::TString Replace(const CharType* s, const CharType* find, const CharType* replace, StringInfo::SearchCase::Enum cmp)
	{
		StringInfo::Traits::Type<CharType>::TString r;
		if (IsNullOrEmpty(s))
			return r;
		if (IsNullOrEmpty(find))
			return s;
		if (replace == nullptr)
			replace = LITERAL(CharType, "");
		size_t findlen = StringInfo::Strlen(find);
		while (*s != LITERAL(CharType, '\0'))
		{
			if ((cmp == StringInfo::SearchCase::Sensitive ? StringInfo::Strncmp(s, find, findlen) : StringInfo::Strnicmp(s, find, findlen)) == 0)
			{
				r += replace;
				s += findlen;
			}
			else
			{
				r += *s;
				s += 1;
			}
		}
		return r;
	}

	template <typename CharType>
	static typename StringInfo::Traits::Type<CharType>::TString Replace(const CharType* s, const CharType find, const CharType replace, StringInfo::SearchCase::Enum cmp)
	{
		StringInfo::Traits::Type<CharType>::TString r;
		if (IsNullOrEmpty(s) == false)
		{
			r = s;
			if (cmp == StringInfo::SearchCase::Sensitive)
			{
				std::replace(r.begin(), r.end(), find, replace);
			}
			else
			{
				const CharType findLower = ToLower(find);
				auto cmpPred = [findLower](CharType c) -> bool { return ToLower(c) == findLower; };
				std::replace_if(r.begin(), r.end(), cmpPred, replace);
			}
		}
		return r;
	}

	template <typename CharType>
	static bool Contains(const CharType* s, const CharType* searchFor, StringInfo::SearchCase::Enum cmp)
	{
		if (IsNullOrEmpty(s) || IsNullOrEmpty(searchFor))
			return false;
		if (cmp == StringInfo::SearchCase::Sensitive)
		{
			if (StringInfo::Strstr(s, searchFor) == nullptr)
				return false;
		}
		else
		{
			if (StringInfo::Stristr(s, searchFor) == nullptr)
				return false;
		}
		return true;
	}

	template <typename CharType>
	static bool Contains(const CharType* s, const CharType searchFor, StringInfo::SearchCase::Enum cmp)
	{
		if (IsNullOrEmpty(s))
			return false;
		if (cmp == StringInfo::SearchCase::Sensitive)
		{
			if (StringInfo::Strchr(s, searchFor) == nullptr)
				return false;
		}
		else
		{
			if (StringInfo::Strichr(s, searchFor) == nullptr)
				return false;
		}
		return true;
	}

	template <typename CharType>
	static bool ContainsToken(const CharType delim, const CharType* searchIn, const CharType* token, StringInfo::SearchCase::Enum cmp)
	{
		if (StringInfo::IsNullOrEmpty(token) == false)
		{
			const size_t tokenlen = StringInfo::Strlen(token);
			for (const CharType* p = searchIn; p != nullptr;)
			{
				const CharType* pnext = StringInfo::Strchr(p, delim);
				const CharType* pend = pnext;
				if (pend == nullptr)
				{
					pend = p + StringInfo::Strlen(p);
				}
				const size_t plen = pend-p;
				if (plen == tokenlen)
				{
					if (cmp == StringInfo::SearchCase::Sensitive)
					{
						if (StringInfo::Strncmp(p, token, plen) == 0)
							return true;
					}
					else
					{
						if (StringInfo::Strnicmp(p, token, plen) == 0)
							return true;
					}
				}
				p = pnext ? pnext + 1 : nullptr;
			}
		}
		return false;
	}

	template <typename CharType>
	static typename StringInfo::Traits::Type<CharType>::TString FormatTime(const struct tm* t, const CharType* fmt)
	{
		struct Local
		{
			static size_t Strftime(char* dest, size_t maxsize, const char* format, const struct tm* timeptr) { return strftime(dest, maxsize, format, timeptr); }
			static size_t Strftime(wchar_t* dest, size_t maxsize, const wchar_t* format, const struct tm* timeptr) { return wcsftime(dest, maxsize, format, timeptr); }
		};
		CharType dst[2048];
		size_t written = Local::Strftime(dst, _countof(dst), fmt, t);
		return written ? StringInfo::Traits::Type<CharType>::TString(dst) : StringInfo::Traits::Type<CharType>::TString();
	}

	template <typename CharType>
	static typename StringInfo::Traits::Type<CharType>::TString FormatLocalTime(time_t t, const CharType* fmt)
	{
		struct tm stm;
		errno_t err = _localtime64_s(&stm, &t);
		return err ? StringInfo::Traits::Type<CharType>::TString() : FormatTime<CharType>(&stm, fmt);
	}

	template <typename CharType>
	static typename StringInfo::Traits::Type<CharType>::TString FormatUtcTime(time_t t, const CharType* fmt)
	{
		struct tm stm;
		errno_t err = _gmtime64_s(&stm, &t);
		return err ? StringInfo::Traits::Type<CharType>::TString() : FormatTime<CharType>(&stm, fmt);
	}

	template <typename CharType>
	static uint64_t HashMd5(const CharType* s, size_t length)
	{
		return StringInfo::HashMd5(s, length*sizeof(CharType));
	}
};

const WString& StringInfo::EmptyW()
{
	return StringInfoInternal::Empty<wchar_t>();
}

const AString& StringInfo::EmptyA()
{
	return StringInfoInternal::Empty<char>();
}

bool StringInfo::IsNullOrEmpty(const wchar_t* s)
{
	return StringInfoInternal::IsNullOrEmpty(s);
}

bool StringInfo::IsNullOrEmpty(const char* s)
{
	return StringInfoInternal::IsNullOrEmpty(s);
}

bool StringInfo::IsNullOrWhiteSpace(const wchar_t* s)
{
	return StringInfoInternal::IsNullOrWhiteSpace(s);
}

bool StringInfo::IsNullOrWhiteSpace(const char* s)
{
	return StringInfoInternal::IsNullOrWhiteSpace(s);
}

WString StringInfo::Format(const wchar_t* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	WString s;
	StringInfo::Sprintfv(s, fmt, va);
	va_end(va);
	return s;
}

AString StringInfo::Format(const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	AString s;
	StringInfo::Sprintfv(s, fmt, va);
	va_end(va);
	return s;
}

WString StringInfo::Formatv(const wchar_t* fmt, va_list va)
{
	WString s;
	StringInfo::Sprintfv(s, fmt, va);
	return s;
}

AString StringInfo::Formatv(const char* fmt, va_list va)
{
	AString s;
	StringInfo::Sprintfv(s, fmt, va);
	return s;
}

void StringInfo::Sprintf(WString& s, const wchar_t* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	StringInfo::Sprintfv(s, fmt, va);
	va_end(va);
}

void StringInfo::Sprintf(AString& s, const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	StringInfo::Sprintfv(s, fmt, va);
	va_end(va);
}

void StringInfo::Sprintfv(WString& s, const wchar_t* fmt, va_list va)
{
	int32_t size = _vsnwprintf(nullptr, 0, fmt, va);
	wchar_t* buffer = new wchar_t[size+1];
	_vsnwprintf(buffer, size, fmt, va);
	va_end(va);

	buffer[size] = TEXT('\0');
	s.assign(buffer);
	delete[] buffer;
}

void StringInfo::Sprintfv(AString& s, const char* fmt, va_list va)
{
	int32_t size = _vsnprintf(nullptr, 0, fmt, va);
	char* buffer = new char[size+1];
	_vsnprintf(buffer, size, fmt, va);
	va_end(va);

	buffer[size] = '\0';
	s.assign(buffer);
	delete[] buffer;
}

bool StringInfo::IsDigit(const wchar_t c)
{
	return StringInfoInternal::IsDigit(c);
}

bool StringInfo::IsDigit(const char c)
{
	return StringInfoInternal::IsDigit(c);
}

bool StringInfo::IsAlpha(const wchar_t c)
{
	return StringInfoInternal::IsAlpha(c);
}

bool StringInfo::IsAlpha(const char c)
{
	return StringInfoInternal::IsAlpha(c);
}

bool StringInfo::IsAlphaNum(const wchar_t c)
{
	return StringInfoInternal::IsAlphaNum(c);
}

bool StringInfo::IsAlphaNum(const char c)
{
	return StringInfoInternal::IsAlphaNum(c);
}

bool StringInfo::IsWhite(const wchar_t c)
{
	return StringInfoInternal::IsWhite(c);
}

bool StringInfo::IsWhite(const char c)
{
	return StringInfoInternal::IsWhite(c);
}

wchar_t StringInfo::ToLower(const wchar_t c)
{
	return StringInfoInternal::ToLower(c);
}

char StringInfo::ToLower(const char c)
{
	return StringInfoInternal::ToLower(c);
}

WString StringInfo::ToLower(const wchar_t* s)
{
	return StringInfoInternal::ToLower(s);
}

AString StringInfo::ToLower(const char* s)
{
	return StringInfoInternal::ToLower(s);
}

wchar_t StringInfo::ToUpper(const wchar_t c)
{
	return StringInfoInternal::ToUpper(c);
}

char StringInfo::ToUpper(const char c)
{
	return StringInfoInternal::ToUpper(c);
}

WString StringInfo::ToUpper(const wchar_t* s)
{
	return StringInfoInternal::ToUpper(s);
}

AString StringInfo::ToUpper(const char* s)
{
	return StringInfoInternal::ToUpper(s);
}

WString StringInfo::Capitalize(const wchar_t* s)
{
	return StringInfoInternal::Capitalize(s);
}

AString StringInfo::Capitalize(const char* s)
{
	return StringInfoInternal::Capitalize(s);
}

WString StringInfo::Trim(const wchar_t* s, const wchar_t* trimChrs)
{
	return StringInfoInternal::Trim(s, trimChrs);
}

AString StringInfo::Trim(const char* s, const char* trimChrs)
{
	return StringInfoInternal::Trim(s, trimChrs);
}

WString StringInfo::TrimRight(const wchar_t* s, const wchar_t* trimChrs)
{
	return StringInfoInternal::TrimRight(s, trimChrs);
}

AString StringInfo::TrimRight(const char* s, const char* trimChrs)
{
	return StringInfoInternal::TrimRight(s, trimChrs);
}

WString StringInfo::TrimLeft(const wchar_t* s, const wchar_t* trimChrs)
{
	return StringInfoInternal::TrimLeft(s, trimChrs);
}

AString StringInfo::TrimLeft(const char* s, const char* trimChrs)
{
	return StringInfoInternal::TrimLeft(s, trimChrs);
}

bool StringInfo::StartsWith(const wchar_t* searchIn, const wchar_t* searchFor, SearchCase::Enum cmp)
{
	return StringInfoInternal::StartsWith(searchIn, searchFor, cmp);
}

bool StringInfo::StartsWith(const char* searchIn, const char* searchFor, SearchCase::Enum cmp)
{
	return StringInfoInternal::StartsWith(searchIn, searchFor, cmp);
}

bool StringInfo::StartsWith(const wchar_t* searchIn, const wchar_t searchFor, SearchCase::Enum cmp)
{
	return StringInfoInternal::StartsWith(searchIn, searchFor, cmp);
}

bool StringInfo::StartsWith(const char* searchIn, const char searchFor, SearchCase::Enum cmp)
{
	return StringInfoInternal::StartsWith(searchIn, searchFor, cmp);
}

bool StringInfo::EndsWith(const wchar_t* searchIn, const wchar_t* searchFor, SearchCase::Enum cmp)
{
	return StringInfoInternal::EndsWith(searchIn, searchFor, cmp);
}

bool StringInfo::EndsWith(const char* searchIn, const char* searchFor, SearchCase::Enum cmp)
{
	return StringInfoInternal::EndsWith(searchIn, searchFor, cmp);
}

bool StringInfo::EndsWith(const wchar_t* searchIn, const wchar_t searchFor, SearchCase::Enum cmp)
{
	return StringInfoInternal::EndsWith(searchIn, searchFor, cmp);
}

bool StringInfo::EndsWith(const char* searchIn, const char searchFor, SearchCase::Enum cmp)
{
	return StringInfoInternal::EndsWith(searchIn, searchFor, cmp);
}

WString StringInfo::Replace(const wchar_t* s, const wchar_t* find, const wchar_t* replace, SearchCase::Enum cmp)
{
	return StringInfoInternal::Replace(s, find, replace, cmp);
}

AString StringInfo::Replace(const char* s, const char* find, const char* replace, SearchCase::Enum cmp)
{
	return StringInfoInternal::Replace(s, find, replace, cmp);
}

WString StringInfo::Replace(const wchar_t* s, const wchar_t find, const wchar_t replace, SearchCase::Enum cmp)
{
	return StringInfoInternal::Replace(s, find, replace, cmp);
}

AString StringInfo::Replace(const char* s, const char find, const char replace, SearchCase::Enum cmp)
{
	return StringInfoInternal::Replace(s, find, replace, cmp);
}

bool StringInfo::Contains(const wchar_t* s, const wchar_t* searchFor, SearchCase::Enum cmp)
{
	return StringInfoInternal::Contains(s, searchFor, cmp);
}

bool StringInfo::Contains(const char* s, const char* searchFor, SearchCase::Enum cmp)
{
	return StringInfoInternal::Contains(s, searchFor, cmp);
}

bool StringInfo::Contains(const wchar_t* s, const wchar_t searchFor, SearchCase::Enum cmp)
{
	return StringInfoInternal::Contains(s, searchFor, cmp);
}

bool StringInfo::Contains(const char* s, const char searchFor, SearchCase::Enum cmp)
{
	return StringInfoInternal::Contains(s, searchFor, cmp);
}

bool StringInfo::ContainsToken(const char delim, const char* searchIn, const char* token, SearchCase::Enum cmp)
{
	return StringInfoInternal::ContainsToken(delim, searchIn, token, cmp);
}

bool StringInfo::ContainsToken(const wchar_t delim, const wchar_t* searchIn, const wchar_t* token, SearchCase::Enum cmp)
{
	return StringInfoInternal::ContainsToken(delim, searchIn, token, cmp);
}

WString StringInfo::ReadLine(const wchar_t*& s)
{
	WString r;
	for (; *s != EOF && *s != TEXT('\0'); ++s)
	{
		if (*s == TEXT('\n'))
		{
			++s;
			break;
		}
		if (*s != TEXT('\r'))
			r += *s;
	}
	return r;
}

WString StringInfo::Escape(const wchar_t* s, EscapeDirection::Enum direction)
{
	WString d;
	if (IsNullOrEmpty(s) == false)
	{
		typedef const wchar_t* Encoding[2];
		const Encoding text[] = { {TEXT("\\\""), TEXT("\"")}, {TEXT("\\\\"), TEXT("\\")}, {TEXT("\\n"), TEXT("\n")}, {TEXT("\\r"), TEXT("\r")}, {TEXT("\\t"), TEXT("\t")} };
		const int32_t src = (direction == EscapeDirection::Encode ? 1 : 0);
		const int32_t dst = src ^ 1;
		while (s != TEXT('\0'))
		{
			size_t i = 0;
			for (; i < _countof(text); ++i)
			{
				if (StringInfo::Strncmp(s, text[i][src], StringInfo::Strlen(text[i][src])) == 0)
					break;
			}

			if (i < _countof(text))
			{
				d += text[i][dst];
				s += StringInfo::Strlen(text[i][src]);
			}
			else
			{
				d += *s;
				s += 1;
			}
		}
	}
	return d;
}

template <typename CharType>
static typename StringInfo::Traits::Type<CharType>::TStringArray StringInfoSplit(const CharType* text, const CharType* delims, StringInfo::SplitFlags::Enum flags)
{
	StringInfo::Traits::Type<CharType>::TStringArray result;
	if (text == nullptr || *text == LITERAL(CharType, '\0'))
		return result;
	if (delims == nullptr)
		delims = LITERAL(CharType, "");

	StringInfo::Traits::Type<CharType>::TString current;
	for (; *text != LITERAL(CharType, '\0'); ++text)
	{
		if (StringInfo::Strchr(delims, *text) != nullptr)
		{
			if ((flags & StringInfo::SplitFlags::RemoveEmptyEntries) == 0 || current.length() > 0)
				result.push_back(current);
			current = LITERAL(CharType, "");
		}
		else
		{
			current += *text;
		}
	}

	if ((flags & StringInfo::SplitFlags::RemoveEmptyEntries) == 0 || current.length() > 0)
	{
		result.push_back(current);
	}
	return result;
}

AStringArray StringInfo::Split(const char* text, const char* delims, SplitFlags::Enum flags)
{
	return StringInfoSplit<char>(text, delims, flags);
}

WStringArray StringInfo::Split(const wchar_t* text, const wchar_t* delims, SplitFlags::Enum flags)
{
	return StringInfoSplit<wchar_t>(text, delims, flags);
}

template <typename CharType>
static typename StringInfo::Traits::Type<CharType>::TString StringInfoJoin(const typename StringInfo::Traits::Type<CharType>::TStringArray& tokens, const CharType* delim, size_t start, size_t count)
{
	if (delim == nullptr)
		delim = LITERAL(CharType, "");
	StringInfo::Traits::Type<CharType>::TString result;
	const size_t end = tokens.size();
	for (size_t i = 0; i+start < end && i < count; ++i)
	{
		if (i > 0)
			result += delim;
		result += tokens[i+start];
	}
	return result;
}

WString StringInfo::Join(const WStringArray& tokens, const wchar_t* delim, size_t start, size_t count)
{
	return StringInfoJoin(tokens, delim, start, count);
}

AString StringInfo::Join(const AStringArray& tokens, const char* delim, size_t start, size_t count)
{
	return StringInfoJoin(tokens, delim, start, count);
}

int32_t StringInfo::Strcmp(const wchar_t* a, const wchar_t* b)
{
	return wcscmp(a, b);
}

int32_t StringInfo::Strcmp(const char* a, const char* b)
{
	return strcmp(a, b);
}

int32_t StringInfo::Stricmp(const wchar_t* a, const wchar_t* b)
{
	return _wcsicmp(a, b);
}

int32_t StringInfo::Stricmp(const char* a, const char* b)
{
	return _stricmp(a, b);
}

int32_t StringInfo::Strncmp(const wchar_t* a, const wchar_t* b, size_t count)
{
	return wcsncmp(a, b, count);
}

int32_t StringInfo::Strncmp(const char* a, const char* b, size_t count)
{
	return strncmp(a, b, count);
}

int32_t StringInfo::Strnicmp(const wchar_t* a, const wchar_t* b, size_t count)
{
	return _wcsnicmp(a, b, count);
}

int32_t StringInfo::Strnicmp(const char* a, const char* b, size_t count)
{
	return _strnicmp(a, b, count);
}

wchar_t* StringInfo::Strncpy(wchar_t* dst, const wchar_t* src, size_t count)
{
	if (dst != nullptr && count > 0)
	{
		wcsncpy(dst, src ? src : TEXT(""), count-1);
		dst[count-1] = TEXT('\0');
	}
	return dst;
}

char* StringInfo::Strncpy(char* dst, const char* src, size_t dstSize)
{
	if (dst != nullptr && dstSize > 0)
	{
		strncpy(dst, src ? src : "", dstSize-1);
		dst[dstSize-1] = '\0';
	}
	return dst;
}

const wchar_t* StringInfo::Strchr(const wchar_t* str, wchar_t c)
{
	return StrChrW(str, c);
}

const char* StringInfo::Strchr(const char* str, char c) 
{ 
	return StrChrA(str, c); 
}

const wchar_t* StringInfo::Strstr(const wchar_t* str, const wchar_t* strSearch)
{
	return StrStrW(str, strSearch);
}

const char* StringInfo::Strstr(const char* str, const char* strSearch)
{
	return StrStrA(str, strSearch);
}

const wchar_t* StringInfo::Strichr(const wchar_t* str, wchar_t c)
{
	return StrChrIW(str, c);
}

const char* StringInfo::Strichr(const char* str, char c)
{
	return StrChrIA(str, c);
}

const wchar_t* StringInfo::Stristr(const wchar_t* str, const wchar_t* strSearch)
{
	return StrStrIW(str, strSearch);
}

const char* StringInfo::Stristr(const char* str, const char* strSearch)
{
	return StrStrIA(str, strSearch);
}

const wchar_t* StringInfo::Strpbrk(const wchar_t* str, const wchar_t* strCharSet)
{
	return wcspbrk(str, strCharSet);
}

const char* StringInfo::Strpbrk(const char* str, const char* strCharSet)
{
	return strpbrk(str, strCharSet);
}

size_t StringInfo::Strlen(const wchar_t* str)
{
	return wcslen(str);
}

size_t StringInfo::Strlen(const char* str)
{
	return strlen(str);
}

WString StringInfo::ToString(int32_t value)
{
	wchar_t text[64] = {0};
	_itow_s(value, text, _countof(text), 10);
	return text;
}

WString StringInfo::ToString(int64_t value)
{
	wchar_t text[64] = {0};
	_i64tow_s(value, text, _countof(text), 10);
	return text;
}

WString StringInfo::ToString(uint64_t value)
{
	wchar_t text[64] = {0};
	_ui64tow_s(value, text, _countof(text), 10);
	return text;
}

WString StringInfo::ToString(HRESULT value)
{
	wchar_t text[64] = TEXT("0x");
	_ltow_s(value, text+2, _countof(text)-2, 16);
	_wcsupr(text+2);
	return text;
}

AString StringInfo::ToString(const char* str)
{
	return AString(str ? str : "");
}

WString StringInfo::ToString(const wchar_t* str)
{
	return WString(str ? str : TEXT(""));
}

uint64_t StringInfo::HashMd5(const void* data, size_t dataSize)
{
	uint64_t checksum = 0;
	if (data != nullptr && dataSize > 0)
	{
		struct MemoryStreamBuffer : public std::streambuf
		{
			MemoryStreamBuffer(char* data, size_t dataSize) { setg(data, data, data + dataSize); }
		};
		MemoryStreamBuffer membuffer((char*)data, dataSize);
		checksum = HashMd5(std::istream(&membuffer));
	}
	return checksum;
}

uint64_t StringInfo::HashMd5(std::istream& stream)
{
	uint64_t checksum = 0;
	uint64_t totalSize = 0;
	if (stream.good())
	{
		BCRYPT_ALG_HANDLE hAlg = NULL;
		if (BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_MD5_ALGORITHM, NULL, 0)))
		{
			ULONG cbResult = 0;
			ULONG hashObjectSize = 0;
			Array<uint8_t> hashObject;
			if (BCRYPT_SUCCESS(BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<uint8_t*>(&hashObjectSize), sizeof(hashObjectSize), &cbResult, 0)) && hashObjectSize > 0)
			{
				hashObject.resize(hashObjectSize, 0);
				BCRYPT_HASH_HANDLE hHash = NULL;
				if (BCRYPT_SUCCESS(BCryptCreateHash(hAlg, &hHash, hashObject.data(), hashObjectSize, NULL, 0, 0)))
				{
					bool success = true;
					while (success)
					{
						char data[1024];
						ULONG dataSize = ULONG(stream.read(data, sizeof(data)).gcount());
						if (dataSize == 0)
							break;
						totalSize += dataSize;
						success = BCRYPT_SUCCESS(BCryptHashData(hHash, reinterpret_cast<uint8_t*>(data), dataSize, 0));
					}
					if (success)
					{
						ULONG hashSize = 0;
						if (BCRYPT_SUCCESS(BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, reinterpret_cast<uint8_t*>(&hashSize), sizeof(hashSize), &cbResult, 0)) && hashSize > 0)
						{
							Array<uint8_t> hashValue;
							hashValue.resize(std::max(size_t(hashSize), sizeof(uint64_t)), 0);
							if (BCRYPT_SUCCESS(BCryptFinishHash(hHash, hashValue.data(), hashSize, 0)))
								checksum = *reinterpret_cast<uint64_t*>(hashValue.data());
						}
					}
					BCryptDestroyHash(hHash);
				}
			}
			BCryptCloseAlgorithmProvider(hAlg, 0);
		}
	}
	return totalSize ? checksum : 0;
}

uint64_t StringInfo::HashMd5(const WString& s)
{
	return StringInfoInternal::HashMd5(s.c_str(), s.length());
}

uint64_t StringInfo::HashMd5(const AString& s)
{
	return StringInfoInternal::HashMd5(s.c_str(), s.length());
}

AString StringInfo::FormatTime(const struct tm* t, const char* fmt)
{
	return StringInfoInternal::FormatTime(t, fmt);
}

WString StringInfo::FormatTime(const struct tm* t, const wchar_t* fmt)
{
	return StringInfoInternal::FormatTime(t, fmt);
}

AString StringInfo::FormatLocalTime(time_t t, const char* fmt)
{
	return StringInfoInternal::FormatLocalTime(t, fmt);
}

WString StringInfo::FormatLocalTime(time_t t, const wchar_t* fmt)
{
	return StringInfoInternal::FormatLocalTime(t, fmt);
}

AString StringInfo::FormatUtcTime(time_t t, const char* fmt)
{
	return StringInfoInternal::FormatUtcTime(t, fmt);
}

WString StringInfo::FormatUtcTime(time_t t, const wchar_t* fmt)
{
	return StringInfoInternal::FormatUtcTime(t, fmt);
}

AString StringInfo::ToAnsi(const WString& str) 
{ 
	return ToAnsi(str.c_str()); 
}
		
AString StringInfo::ToAnsi(const wchar_t* str)
{
	if (StringInfo::IsNullOrEmpty(str))
		return AString();

	char fixedBuffer[256];
	int32_t charCount = WideCharToMultiByte(CP_ACP, 0, str, -1, fixedBuffer, _countof(fixedBuffer), NULL, NULL);
	if (charCount > 0)
		return AString(fixedBuffer);

	charCount = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
	if (charCount > 0)
	{
		std::unique_ptr<char[]> bufferPtr = std::make_unique<char[]>(size_t(charCount));
		if (bufferPtr.get() && WideCharToMultiByte(CP_ACP, 0, str, -1, bufferPtr.get(), charCount, NULL, NULL) > 0)
			return AString(bufferPtr.get());
	}

	return AString();
}

char StringInfo::ToAnsi(const wchar_t chr)
{
	wchar_t src[2] = { chr, 0 };
	AString dst = ToAnsi(src);
	return dst.size() ? dst[0] : 0;
}

WString StringInfo::ToWide(const AString& str) 
{ 
	return ToWide(str.c_str()); 
}

WString StringInfo::ToWide(const char* str)
{
	if (StringInfo::IsNullOrEmpty(str))
		return WString();

	wchar_t fixedBuffer[256];
	int32_t charCount = MultiByteToWideChar(CP_ACP, 0, str, -1, fixedBuffer, _countof(fixedBuffer));
	if (charCount > 0)
		return WString(fixedBuffer);

	charCount = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
	if (charCount > 0)
	{
		std::unique_ptr<wchar_t[]> bufferPtr = std::make_unique<wchar_t[]>(size_t(charCount));
		if (bufferPtr.get() && MultiByteToWideChar(CP_ACP, 0, str, -1, bufferPtr.get(), charCount) > 0)
			return WString(bufferPtr.get());
	}

	return WString();
}

wchar_t StringInfo::ToWide(const char chr)
{
	char src[2] = { chr, 0 };
	WString dst = ToWide(src);
	return dst.size() ? dst[0] : 0;
}

bool FileInfo::Exists(const wchar_t* filePath)
{
	if (StringInfo::IsNullOrEmpty(filePath))
		return false;
	DWORD attributes = FileInfo::FileAttributes(filePath);
	if (attributes == INVALID_FILE_ATTRIBUTES)
		return false;
	return true;
}

bool FileInfo::IsRegular(const wchar_t* filePath)
{
	if (StringInfo::IsNullOrEmpty(filePath))
		return false;
	DWORD attributes = FileInfo::FileAttributes(filePath);
	if ((attributes == INVALID_FILE_ATTRIBUTES) || (attributes & FILE_ATTRIBUTE_DIRECTORY))
		return false;
	return true;
}

bool FileInfo::IsSymlink(const wchar_t* filePath)
{
	return !SymlinkTarget(filePath).empty();
}

bool FileInfo::IsDirectory(const wchar_t* filePath)
{
	if (StringInfo::IsNullOrEmpty(filePath))
		return false;
	DWORD attributes = FileInfo::FileAttributes(filePath);
	if ((attributes != INVALID_FILE_ATTRIBUTES) && (attributes & FILE_ATTRIBUTE_DIRECTORY))
		return true;
	return false;
}

bool FileInfo::IsReadOnly(const wchar_t* filePath)
{
	DWORD attributes = FileInfo::FileAttributes(filePath);
	if ((attributes != INVALID_FILE_ATTRIBUTES) && (attributes & FILE_ATTRIBUTE_READONLY))
		return true;
	return false;
}

DWORD FileInfo::FileAttributes(const wchar_t* filePath)
{
	if (StringInfo::IsNullOrEmpty(filePath))
		return INVALID_FILE_ATTRIBUTES;

	AutoHandle hFile = CreateFile(filePath, 0, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (hFile.Handle() == INVALID_HANDLE_VALUE)
		return INVALID_FILE_ATTRIBUTES;

	BY_HANDLE_FILE_INFORMATION fileInfo = {0};
	if (GetFileInformationByHandle(hFile.Handle(), &fileInfo) == FALSE)
		return INVALID_FILE_ATTRIBUTES;

	return fileInfo.dwFileAttributes;
}

bool FileInfo::SetReadOnly(const wchar_t* filePath, bool readOnly)
{
	DWORD attributes = FileInfo::FileAttributes(filePath);
	if (attributes != INVALID_FILE_ATTRIBUTES)
	{
		if (readOnly)
			return !!SetFileAttributes(filePath, attributes | FILE_ATTRIBUTE_READONLY);
		else
			return !!SetFileAttributes(filePath, attributes & ~FILE_ATTRIBUTE_READONLY);
	}
	return false;
}

String FileInfo::FullPath(const wchar_t* filePath)
{
	if (StringInfo::IsNullOrEmpty(filePath))
		return String();

	wchar_t path[MAX_PATH];
	DWORD len = GetFullPathName(filePath, _countof(path), path, nullptr);
	if (len != 0)
	{
		if (len < _countof(path))
			return String(path);

		if (len > _countof(path))
		{
			std::unique_ptr<wchar_t[]> longPath = std::make_unique<wchar_t[]>(len);
			DWORD longLen = GetFullPathName(filePath, len, longPath.get(), nullptr);
			if (longLen != 0 && longLen < len)
				return ExtendedPath(longPath.get());
		}
	}
	return String();
}

String FileInfo::RootPath(const wchar_t* filePath)
{
	if (StringInfo::IsNullOrEmpty(filePath))
		return String();
	String fullPath = FileInfo::FullPath(filePath);
	if (fullPath.size() == 0)
		return String();
	wchar_t* path = &fullPath[0];
	if (StringInfo::Strncmp(path, ExtendedPathPrefix, ExtendedPathPrefixLength) == 0)
		path += ExtendedPathPrefixLength;
	size_t end = 0;
	const size_t pathLen = StringInfo::Strlen(path);
	while (end < pathLen && StringInfo::Strchr(TEXT("\\/"), path[end]) == nullptr)
		++end;
	if (end < pathLen)
		path[end] = TEXT('\0');
	return path;
}

String FileInfo::FolderPath(const wchar_t* filePath)
{
	String path = (filePath != nullptr ? filePath : TEXT(""));
	if (path.empty())
		return path;
	size_t end = path.length();
	while (end > 0 && StringInfo::Strchr(TEXT("\\/"), path[end-1]) != nullptr)
		--end;
	while (end > 0 && StringInfo::Strchr(TEXT("\\/"), path[end-1]) == nullptr)
		--end;
	while (end > 0 && StringInfo::Strchr(TEXT("\\/"), path[end-1]) != nullptr)
		--end;
	path.resize(end);
	return path;
}

String FileInfo::FullFolderPath(const wchar_t* filePath)
{
	return FileInfo::FolderPath(FileInfo::FullPath(filePath).c_str());
}

String FileInfo::FileTitle(const wchar_t* filePath)
{
	if (StringInfo::IsNullOrEmpty(filePath))
		return String();
	size_t end = StringInfo::Strlen(filePath);
	while (end > 0 && StringInfo::Strchr(TEXT("\\/"), filePath[end-1]) == nullptr)
		--end;
	filePath += end;
	end = StringInfo::Strlen(filePath);
	while (end > 0 && StringInfo::Strchr(TEXT("."), filePath[end-1]) == nullptr)
		--end;
	if (end == 0)
		return filePath;
	return String(filePath, end-1);
}

String FileInfo::FileShortTitle(const wchar_t* filePath)
{
	if (StringInfo::IsNullOrEmpty(filePath))
		return String();
	size_t end = StringInfo::Strlen(filePath);
	while (end > 0 && StringInfo::Strchr(TEXT("\\/"), filePath[end-1]) == nullptr)
		--end;
	filePath += end;
	end = 0;
	while (filePath[end] != TEXT('.') && filePath[end] != TEXT('\0'))
		++end;
	if (end == 0)
		return filePath;
	return String(filePath, end);
}

String FileInfo::FileExtension(const wchar_t* filePath)
{
	if (StringInfo::IsNullOrEmpty(filePath))
		return String();
	size_t end = StringInfo::Strlen(filePath);
	while (end > 0 && StringInfo::Strchr(TEXT("\\/."), filePath[end-1]) == nullptr)
		--end;
	if (end > 0 && filePath[end-1] == '.')
		return filePath+end-1;
	return String();
}

String FileInfo::FileName(const wchar_t* filePath)
{
	if (StringInfo::IsNullOrEmpty(filePath))
		return String();
	size_t end = StringInfo::Strlen(filePath);
	while (end > 0 && StringInfo::Strchr(TEXT("\\/"), filePath[end-1]) == nullptr)
		--end;
	return filePath+end;
}

int64_t FileInfo::FileSize(const wchar_t* filePath)
{
	if (StringInfo::IsNullOrEmpty(filePath))
		return -1;
	WIN32_FILE_ATTRIBUTE_DATA fileInfo;
	if (GetFileAttributesEx(filePath, GetFileExInfoStandard, &fileInfo) == FALSE)
		return -1;
	if ((fileInfo.dwFileAttributes == INVALID_FILE_ATTRIBUTES) || (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		return -1;
	LARGE_INTEGER fileSize = {0};
	fileSize.HighPart = fileInfo.nFileSizeHigh;
	fileSize.LowPart = fileInfo.nFileSizeLow;
	return fileSize.QuadPart;
}

int64_t FileInfo::FileUncompressedSize(const wchar_t* filePath)
{
	if (StringInfo::IsNullOrEmpty(filePath))
		return -1;
	AutoHandle hFile = CreateFile(filePath, FILE_GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile.IsValid() == false)
		return -1;
	LARGE_INTEGER fileSize = {0};
	if (GetFileSizeEx(hFile.Handle(), &fileSize) == FALSE)
		return -1;
	return fileSize.QuadPart;
}

int64_t FileInfo::FileDiskSize(const wchar_t* filePath)
{
	if (StringInfo::IsNullOrEmpty(filePath))
		return -1;
	DWORD attributes = FileInfo::FileAttributes(filePath);
	if ((attributes == INVALID_FILE_ATTRIBUTES) || (attributes & FILE_ATTRIBUTE_DIRECTORY))
		return -1;
		
	AutoHandle hFile = CreateFile(filePath, FILE_GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, NULL);
	if (hFile.IsValid() == false)
		return -1;
	BY_HANDLE_FILE_INFORMATION fileInfo = {0};
	if (GetFileInformationByHandle(hFile.Handle(), &fileInfo) == FALSE)
		return -1;

	String volumePath = StringInfo::Format(TEXT("\\\\.\\%s"), FileInfo::RootPath(filePath).c_str());
	AutoHandle hVolume = CreateFile(volumePath.c_str(), FILE_GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, NULL);
	if (hVolume.IsValid() == false)
		return -1;
			
	QUERY_FILE_LAYOUT_INPUT layoutInput = {0};
	layoutInput.Flags = QUERY_FILE_LAYOUT_RESTART | QUERY_FILE_LAYOUT_INCLUDE_STREAMS | QUERY_FILE_LAYOUT_INCLUDE_STREAMS_WITH_NO_CLUSTERS_ALLOCATED;
	layoutInput.FilterType = QUERY_FILE_LAYOUT_FILTER_TYPE_FILEID;

	ULARGE_INTEGER fileReferenceNum;
	fileReferenceNum.LowPart = fileInfo.nFileIndexLow;
	fileReferenceNum.HighPart = fileInfo.nFileIndexHigh;
	layoutInput.NumberOfPairs = 1;
	layoutInput.Filter.FileReferenceRanges[0].StartingFileReferenceNumber = fileReferenceNum.QuadPart;
	layoutInput.Filter.FileReferenceRanges[0].EndingFileReferenceNumber = fileReferenceNum.QuadPart;

	DWORD bytesRead = 0;
	Array<uint8_t> layoutOutputBuffer;
	layoutOutputBuffer.resize(sizeof(QUERY_FILE_LAYOUT_OUTPUT)*32);
	while (DeviceIoControl(hVolume.Handle(), FSCTL_QUERY_FILE_LAYOUT, &layoutInput, DWORD(sizeof(layoutInput)), layoutOutputBuffer.data(), DWORD(layoutOutputBuffer.size()), &bytesRead, NULL) == FALSE)
	{
		DWORD dwError = GetLastError();
		if (dwError != ERROR_INSUFFICIENT_BUFFER && dwError != ERROR_MORE_DATA)
			return -1;
		layoutOutputBuffer.resize(layoutOutputBuffer.size()*2);
	}

	LARGE_INTEGER fileSize;
	fileSize.QuadPart = -1;

	const QUERY_FILE_LAYOUT_OUTPUT* layoutOutput = reinterpret_cast<QUERY_FILE_LAYOUT_OUTPUT*>(layoutOutputBuffer.data());
	const uint8_t* fileEntryOrigin = layoutOutputBuffer.data();
	for (uint32_t fileEntryOffset = layoutOutput->FirstFileOffset; fileEntryOffset > 0;)
	{
		fileEntryOrigin += fileEntryOffset;
		const FILE_LAYOUT_ENTRY* fileEntry = reinterpret_cast<const FILE_LAYOUT_ENTRY*>(fileEntryOrigin);
		fileEntryOffset = fileEntry->NextFileOffset;

		if (fileEntry->FileReferenceNumber == fileReferenceNum.QuadPart)
		{
			fileSize.QuadPart = 0;
			const uint8_t* streamEntryOrigin = fileEntryOrigin;
			for (uint32_t streamEntryOffset = fileEntry->FirstStreamOffset; streamEntryOffset > 0;)
			{
				streamEntryOrigin += streamEntryOffset;
				const STREAM_LAYOUT_ENTRY* streamEntry = reinterpret_cast<const STREAM_LAYOUT_ENTRY*>(streamEntryOrigin);
				streamEntryOffset = streamEntry->NextStreamOffset; 
				if (streamEntry->AttributeFlags & FILE_ATTRIBUTE_SPARSE_FILE)
					continue;

				const WCHAR dataStreamName[] = L":$DATA";
				const size_t dataStreamNameSize = (_countof(dataStreamName)-1)*sizeof(WCHAR);
				if (streamEntry->StreamIdentifierLength == 0)
					fileSize.QuadPart += streamEntry->AllocationSize.QuadPart;
				else if (streamEntry->StreamIdentifierLength >= dataStreamNameSize && memcmp(dataStreamName, reinterpret_cast<const uint8_t*>(streamEntry->StreamIdentifier)+streamEntry->StreamIdentifierLength-dataStreamNameSize, dataStreamNameSize) == 0)
					fileSize.QuadPart += streamEntry->AllocationSize.QuadPart;
			}
			break;
		}
	}
	return fileSize.QuadPart;
}

uint64_t FileInfo::FileHashMd5(const wchar_t* filePath)
{
	if (StringInfo::IsNullOrEmpty(filePath) == false)
	{
		std::ifstream fileStream(filePath, std::ios::in | std::ios::binary);
		if (fileStream.is_open())
			return StringInfo::HashMd5(fileStream);
	}
	return 0;
}

bool FileInfo::ReadFile(const wchar_t* filePath, Array<uint8_t>& contents)
{
	contents.clear();
	AutoCrtFile file = fopen(StringInfo::WtoA(filePath), "rb");
	if (file.IsValid() == false)
		return false;
	fseek(file.Get(), 0, SEEK_END);
	long fileSize = ftell(file.Get());
	if (fileSize == 0)
		return true;
	fseek(file.Get(), 0, SEEK_SET);
	contents.resize(fileSize);
	if (fread(contents.data(), 1, fileSize, file.Get()) != fileSize)
	{
		contents.clear();
		return false;
	}
	return true;
}

bool FileInfo::ReadFileLines(const wchar_t* filePath, Array<AString>& lines)
{
	lines.clear();
	return FileInfo::ReadFileLines(filePath, [&lines](const AString& line) -> bool
	{
		lines.push_back(line);
		return true;
	});
}

bool FileInfo::ReadFileLines(const wchar_t* filePath, const std::function<bool(const AString&)>& readline)
{
	AutoCrtFile file = fopen(StringInfo::WtoA(filePath), "rb");
	if (file.IsValid() == false)
		return false;
	AString line;
	for (int32_t c = fgetc(file.Get()); c != EOF; c = fgetc(file.Get()))
	{
		if (c == '\n')
		{
			if (readline(line) == false)
				return true;
			line.clear();
		}
		else if (c != '\r')
		{
			line += char(c);
		}
	}
	if (line.size() > 0)
	{
		readline(line);
	}
	return true;
}

String FileInfo::SymlinkTarget(const wchar_t* filePath)
{
	if (StringInfo::IsNullOrEmpty(filePath))
		return String();
	AutoHandle hFile = CreateFile(filePath, FILE_GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, NULL);
	if (hFile.IsValid() == false)
		return String();

	DWORD dwBytesReturned = 0;
	REPARSE_GUID_DATA_BUFFER reparseHeader = {0};
	if (DeviceIoControl(hFile.Handle(), FSCTL_GET_REPARSE_POINT, NULL, 0, &reparseHeader, sizeof(reparseHeader), &dwBytesReturned, NULL) == FALSE)
	{
		DWORD dwError = GetLastError();
		if (dwError != ERROR_INSUFFICIENT_BUFFER && dwError != ERROR_MORE_DATA)
			return String();
	}

	Array<uint8_t> reparsePointBuffer;
	reparsePointBuffer.resize(reparseHeader.ReparseDataLength + sizeof(REPARSE_GUID_DATA_BUFFER), 0);
    REPARSE_GUID_DATA_BUFFER* reparsePoint = reinterpret_cast<REPARSE_GUID_DATA_BUFFER*>(reparsePointBuffer.data());

	if (DeviceIoControl(hFile.Handle(), FSCTL_GET_REPARSE_POINT, NULL, 0, reparsePoint, DWORD(reparsePointBuffer.size()), &dwBytesReturned, NULL) == FALSE)
		return String();
	if (reparsePoint->ReparseTag != IO_REPARSE_TAG_SYMLINK)
		return String();

	// Taken from union in struct REPARSE_DATA_BUFFER in kernel mode Windows 8.1 api "km/ntifs.h"
    struct SymbolicLinkReparseBuffer 
	{
        USHORT SubstituteNameOffset;
        USHORT SubstituteNameLength;
        USHORT PrintNameOffset;
        USHORT PrintNameLength;
        ULONG Flags;
        WCHAR PathBuffer[1];
    };

	const SymbolicLinkReparseBuffer* symlink = reinterpret_cast<SymbolicLinkReparseBuffer*>(&reparsePoint->ReparseGuid);
	if (symlink->SubstituteNameLength == 0)
		return String();

	const wchar_t* targetStart = reinterpret_cast<const wchar_t*>(reinterpret_cast<const uint8_t*>(symlink->PathBuffer)+symlink->SubstituteNameOffset);
	const wchar_t* targetEnd = reinterpret_cast<const wchar_t*>(reinterpret_cast<const uint8_t*>(targetStart)+symlink->SubstituteNameLength);
    if (_wcsnicmp(targetStart, L"\\??\\", 4) == 0)
		targetStart += 4;
	if (targetStart >= targetEnd)
		return String();

	return String(targetStart, targetEnd);
}

bool FileInfo::Delete(const wchar_t* filePath)
{
	if (StringInfo::IsNullOrEmpty(filePath))
		return false;
	if (FileInfo::Exists(filePath) == false)
		return true;
	if (FileInfo::SetReadOnly(filePath, false) == false)
		return false;
	if (::DeleteFile(filePath) == FALSE)
		return false;
	return true;
}

bool FileInfo::Move(const wchar_t* srcPath, const wchar_t* dstPath)
{
	if (StringInfo::IsNullOrEmpty(srcPath) || StringInfo::IsNullOrEmpty(dstPath))
		return false;
	if (FileInfo::Exists(srcPath) == false)
		return false;
	FileInfo::Delete(dstPath);
	if (::MoveFile(srcPath, dstPath) == FALSE)
		return false;
	return true;
}

int32_t FileInfo::Compare(const wchar_t* filePath0, const wchar_t* filePath1)
{
	const bool fileExists0 = FileInfo::IsRegular(filePath0);
	const bool fileExists1 = FileInfo::IsRegular(filePath1);
	if (fileExists0 && fileExists1)
	{
		const uint64_t fileHash0 = FileInfo::FileHashMd5(filePath0);
		const uint64_t fileHash1 = FileInfo::FileHashMd5(filePath1);		
		return fileHash0 < fileHash1 ? -1 : fileHash0 > fileHash1 ? 1 : 0;
	}
	return fileExists0 ? -1 : fileExists1 ? 1 : 0;
}

bool FileInfo::CreateWritableFile(const wchar_t* filePath)
{
	if (StringInfo::IsNullOrEmpty(filePath))
		return false;
	if (FileInfo::IsRegular(filePath))
		return FileInfo::SetReadOnly(filePath, false);
	AutoHandle hFile = CreateFile(filePath, FILE_GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile.IsValid())
		return true;
	return false;
}

bool FileInfo::CreateDirectory(const wchar_t* folderPath)
{
	if (StringInfo::IsNullOrEmpty(folderPath))
		return false;
	
	String fullFolderPath = FileInfo::FullPath(folderPath);
	if (fullFolderPath.length() == 0)
		return false;
	if (fullFolderPath[fullFolderPath.length()-1] != L'\\')
		fullFolderPath += L'\\';

	wchar_t* path = const_cast<wchar_t*>(fullFolderPath.c_str());
	wchar_t* split = path;
	
	while (*split != L'\0' && wcschr(L"<>:\"/\\|?*", *split) != nullptr)
		++split;

	split = wcschr(split, L'\\');
	for (int32_t splitNum = 0; split != nullptr; ++splitNum) 
	{
		if (splitNum > 0)
		{
			*split = L'\0';
			if (IsDirectory(path) == false)
			{
				if (::CreateDirectory(path, NULL) == FALSE && GetLastError() != ERROR_ALREADY_EXISTS)
					return false;
			}
			*split = L'\\';
		}
		split = wcschr(split+1, L'\\');
	}
	return true;
}

bool FileInfo::CreateFileDirectory(const wchar_t* filePath)
{
	String folderPath = FileInfo::FullFolderPath(filePath);
	if (folderPath.empty())
		return false;
	return FileInfo::CreateDirectory(folderPath.c_str());
}

bool FileInfo::DeleteDirectoryRecursively(const wchar_t* folderPath)
{
	bool success = true;
	if (FileInfo::IsDirectory(folderPath))
	{
		WIN32_FIND_DATA findFileData = {0};
		HANDLE hFind = ::FindFirstFile(StringInfo::Format(TEXT("%s\\*"), folderPath).c_str(), &findFileData);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					if (StringInfo::Strcmp(findFileData.cFileName, TEXT(".")) == 0 || 
						StringInfo::Strcmp(findFileData.cFileName, TEXT("..")) == 0)
						continue;
				}

				String filePath = StringInfo::Format(TEXT("%s\\%s"), folderPath, findFileData.cFileName);
				if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					success &= DeleteDirectoryRecursively(filePath.c_str());
				else 
					success &= FileInfo::Delete(filePath.c_str());
			}
			while (::FindNextFile(hFind, &findFileData));
			::FindClose(hFind);
		}

		if (RemoveDirectory(folderPath) == FALSE)
			success = false;
	}
	return success;
}

bool FileInfo::DeleteEmptyDirectories(const wchar_t* folderPath, bool deleteEmptyParents)
{
	bool success = true;
	if (FileInfo::IsDirectory(folderPath))
	{
		if (FileInfo::IsDirectoryEmpty(folderPath))
		{
			if (RemoveDirectory(folderPath) == FALSE)
				success = false;
		}
	}
	
	if (FileInfo::IsDirectory(folderPath) == false)
	{
		String parentFolderPath = FileInfo::FullFolderPath(folderPath);
		if (parentFolderPath.empty() == false && StringInfo::Stricmp(parentFolderPath.c_str(), folderPath) != 0)
		{
			success &= FileInfo::DeleteEmptyDirectories(parentFolderPath.c_str(), true);
		}
	}
	return success;
}

bool FileInfo::IsDirectoryEmpty(const wchar_t* folderPath)
{
	bool isEmpty = true;
	if (StringInfo::IsNullOrEmpty(folderPath) == false)
	{
		WIN32_FIND_DATA findFileData = {0};
		HANDLE hFind = ::FindFirstFile(StringInfo::Format(TEXT("%s\\*"), folderPath).c_str(), &findFileData);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					if (StringInfo::Strcmp(findFileData.cFileName, TEXT(".")) == 0 || 
						StringInfo::Strcmp(findFileData.cFileName, TEXT("..")) == 0)
						continue;
				}
				isEmpty = false;
				break;
			}
			while (::FindNextFile(hFind, &findFileData));
			::FindClose(hFind);
		}
	}
	return isEmpty;
}

void FileInfo::FindFiles(StringArray& files, const wchar_t* folderPath, const wchar_t* pattern, Find::Enum flags)
{
	String root;
	if (StringInfo::IsNullOrEmpty(folderPath) == false)
		root = FileInfo::FullPath(folderPath);
	if (root.size() > 0 && root[root.size()-1] != TEXT('\\'))
		root += TEXT("\\");
	
	WIN32_FIND_DATA findFileData = {0};
	HANDLE hFind = ::FindFirstFile(StringInfo::Format(TEXT("%s%s"), root.c_str(), StringInfo::IsNullOrEmpty(pattern) ? TEXT("*") : pattern).c_str(), &findFileData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			String path = root + findFileData.cFileName;
			if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (StringInfo::Strcmp(findFileData.cFileName, TEXT(".")) != 0 && StringInfo::Strcmp(findFileData.cFileName, TEXT("..")) != 0)
				{
					if (flags & Find::kDirectories)
						files.push_back(path);
					if (flags & Find::kRecursive)
						FindFiles(files, path.c_str(), pattern, flags);
				}
			}
			else if (flags & Find::kFiles)
			{
				files.push_back(path);
			}
		}
		while (::FindNextFile(hFind, &findFileData));
		::FindClose(hFind);
	}
}

String FileInfo::ApplicationFilePath()
{
	wchar_t path[MAX_PATH] = {0};
	DWORD len = GetModuleFileName(nullptr, path, _countof(path));
	if (len == 0 || len >= _countof(path))
		return String();
	return FullPath(path);
}

String FileInfo::CreateTempFile(const wchar_t* folderPath, const wchar_t* filePrefix)
{
	wchar_t tempFolderPath[MAX_PATH] = {0};
	if (StringInfo::IsNullOrEmpty(folderPath))
	{
		DWORD len = ::GetTempPath(_countof(tempFolderPath), tempFolderPath);
		if (len == 0 || len >= _countof(tempFolderPath))
			return String();
		folderPath = tempFolderPath;
	}

	if (StringInfo::IsNullOrEmpty(filePrefix))
		filePrefix = TEXT("p4vfs");

	wchar_t tempFilePath[MAX_PATH] = {0};
	if (::GetTempFileName(folderPath, filePrefix, 0, tempFilePath) == 0)
		return String();

	return tempFilePath;
}

bool FileInfo::IsExtendedPath(const wchar_t* path)
{
	return StringInfo::StartsWith(path, ExtendedPathPrefix);
}

String FileInfo::ExtendedPath(const wchar_t* path)
{
	if (StringInfo::IsNullOrEmpty(path))
		return String();
	
	String extPath;
	if (IsExtendedPath(path) == false)
		extPath += ExtendedPathPrefix;
	
	extPath += path;
	return extPath;
}

String FileInfo::UnextendedPath(const wchar_t* path)
{
	if (StringInfo::IsNullOrEmpty(path))
		return String();

	if (IsExtendedPath(path))
		return path + ExtendedPathPrefixLength;

	return path;
}

time_t TimeInfo::GetTime()
{
	time_t t = 0;
	_time64(&t);
	return t;
}

FILETIME TimeInfo::GetUtcFileTime()
{
	FILETIME systemTime;
	::GetSystemTimeAsFileTime(&systemTime);
	return systemTime;
}

uint64_t TimeInfo::FileTimeToUInt64(const FILETIME& v)
{
	return uint64_t((uint64_t(v.dwHighDateTime) << 32) | v.dwLowDateTime);
}

FILETIME TimeInfo::UInt64ToFileTime(uint64_t v)
{
	return FILETIME { static_cast<DWORD>(v & (1ULL<<32)-1), static_cast<DWORD>(v >> 32) };
}

AutoHandle::AutoHandle(HANDLE hHandle) :
	m_hHandle(hHandle)
{
}

AutoHandle::~AutoHandle()
{
	Close();
}

bool AutoHandle::IsValid() const
{
	return (m_hHandle != INVALID_HANDLE_VALUE && m_hHandle != NULL);
}

HANDLE AutoHandle::Handle() const
{
	return m_hHandle;
}

HANDLE* AutoHandle::HandlePtr()
{
	return &m_hHandle;
}

HRESULT AutoHandle::Close()
{
	HRESULT hr = S_OK;
	if (m_hHandle != INVALID_HANDLE_VALUE && m_hHandle != NULL)
	{
		if (!CloseHandle(m_hHandle))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
		m_hHandle = INVALID_HANDLE_VALUE;
	}
	return hr;
}

void AutoHandle::Reset(HANDLE hHandle)
{
	Close();
	m_hHandle = hHandle;
}

AutoCrtFile::AutoCrtFile(FILE* file) :
	m_File(file)
{
}

AutoCrtFile::~AutoCrtFile()
{
	Close();
}

bool AutoCrtFile::IsValid() const
{
	return m_File != nullptr;
}

void AutoCrtFile::Set(FILE* file)
{
	Close();
	m_File = file;
}

FILE* AutoCrtFile::Get()
{
	return m_File;
}

void AutoCrtFile::Close()
{
	if (m_File != nullptr)
	{
		fclose(m_File);
		m_File = nullptr;
	}
}

AutoTempFile::AutoTempFile(const wchar_t* filePath) :
	m_FilePath(filePath ? filePath : TEXT(""))
{
}

AutoTempFile::~AutoTempFile()
{
	if (m_FilePath.empty() == false)
	{
		FileInfo::Delete(m_FilePath.c_str());
		m_FilePath.clear();
	}
}

const String& AutoTempFile::GetFilePath() const
{
	return m_FilePath;
}

AutoServiceHandle::AutoServiceHandle(SC_HANDLE hHandle) :
	m_hHandle(hHandle)
{
}

AutoServiceHandle::~AutoServiceHandle()
{
	Close();
}

bool AutoServiceHandle::IsValid() const
{
	return m_hHandle != NULL;
}

SC_HANDLE AutoServiceHandle::Handle() const
{
	return m_hHandle;
}

SC_HANDLE* AutoServiceHandle::HandlePtr()
{
	return &m_hHandle;
}

HRESULT AutoServiceHandle::Close()
{
	HRESULT hr = S_OK;
	if (m_hHandle != NULL)
	{
		if (!CloseServiceHandle(m_hHandle))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
		m_hHandle = NULL;
	}
	return hr;
}

void AutoServiceHandle::Reset(SC_HANDLE hHandle)
{
	Close();
	m_hHandle = hHandle;
}

AutoMutex::AutoMutex(const AutoHandle& mutex) :
	AutoMutex(mutex.Handle())
{
}

AutoMutex::AutoMutex(HANDLE mutex) : 
	m_Mutex(NULL)
{
	if (WaitForSingleObject(mutex, INFINITE) == WAIT_OBJECT_0)
	{
		m_Mutex = mutex;
	}
}

AutoMutex::~AutoMutex()
{
	if (m_Mutex != NULL)
	{
		ReleaseMutex(m_Mutex);
		m_Mutex = NULL;
	}
}

bool AutoMutex::IsAquired() const
{
	return m_Mutex != NULL;
}

bool FileStream::CanWrite()
{ 
	return false; 
}

bool FileStream::CanRead() 
{ 
	return false; 
}

HRESULT FileStream::Write(HANDLE hReadHandle, UINT64* bytesRead)
{ 
	return E_FAIL; 
}

HRESULT FileStream::Read(HANDLE hWriteHandle, UINT64* bytesWritten)
{ 
	return E_FAIL; 
}

Process::ExecuteResult Process::Execute(const wchar_t* cmd, const wchar_t* dir, Process::ExecuteFlags::Enum flags, HANDLE hUserToken)
{
	ExecuteResult result = {0};
	result.m_HR = E_FAIL;
	result.m_ExitCode = DWORD(-1);
	result.m_ProcessId = 0;

	if (StringInfo::IsNullOrEmpty(cmd))
	{
		result.m_HR = HRESULT_FROM_WIN32(ERROR_BAD_COMMAND);
		return result;
	}

	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	memset(&pi, 0, sizeof(pi));
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);

	AutoHandle hRead;
	AutoHandle hWrite;
	if (flags & ExecuteFlags::StdOut)
	{
		SECURITY_ATTRIBUTES sa;
		memset(&sa, 0, sizeof(sa));
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.bInheritHandle = true;

		if (CreatePipe(hRead.HandlePtr(), hWrite.HandlePtr(), &sa, 0) == FALSE)
		{
			result.m_HR = HRESULT_FROM_WIN32(GetLastError());
			return result;
		}

		si.dwFlags = STARTF_USESTDHANDLES;
		si.hStdError = hWrite.Handle();
		si.hStdOutput = hWrite.Handle();
	}

	DWORD creationFlags = 0;
	if (flags & ExecuteFlags::HideWindow)
		creationFlags |= CREATE_NO_WINDOW;

	WString commandLineRw = cmd;
	if (hUserToken != NULL && hUserToken != INVALID_HANDLE_VALUE)
	{
		creationFlags |= CREATE_UNICODE_ENVIRONMENT;
		PVOID environment = nullptr;

		if (CreateEnvironmentBlock(&environment, hUserToken, FALSE) == FALSE)
		{
			result.m_HR = ERROR_INVALID_ENVIRONMENT;
			return result;
		}
		std::shared_ptr<void> environmentScope(nullptr, [environment](void*) 
		{ 
			DestroyEnvironmentBlock(environment);
		});

		if (CreateProcessAsUserW(hUserToken,  NULL, &commandLineRw[0], NULL, NULL, TRUE, creationFlags, environment, dir, &si, &pi) == FALSE)
		{
			result.m_HR = HRESULT_FROM_WIN32(GetLastError());
			return result;
		}
	}
	else
	{
		if (CreateProcessW(NULL, &commandLineRw[0], NULL, NULL, TRUE, creationFlags, NULL, dir, &si, &pi) == FALSE)
		{
			result.m_HR = HRESULT_FROM_WIN32(GetLastError());
			return result;
		}
	}

	if (flags & ExecuteFlags::StdOut)
	{
		for (;;)
		{
			const bool processAlive = WaitForSingleObject(pi.hProcess, 0) == WAIT_TIMEOUT;

			DWORD availableLength = 0;
			if (PeekNamedPipe(hRead.Handle(), NULL, 0, NULL, &availableLength, NULL) == FALSE)   
				break;

			if (availableLength == 0)
			{
				if (processAlive == false)
					break;

				Sleep(0);
				continue;
			}

			char readText[512];
			DWORD readLength = 0;
			if (ReadFile(hRead.Handle(), readText, std::min<DWORD>(sizeof(readText)-1, availableLength), &readLength, NULL) == FALSE || readLength == 0)  
				break;

			readText[readLength] = 0;
			result.m_StdOut.append(readText);
		}

		hWrite.Close();
		hRead.Close();
	}

	if (flags & (ExecuteFlags::StdOut|ExecuteFlags::WaitForExit))
	{
		WaitForSingleObject(pi.hProcess, INFINITE);
		GetExitCodeProcess(pi.hProcess, &result.m_ExitCode);
	}

	if (flags & ExecuteFlags::KeepOpen)
	{
		result.m_hProcess = pi.hProcess;
		result.m_hThread = pi.hThread;
	}
	else
	{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

	result.m_ProcessId = pi.dwProcessId;
	result.m_HR = S_OK;
	return result;
}

String Process::GetProcessNameById(DWORD processId)
{
	AutoHandle hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
	if (hProcess.IsValid())
	{
		wchar_t processName[1024] = {0};
		DWORD processNameSize = _countof(processName);
		if (QueryFullProcessImageName(hProcess.Handle(), 0, processName, &processNameSize))
			return String(processName);
	}
	return String();
}

String RegistryInfo::GetValueAsString(HKEY hKey, const wchar_t* valueName, LSTATUS* pstatus)
{
	LSTATUS tstatus = 0;
	LSTATUS& status = pstatus ? *pstatus : tstatus;
	
	if (StringInfo::IsNullOrEmpty(valueName))
	{
		status = ERROR_INVALID_PARAMETER;
		return String();
	}

	DWORD valueType = 0;
	uint8_t valueData[1024] = {0};
	DWORD valueSize = sizeof(valueData)-sizeof(wchar_t);
	status = RegQueryValueExW(hKey, valueName, NULL, &valueType, valueData, &valueSize);
	if (status != ERROR_SUCCESS)
		return String();

	if (valueType == REG_SZ || valueType ==  REG_MULTI_SZ || valueType == REG_EXPAND_SZ)
		return String(reinterpret_cast<wchar_t*>(valueData));
	if (valueType == REG_DWORD)
		return StringInfo::Format(TEXT("%u"), *reinterpret_cast<DWORD*>(valueData));

	status = ERROR_UNSUPPORTED_TYPE;
	return String();
}

String RegistryInfo::GetKeyValueAsString(HKEY hKey, const wchar_t* subkeyName, const wchar_t* valueName, LSTATUS* pstatus)
{
	LSTATUS tstatus = 0;
	LSTATUS& status = pstatus ? *pstatus : tstatus;
	
	if (StringInfo::IsNullOrEmpty(subkeyName))
	{
		status = ERROR_INVALID_PARAMETER;
		return String();
	}

	HKEY hSubKey = NULL;
	status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, subkeyName, 0, KEY_READ, &hSubKey);
	if (status != ERROR_SUCCESS)
		return String();

	String value = RegistryInfo::GetValueAsString(hSubKey, valueName, pstatus);
	RegCloseKey(hSubKey);
	return value;
}

}}}
