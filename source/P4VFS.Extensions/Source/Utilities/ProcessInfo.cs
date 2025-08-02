// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.IO;
using System.Linq;
using System.Text;
using System.Diagnostics;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;

namespace Microsoft.P4VFS.Extensions.Utilities
{
	public static class ProcessInfo
	{
		public enum OutputChannel
		{
			StdOut,
			StdErr,
		}

		public class ExecuteParams
		{
			public string FileName;
			public string Arguments = "";
			public string Directory = "";
			public bool LogCommand = true;
			public bool LogOutput = true;
			public bool UseShell = false;
			public bool ShowWindow = false;
			public bool AsAdmin = false;
			public bool TerminateOnCancel = true;
			public string Input;
			public Action<OutputData> Output;
			public ProcessPriorityClass? Priority;
			public CancellationToken? CancellationToken;
			public Dictionary<string, string> Environment;
		}

		public class ExecuteResult
		{
			public int ExitCode;
			public int ProcessID;
			public bool WasKilled;
			public bool WasCanceled;
			public Exception Exception;
		}

		public struct OutputData
		{
			public OutputChannel Channel;
			public string Text;
		}

		public class ExecuteResultOutput : ExecuteResult
		{
			public OutputData[] Data;
			
			public string Text
			{
				get { return String.Join("\n", Data?.Select(d => d.Text).Where(s => s != null) ?? new string[0]); }
			}

			public string[] Lines
			{
				get { return Text.Split('\n'); }
			}

			public bool HasChannel(OutputChannel channel)
			{
				return Data?.Any(d => d.Channel == channel) == true;
			}

			public bool HasStdErr
			{
				get { return HasChannel(OutputChannel.StdErr); }
			}

			public bool HasStdOut
			{
				get { return HasChannel(OutputChannel.StdOut); }
			}
		}

		public static Task<ExecuteResult> ExecuteAsync(ExecuteParams ep)
		{
			return Task.Run(() => ExecuteWait(ep));
		}

		public static ExecuteResult ExecuteWait(ExecuteParams ep)
		{
			ExecuteResult result = new ExecuteResult();
			using (Process process = new Process())
			{
				if (ep.LogCommand)
				{
					VirtualFileSystemLog.Info(String.Format("\"{0}\" {1}", ep.FileName, ep.Arguments));
				}

				process.StartInfo.FileName = ep.FileName;
				process.StartInfo.Arguments = ep.Arguments ?? "";
				process.StartInfo.UseShellExecute = ep.UseShell;
				process.StartInfo.CreateNoWindow = !ep.ShowWindow;
				process.StartInfo.WindowStyle = ep.ShowWindow ? ProcessWindowStyle.Normal : ProcessWindowStyle.Hidden;
				process.StartInfo.RedirectStandardInput = ep.Input != null;

				if (ep.AsAdmin)
				{
					process.StartInfo.Verb = "runas";
					process.StartInfo.UseShellExecute = true;
				}

				if (Directory.Exists(ep.Directory))
				{
					process.StartInfo.WorkingDirectory = ep.Directory;
				}

				if (ep.Environment != null)
				{
					foreach (KeyValuePair<string, string> variable in ep.Environment)
					{
						process.StartInfo.EnvironmentVariables[variable.Key] = variable.Value;
					}
				}

				object outputLock = new object();
				Action<OutputChannel, string> outputReceiver = null;
				if (ep.LogOutput || ep.Output != null)
				{
					outputReceiver = (channel, text) =>
					{
						if (text != null)
						{
							lock (outputLock)
							{
								if (ep.LogOutput)
								{
									if (channel == OutputChannel.StdOut)
									{
										VirtualFileSystemLog.Info(text);
									}
									else
									{
										VirtualFileSystemLog.Error(text);
									}
								}
								if (ep.Output != null)
								{
									ep.Output(new OutputData{ Channel=channel, Text=text });
								}
							}
						}
					};

					process.StartInfo.CreateNoWindow = true;
					process.StartInfo.RedirectStandardOutput = true;
					process.StartInfo.RedirectStandardError = true;
					process.OutputDataReceived += (s, e) => outputReceiver(OutputChannel.StdOut, e.Data);
					process.ErrorDataReceived += (s, e) => outputReceiver(OutputChannel.StdErr, e.Data);
				}

				try
				{
					process.Start();
					result.ProcessID = process.Id;

					if (outputReceiver != null)
					{
						process.BeginOutputReadLine();
						process.BeginErrorReadLine();
					}

					if (process.StartInfo.RedirectStandardInput == true)
					{
						process.StandardInput.Write(ep.Input);
						process.StandardInput.Close();
					}

					if (ep.Priority != null)
					{
						process.PriorityClass = ep.Priority.Value;
					}
					
					List<IntPtr> handleList = new List<IntPtr>();
					handleList.Add(process.Handle);
					if (ep.CancellationToken != null)
					{
						handleList.Add(ep.CancellationToken.Value.WaitHandle.SafeWaitHandle.DangerousGetHandle());
					}

					IntPtr[] handles = handleList.ToArray();
					WindowsInterop.WaitForMultipleObjects((uint)handles.Length, handles, false, WindowsInterop.INFINITE);

					if (ep.CancellationToken != null)
					{
						ep.CancellationToken.Value.ThrowIfCancellationRequested();
					}

					process.WaitForExit();
					result.ExitCode = process.ExitCode;
				}
				catch (Exception e)
				{
					try
					{
						result.Exception = e;
						result.ExitCode = -1;
						result.WasCanceled = e is OperationCanceledException;
						if (process.HasExited == false && ep.TerminateOnCancel)
						{
							result.WasKilled = true;
							process.Kill();
						}
					}
					catch {}
				}
			}
			return result;
		}

