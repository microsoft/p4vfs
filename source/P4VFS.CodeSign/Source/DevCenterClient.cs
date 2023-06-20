// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Linq;
using System.IO;
using System.Collections.Generic;
using System.Net.Http;
using System.Net.Http.Headers;
using System.Text;
using System.Text.RegularExpressions;
using System.Security.Cryptography.X509Certificates;
using System.Threading.Tasks;
using Azure.Security.KeyVault.Secrets;
using Azure.Storage.Blobs;
using Microsoft.P4VFS.Extensions;
using Microsoft.P4VFS.Extensions.Utilities;
using Microsoft.P4VFS.Extensions.Linq;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;


namespace Microsoft.P4VFS.CodeSign
{
	public class DevCenterClient : ICodeSignClient
	{
		private static readonly string HdcVersion = "1.0";
		private static readonly string HdcTenant = "my";
		private static readonly string HdcRootUri = $"/v{HdcVersion}/{HdcTenant}/hardware";

		private DevCenterJob m_Job;
		private HttpClient m_HttpClient;
		private string m_SignFilePath;
		private string m_ArtifactsFolder;
		private AccessToken m_AccessToken;
		
		public DevCenterClient()
		{
		}

		public void Dispose()
		{
			if (m_HttpClient != null)
			{
				m_HttpClient.Dispose();
				m_HttpClient = null;
			}
		}

		public bool Submit(CodeSignJob job)
		{
			JToken tokenSignInput = JObject.Parse(CodeSignUtilities.ExtractResourceToString(job.SignInputName));
			foreach (JToken tokenManifest in tokenSignInput.SelectTokens("$.SignBatches[*].SignRequestFiles[*].Manifest"))
			{
				JToken tokenSignRequestFile = tokenManifest.Ancestors().Skip(1).FirstOrDefault();
				string signFileName = tokenSignRequestFile?.SelectToken("$.SourceLocation")?.Value<string>();
				if (Regex.IsMatch(signFileName ?? "", @"\.(cab|hlkx)$", RegexOptions.IgnoreCase) == false)
				{
					VirtualFileSystemLog.Error("Unsupported input file: {0}", signFileName);
					return false;
				}

				m_SignFilePath = Path.Combine(job.SourceFolder, signFileName);
				if (File.Exists(m_SignFilePath) == false)
				{
					VirtualFileSystemLog.Error("Failed to find input file: {0}", m_SignFilePath);
					return false;
				}

				m_ArtifactsFolder = String.Format("{0}.hdc", m_SignFilePath);
				FileUtilities.DeleteDirectoryAndFiles(m_ArtifactsFolder);
				FileUtilities.CreateDirectory(m_ArtifactsFolder);

				InitializeJob(job, tokenManifest);
				UpdateJobProduct();
				UpdateJobHttpClient();

				VirtualFileSystemLog.Info("Performing HDC codesign");
				if (SubmitDevCenterJobAsync().Result == false)
				{
					VirtualFileSystemLog.Error("Failed HDC codesign submission: {0}", m_SignFilePath);
					return false;
				}

				VirtualFileSystemLog.Info("Deploying signed artifacts");
				foreach (JToken tokenOutputFile in tokenManifest.SelectTokens("$.OutputFiles[*]"))
				{
					string fileSourceLocation = tokenOutputFile.SelectToken("$.SourceLocation")?.Value<string>() ?? "";
					string[] sourceFilePaths = Directory.GetFiles(m_ArtifactsFolder, "*", SearchOption.AllDirectories).Where(f => f.EndsWith("\\"+fileSourceLocation, StringComparison.CurrentCultureIgnoreCase)).ToArray();
					if (sourceFilePaths.Length != 1)
					{
						VirtualFileSystemLog.Error("Failed to find output file \"{0}\". Found: {1}", fileSourceLocation, sourceFilePaths.ToNiceString());
						return false;
					}

					string sourceFilePath = sourceFilePaths[0];
					string fileTargetDirectory = tokenOutputFile.SelectToken("$.TargetDirectory")?.Value<string>() ?? "";
					string targetFolderPath = Path.Combine(job.TargetFolder, fileTargetDirectory);

					VirtualFileSystemLog.Info("Copying file: {0} -> {1}", sourceFilePath, targetFolderPath);
					if (FileUtilities.CopyFile(sourceFilePath, targetFolderPath, overwrite:true) == false)
					{
						VirtualFileSystemLog.Error("Failed to copy file to folder: {0} -> {1}", sourceFilePath, targetFolderPath);
						return false;
					}
				}
			}
			return true;
		}

