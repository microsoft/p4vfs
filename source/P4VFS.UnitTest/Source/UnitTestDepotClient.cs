// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using System.Globalization;
using Microsoft.P4VFS.Extensions;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Microsoft.P4VFS.UnitTest
{
	[TestClass, BasePriority(7000)]
	public class UnitTestDepotClient : UnitTestBase
	{
		[TestMethod, Priority(0)]
		public void ParseRevisionChangelistTest()
		{
			var changelist = 10;
			var revisionString = $"@{changelist}";
			var revision = DepotRevision.FromString(revisionString);
			var clRevision = revision as DepotRevisionChangelist;

			Assert(clRevision != null);
			Assert(changelist == clRevision.Value);
			Assert(revisionString == clRevision.ToString());
		}

		[TestMethod, Priority(1)]
		public void ParseRevisionNumberTest()
		{
			var revisionNumber = 10;
			var revisionString = $"#{revisionNumber}";
			var revision = DepotRevision.FromString(revisionString);
			var numRevision = revision as DepotRevisionNumber;

			Assert(numRevision != null);
			Assert(revisionNumber == numRevision.Value);
			Assert(revisionString == numRevision.ToString());
		}

		[TestMethod, Priority(2)]
		public void ParseNoneRevisionTest()
		{
			var revisionString = "#none";
			var revision = DepotRevision.FromString(revisionString);
			var noneRevision = revision as DepotRevisionNone;

			Assert(noneRevision != null);
			Assert(revisionString == noneRevision.ToString());
		}

		[TestMethod]
		public void ParseHaveRevisionTest()
		{
			var revisionString = "#have";
			var revision = DepotRevision.FromString(revisionString);
			var haveRevision = revision as DepotRevisionHave;

			Assert(haveRevision != null);
			Assert(revisionString == haveRevision.ToString());
		}

		[TestMethod, Priority(3)]
		public void ParseHeadRevisionTest()
		{
			var revisionString = "#head";
			var revision = DepotRevision.FromString(revisionString);
			var headRevision = revision as DepotRevisionHead;

			Assert(headRevision != null);
			Assert(revisionString == headRevision.ToString());
		}

		[TestMethod, Priority(4)]
		public void ParseNowRevisionTest()
		{
			var revisionString = "@now";
			var revision = DepotRevision.FromString(revisionString);
			var nowRevision = revision as DepotRevisionNow;

			Assert(nowRevision != null);
			Assert(revisionString == nowRevision.ToString());
		}

		[TestMethod, Priority(5)]
		public void ParseRevisionRangeTest()
		{
			var starting = 10;
			var ending = 20;
			var revisionString = $"#{starting},{ending}";
			var revision = DepotRevision.FromString(revisionString);
			var rangeRevision = revision as DepotRevisionRange;

			Assert(rangeRevision != null);
			Assert(revisionString == rangeRevision.ToString());

			var startingRevision = rangeRevision.StartingDepotRevision as DepotRevisionNumber;
			var endingRevision = rangeRevision.EndingDepotRevision as DepotRevisionNumber;
			var startingRevisionString = $"#{starting}";
			var endingRevisionString = $"#{ending}";

			Assert(startingRevision != null);
			Assert(endingRevision != null);
			Assert(starting == startingRevision.Value);
			Assert(ending == endingRevision.Value);
			Assert(startingRevisionString == startingRevision.ToString());
			Assert(endingRevisionString == endingRevision.ToString());
		}

		[TestMethod, Priority(6)]
		public void ParseRevisionLabelTest()
		{
			var label = "mylabel";
			var revisionString = $"@{label}";
			var revision = DepotRevision.FromString(revisionString);
			var labelRevision = revision as DepotRevisionLabel;

			Assert(labelRevision != null);
			Assert(label == labelRevision.Value);
			Assert(revisionString == labelRevision.ToString());
		}

		[TestMethod, Priority(7)]
		public void ParseRevisionDateTest()
		{
			var dateString = "2019/08/01:11:24:11";
			var date = DateTime.ParseExact(dateString, "yyyy/MM/dd:HH:mm:ss", CultureInfo.InvariantCulture);
			var revisionString = $"@{dateString}";
			var revision = DepotRevision.FromString(revisionString);
			var labelRevision = revision as DepotRevisionDate;

			Assert(labelRevision != null);
			Assert(date == labelRevision.Value);
			Assert(revisionString == labelRevision.ToString());
		}

		[TestMethod, Priority(8)]
		public void ParseRevisionDateWithSpaceTest()
		{
			var dateString = "2019/12/07 10:59:59";
			var date = DateTime.ParseExact(dateString, "yyyy/MM/dd HH:mm:ss", CultureInfo.InvariantCulture);
			var revisionString = $"@{date:yyyy'/'MM'/'dd':'HH':'mm':'ss}";
			var revision = DepotRevision.FromString(revisionString);
			var labelRevision = revision as DepotRevisionDate;

			Assert(labelRevision != null);
			Assert(date == labelRevision.Value);
			Assert(revisionString == labelRevision.ToString());
		}

		[TestMethod, Priority(9)]
		public void ParseRevisionDateWithLocaleTest()
		{
			var dateString = "2019-12-07 10:59:59";
			var date = DateTime.ParseExact(dateString, "yyyy-MM-dd HH:mm:ss", CultureInfo.InvariantCulture);
			var revisionString = $"@{date:yyyy'/'MM'/'dd':'HH':'mm':'ss}";
			var revision = DepotRevision.FromString(revisionString);
			var labelRevision = revision as DepotRevisionDate;

			Assert(labelRevision != null);
			Assert(date == labelRevision.Value);
			Assert(revisionString == labelRevision.ToString());
		}
	}
}