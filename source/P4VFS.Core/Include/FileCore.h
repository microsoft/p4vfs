// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#pragma managed(push, off)
#include <string>
#include <vector>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <initializer_list>
#include <array>
#include <memory>
#include <istream>
#include <functional>

#if defined(P4VFS_CORE_EXPORTS)
#define P4VFS_CORE_API __declspec(dllexport)
#else
#define P4VFS_CORE_API __declspec(dllimport)
#endif

namespace Microsoft {
namespace P4VFS {
namespace FileCore {

	enum EForceNoInit
	{ 
		ForceNoInit
	};

	enum EForceInit
	{ 
		ForceInit
	};

	enum 
	{ 
		DefaultAlignment = 8 
	};

	P4VFS_CORE_API void* GAlloc(size_t size, size_t align = DefaultAlignment);
	P4VFS_CORE_API void* GRealloc(void* ptr, size_t size, size_t align = DefaultAlignment);
	P4VFS_CORE_API void GFree(void* ptr);

	struct GFreeDeleter
	{
		void operator()(const void* ptr) const { GFree((void*)ptr); }
	};

	template <typename T>
	using GAllocPtr = std::unique_ptr<T, GFreeDeleter>;

	template <typename ValueType>
	using List = std::list<ValueType>;

	template <typename KeyType, typename ValueType, typename LessType = std::less<KeyType>>
	using Map = std::map<KeyType, ValueType, LessType>;

	template <typename KeyType, typename ValueType, typename HashType = std::hash<KeyType>, typename EqualType = std::equal_to<KeyType>>
	using UnorderedMultiMap = std::unordered_multimap<KeyType, ValueType, HashType, EqualType>;

	template <typename KeyType, typename ValueType, typename HashType = std::hash<KeyType>, typename EqualType = std::equal_to<KeyType>>
	using HashMap = std::unordered_map<KeyType, ValueType, HashType, EqualType>;

	template <typename KeyType, typename LessType = std::less<KeyType>>
	using Set = std::set<KeyType, LessType>;

	template <typename KeyType, typename EqualType = std::equal_to<KeyType>>
	using HashSet = std::unordered_set<KeyType, std::hash<KeyType>, EqualType>;

	template <typename ValueType>
	using Array = std::vector<ValueType>;

	template <typename T, size_t N> 
	using StaticArrayBase = std::array<T,N>;

	template <typename T, size_t N> 
	struct StaticArray : StaticArrayBase<T,N>
	{
	};

	template <typename T> 
	struct StaticArray<T,2> : StaticArrayBase<T,2> 
	{
		StaticArray() {}
		StaticArray(const T& t0, const T& t1) { (*this)[0] = t0; (*this)[1] = t1; }
	};

	template <typename T> 
	struct StaticArray<T,3> : StaticArrayBase<T,3> 
	{
		StaticArray() {}
		StaticArray(const T& t0, const T& t1, const T& t2) { (*this)[0] = t0; (*this)[1] = t1; (*this)[2] = t2; }
	};

	template <typename T> 
	struct StaticArray<T,4> : StaticArrayBase<T,4> 
	{
		StaticArray() {}
		StaticArray(const T& t0, const T& t1, const T& t2, const T& t3) { (*this)[0] = t0; (*this)[1] = t1; (*this)[2] = t2; (*this)[3] = t3; }
	};

	// Aligns a value to the nearest higher multiple of 'Alignment', which must be a power of two.
	template <class T> 
	inline T Align(const T Ptr, int32_t Alignment)
	{
		return (T)(((size_t)Ptr + Alignment - 1) & ~(Alignment-1));
	}

	// Aligns a value to the nearest higher multiple of 'Alignment'.
	template <class T> 
	inline T AlignArbitrary(const T Ptr, uint32_t Alignment)
	{
		return (T)((((size_t)Ptr + Alignment - 1) / Alignment) * Alignment);
	}

