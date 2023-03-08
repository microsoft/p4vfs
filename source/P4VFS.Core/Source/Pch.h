// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once

#pragma warning(disable: 4127)	// conditional expression is constant
#pragma warning(disable: 4100)	// unreferenced formal parameter
#pragma warning(disable: 4239)	// Nonstandard extension used : 'token' : conversion from 'type' to 'type'
#pragma warning(disable: 4458)	// Declaration of 'identifier' hides class member
#pragma warning(disable: 6031)  // Return value ignored
#pragma warning(disable: 26444) // Avoid unnamed objects with custom construction and destruction
#pragma warning(disable: 26451) // Arithmetic overflow: Using operator '%operator%' on a %size1% byte value and then casting the result to a %size2% byte value
#pragma warning(disable: 26495)	// Variable '%variable%' is uninitialized. Always initialize a member variable
#pragma warning(disable: 26812) // Prefer 'enum class' over 'enum'

#include <SDKDDKVer.h>

#include <tchar.h>
#include <cstdio>
#include <cassert>
#include <memory>
#include <string>
#include <algorithm>
#include <functional>
#include <cvt/wstring>
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
#include <ppl.h>
