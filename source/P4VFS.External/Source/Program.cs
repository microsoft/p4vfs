// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Linq;
using System.IO;
using System.Text.RegularExpressions;
using System.Reflection;
using Microsoft.P4VFS.Extensions;

namespace Microsoft.P4VFS.External
{
	public class Program
	{
		public static int Main(string[] args)
		{
			Trace.Listeners.Add(new NamedConsoleTraceListener("P4VFS.External"));

			ModuleProperties properties = new ModuleProperties();
			for (int argIndex = 0; argIndex < args.Length; ++argIndex)
			{
				Match m = Regex.Match(args[argIndex], @"^[/-]p:(?<name>[^\s=]+)(\s*=\s*(?<value>.*))?", RegexOptions.IgnoreCase);
				if (m.Success)
				{
					properties.Set(m.Groups["name"].Value, m.Groups["value"].Value);
					continue;
				}

				Trace.TraceError("Unsupported parameter: {0}", args[argIndex]);
				return 1;
			}

			if (properties.GetBool(ReservedProperty.DebuggerLaunch, false))
			{
				Debugger.Launch();
			}
			if (properties.IsSet(ReservedProperty.NugetPath) == false)
			{
				properties.Set(ReservedProperty.NugetPath, Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), "nuget.exe"));
			}
			if (properties.IsSet(ReservedProperty.RootDir) == false)
			{
				properties.Set(ReservedProperty.RootDir, Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location));
			}
			if (properties.IsSet(ReservedProperty.ExternalDir) == false)
			{
				properties.Set(ReservedProperty.ExternalDir, Path.GetFullPath(Path.Combine(properties.Get(ReservedProperty.RootDir), "external")));
			}
			if (properties.IsSet(ReservedProperty.PackagesDir) == false)
			{
				properties.Set(ReservedProperty.PackagesDir, Path.GetFullPath(Path.Combine(properties.Get(ReservedProperty.RootDir), "packages")));
			}
			if (properties.IsSet(ReservedProperty.VisualStudioEdition) == false)
			{
				properties.Set(ReservedProperty.VisualStudioEdition, "2022");
			}
			if (properties.IsSet(ReservedProperty.VisualStudioInstallFolder) == false)
			{
				properties.Set(ReservedProperty.VisualStudioInstallFolder, ModuleInfo.GetVisualStudioInstallFolder(properties.Get(ReservedProperty.VisualStudioEdition)));
			}
			if (properties.IsSet(ReservedProperty.MsBuildPath) == false)
			{
				properties.Set(ReservedProperty.MsBuildPath, Path.GetFullPath(Path.Combine(properties.Get(ReservedProperty.VisualStudioInstallFolder), "MSBuild\\Current\\Bin\\msbuild.exe")));
			}

			ModuleContext context = new ModuleContext();
			context.Properties = properties;
			ModuleInfo.TraceContext(context);

			Type[] moduleTypes = Assembly.GetExecutingAssembly()
				.GetTypes().Where(t => t.IsAbstract == false && typeof(Module).IsAssignableFrom(t))
				.OrderBy(t => t.Name)
				.ToArray();

			foreach (Type moduleType in moduleTypes)
			{
				try
				{
					Module module = Activator.CreateInstance(moduleType) as Module;
					module.Context = context;
					module.Context.Properties.Set(ReservedProperty.ModuleDir, Path.Combine(module.Context.Properties.Get(ReservedProperty.ExternalDir), module.Name));
					if (ModuleInfo.IsModuleRestored(module.Context))
					{
						Trace.TraceInformation("Module already restored {0} [{1}]", module.Description, moduleType.FullName);
						continue;
					}
					if (properties.IsSet(ReservedProperty.ModulePattern) && Regex.IsMatch(module.Name, properties.Get(ReservedProperty.ModulePattern), RegexOptions.IgnoreCase) == false)
					{
						Trace.TraceInformation("Module restore skipped {0} [{1}]", module.Description, moduleType.FullName);
						continue;
					}

					Trace.TraceInformation("Restoring {0} [{1}]", module.Description, moduleType.FullName);
					module.Restore();
					ModuleInfo.SetModuleRestored(module.Context);
					Trace.TraceInformation("Module restored {0} [{1}]", module.Description, moduleType.FullName);
				}
				catch (WarningException e)
				{
					Trace.TraceWarning("Unable to restore [{0}] at this time. Not all configurations may be supported. {1}", moduleType.FullName, e.Message);
					return 0;
				}
				catch (Exception e)
				{
					Trace.TraceError("Failed to restore [{0}]. Build will likely fail. {1}", moduleType.FullName, e.Message);
					return 1;
				}
			}
			return 0;
		}
	}
}
