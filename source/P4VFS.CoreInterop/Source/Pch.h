// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once

#pragma warning(disable : 4127) // conditional expression is constant
#pragma warning(disable : 4100) // unreferenced formal parameter
#pragma warning(disable : 4239) // nonstandard extension used : 'token' : conversion from 'type' to 'type'
#include <SDKDDKVer.h>

#include <tchar.h>
#include <cstdio>
#include <cassert>
#include <memory>
#include <string>
#include <algorithm>
#include <functional>
#include <codecvt>
#include <regex>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <winioctl.h>
#include <wtsapi32.h>
#include <shlobj.h>
#include <fltuser.h>
#include <userenv.h>

#include <msclr/marshal.h>
#include <msclr/marshal_windows.h>
