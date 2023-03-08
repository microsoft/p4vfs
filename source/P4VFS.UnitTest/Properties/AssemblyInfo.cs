using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

// General Information about an assembly is controlled through the following 
// set of attributes. Change these attribute values to modify the information
// associated with an assembly.
[assembly: AssemblyTitle("Microsoft.P4VFS.UnitTest")]
[assembly: AssemblyDescription("")]
[assembly: AssemblyCompany("")]
[assembly: AssemblyProduct("Microsoft.P4VFS.UnitTest")]
[assembly: AssemblyCopyright("Copyright Microsoft Corporation")]
[assembly: AssemblyTrademark("")]
[assembly: AssemblyCulture("")]

#if VFS_UNITTEST_RELEASE
[assembly: AssemblyConfiguration("Release")]
#elif VFS_UNITTEST_RELEASEDEV
[assembly: AssemblyConfiguration("ReleaseDev")]
#elif VFS_UNITTEST_RELEASESIGN
[assembly: AssemblyConfiguration("ReleaseSign")]
#elif VFS_UNITTEST_DEBUG
[assembly: AssemblyConfiguration("Debug")]
#elif VFS_UNITTEST_DEBUGDEV
[assembly: AssemblyConfiguration("DebugDev")]
#else
#error VFS_UNITTEST assembly configuration not defined
#endif

// Setting ComVisible to false makes the types in this assembly not visible 
// to COM components.  If you need to access a type in this assembly from 
// COM, set the ComVisible attribute to true on that type.
[assembly: ComVisible(false)]

// The following GUID is for the ID of the typelib if this project is exposed to COM
[assembly: Guid("ea30f3f4-1a09-436b-9151-394094668fdc")]

// Version information for an assembly consists of the following four values:
//
//      Major Version
//      Minor Version 
//      Build Number
//      Revision
//
// You can specify all the values or you can default the Build and Revision Numbers 
// by using the '*' as shown below:
// [assembly: AssemblyVersion("1.0.*")]
[assembly: AssemblyVersion(Microsoft.P4VFS.CoreInterop.NativeConstants.Version)]
[assembly: AssemblyFileVersion(Microsoft.P4VFS.CoreInterop.NativeConstants.Version)]