		public static bool PreprocessSignRequestFile(CodeSignJob job, JToken tokenSignRequestFile, string signFilePath)
		{
			JToken tokenManifest = tokenSignRequestFile.SelectToken("$.Manifest");
			if (tokenManifest != null)
			{
				if (signFilePath.EndsWith(".cab", StringComparison.InvariantCultureIgnoreCase))
				{
					string tmpCabFolder = Path.Combine(job.TargetFolder, Path.GetFileName(signFilePath) + ".tmp");
					FileUtilities.DeleteDirectoryAndFiles(tmpCabFolder);
					FileUtilities.CreateDirectory(tmpCabFolder);

					string stageCabFolder = Path.Combine(job.TargetFolder, Path.GetFileName(signFilePath) + ".stage");
					FileUtilities.DeleteDirectoryAndFiles(stageCabFolder);
					FileUtilities.CreateDirectory(stageCabFolder);

					foreach (JToken tokenInputFile in tokenManifest.SelectTokens("$.InputFiles[*]"))
					{
						string fileSourceLocation = tokenInputFile.SelectToken("$.SourceLocation")?.Value<string>() ?? "";
						string fileTargetDirectory = tokenInputFile.SelectToken("$.TargetDirectory")?.Value<string>() ?? "";
						string sourceFilePath = Path.Combine(job.SourceFolder, fileSourceLocation);
						string stageFolderPath = Path.Combine(stageCabFolder, fileTargetDirectory);

						VirtualFileSystemLog.Info("Copying file: {0} -> {1}", sourceFilePath, stageFolderPath);
						if (FileUtilities.CopyFile(sourceFilePath, stageFolderPath) == false)
						{
							VirtualFileSystemLog.Error("Failed to copy file to folder: {0} -> {1}", sourceFilePath, stageFolderPath);
							return false;
						}

						bool removeSignature = tokenInputFile.SelectToken("$.RemoveSignature")?.Value<bool>() ?? false;
						if (removeSignature)
						{
							string stageFilePath = Path.Combine(stageFolderPath, Path.GetFileName(fileSourceLocation));
							VirtualFileSystemLog.Info("Removing existing signature from file: {0}", stageFilePath);
							if (ProcessInfo.ExecuteWait(WdkUtilities.GetSignToolExe(), String.Format("remove /s \"{0}\"", stageFilePath), echo:true, log:true) != 0)
							{
								VirtualFileSystemLog.Info("Failed to remove signature from file: {0}", stageFilePath);
								return false;
							}
						}
					}

					FileUtilities.DeleteFile(signFilePath);
					if (CodeSignInterop.CreateCabFileFromFolder(signFilePath, stageCabFolder, tmpCabFolder, null, 0) != WindowsInterop.S_OK)
					{
						VirtualFileSystemLog.Error("Failed to create cab file: {0}", signFilePath);
						return false;
					}
				}
				else if (signFilePath.EndsWith(".hlkx", StringComparison.InvariantCultureIgnoreCase))
				{
					HardwareLabKitJob hlkJob = JsonConvert.DeserializeObject<HardwareLabKitJob>(tokenManifest.SelectToken("$.DevCenterSign.HLK").ToString());
					hlkJob.CodeSignJob = job;

					using (DevCenterClient client = new DevCenterClient())
					{
						client.InitializeJob(job, tokenManifest);
						client.m_Job.SecretName = hlkJob.AgentSecretName;
						hlkJob.AgentPassword = client.GetClientSecretFromKeyVaultAsync().Result;
					}

					using (HardwareLabKitPackage hlk = new HardwareLabKitPackage())
					{
						if (hlk.GeneratePackage(hlkJob, signFilePath) == false)
						{
							VirtualFileSystemLog.Error("Failed to create hlkx file: {0}", signFilePath);
							return false;
						}
					}
				}

				tokenManifest.Parent.Remove();
			}
			return true;
		}

