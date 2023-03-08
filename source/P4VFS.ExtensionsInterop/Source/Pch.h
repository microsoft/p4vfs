// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once

#pragma warning(disable : 4127) // conditional expression is constant
#pragma warning(disable : 4100) // unreferenced formal parameter
#include <SDKDDKVer.h>

#include <tchar.h>
#include <cstdio>
#include <cassert>
#include <memory>
#include <string>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UMDF_USING_NTSTATUS
#include <ntstatus.h>
#include <windows.h>

#include <msclr/marshal.h>
#include <msclr/marshal_windows.h>
