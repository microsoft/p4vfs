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

// begin_wpp config

// FUNC P4vfsTraceError{ LEVEL=TRACE_LEVEL_ERROR }(FLAGS, MSG, ...);
// USEPREFIX(P4vfsTraceError, "%!STDPREFIX! [%!FILE! @ %!LINE!] ERROR:%!SPACE!");

// FUNC P4vfsTraceWarning{ LEVEL=TRACE_LEVEL_WARNING }(FLAGS, MSG, ...);
// USEPREFIX(P4vfsTraceWarning, "%!STDPREFIX! [%!FILE! @ %!LINE!] WARNING:%!SPACE!");

// FUNC P4vfsTraceInfo{ LEVEL=TRACE_LEVEL_INFORMATION }(FLAGS, MSG, ...);
// USEPREFIX(P4vfsTraceInfo, "%!STDPREFIX! [%!FILE! @ %!LINE!] INFO:%!SPACE!");

// end_wpp