		private void InitializeJob(CodeSignJob job, JToken tokenManifest)
		{
			JToken tokenDevCenterSign = tokenManifest.SelectToken("$.DevCenterSign");
			if (tokenDevCenterSign == null)
			{
				throw new Exception("Missing required DevCenterSign configuration");
			}

			m_Job = JsonConvert.DeserializeObject<DevCenterJob>(tokenDevCenterSign.ToString());
			m_Job.CodeSignJob = job;

			JToken tokenSignAuth = JObject.Parse(CodeSignUtilities.ExtractResourceToString(CodeSignResources.SignAuthJson));
			m_Job.ClientId = tokenSignAuth.SelectToken("$.ClientId").Value<string>();
			m_Job.AuthCert = JsonConvert.DeserializeObject<DevCenterAuthCert>(tokenSignAuth.SelectToken("$.AuthCert").ToString());
		}

		private void UpdateJobProduct()
		{
			m_Job.Product["AnnouncementDate"] = DateTime.UtcNow.ToString();
			m_Job.Product["FirmwareVersion"] = CoreInterop.NativeConstants.Version;
			m_Job.Product["DeviceMetadataIds"] = new JArray(new string[] { Guid.NewGuid().ToString(), Guid.NewGuid().ToString() });
		}

		private void UpdateJobHttpClient()
		{
			if (m_HttpClient == null)
			{
				m_HttpClient = new HttpClient();
			}
			m_HttpClient.BaseAddress = new Uri(m_Job.ServiceUri);
		}

		private async Task<bool> SubmitDevCenterJobAsync()
		{
			try
			{
				VirtualFileSystemLog.Info("Requesting new product");
				JObject activeProduct = await RequestProductAsync();
				VirtualFileSystemLog.Info("Created product:\n{0}", activeProduct);

				VirtualFileSystemLog.Info("Requesting new submission");
				JObject activeSubmission = await RequestSubmissionAsync(activeProduct);
				VirtualFileSystemLog.Info("Created submission:\n{0}", activeSubmission);

				VirtualFileSystemLog.Info("Requesting commit of package");
				JObject activeCommit = await RequestCommitPackageAsync(activeSubmission);
				VirtualFileSystemLog.Info("Committing package submission:\n{0}", activeCommit);

				VirtualFileSystemLog.Info("Waiting for package sign to complete");
				JObject activeStatus = await RequestCompletedPackageAsync(activeSubmission);
				VirtualFileSystemLog.Info("Package sign finished:\n{0}", activeStatus);

				VirtualFileSystemLog.Info("Request package artifacts");
				string[] artifactFiles = await RequestPackageArtifacts(activeStatus);
				VirtualFileSystemLog.Info("Received {0} package artifact files", artifactFiles.Length);

				VirtualFileSystemLog.Info("Extracting package artifacts");
				ExtractPackageArtifacts(artifactFiles);
				VirtualFileSystemLog.Info("Extracted package artifacts");
				return true;
			}
			catch (Exception e)
			{
				VirtualFileSystemLog.Error("Exception submitting job: {0}\n{1}", e.Message, e.StackTrace);
			}
			return false;
		}