	// P4VFS string encoding
	//
	// AString 
	//     Otherwise referred to as "Ansi" string, but is not strictly ANSI. We use the term to refer to the 
	//     CP-1252 single-byte encoding. This should not be confused as a UTF-8 multi-byte encoding. Unless
	//     otherwise specified, all char* strings in P4VFS are assumed to be AString.
	//
	// WString
	//     Otherwise referred to as "Wide" string. This is UTF-16 multi-byte encoding. Windows operates natively 
	//     in UTF-16 (or WCHAR) and majority of P4VFS api's require this encoding. Unless otherwise specified, all
	//     wchar_t* strings in P4VFS are assumed to be WString.

	typedef std::string AString;
	typedef std::wstring WString;
	typedef WString String;

	typedef Array<String> StringArray;
	typedef Array<WString> WStringArray;
	typedef Array<AString> AStringArray;

	struct P4VFS_CORE_API StringInfo
	{
		struct SplitFlags { enum Enum
		{ 
			None				= 0,
			RemoveEmptyEntries	= 1<<0,
		};};

		struct EscapeDirection { enum Enum 
		{ 
			Encode, 
			Decode 
		};};

		struct SearchCase { enum Enum 
		{ 
			Sensitive, 
			Insensitive 
		};};

		struct SearchDirection { enum Enum 
		{ 
			FromStart, 
			FromEnd 
		};};

