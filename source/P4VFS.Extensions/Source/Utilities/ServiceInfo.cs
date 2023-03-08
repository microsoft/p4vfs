// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Diagnostics;
using System.Linq;
using System.Management;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text.RegularExpressions;

namespace Microsoft.P4VFS.Extensions.Utilities
{
	/// <summary>
	/// Encapsulates the WMI service object and its operations.
	/// </summary>
    [DebuggerDisplay("{Name} {State} {StartName}")]
	public class ServiceInfo
	{
		public bool AcceptPause { get; private set; }
		public bool AcceptStop { get; private set; }
		public string Caption { get; private set; }
		public string CheckPoint { get; private set; }
		public string CreationClassName { get; private set; }
		public string Description { get; private set; }
		public bool DesktopInteract { get; private set; }
		public string DisplayName { get; private set; }
		public string ErrorControl { get; private set; }
		public long ExitCode { get; private set; }
		public DateTime InstallDate { get; private set; }
		public string Name { get; private set; }
		public string PathName { get; private set; }
		public long ProcessId { get; private set; }
		public long ServiceSpecificExitCode { get; private set; }
		public string ServiceType { get; private set; }
		public bool Started { get; private set; }
		public string StartMode { get; private set; }
		public string StartName { get; private set; }
		public string State { get; private set; }
		public string Status { get; private set; }
		public string SystemCreationClassName { get; private set; }
		public string SystemName { get; private set; }
		public long TagId { get; private set; }
		public string WaitHint { get; private set; }

		public static UInt32 SERVICE_STOPPED			= 0x00000001;
		public static UInt32 SERVICE_START_PENDING		= 0x00000002;
		public static UInt32 SERVICE_STOP_PENDING		= 0x00000003;
		public static UInt32 SERVICE_RUNNING			= 0x00000004;
		public static UInt32 SERVICE_CONTINUE_PENDING	= 0x00000005;
		public static UInt32 SERVICE_PAUSE_PENDING		= 0x00000006;
		public static UInt32 SERVICE_PAUSED				= 0x00000007;

		/// <summary>
		/// Gets a value indicating whether a given service is running.
		/// </summary>
		/// <param name="serviceName">The service to test.</param>
		/// <returns>True if running, false otherwise.</returns>
        public static bool IsServiceRunning(string serviceName)
        {
            return IsServiceRunning(Environment.MachineName, serviceName);
        }

		/// <summary>
		/// Gets a value indicating whether a given service is running.
		/// </summary>
        /// <param name="machineName">The name of the machine to query for services.</param>
        /// <param name="serviceName">The service to test.</param>
		/// <returns>True if running, false otherwise.</returns>
        public static bool IsServiceRunning(string machineName, string serviceName)
		{
            using (ManagementObjectSearcher searcher = new ManagementObjectSearcher(String.Format(@"\\{0}\root\CIMV2", machineName), String.Format("SELECT Started FROM Win32_Service WHERE Name = '{0}'", serviceName)))
			{
				using (ManagementObjectCollection serviceCollection = searcher.Get())
				{
					foreach (ManagementObject service in serviceCollection)
					{
                        if (Convert.ToBoolean(service["Started"]))
                        {
                            return true;
                        }
					}
				}
			}

			return false;
		}

		/// <summary>
		/// Gets the collection of services on the given machine.
		/// </summary>
		/// <returns>The collection of services on the given machine.</returns>
        public static IEnumerable<ServiceInfo> GetServices()
        {
            return GetServices(Environment.MachineName);
        }