		private void ExtractPackageArtifacts(string[] artifactFiles)
		{
			foreach (string filePath in artifactFiles)
			{
				Match archiveMatch = Regex.Match(filePath, @"\.(?<type>zip|cab)$", RegexOptions.IgnoreCase);
				if (archiveMatch.Success)
				{
					string archiveType = archiveMatch.Groups["type"].Value.ToLower();
					string extractFolderPath = String.Format("{0}.ext", filePath);
					FileUtilities.DeleteDirectoryAndFiles(extractFolderPath);
					FileUtilities.CreateDirectory(extractFolderPath);

					VirtualFileSystemLog.Info("Extracting artifact \"{0}\" to folder \"{1}\"", Path.GetFileName(filePath), extractFolderPath);

					switch (archiveType)
					{
						case "zip":
						{
							System.IO.Compression.ZipFile.ExtractToDirectory(filePath, extractFolderPath);
							break;
						}
						case "cab":
						{
							CodeSignInterop.ExtractCabFileToFolder(filePath, extractFolderPath);
							break;
						}
					}
				}
			}
		}

		private async Task<string[]> RequestPackageArtifacts(JObject activeStatus)
		{
			List<string> artifactFiles = new List<string>();
			foreach (JObject downloadItem in activeStatus.SelectTokens("$.downloads.items[?(@.type!='initialPackage')]"))
			{
				string itemType = downloadItem.Value<string>("type");
				string itemUrl = downloadItem.Value<string>("url");

				Uri itemUri = new Uri(itemUrl);
				Match itemMatch = Regex.Match(Uri.UnescapeDataString(itemUri.Query), @"filename\s*=\s*(?<name>[\w\.]+)", RegexOptions.IgnoreCase);
				if (itemMatch.Success == false)
				{
					throw new Exception(String.Format("Failed to match filename from Uri: {0}", itemUrl));
				}

				string itemFilePath = String.Format("{0}\\{1}", m_ArtifactsFolder, itemMatch.Groups["name"].Value);
				artifactFiles.Add(itemFilePath);
				FileUtilities.DeleteFile(itemFilePath);

				VirtualFileSystemLog.Info("Downloading artifact: {0} -> {1}", itemType, itemFilePath);
				await DownloadBlobToFileAsync(itemUri, itemFilePath);

				if (File.Exists(itemFilePath) == false)
				{
					throw new FileNotFoundException("Failed to download artifact", itemFilePath);
				}
			}

			return artifactFiles.ToArray();
		}

		private async Task<JObject> RequestCompletedPackageAsync(JObject activeSubmission)
		{
			string productId = activeSubmission.Value<string>("productId");
			string submissionId = activeSubmission.Value<string>("id");
			string statusUri = String.Format("{0}/products/{1}/submissions/{2}", HdcRootUri, productId, submissionId);

			bool success = false;
			JObject activeStatus = null;
			DateTime startTime = DateTime.Now;
			while (true)
			{
				activeStatus = await RequestAsync(HttpMethod.Get, statusUri);

				string commitStatus = activeStatus?.SelectToken("$.commitStatus")?.Value<string>();
				string workflowState = activeStatus?.SelectToken("$.workflowStatus.state")?.Value<string>();
				string workflowCurrentStep = activeStatus?.SelectToken("$.workflowStatus.currentStep")?.Value<string>();
				VirtualFileSystemLog.Info("[{0:d\\.hh\\:mm\\:ss}] Status: commitStatus=\"{1}\" workflowState=\"{2}\" workflowCurrentStep=\"{3}\"", DateTime.Now-startTime, commitStatus, workflowState, workflowCurrentStep);
				
				if (Regex.IsMatch(commitStatus, "^(CommitFailed)$", RegexOptions.IgnoreCase))
				{
					break;
				}

				if (Regex.IsMatch(workflowState, "^(Failed|Canceled)$", RegexOptions.IgnoreCase))
				{
					break;
				}

				if (Regex.IsMatch(workflowState, "^(Completed)$", RegexOptions.IgnoreCase))
				{
					if (String.IsNullOrEmpty(activeStatus.SelectToken("$.downloads.items[?(@.type=='signedPackage')].url").Value<string>()) == false)
					{
						success = true;
						break;
					}
				}

				Task.Delay(TimeSpan.FromMinutes(1)).Wait();
			}

			if (success == false)
			{
				throw new Exception(String.Format("Package submission failed: {0}", activeStatus));
			}
			return activeStatus;
		}