		static const WString&	EmptyW();
		static const AString&	EmptyA();
		static bool				IsNullOrEmpty(const wchar_t* s);
		static bool				IsNullOrEmpty(const char* s);
		static bool				IsNullOrWhiteSpace(const wchar_t* s);
		static bool				IsNullOrWhiteSpace(const char* s);
		static WString			Format(const wchar_t* fmt, ...);
		static AString			Format(const char* fmt, ...);
		static WString			Formatv(const wchar_t* fmt, va_list va);
		static AString			Formatv(const char* fmt, va_list va);
		static void				Sprintf(WString& s, const wchar_t* fmt, ...);
		static void				Sprintf(AString& s, const char* fmt, ...);
		static void				Sprintfv(WString& s, const wchar_t* fmt, va_list va);
		static void				Sprintfv(AString& s, const char* fmt, va_list va);
		static bool				IsDigit(const wchar_t c);
		static bool				IsDigit(const char c);
		static bool				IsAlpha(const wchar_t c);
		static bool				IsAlpha(const char c);
		static bool				IsAlphaNum(const wchar_t c);
		static bool				IsAlphaNum(const char c);
		static bool				IsWhite(const wchar_t c);
		static bool				IsWhite(const char c);
		static wchar_t			ToLower(const wchar_t c);
		static char				ToLower(const char c);
		static WString			ToLower(const wchar_t* s);
		static AString			ToLower(const char* s);
		static wchar_t			ToUpper(const wchar_t c);
		static char				ToUpper(const char c);
		static WString			ToUpper(const wchar_t* s);
		static AString			ToUpper(const char* s);
		static WString			Capitalize(const wchar_t* s);
		static AString			Capitalize(const char* s);
		static WString			Trim(const wchar_t* s, const wchar_t* trimChrs = nullptr);
		static AString			Trim(const char* s, const char* trimChrs = nullptr);
		static WString			TrimRight(const wchar_t* s, const wchar_t* trimChrs = nullptr);
		static AString			TrimRight(const char* s, const char* trimChrs = nullptr);
		static WString			TrimLeft(const wchar_t* s, const wchar_t* trimChrs = nullptr);
		static AString			TrimLeft(const char* s, const char* trimChrs = nullptr);
		static bool				StartsWith(const wchar_t* searchIn, const wchar_t* searchFor, SearchCase::Enum cmp = SearchCase::Sensitive);
		static bool				StartsWith(const char* searchIn, const char* searchFor, SearchCase::Enum cmp = SearchCase::Sensitive);
		static bool				StartsWith(const wchar_t* searchIn, const wchar_t searchFor, SearchCase::Enum cmp = SearchCase::Sensitive);
		static bool				StartsWith(const char* searchIn, const char searchFor, SearchCase::Enum cmp = SearchCase::Sensitive);
		static bool				EndsWith(const wchar_t* searchIn, const wchar_t* searchFor, SearchCase::Enum cmp = SearchCase::Sensitive);
		static bool				EndsWith(const char* searchIn, const char* searchFor, SearchCase::Enum cmp = SearchCase::Sensitive);
		static bool				EndsWith(const wchar_t* searchIn, const wchar_t searchFor, SearchCase::Enum cmp = SearchCase::Sensitive);
		static bool				EndsWith(const char* searchIn, const char searchFor, SearchCase::Enum cmp = SearchCase::Sensitive);
		static WString			Replace(const wchar_t* s, const wchar_t* find, const wchar_t* replace, SearchCase::Enum cmp = SearchCase::Sensitive);
		static AString			Replace(const char* s, const char* find, const char* replace, SearchCase::Enum cmp = SearchCase::Sensitive);
		static WString			Replace(const wchar_t* s, const wchar_t find, const wchar_t replace, SearchCase::Enum cmp = SearchCase::Sensitive);
		static AString			Replace(const char* s, const char find, const char replace, SearchCase::Enum cmp = SearchCase::Sensitive);
		static bool				Contains(const wchar_t* s, const wchar_t* searchFor, SearchCase::Enum cmp = SearchCase::Sensitive);
		static bool				Contains(const char* s, const char* searchFor, SearchCase::Enum cmp = SearchCase::Sensitive);
		static bool				Contains(const wchar_t* s, const wchar_t searchFor, SearchCase::Enum cmp = SearchCase::Sensitive);
		static bool				Contains(const char* s, const char searchFor, SearchCase::Enum cmp = SearchCase::Sensitive);
		static bool				ContainsToken(const char delim, const char* searchIn, const char* token, SearchCase::Enum cmp = SearchCase::Sensitive);
		static bool				ContainsToken(const wchar_t delim, const wchar_t* searchIn, const wchar_t* token, SearchCase::Enum cmp = SearchCase::Sensitive);
		static const wchar_t*	CStr(const WString& s) { return s.c_str(); }
		static const char*		CStr(const AString& s) { return s.c_str(); }
		static const wchar_t*	CStr(const wchar_t* s) { return s; }
		static const char*		CStr(const char* s) { return s; }
		static const wchar_t*	WCStr(const WString& s) { return s.c_str(); }
		static const char*		ACStr(const AString& s) { return s.c_str(); }
		static const wchar_t*	WCStr(const wchar_t* s) { return s; }
		static const char*		ACStr(const char* s) { return s; }
		static WString			ReadLine(const wchar_t*& s);
		static WString			Escape(const wchar_t* s, EscapeDirection::Enum direction = EscapeDirection::Encode);
		static WStringArray		Split(const wchar_t* text, const wchar_t* delims, SplitFlags::Enum flags);
		static AStringArray		Split(const char* text, const char* delims, SplitFlags::Enum flags);
		static WString			Join(const WStringArray& tokens, const wchar_t* delim, size_t start = 0, size_t count = -1);
		static AString			Join(const AStringArray& tokens, const char* delim, size_t start = 0, size_t count = -1);
		static int32_t			Strcmp(const wchar_t* a, const wchar_t* b);
		static int32_t			Strcmp(const char* a, const char* b);
		static int32_t			Stricmp(const wchar_t* a, const wchar_t* b);
		static int32_t			Stricmp(const char* a, const char* b);
		static int32_t			Strncmp(const wchar_t* a, const wchar_t* b, size_t count);
		static int32_t			Strncmp(const char* a, const char* b, size_t count);
		static int32_t			Strnicmp(const wchar_t* a, const wchar_t* b, size_t count);
		static int32_t			Strnicmp(const char* a, const char* b, size_t count);
		static wchar_t*			Strncpy(wchar_t* dst, const wchar_t* src, size_t count);
		static char*			Strncpy(char* dst, const char* src, size_t count);
		static const wchar_t*	Strchr(const wchar_t* str, wchar_t c);
		static const char*		Strchr(const char* str, char c);
		static const wchar_t*	Strstr(const wchar_t* str, const wchar_t* strSearch);
		static const char*		Strstr(const char* str, const char* strSearch);
		static const wchar_t*	Strichr(const wchar_t* str, wchar_t c);
		static const char*		Strichr(const char* str, char c);
		static const wchar_t*	Stristr(const wchar_t* str, const wchar_t* strSearch);
		static const char*		Stristr(const char* str, const char* strSearch);
		static const wchar_t*	Strpbrk(const wchar_t* str, const wchar_t* strCharSet);
		static const char*		Strpbrk(const char* str, const char* strCharSet);
		static size_t			Strlen(const wchar_t* str);
		static size_t			Strlen(const char* str);
		static WString			ToString(float value);
		static WString			ToString(int32_t value);
		static WString			ToString(int64_t value);
		static WString			ToString(uint64_t value);
		static WString			ToString(HRESULT value);
		static AString			ToString(const char* str);
		static WString			ToString(const wchar_t* str);
		static uint64_t			HashMd5(const void* data, size_t dataSize);
		static uint64_t			HashMd5(std::istream& stream);
		static uint64_t			HashMd5(const WString& s);
		static uint64_t			HashMd5(const AString& s);
		static AString			FormatTime(const struct tm* t, const char* fmt);
		static WString			FormatTime(const struct tm* t, const wchar_t* fmt);
		static AString			FormatLocalTime(time_t t, const char* fmt);
		static WString			FormatLocalTime(time_t t, const wchar_t* fmt);
		static AString			FormatUtcTime(time_t t, const char* fmt);
		static WString			FormatUtcTime(time_t t, const wchar_t* fmt);

