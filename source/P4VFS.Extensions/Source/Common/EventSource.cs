// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using Microsoft.P4VFS.CoreInterop;
using System;
using System.Collections.Generic;
using System.Diagnostics.Tracing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Microsoft.P4VFS.Extensions
{
    [EventSource(Name = EventSourceName)]
    public class EventSource : System.Diagnostics.Tracing.EventSource
    {
        public const string EventSourceName = "P4VFS";
        public static EventSource Log = new EventSource();

        #region Sync Events
        [Event(100,
            Message =
            "Virtual Sync Summary: " +
            "Server: {0} " +
            "Total Files: {1} " +
            "Total Time: {2} (ms) " +
            "Virtual File Size: {3} (bytes) " +
            "Disk File Size: {4} (bytes) " +
            "Modification Time:{5} (ms) " +
            "Placeholder Time: {6} (ms) " +
            "Flush Time: {7} (ms) " +
            "Sync Time: {8} (ms) " +
            "Batch Mode: {9}",
            Level = EventLevel.Informational)]

        public void VirtualSyncSummary(string server, int count, long totalTime, long virtualFileSize, long diskFileSize, long fileModTime, long placeholderTime, long flushTime, long syncTime, string flushType)
        {
            this.WriteEvent(100, server, count, totalTime, virtualFileSize, diskFileSize, fileModTime, placeholderTime, flushTime, syncTime, flushType);
        }

        [NonEvent]
        public void VirtualSyncError(Exception exception)
        {
            //this.VirtualSyncError(Corinth.Logging.Utilities.GetSerializedObjectString(exception));
        }

        [Event(101, Message = "Virtual Sync Error: {0}", Level = EventLevel.Error)]
        public void VirtualSyncError(string exception)
        {
            this.WriteEvent(101, exception);
        }

        [NonEvent]
        public void VirtualSyncError(string file, Exception exception)
        {
            //this.VirtualSyncFileError(file, Corinth.Logging.Utilities.GetSerializedObjectString(exception));
        }

        [Event(102, Message = "Virtual Sync Error: {0}: {1}", Level = EventLevel.Error)]
        public void VirtualSyncFileError(string file, string exception)
        {
            this.WriteEvent(102, file, exception);
        }

        #endregion


        #region Populate Events
        [Event(200, 
            Message = 
            "Populate File Summary: " +
            "File: {0} " +
            "Revision: {1} " +
            "Server: {2}" +
            "Method: {3} " + 
            "Total Time: {4} (ms) " +
            "Download Time: {5} (ms)", Level = EventLevel.Informational)]
        public void PopulateFileSummary(string file, string revision, string server, string method, long totalTime, long downloadTime)
        {
            this.WriteEvent(200, file, revision, server, method, totalTime, downloadTime);
        }

        [NonEvent]
        public void PopulateFileSummary(string file, string revision, string server, FilePopulateMethod method, long totalTime, long downloadTime)
        {
            this.PopulateFileSummary(file, revision, server, method.ToString(), totalTime, downloadTime);
        }

        [Event(201, Message = "MakeFileResident failed to {0} '{1}' for file '{2}'.", Level = EventLevel.Error)]
        public void PopulateFileError(string operation, string depotPath, string path)
        {
            this.WriteEvent(201, operation, depotPath, path);
        }

        #endregion
    }
}