		private async Task<JObject> RequestCommitPackageAsync(JObject activeSubmission)
		{
			string packageUploadUrl = activeSubmission.SelectToken("$.downloads.items[?(@.type=='initialPackage')].url").Value<string>();

			VirtualFileSystemLog.Info("Uploading package to URL: {0}", packageUploadUrl);
			await UploadFileToBlobAsync(m_SignFilePath, new Uri(packageUploadUrl));

			string productId = activeSubmission.Value<string>("productId");
			string submissionId = activeSubmission.Value<string>("id");
			string commitUri = String.Format("{0}/products/{1}/submissions/{2}/commit", HdcRootUri, productId, submissionId);

			JObject activeCommit = await RequestAsync(HttpMethod.Post, commitUri);
			return activeCommit;
		}

		private async Task<JObject> RequestProductAsync()
		{
			string activeProductUri = String.Format("{0}/products", HdcRootUri);
			JObject activeProduct = await RequestAsync(HttpMethod.Post, activeProductUri, m_Job.Product);
			return activeProduct;
		}

		private async Task<JObject> RequestSubmissionAsync(JObject activeProduct)
		{
			JObject submissionRequest = new JObject();
			submissionRequest["Name"] = String.Format("P4VFS_DRIVER_{0:yyyy_MM_dd_HH_mm_ss}", DateTime.Now);
			submissionRequest["Type"] = "Initial";

			string productId = activeProduct.Value<string>("id");
			string submissionUri = String.Format("{0}/products/{1}/submissions", HdcRootUri, productId);

			JObject activeSubmission = await RequestAsync(HttpMethod.Post, submissionUri, submissionRequest);
			return activeSubmission;
		}

		private async Task<string> GetClientSecretFromKeyVaultAsync()
		{
			VirtualFileSystemLog.Info("Requesting secret '{0}' from KeyVault '{1}'", m_Job.SecretName, m_Job.KeyVaultUri);

			X509Store certificateStore = new X509Store(
				Converters.ToEnum<StoreName>(m_Job.AuthCert.StoreName).Value,
				Converters.ToEnum<StoreLocation>(m_Job.AuthCert.StoreLocation).Value);

			certificateStore.Open(OpenFlags.ReadOnly);

			X509Certificate2Collection certificates = certificateStore.Certificates.Find(
				X509FindType.FindBySubjectName,
				m_Job.AuthCert.SubjectName,
				true);

			foreach (X509Certificate2 certificate in certificates.OfType<X509Certificate2>())
			{
				try
				{
					SecretClient keyVault = new SecretClient(
						new Uri(m_Job.KeyVaultUri),
						new Azure.Identity.ClientCertificateCredential(m_Job.TenantId, m_Job.ClientId, certificate));

					Azure.Response<KeyVaultSecret> appRegSecret = await keyVault.GetSecretAsync(m_Job.SecretName);
					string appRegSecretString = appRegSecret?.Value?.Value;
					if (String.IsNullOrEmpty(appRegSecretString) == false)
					{
						return appRegSecretString;
					}
				}
				catch (Exception e)
				{
					VirtualFileSystemLog.Error("Exception using certificate '{0}' to request secret from KeyVault '{1}': {2}", certificate.ToString(), m_Job.KeyVaultUri, e.Message);
				}
			}
			throw new Exception(String.Format("Failed get valid secret '{0}' from KeyVault '{1}'", m_Job.SecretName, m_Job.KeyVaultUri));
		}

		private async Task<string> GetCachedAccessTokenAsync()
		{
			if (m_AccessToken == null || m_AccessToken.ExpirationDate <= DateTime.Now)
			{
				VirtualFileSystemLog.Info("Requesting new access token from HDC");
				JObject token = await GetClientCredentialAccessTokenAsync();
				
				string text = token?.Value<string>("access_token");
				if (String.IsNullOrEmpty(text))
				{
					throw new Exception(String.Format("Failed to get dev center access token: {0}", token));
				}

				int expirationSeconds = token?.Value<int>("expires_in") ?? 0;
				int expirationLatencySeconds = 30 * 60;
				DateTime expirationDate = DateTime.Now + TimeSpan.FromSeconds(Math.Max(0, expirationSeconds - expirationLatencySeconds));
				m_AccessToken = new AccessToken{ Text = text, ExpirationDate = expirationDate };
			}
			return m_AccessToken.Text;
		}

