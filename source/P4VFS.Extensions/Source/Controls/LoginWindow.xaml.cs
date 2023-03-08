// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace Microsoft.P4VFS.Extensions.Controls
{
	public partial class LoginWindow : Window
	{
		public LoginWindow()
		{
			InitializeComponent();
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