		static AString			ToAnsi(const WString& str);
		static AString			ToAnsi(const wchar_t* str);
		static char				ToAnsi(const wchar_t chr);
		static WString			ToWide(const AString& str);
		static WString			ToWide(const char* str);
		static wchar_t			ToWide(const char chr);

		struct Hash
		{
			template <typename T>
			size_t operator()(const T& a) const { return StringInfo::HashMd5(a.c_str(), a.size()); }
		};

		struct Less 
		{
			template <typename TL, typename TR>
			bool operator()(const TL& a, const TR& b) const { return StringInfo::Strcmp(StringInfo::CStr(a), StringInfo::CStr(b)) < 0; }
		};
		
		struct LessInsensitive
		{
			template <typename TL, typename TR>
			bool operator()(const TL& a, const TR& b) const { return StringInfo::Stricmp(StringInfo::CStr(a), StringInfo::CStr(b)) < 0; }
		};

		struct Greater 
		{
			template <typename TL, typename TR>
			bool operator()(const TL& a, const TR& b) const { return StringInfo::Strcmp(StringInfo::CStr(a), StringInfo::CStr(b)) > 0; }
		};
		
		struct GreaterInsensitive
		{
			template <typename TL, typename TR>
			bool operator()(const TL& a, const TR& b) const { return StringInfo::Stricmp(StringInfo::CStr(a), StringInfo::CStr(b)) > 0; }
		};

		struct Equal
		{
			template <typename TL, typename TR>
			bool operator()(const TL& a, const TR& b) const { return StringInfo::Strcmp(StringInfo::CStr(a), StringInfo::CStr(b)) == 0; }
		};
		
		struct EqualInsensitive
		{
			template <typename TL, typename TR>
			bool operator()(const TL& a, const TR& b) const { return StringInfo::Stricmp(StringInfo::CStr(a), StringInfo::CStr(b)) == 0; }
		};