		/// <summary>
		/// Gets the collection of services on the given machine.
		/// </summary>
		/// <param name="machineName">The name of the machine to query for services.</param>
		/// <returns>The collection of services on the given machine.</returns>
		public static IEnumerable<ServiceInfo> GetServices(string machineName)
		{
            using (ManagementObjectSearcher searcher = new ManagementObjectSearcher(String.Format(@"\\{0}\root\CIMV2", machineName), "SELECT * FROM Win32_Service"))
			{
				using (ManagementObjectCollection serviceCollection = searcher.Get())
				{
					List<ServiceInfo> services = new List<ServiceInfo>();
					foreach (ManagementObject service in serviceCollection)
					{
                        services.Add(ServiceFromManagementObject(service));
					}

					return services;
				}
			}
		}

        /// <summary>
        /// Get a service on the given machine.
        /// </summary>
        /// <param name="serviceName">The service name.</param>
        /// <returns>The Service.</returns>
        public static ServiceInfo GetService(string serviceName)
        {
            return GetService(Environment.MachineName, serviceName);
        }

        /// <summary>
        /// Get the first service on the given machine that matches the service name.
        /// </summary>
        /// <param name="machineName">The name of the machine to query for services.</param>
        /// <param name="serviceName">The service name.</param>
        /// <returns>The Service.</returns>
        public static ServiceInfo GetService(string machineName, string serviceName)
        {
            using (ManagementObjectSearcher searcher = new ManagementObjectSearcher(String.Format(@"\\{0}\root\CIMV2", machineName), String.Format("SELECT * FROM Win32_Service WHERE Name = '{0}'", serviceName)))
            {
                using (ManagementObjectCollection serviceCollection = searcher.Get())
                {
                    ServiceInfo service = null;
                    foreach (ManagementObject serviceObject in serviceCollection)
                    {
                        service = ServiceFromManagementObject(serviceObject);
						break;
                    }

                    return service;
                }
            }
        }

        private static ServiceInfo ServiceFromManagementObject(ManagementObject service)
        {
            ServiceInfo serviceInfo = new ServiceInfo();

            serviceInfo.AcceptPause = Convert.ToBoolean(service["AcceptPause"]);
            serviceInfo.AcceptStop = Convert.ToBoolean(service["AcceptStop"]);
            serviceInfo.Caption = Convert.ToString(service["Caption"]);
            serviceInfo.CheckPoint = Convert.ToString(service["CheckPoint"]);
            serviceInfo.CreationClassName = Convert.ToString(service["CreationClassName"]);
            serviceInfo.Description = Convert.ToString(service["Description"]);
            serviceInfo.DesktopInteract = Convert.ToBoolean(service["DesktopInteract"]);
            serviceInfo.DisplayName = Convert.ToString(service["DisplayName"]);
            serviceInfo.ErrorControl = Convert.ToString(service["ErrorControl"]);
            serviceInfo.ExitCode = Convert.ToInt64(service["ExitCode"]);
            serviceInfo.InstallDate = Convert.ToDateTime(service["InstallDate"]);
            serviceInfo.Name = Convert.ToString(service["Name"]);
            serviceInfo.PathName = Convert.ToString(service["PathName"]);
            serviceInfo.ProcessId = Convert.ToInt64(service["ProcessId"]);
            serviceInfo.ServiceSpecificExitCode = Convert.ToInt64(service["ServiceSpecificExitCode"]);
            serviceInfo.ServiceType = Convert.ToString(service["ServiceType"]);
            serviceInfo.Started = Convert.ToBoolean(service["Started"]);
            serviceInfo.StartMode = Convert.ToString(service["StartMode"]);
            serviceInfo.StartName = Convert.ToString(service["StartName"]);
            serviceInfo.State = Convert.ToString(service["State"]);
            serviceInfo.Status = Convert.ToString(service["Status"]);
            serviceInfo.SystemCreationClassName = Convert.ToString(service["SystemCreationClassName"]);
            serviceInfo.SystemName = Convert.ToString(service["SystemName"]);
            serviceInfo.TagId = Convert.ToInt64(service["TagId"]);
            serviceInfo.WaitHint = Convert.ToString(service["WaitHint"]);

            return serviceInfo;
        }
	}
}