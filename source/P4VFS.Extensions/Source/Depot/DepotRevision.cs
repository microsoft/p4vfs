// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text.RegularExpressions;

namespace Microsoft.P4VFS.Extensions
{
	[Serializable]
	public abstract class DepotRevision
	{
		public static bool TryParse(string revision, out DepotRevision result)
		{
			result = DepotRevision.FromString(revision);
			return result != null;
		}

		public static DepotRevision FromString(string revisionString)
		{
			DepotRevision revision = null;
			if (String.IsNullOrWhiteSpace(revisionString) == false)
			{
				revision = DepotRevisionRange.TryParse(revisionString);
				if (revision != null)
					return revision;

				revision = DepotRevisionDate.TryParse(revisionString);
				if (revision != null)
					return revision;

				revision = DepotRevisionNumber.TryParse(revisionString);
				if (revision != null)
					return revision;

				revision = DepotRevisionNow.TryParse(revisionString);
				if (revision != null)
					return revision;

				revision = DepotRevisionNone.TryParse(revisionString);
				if (revision != null)
					return revision;

				revision = DepotRevisionChangelist.TryParse(revisionString);
				if (revision != null)
					return revision;

				revision = DepotRevisionHave.TryParse(revisionString);
				if (revision != null)
					return revision;

				revision = DepotRevisionHead.TryParse(revisionString);
				if (revision != null)
					return revision;

				revision = DepotRevisionLabel.TryParse(revisionString);
				if (revision != null)
					return revision;
			}
			return revision;
		}

		public override string ToString()
		{
			return "";
		}

		public bool IsHeadRevision()
		{
			return IsRevisionString("#head");
		}

		public bool IsHaveRevision()
		{
			return IsRevisionString("#have");
		}

		public bool IsNoneRevision()
		{
			return IsRevisionString("#none");
		}

		public bool IsRevisionString(string id)
		{
			id = (id == null ? "" : id.Trim());
			return String.Compare(ToString().Trim(), id, StringComparison.CurrentCultureIgnoreCase) == 0;
		}
	}

	/// <summary>
	/// Encapsulates a revision at change #0.
	/// </summary>
	public class DepotRevisionNone : DepotRevision
	{
		/// <summary>
		/// The string representation of this revision
		/// </summary>
		/// <returns>The string representation of this revision</returns>
		public override string ToString()
		{
			return "#none";
		}

		/// <summary>
		/// Factory method for converting a string o a DepotRevisionNone object.
		/// </summary>
		/// <param name="revisionString">The string to parse and convert. Both #none and #0 are acceptable.</param>
		/// <returns>An allocated DepotRevisionNone object on success, null on failure.</returns>
		public static DepotRevision TryParse(string revisionString)
		{
			if (revisionString.Equals("#none", StringComparison.OrdinalIgnoreCase) || revisionString.Equals("#0"))
				return new DepotRevisionNone();
			return null;
		}
	}

	public class DepotRevisionHave : DepotRevision
	{
		/// <summary>
		/// The string representation of this revision
		/// </summary>
		/// <returns>The string representation of this revision</returns>
		public override string ToString()
		{
			return "#have";
		}

		public static DepotRevision TryParse(string revisionString)
		{
			if (revisionString.Equals("#have", StringComparison.OrdinalIgnoreCase))
				return new DepotRevisionHave();
			return null;
		}
	}

	public class DepotRevisionHead : DepotRevision
	{
		/// <summary>
		/// The string representation of this revision number which is used for depot commands (ie #1)
		/// </summary>
		/// <returns>The string representation of this revision number</returns>
		public override string ToString()
		{
			return "#head";
		}

		public static DepotRevision TryParse(string revisionString)
		{
			if (revisionString.Equals("#head", StringComparison.OrdinalIgnoreCase))
				return new DepotRevisionHead();
			return null;
		}
	}

	public class DepotRevisionNumber : DepotRevision
	{
		private int _revisionNumber;

		/// <summary>
		/// This class is used to represent a file version as an
		/// integer that is incremented each time a file is changed
		/// </summary>
		/// <param name="revisionNumber">The revision number that this class represents</param>
		public DepotRevisionNumber(int revisionNumber)
		{
			this._revisionNumber = revisionNumber;
		}

		/// <summary>
		/// The integer value of this revision number
		/// </summary>
		public int Value
		{
			get {	return this._revisionNumber;		}
			set {	this._revisionNumber = value;		}
		}

		/// <summary>
		/// The string representation of this revision number which is used for depot commands (ie #1)
		/// </summary>
		/// <returns>The string representation of this revision number</returns>
		public override string ToString()
		{
			return String.Format("#{0}", this._revisionNumber);
		}

		public static DepotRevision TryParse(string revisionString)
		{
            string revisionValue = revisionString;

			Match match = new Regex(@"#(?<value>\d+)").Match(revisionString);
			if (match != null && match.Success)
            {
                revisionValue = match.Groups["value"].Value;
            }

            int revision;
            if (Int32.TryParse(revisionValue, out revision))
            {
                return new DepotRevisionNumber(revision);
            }

			return null;
		}
	}

	public class DepotRevisionLabel : DepotRevision
	{
		/// <summary>
		/// This class is used to represent a file version associated with a special variant, or label
		/// </summary>
		/// <param name="label">The label that this class represents</param>
		/// <remarks>Use DepotRevisionNumber, DepotRevisionChangelist, and DepotRevisionDate over this class when possible.</remarks>
		public DepotRevisionLabel(string label)
		{
			this.Value = label;
		}