		class WtoA
		{
		public:
			WtoA(const wchar_t* str) : m_S(StringInfo::ToAnsi(str)) {}
			WtoA(const WString& str) : m_S(StringInfo::ToAnsi(str.c_str())) {}
			operator const AString&() const { return m_S; }
			operator const char*() const { return m_S.c_str(); }
			const char* c_str() const { return m_S.c_str(); }

		private:
			AString m_S;
		};

		class AtoW
		{
		public:
			AtoW(const char* str) : m_S(StringInfo::ToWide(str)) {}
			AtoW(const AString& str) : m_S(StringInfo::ToWide(str.c_str())) {}
			operator const WString&() const { return m_S; }
			operator const wchar_t*() const { return m_S.c_str(); }
			const wchar_t* c_str() const { return m_S.c_str(); }

		private:
			WString m_S;
		};

		#define CSTR_WTOA(str)	Microsoft::P4VFS::FileCore::StringInfo::WtoA(str).c_str()
		#define CSTR_ATOW(str)	Microsoft::P4VFS::FileCore::StringInfo::AtoW(str).c_str()

		struct Traits
		{
			template <typename CharType> struct Type
			{
			};

			template <> struct Type<char>
			{ 
				typedef AStringArray TStringArray;
				typedef AString TString;
				typedef char TChar;

				static char* Literal(char* A, wchar_t* W) { return A; }
				static char  Literal(char A, wchar_t W) { return A; }
			};

			template <> struct Type<wchar_t>
			{ 
				typedef WStringArray TStringArray;
				typedef WString TString;
				typedef wchar_t TChar;

				static wchar_t* Literal(char* A, wchar_t* W) { return W; }
				static wchar_t  Literal(char A, wchar_t W) { return W; }
			};
		};

		#define LITERAL(CharType, str) ::Microsoft::P4VFS::FileCore::StringInfo::Traits::Type<CharType>::Literal(str, L##str)
	};

	struct P4VFS_CORE_API FileInfo
	{
		static constexpr wchar_t ExtendedPathPrefix[] = L"\\\\?\\";
		static constexpr size_t ExtendedPathPrefixLength = _countof(ExtendedPathPrefix)-1;

		struct Find { enum Enum
		{
			kNone			= 0,
			kFiles			= (1<<0),
			kDirectories	= (1<<1),
			kRecursive		= (1<<2),
		};};

		static bool			Exists(const wchar_t* filePath);
		static bool			IsRegular(const wchar_t* filePath);
		static bool			IsSymlink(const wchar_t* filePath);
		static bool			IsDirectory(const wchar_t* filePath);
		static bool			IsReadOnly(const wchar_t* filePath);
		static DWORD		FileAttributes(const wchar_t* filePath);
		static bool			SetReadOnly(const wchar_t* filePath, bool readOnly);
		static String		FullPath(const wchar_t* filePath);
		static String		RootPath(const wchar_t* filePath);
		static String		FolderPath(const wchar_t* filePath);
		static String		FullFolderPath(const wchar_t* filePath);
		static String		FileTitle(const wchar_t* fileName);
		static String		FileShortTitle(const wchar_t* filePath);
		static String		FileExtension(const wchar_t* fileName);
		static String		FileName(const wchar_t* filePath);
		static int64_t		FileSize(const wchar_t* filePath);
		static int64_t		FileUncompressedSize(const wchar_t* filePath);
		static int64_t		FileDiskSize(const wchar_t* filePath);
		static uint64_t		FileHashMd5(const wchar_t* filePath);
		static bool			ReadFile(const wchar_t* filePath, Array<uint8_t>& contents);
		static bool			ReadFileLines(const wchar_t* filePath, Array<AString>& lines);
		static bool			ReadFileLines(const wchar_t* filePath, const std::function<bool(const AString&)>& readline);
		static String		SymlinkTarget(const wchar_t* filePath);
		static bool			Delete(const wchar_t* filePath);
		static bool			Move(const wchar_t* srcFilePath, const wchar_t* dstFilePath);
		static int32_t		Compare(const wchar_t* filePath0, const wchar_t* filePath1);
		static bool			CreateWritableFile(const wchar_t* filePath);
		static bool			CreateDirectory(const wchar_t* folderPath);
		static bool			CreateFileDirectory(const wchar_t* filePath);
		static bool			DeleteDirectoryRecursively(const wchar_t* folderPath);
		static bool			DeleteEmptyDirectories(const wchar_t* folderPath, bool deleteEmptyParents = true);
		static bool			IsDirectoryEmpty(const wchar_t* folderPath);
		static void			FindFiles(StringArray& files, const wchar_t* folderPath, const wchar_t* pattern, Find::Enum flags = Find::kFiles);
		static String		ApplicationFilePath();
		static String		CreateTempFile(const wchar_t* folderPath = nullptr, const wchar_t* filePrefix = nullptr);
		static bool			IsExtendedPath(const wchar_t* path);
		static String		ExtendedPath(const wchar_t* path);
		static String		UnextendedPath(const wchar_t* path);
	};