		public static int ExecuteWait(
			string fileName, 
			string arguments, 
			string directory = "", 
			bool echo = false, 
			bool log = false, 
			bool shell = false, 
			bool window = false, 
			StringBuilder stdout = null, 
			string stdin = null)
		{
			ExecuteParams ep = new ExecuteParams();
			ep.FileName = fileName;
			ep.Arguments = arguments;
			ep.Directory = directory;
			ep.LogCommand = echo;
			ep.LogOutput = log;
			ep.UseShell = shell;
			ep.ShowWindow = window;
			ep.Input = stdin;
			ep.Output = stdout != null ? new Action<OutputData>(d => stdout.Append($"{d.Text}\n")) : null;
			return ExecuteWait(ep).ExitCode;
		}

		public static ExecuteResultOutput ExecuteWaitOutput(
			string fileName, 
			string arguments, 
			string directory = "", 
			bool echo = false, 
			bool log = false, 
			bool shell = false, 
			bool window = false)
		{
			List<OutputData> output = new List<OutputData>();

			ExecuteParams ep = new ExecuteParams();
			ep.FileName = fileName;
			ep.Arguments = arguments;
			ep.Directory = directory;
			ep.LogCommand = echo;
			ep.LogOutput = log;
			ep.UseShell = shell;
			ep.ShowWindow = window;
			ep.Output = d => output.Add(d);

			int exitCode = ExecuteWait(ep).ExitCode;
			return new ExecuteResultOutput { ExitCode = exitCode, Data = output.ToArray() };
		}

		public static int RemoteExecuteWait(
			string cmd,
			string remoteHost,
			string remoteUser,
			string remotePasswd,
			bool admin = false,
			bool shell = false,
			bool copy = false,
			bool interactive = false,
			StringBuilder stdout = null,
			string sysInternalsFolder = null)
		{
			if (String.IsNullOrEmpty(sysInternalsFolder))
			{
				sysInternalsFolder = String.Format("{0}\\SysinternalsSuite", Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles));
			}

			string psExecExe = String.Format("{0}\\psexec.exe", sysInternalsFolder);
			string psExecArgs = String.Format("\\\\{0} -u {1} -p {2} -accepteula -nobanner", remoteHost, remoteUser, remotePasswd);
			if (admin)
			{
				psExecArgs += " -h";
			}
			if (copy)
			{
				psExecArgs += " -c";
			}
			if (interactive)
			{
				psExecArgs += " -i";
			}
			if (shell)
			{
				psExecArgs += " cmd.exe /c";
			}

			VirtualFileSystemLog.Info("RemoteExecute [{0}] {1}", remoteHost, cmd);
			return ProcessInfo.ExecuteWait(psExecExe, String.Format("{0} {1}", psExecArgs, cmd), echo:false, log:true, stdout:stdout);
		}
	}
}

