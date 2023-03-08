// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once

#pragma warning(disable : 4100) // unreferenced formal parameter
#pragma warning(disable : 4127) // conditional expression is constant
#pragma warning(disable : 6102) // using variable from failed function call
#pragma warning(disable: 26812) // Prefer 'enum class' over 'enum'

#include <SDKDDKVer.h>

#include <tchar.h>
#include <cstdio>
#include <cassert>
#include <memory>
#include <string>
#include <functional>

#define _ENABLE_ATOMIC_ALIGNMENT_FIX
#include <atomic>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UMDF_USING_NTSTATUS
#include <ntstatus.h>
#include <windows.h>
#include <wtsapi32.h>
#include <fltuser.h>

#include "ExtensionsInterop.h"

