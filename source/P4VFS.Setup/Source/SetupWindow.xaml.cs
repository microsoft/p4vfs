// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Threading;
using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace Microsoft.P4VFS.Setup
{
	public partial class SetupWindow : Window, INotifyPropertyChanged
	{
		public event PropertyChangedEventHandler PropertyChanged;
		public Func<bool> DoWork;

		protected DispatcherTimer _UpdateTimer;
		protected List<ProgressLayer> _ProgressLayers;
		protected string _DescriptionText;
		protected string _StatusText;
		protected double _ProgressAngle;
		protected double _CollapsedHeight;
		protected double _ExpandedMinHeight;
		protected double _ExpandedHeight;

		public SetupWindow()
		{
			InitializeComponent();
			DataContext = this;
			Loaded += OnLoaded;
			_ProgressLayers = new List<ProgressLayer>();
			_ProgressLayers.Add(new ProgressLayer{ CurrentStep = 0, TotalSteps = 1 });
			_DescriptionText = String.Format("Microsoft P4VFS {0}", Microsoft.P4VFS.CoreInterop.NativeConstants.Version);
			_StatusText = "";
			_Text.Clear();
			_UpdateTimer = new DispatcherTimer();
			_UpdateTimer.Interval = TimeSpan.FromSeconds(1.0/30.0);
			_UpdateTimer.Tick += OnTickUpdateTimer;
			_UpdateTimer.Start();
		}

		public double Progress
		{
			get 
			{
				float v = 0;
				float d = 1;
				foreach (ProgressLayer layer in _ProgressLayers)
				{
					float total = (float)Math.Max(1, layer.TotalSteps);
					float step = (float)Math.Max(0, Math.Min(layer.CurrentStep, total));
					v += d * (step / total);
					d *= 1 / total;
				}
				return v * 100;
			}
		}

		public string DescriptionText
		{
			get 
			{ 
				return _DescriptionText; 
			}
		}

		public string StatusText
		{
			get 
			{ 
				return _StatusText; 
			}
			set 
			{ 
				_StatusText = value; 
				NotifyPropertyChanged();
			}
		}

		public double ProgressAngle
		{
			get
			{
				return _ProgressAngle;
			}
			set
			{
				_ProgressAngle = value;
				NotifyPropertyChanged();
			}
		}

		public new bool? ShowDialog()
		{
			var worker = new System.ComponentModel.BackgroundWorker();
			worker.DoWork += this.OnWorkerBegin;
			worker.RunWorkerCompleted += this.OnWorkerEnd;
			worker.RunWorkerAsync();		
			return base.ShowDialog();
		}

		private void OnTickUpdateTimer(object sender, EventArgs e)
		{
			if (_CloseButton.IsEnabled == false)
				this.ProgressAngle += 8;
		}

		private void OnWorkerBegin(object sender, DoWorkEventArgs e)
		{
			DoWork?.Invoke();
		}

		private void OnWorkerEnd(object sender, RunWorkerCompletedEventArgs e)
		{
			Dispatcher.Invoke(() => 
			{
				_CloseButton.IsEnabled = true;
			});
		}

		public void Update()
		{
			_ProgressBar.Value = Math.Max(Progress, _ProgressBar.Value);
		}

		public void SetProgress(int step)
		{
			if (_ProgressLayers.Count > 0)
				_ProgressLayers.Last().CurrentStep = step;

			Update();
		}

		public void IncrementProgress()
		{
			if (_ProgressLayers.Count > 0)
				_ProgressLayers.Last().CurrentStep++;

			Update();
		}

		public void PushProgress(int step, int total)
		{
			_ProgressLayers.Add(new ProgressLayer{ CurrentStep = step, TotalSteps = total });
			Update();
		}

		public void PopProgress()
		{
			if (_ProgressLayers.Count > 0)
			{
				if (_ProgressLayers.Count > 1)
					_ProgressLayers.RemoveAt(_ProgressLayers.Count-1);
				if (_ProgressLayers.Count > 0)
					_ProgressLayers.Last().CurrentStep++;
			}
			Update();		
		}

		public void WriteLine(string text)
		{
			if (String.IsNullOrEmpty(text) == false)
				_Text.AppendText(String.Format("{0}\n", text));
		}

		protected override void OnClosing(CancelEventArgs e)
		{
			e.Cancel = !_CloseButton.IsEnabled;
			base.OnClosing(e);
		}

		protected void OnCloseButtonClick(object sender, RoutedEventArgs e)
		{
			Close();
		}

		private void NotifyPropertyChanged([CallerMemberName] String propertyName = "")
		{
			if (PropertyChanged != null)
				PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
		}

		protected void OnDetailsToggled(object sender, RoutedEventArgs e)
		{
			UpdateDetailsLayout();
		}

		private void OnLoaded(object sender, RoutedEventArgs e)
		{
			MinWidth = Width;

			_CollapsedHeight = Height;
			_ExpandedMinHeight = Height + 180;
			_ExpandedHeight = _ExpandedMinHeight;

			UpdateDetailsLayout();
		}

		private void UpdateDetailsLayout()
		{
			if (_DetailsExpander.IsExpanded)
			{
				MinHeight = _ExpandedMinHeight;
				MaxHeight = double.PositiveInfinity;
				Height = _ExpandedHeight;
				ResizeMode = ResizeMode.CanResizeWithGrip;
			}
			else
			{
				_ExpandedHeight = Height;
				MinHeight = _CollapsedHeight;
				MaxHeight = _CollapsedHeight;
				Height = _CollapsedHeight;
				ResizeMode = ResizeMode.CanResize;
			}
		}
		
	}

	public class ProgressLayer
	{
		public int	CurrentStep;
		public int	TotalSteps;
	}
}