	struct P4VFS_CORE_API TimeInfo
	{
		static time_t		GetTime();
		static FILETIME		GetUtcFileTime();
		static uint64_t		FileTimeToUInt64(const FILETIME& v);
		static FILETIME		UInt64ToFileTime(uint64_t v);
	};

	constexpr FILETIME MinFileTime = FILETIME { 0, 0 };
	constexpr FILETIME MaxFileTime = FILETIME { (DWORD)-1, (DWORD)-1 };

	template <typename T>
	class NonCopyable
	{
	protected:
		NonCopyable() {}
		~NonCopyable() {}
	private:
		NonCopyable(const NonCopyable&) {}
		void operator=(const NonCopyable&) {}
	};

	class P4VFS_CORE_API AutoHandle : NonCopyable<AutoHandle>
	{
	public:
		AutoHandle(HANDLE hHandle = INVALID_HANDLE_VALUE);
		~AutoHandle();

		bool IsValid() const;
		HANDLE Handle() const;
		HANDLE* HandlePtr();
		HRESULT Close();
		void Reset(HANDLE hHandle);

	private:
		HANDLE m_hHandle;
	};

	class P4VFS_CORE_API AutoCrtFile : NonCopyable<AutoCrtFile>
	{
	public:
		AutoCrtFile(FILE* file = nullptr);
		~AutoCrtFile();

		bool IsValid() const;
		void Set(FILE* file);
		FILE* Get();
		void Close();

	private:
		FILE* m_File;
	};

	class AutoTempFile : NonCopyable<AutoCrtFile>
	{
	public:
		AutoTempFile(const wchar_t* filePath);
		~AutoTempFile();

		const String& GetFilePath() const;

	private:
		String m_FilePath;
	};

	class P4VFS_CORE_API AutoServiceHandle : NonCopyable<AutoServiceHandle>
	{
	public:
		AutoServiceHandle(SC_HANDLE hHandle = NULL);
		~AutoServiceHandle();

		bool IsValid() const;
		SC_HANDLE Handle() const;
		SC_HANDLE* HandlePtr();
		HRESULT Close();
		void Reset(SC_HANDLE hHandle);

	private:
		SC_HANDLE m_hHandle;
	};

	class P4VFS_CORE_API AutoMutex : NonCopyable<AutoMutex>
	{
	public:
		AutoMutex(const AutoHandle& mutex);
		AutoMutex(HANDLE mutex);
		~AutoMutex();
		bool IsAquired() const;

	private:
		HANDLE m_Mutex;
	};

	class P4VFS_CORE_API FileStream
	{
	public:
		virtual bool CanWrite();
		virtual bool CanRead();
		virtual HRESULT Write(HANDLE hReadHandle, UINT64* bytesRead);
		virtual HRESULT Read(HANDLE hWriteHandle, UINT64* bytesWritten);
	};

