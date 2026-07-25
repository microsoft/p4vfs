// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once

#define P4VFS_WPP_CONTROL_GUID \
			082E7434-2FF0-4DE7-8470-1BBBD2E48237

#define WPP_CONTROL_GUIDS																\
			WPP_DEFINE_CONTROL_GUID(													\
				P4VFS_DRIVER_TRACE_GUID, (082E7434,	2FF0, 4DE7,	8470, 1BBBD2E48237),	\
				WPP_DEFINE_BIT(Init)													\
				WPP_DEFINE_BIT(Shutdown)												\
				WPP_DEFINE_BIT(Filter)													\
				WPP_DEFINE_BIT(Core)													\
				)

#define WPP_LEVEL_FLAGS_LOGGER(lvl, flags) \
			WPP_LEVEL_LOGGER(flags)

#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) \
			(WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)

#define WPP_LOGFILEIDPATH(x) \
    WPP_LOGPAIR(sizeof(USHORT), &(x)->Length) \
    WPP_LOGPAIR((x)->Length, (x)->Buffer)

#define WPP_LOGHEXDUMP(x) \
    WPP_LOGPAIR(sizeof(USHORT), &(x).Length) \
    WPP_LOGPAIR((x).Length, (x).Buffer)

typedef struct _WPP_HEXDUMP
{
    USHORT Length;
    __field_bcount(Length) CONST VOID* Buffer;
} WPP_HEXDUMP, *PWPP_HEXDUMP;

__inline
WPP_HEXDUMP
P4vfsCreateWppHexDump(
    __in ULONG length,
    __in_bcount(length) CONST VOID* buffer
    )
{
    WPP_HEXDUMP WppHexDump;
    if (length > USHORT_MAX) 
	{
        length = USHORT_MAX;
    }
    WppHexDump.Length = (USHORT)length;
    WppHexDump.Buffer = buffer;
    return WppHexDump;
}

#define LOG_HEXDUMP(length, buffer) \
	P4vfsCreateWppHexDump(length, buffer)


// begin_wpp config

// FUNC P4vfsTraceError{ LEVEL=TRACE_LEVEL_ERROR }(FLAGS, MSG, ...);
// USEPREFIX(P4vfsTraceError, "%!STDPREFIX! [%!FILE! @ %!LINE!] ERROR:%!SPACE!");

// FUNC P4vfsTraceWarning{ LEVEL=TRACE_LEVEL_WARNING }(FLAGS, MSG, ...);
// USEPREFIX(P4vfsTraceWarning, "%!STDPREFIX! [%!FILE! @ %!LINE!] WARNING:%!SPACE!");

// FUNC P4vfsTraceInfo{ LEVEL=TRACE_LEVEL_INFORMATION }(FLAGS, MSG, ...);
// USEPREFIX(P4vfsTraceInfo, "%!STDPREFIX! [%!FILE! @ %!LINE!] INFO:%!SPACE!");

// DEFINE_CPLX_TYPE(HEXDUMP, WPP_LOGHEXDUMP, WPP_HEXDUMP, ItemHEXDump, "s", _HEX_, 0, 2);
// DEFINE_CPLX_TYPE(FILEIDPATH, WPP_LOGFILEIDPATH, PUNICODE_STRING, ItemHexBytes, "s", _HEX_, 0, 2);

// end_wpp