		/// <summary>
		/// The integer value of this revision number
		/// </summary>
		public string Value { get; protected set; }

		/// <summary>
		/// The string representation of this revision label which is used for depot commands (ie #1)
		/// </summary>
		/// <returns>The string representation of this revision label</returns>
		public override string ToString()
		{
			return String.Format("@{0}", this.Value);
		}

		public static DepotRevision TryParse(string revisionString)
		{
			Match match = new Regex(@"@(?<label>.+)").Match(revisionString);
			if (match != null && match.Success)
				return new DepotRevisionLabel(match.Groups["label"].Value);
			return null;
		}
	}

	public class DepotRevisionRange : DepotRevision
	{
		public DepotRevisionRange(DepotRevision startingDepotRevision, DepotRevision endingDepotRevision)
		{
			this.StartingDepotRevision = startingDepotRevision;
			this.EndingDepotRevision = endingDepotRevision;
		}

		public DepotRevision StartingDepotRevision { get; protected set; }
		public DepotRevision EndingDepotRevision { get; protected set; }

		public override string ToString()
		{
			string result;
			if (this.StartingDepotRevision != null && this.EndingDepotRevision != null)
			{
				result = String.Format("{0},{1}", this.StartingDepotRevision, this.EndingDepotRevision.ToString().TrimStart('@', '#'));
			}
			else if (this.StartingDepotRevision != null && this.EndingDepotRevision == null)
			{
				result = String.Format("{0},", this.StartingDepotRevision);
			}
			else if (this.StartingDepotRevision == null && this.EndingDepotRevision != null)
			{
				result = String.Format("{0}", this.EndingDepotRevision);
			}
			else
			{
				result = String.Empty;
			}
			return result;
		}

		public static DepotRevision TryParse(string revisionString)
		{
			Match match = new Regex(@"(?<startingrevision>[^,].+),(?<endingrevision>.+)").Match(revisionString);
			if (match != null && match.Success)
			{
				DepotRevision startingDepotRevision = DepotRevision.FromString(match.Groups["startingrevision"].Value);
				DepotRevision endingDepotRevision = DepotRevision.FromString(match.Groups["endingrevision"].Value);
				if (startingDepotRevision != null && endingDepotRevision != null)
					return new DepotRevisionRange(startingDepotRevision, endingDepotRevision);
			}
			return null;
		}
	}

	public class DepotRevisionChangelist : DepotRevision
	{
		/// <summary>
		/// This class is used to represent a file version as an integer
		/// that is incremented for each changelist submitted to the depot
		/// </summary>
		/// <param name="changelistNumber">The changelist number that
		/// this class represents</param>
		public DepotRevisionChangelist(int changelistNumber)
		{
			this.Value = changelistNumber;
		}

		/// <summary>
		/// The integer value of this changelist number
		/// </summary>
		public int Value { get; protected set; }

		/// <summary>
		/// The string representation of this changelist number which is used for depot commands (ie @1)
		/// </summary>
		/// <returns>The string representation of this changelist number</returns>
		public override string ToString()
		{
			return String.Format("@{0}", this.Value);
		}

		public static DepotRevision TryParse(string revisionString)
		{
			Match match = new Regex(@"@(?<value>\d+)").Match(revisionString);
			if (match != null && match.Success)
				return new DepotRevisionChangelist(Int32.Parse(match.Groups["value"].Value));
			return null;
		}
	}

	public class DepotRevisionNow : DepotRevision
	{
		/// <summary>
		/// The string representation of the revision at the current time which is used for depot commands (ie #1)
		/// </summary>
		/// <returns>The string representation of the revision at the current time</returns>
		public override string ToString()
		{
			return "@now";
		}

		public static DepotRevision TryParse(string revisionString)
		{
			if (revisionString.Equals("@now", StringComparison.OrdinalIgnoreCase))
				return new DepotRevisionNow();
			return null;
		}
	}

	public class DepotRevisionDate : DepotRevision
	{
        private const string PerforceDateFormat = @"yyyy'/'MM'/'dd':'HH':'mm':'ss";

        private static readonly string[] DateFormatStrings = new string[] {
            PerforceDateFormat,
            @"yyyy'/'MM'/'dd"
        };

		public DepotRevisionDate(DateTime date)
		{
			this.Value = date;
		}

		/// <summary>
		/// The date value of this date revision.
		/// </summary>
		public DateTime Value { get; protected set; }

		/// <summary>
		/// The string representation of this date revision which is used for depot commands (ie @2019/01/07:10:01:59)
		/// </summary> yyyy/MM/dd:hh:mm:ss
		/// <returns>The string representation of a date revision</returns>
		public override string ToString()
		{
            return $"@{this.Value.ToString(PerforceDateFormat)}";
		}

		public static DepotRevision TryParse(string revisionString)
		{
            // Perforce only accepts one datetime format, we prefer that if possible, but if not, we try to parse in the current culture, just for convenience
            if (String.IsNullOrEmpty(revisionString))
            {
                return null;
            }

            var dateString = revisionString.Trim();
            if (!dateString.StartsWith("@"))
            {
                return null;
            }

            dateString = dateString.Substring(1);
            DateTime date;
            if (DateTime.TryParseExact(dateString, DateFormatStrings, CultureInfo.InvariantCulture, DateTimeStyles.None, out date) ||
                DateTime.TryParse(dateString, CultureInfo.CurrentCulture, DateTimeStyles.None, out date))
            {
                return new DepotRevisionDate(date);
            }

			return null;
		}
	}
}