		private async Task<JObject> GetClientCredentialAccessTokenAsync()
		{
			if (String.IsNullOrEmpty(m_Job.ClientSecret))
			{
				VirtualFileSystemLog.Info("Requesting client secret from KeyVault");
				m_Job.ClientSecret = await GetClientSecretFromKeyVaultAsync();
				if (String.IsNullOrEmpty(m_Job.ClientSecret))
				{
					throw new Exception(String.Format("Failed to get secret \"{0}\" from KeyVault \"{1}\"", m_Job.SecretName, m_Job.KeyVaultUri));
				}
			}

			using (HttpClient client = new HttpClient())
			{
				using (HttpRequestMessage request = new HttpRequestMessage(HttpMethod.Post, m_Job.TokenEndpoint))
				{
					string content = String.Format(
						"grant_type=client_credentials&client_id={0}&client_secret={1}&resource={2}",
						m_Job.ClientId,
						m_Job.ClientSecret,
						m_Job.Scope);

					request.Content = new StringContent(content, Encoding.UTF8, "application/x-www-form-urlencoded");
					using (HttpResponseMessage response = await client.SendAsync(request))
					{
						string responseContent = await response.Content.ReadAsStringAsync();
						return JObject.Parse(responseContent);
					}
				}
			}
		}

		private async Task<JObject> RequestAsync(HttpMethod httpMethod, string relativeUrl, JObject requestContent = null)
		{
			using (var request = new HttpRequestMessage(httpMethod, relativeUrl))
			{
				request.Headers.Add("MS-CorrelationId", Guid.NewGuid().ToString());
				request.Headers.Add("MS-RequestId", Guid.NewGuid().ToString());
				request.Headers.Authorization = new AuthenticationHeaderValue("Bearer", await GetCachedAccessTokenAsync());

				if (requestContent != null)
				{
					request.Content = new StringContent(requestContent.ToString(), Encoding.UTF8, "application/json");
				}

				using (HttpResponseMessage response = await m_HttpClient.SendAsync(request))
				{
					string responseContentString = await response.Content.ReadAsStringAsync();
					if (response.IsSuccessStatusCode == false)
					{
						throw new Exception(String.Format("Reponse unsuccessfull: {0}", responseContentString));
					}
					if (String.IsNullOrEmpty(responseContentString))
					{
						return new JObject();
					}
					return JObject.Parse(responseContentString);
				}
			}
		}

		public async Task UploadFileToBlobAsync(string fileName, Uri blobSasUrl)
		{
			using (Stream stream = new FileStream(fileName, FileMode.Open))
			{
				BlobClient blobClient = new BlobClient(blobSasUrl);
				await blobClient.UploadAsync(stream);
			}
		}

		public async Task DownloadBlobToFileAsync(Uri blobSasUrl, string fileName)
		{
			using (Stream stream = new FileStream(fileName, FileMode.Create))
			{
				BlobClient blobClient = new BlobClient(blobSasUrl);
				await blobClient.DownloadToAsync(stream);
			}
		}

		private class AccessToken
		{
			public string Text { get; set; }
			public DateTime ExpirationDate { get; set; }
		}

		private class DevCenterAuthCert
		{
			public string SubjectName { get; set; }
			public string StoreLocation { get; set; }
			public string StoreName { get; set; }
		}

		private class DevCenterJob
		{
			public CodeSignJob CodeSignJob { get; set; }
			public DevCenterAuthCert AuthCert { get; set; }
			public JObject Product { get; set; }
			public string ClientId { get; set; }
			public string ClientSecret { get; set; }
			public string TenantId { get; set; }
			public string KeyVaultUri { get; set; }
			public string SecretName { get; set; }
			public string ServiceUri { get; set; }
			public string TokenEndpoint { get; set; }
			public string Scope { get; set; }
		}
	}
}