	template <typename T>
	inline void SafeDeletePointer(T*& p)
	{
		delete p;
		p = nullptr;
	}

	inline void SafeCloseHandle(HANDLE& h)
	{
		CloseHandle(h);
		h = NULL;
	}

	class P4VFS_CORE_API CriticalSection : private CRITICAL_SECTION
	{
	public:
		CriticalSection()
		{
			InitializeCriticalSectionEx(this, 10, 0);
		}
		~CriticalSection()
		{
			DeleteCriticalSection(this);
		}
		void Lock()
		{
			EnterCriticalSection(this);
		}
		void Release()
		{
			LeaveCriticalSection(this);
		}
	};
	
	class P4VFS_CORE_API AutoCriticalSection
	{
	public:
		AutoCriticalSection(CriticalSection& section) : m_Section(section)
		{
			m_Section.Lock();
		}
		~AutoCriticalSection()
		{
			m_Section.Release();
		}
	private:
		CriticalSection& m_Section;
	};

	struct Algo
	{
		template <typename ValueType>
		static bool Contains(const Array<ValueType>& elements, const ValueType& v)
		{
			return std::find(elements.begin(), elements.end(), v) != elements.end();
		}

		template <typename KeyType, typename EqualType>
		static bool Contains(const HashSet<KeyType, EqualType>& elements, const KeyType& v)
		{
			return elements.find(v) != elements.end();
		}

		template <typename ArrayType, typename Predicate>
		static bool Any(const ArrayType& elements, Predicate predicate)
		{
			return std::any_of(elements.begin(), elements.end(), predicate);
		}

		template <typename ArrayType, typename Predicate>
		static void RemoveIf(ArrayType& elements, Predicate predicate)
		{
			for (ArrayType::iterator i = elements.begin(); i != elements.end();)
			{
				if (predicate(*i))
					i = elements.erase(i);
				else
					++i;
			}
		}

		template <typename ArrayType, typename ValueType>
		static void Remove(ArrayType& elements, const ValueType& v)
		{
			return RemoveIf(elements, [&v](const auto& i) -> bool { return i == v; });
		}

		template <typename DstArrayType, typename SrcArrayType>
		static void Append(DstArrayType& dst, const SrcArrayType& src)
		{
			dst.reserve(dst.size()+src.size());
			for (size_t i = 0, iend = src.size(); i < iend; ++i)
				dst.push_back(src[i]);
		}

		template <typename DstArrayType, typename SrcElemType>
		static void Append(DstArrayType& dst, const SrcElemType* src, size_t srcElemCount)
		{
			dst.reserve(dst.size()+srcElemCount);
			for (size_t i = 0; i < srcElemCount; ++i)
				dst.push_back(src[i]);
		}

		template <typename DstArrayType, typename SrcElemType>
		static void Append(DstArrayType& dst, const std::initializer_list<SrcElemType>& src)
		{
			dst.reserve(dst.size()+src.size());
			for (size_t i = 0, iend = src.size(); i < iend; ++i)
				dst.push_back(src.begin()[i]);
		}

		template <typename DstArrayType, typename SrcArrayType, typename Predicate>
		static void AppendIf(DstArrayType& dst, const SrcArrayType& src, Predicate predicate)
		{
			dst.reserve(dst.size()+src.size());
			for (size_t i = 0, iend = src.size(); i < iend; ++i)
			{
				if (predicate(src[i]))
					dst.push_back(src[i]);
			}
		}

		template <typename ArrayType>
		static void ClearDelete(ArrayType& elements)
		{
			for (ArrayType::iterator i = elements.begin(); i != elements.end(); ++i)
				delete *i;
			elements.clear();
		}

		template <typename MapType>
		static typename MapType::mapped_type* Find(MapType& srcMap, const typename MapType::key_type& srcKey)
		{
			MapType::iterator i = srcMap.find(srcKey);
			return i == srcMap.end() ? nullptr : &i->second;
		}

