// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Windows;
using System.Windows.Threading;

namespace Microsoft.P4VFS.Extensions.Controls
{
	public partial class LoginWindow : Window
	{
		private CancellationToken? m_CancellationToken;
		private DispatcherTimer m_UpdateTimer; 

		public LoginWindow()
		{
			InitializeComponent();

			m_UpdateTimer = new DispatcherTimer();
			m_UpdateTimer.Interval = TimeSpan.FromSeconds(1.0/30.0);
			m_UpdateTimer.Tick += OnTickUpdateTimer;
			m_UpdateTimer.Start();
		}

		public string Port
		{
			get { return m_Port.Text; }
			set { m_Port.Text = value; }
		}

		public string User
		{
			get { return m_User.Text; }
			set { m_User.Text = value; }
		}

		public string Client
		{
			get { return m_Client.Text; }
			set { m_Client.Text = value; }
		}

		public string Passwd
		{
			get { return m_Passwd.Password; }
			set { m_Passwd.Password = value; }
		}

		public CancellationToken? CancellationToken
		{
			get { return m_CancellationToken; }
			set { m_CancellationToken = value; }
		}

		private void OnTickUpdateTimer(object sender, EventArgs e)
		{
			if (m_CancellationToken?.IsCancellationRequested == true)
			{
				this.DialogResult = false;
			}
		}		

		private void OnClickButtonCancel(object sender, RoutedEventArgs e)
		{
			this.DialogResult = false;
		}

		private void OnClickButtonOK(object sender, RoutedEventArgs e)
		{
			this.DialogResult = true;
		}
	}
}
