// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "DriverVersion.h"
#include <CodeAnalysis\SourceAnnotations.h>

using namespace System;
using namespace System::Reflection;
using namespace System::Runtime::CompilerServices;
using namespace System::Runtime::InteropServices;
using namespace System::Security::Permissions;

//
// General Information about an assembly is controlled through the following
// set of attributes. Change these attribute values to modify the information
// associated with an assembly.
//
[assembly:AssemblyTitleAttribute("P4VFS.ExtensionsInterop")];
[assembly:AssemblyDescriptionAttribute("")];
[assembly:AssemblyConfigurationAttribute("")];
[assembly:AssemblyCompanyAttribute("")];
[assembly:AssemblyProductAttribute("P4VFS.ExtensionsInterop")];
[assembly:AssemblyCopyrightAttribute("Copyright Microsoft")];
[assembly:AssemblyTrademarkAttribute("")];
[assembly:AssemblyCultureAttribute("")];

//
// Version information for an assembly consists of the following four values:
//
//      Major Version
//      Minor Version
//      Build Number
//      Revision
//
// You can specify all the value or you can default the Revision and Build Numbers
// by using the '*' as shown below:

[assembly:AssemblyVersionAttribute(P4VFS_VER_VERSION_STRING)];
[assembly:AssemblyFileVersionAttribute(P4VFS_VER_VERSION_STRING)];
[assembly:ComVisible(false)];
[assembly:CLSCompliantAttribute(true)];

//
// CA_GLOBAL_SUPPRESS_MESSAGE macros that are applied to this project.
//
CA_GLOBAL_SUPPRESS_MESSAGE("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="Microsoft.P4VFS.ExtensionsProtocol.ManagedServiceHost.#IsDriverConnected()");
CA_GLOBAL_SUPPRESS_MESSAGE("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="Microsoft.P4VFS.ExtensionsProtocol.ManagedServiceHost.#GetLastRequestTime()");
CA_GLOBAL_SUPPRESS_MESSAGE("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="Microsoft.P4VFS.ExtensionsProtocol.ManagedServiceHost.#NotifyRequestTime()");
CA_GLOBAL_SUPPRESS_MESSAGE("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="Microsoft.P4VFS.ExtensionsProtocol.ManagedServiceHost.#GetLastModifiedTime()");
CA_GLOBAL_SUPPRESS_MESSAGE("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="Microsoft.P4VFS.ExtensionsProtocol.ManagedServiceHost.#NotifyModifiedTime()");
CA_GLOBAL_SUPPRESS_MESSAGE("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="Microsoft.P4VFS.ExtensionsProtocol.ManagedServiceHost.#GarbageCollect(System.Int64)");