		template <typename MapType>
		static const typename MapType::mapped_type* Find(const MapType& srcMap, const typename MapType::key_type& srcKey)
		{
			MapType::const_iterator i = srcMap.find(srcKey);
			return i == srcMap.end() ? nullptr : &i->second;
		}

		template <typename ValueType, typename ArrayType, typename Predicate>
		static ValueType Sum(const ArrayType& src, Predicate predicate)
		{
			ValueType s = 0;
			for (ArrayType::const_iterator i = src.begin(); i != src.end(); ++i)
				s += predicate(*i);
			return s;
		}
	};

	struct P4VFS_CORE_API Process
	{
		struct ExecuteFlags { enum Enum
		{
			None		= 0,
			WaitForExit	= (1<<0),
			HideWindow	= (1<<1),
			StdOut		= (1<<2),
			KeepOpen	= (1<<3),
			Unelevated	= (1<<4),
			Default		= WaitForExit,
		};};

		struct ExecuteResult
		{
			HRESULT m_HR;
			AString m_StdOut;
			DWORD m_ExitCode;
			DWORD m_ProcessId;
			HANDLE m_hProcess;
			HANDLE m_hThread;
		};

		static ExecuteResult	Execute(const wchar_t* cmd, const wchar_t* dir, ExecuteFlags::Enum flags = ExecuteFlags::Default, HANDLE hUserToken = NULL);
		static String			GetProcessNameById(DWORD processID);
	};

	struct RegistryValue
	{
		DWORD m_Type = REG_NONE;
		Array<BYTE> m_Data;

		P4VFS_CORE_API bool						IsValid() const;
		P4VFS_CORE_API String					ToString(LSTATUS* pstatus = nullptr) const;
		P4VFS_CORE_API StringArray				ToStringArray(LSTATUS* pstatus = nullptr) const;
		P4VFS_CORE_API static RegistryValue		FromString(const String& value);
		P4VFS_CORE_API static RegistryValue		FromStringArray(const StringArray& value);
	};

	struct P4VFS_CORE_API RegistryInfo
	{
		static RegistryValue	GetValue(HKEY hKey, const wchar_t* valueName, LSTATUS* pstatus = nullptr);
		static RegistryValue	GetKeyValue(HKEY hKey, const wchar_t* subkeyName, const wchar_t* valueName, LSTATUS* pstatus = nullptr);
		static LSTATUS			SetValue(HKEY hKey, const wchar_t* valueName, const RegistryValue& value);
		static LSTATUS			SetKeyValue(HKEY hKey, const wchar_t* subkeyName, const wchar_t* valueName, const RegistryValue& value);
	};

	#define P4VFS_ENUM_TO_STRING_APPEND_FLAG(s, v, ns, ev) \
		do { if (((v) == (ns::ev)) || ((ns::ev) != 0 && ((v) & (ns::ev)))) \
		{ \
			if (s.empty() == false) \
				s += "|"; \
			s += #ev; \
		} } while(0)

	#define P4VFS_ENUM_TO_STRING_RETURN(v, ns, ev) \
		do { if ((v) == (ns::ev)) \
		{ \
			return #ev; \
		} } while (0)

	#define P4VFS_STRING_TO_ENUM_RETURN(v, ns, ev) \
		do { if (Microsoft::P4VFS::FileCore::StringInfo::Stricmp(#ev, Microsoft::P4VFS::FileCore::StringInfo::CStr(v)) == 0) \
		{ \
			return ns::ev; \
		} } while (0)

	DEFINE_ENUM_FLAG_OPERATORS(Process::ExecuteFlags::Enum);
	DEFINE_ENUM_FLAG_OPERATORS(StringInfo::SplitFlags::Enum);
	DEFINE_ENUM_FLAG_OPERATORS(FileInfo::Find::Enum);
}}}

#pragma managed(pop)

