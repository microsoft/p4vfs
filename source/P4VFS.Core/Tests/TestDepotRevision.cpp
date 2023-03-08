// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "TestFactory.h"
#include "DepotRevision.h"
#include "DepotClient.h"

using namespace Microsoft::P4VFS::FileCore;
using namespace Microsoft::P4VFS::TestCore;
using namespace Microsoft::P4VFS::P4;

void TestDepotRevisionChangelist(const TestContext& context)
{
	int32_t changelist = 10;
	DepotString revisionString = StringInfo::Format("@%d", changelist);
	DepotRevision revision = FDepotRevision::FromString(revisionString);
	Assert(revision.get() != nullptr);
	Assert(!revision->IsNoneRevision() && !revision->IsHaveRevision() && !revision->IsHeadRevision());

	FDepotRevisionChangelist* clRevision = FDepotRevision::Cast<FDepotRevisionChangelist*>(revision.get());
	Assert(clRevision != nullptr);
	Assert(changelist == clRevision->m_Value);
	Assert(revisionString == clRevision->ToString());
}

void TestDepotRevisionNumber(const TestContext& context)
{
	int32_t revisionNumber = 10;
	DepotString revisionString = StringInfo::Format("#%d", revisionNumber);
	DepotRevision revision = FDepotRevision::FromString(revisionString);
	Assert(revision.get() != nullptr);
	Assert(!revision->IsNoneRevision() && !revision->IsHaveRevision() && !revision->IsHeadRevision());

	FDepotRevisionNumber* numRevision = FDepotRevision::Cast<FDepotRevisionNumber*>(revision.get());
	Assert(numRevision != nullptr);
	Assert(revisionNumber == numRevision->m_Value);
	Assert(revisionString == numRevision->ToString());
}

void TestDepotNoneRevision(const TestContext& context)
{
	DepotString revisionString = "#none";
	DepotRevision revision = FDepotRevision::FromString(revisionString);
	Assert(revision.get() != nullptr);
	Assert(revision->IsNoneRevision() && !revision->IsHaveRevision() && !revision->IsHeadRevision());
		
	FDepotRevisionNone* noneRevision = FDepotRevision::Cast<FDepotRevisionNone*>(revision.get());
	Assert(noneRevision != nullptr);
	Assert(revisionString == noneRevision->ToString());
}

void TestDepotHaveRevision(const TestContext& context)
{
	DepotString revisionString = "#have";
	DepotRevision revision = FDepotRevision::FromString(revisionString);
	Assert(revision.get() != nullptr);
	Assert(!revision->IsNoneRevision() && revision->IsHaveRevision() && !revision->IsHeadRevision());
		
	FDepotRevisionHave* haveRevision = FDepotRevision::Cast<FDepotRevisionHave*>(revision.get());
	Assert(haveRevision != nullptr);
	Assert(revisionString == haveRevision->ToString());
}

void TestDepotHeadRevision(const TestContext& context)
{
	DepotString revisionString = "#head";
	DepotRevision revision = FDepotRevision::FromString(revisionString);
	Assert(revision.get() != nullptr);
	Assert(!revision->IsNoneRevision() && !revision->IsHaveRevision() && revision->IsHeadRevision());

	FDepotRevisionHead* headRevision = FDepotRevision::Cast<FDepotRevisionHead*>(revision.get());
	Assert(headRevision != nullptr);
	Assert(revisionString == headRevision->ToString());
}

void TestDepotNowRevision(const TestContext& context)
{
	DepotString revisionString = "@now";
	DepotRevision revision = FDepotRevision::FromString(revisionString);
	Assert(revision.get() != nullptr);

	FDepotRevisionNow* nowRevision = FDepotRevision::Cast<FDepotRevisionNow*>(revision.get());
	Assert(nowRevision != nullptr);
	Assert(revisionString == nowRevision->ToString());
}

void TestDepotRevisionRange(const TestContext& context)
{
	// Test file revision ranges
	{
		int32_t starting = 10;
		int32_t ending = 20;
		DepotString revisionString0 = StringInfo::Format("#%d,%d", starting, ending);
		DepotString revisionString1 = StringInfo::Format("#%d,#%d", starting, ending);
		DepotRevision revision0 = FDepotRevision::FromString(revisionString0);
		DepotRevision revision1 = FDepotRevision::FromString(revisionString1);
		Assert(revision0.get() != nullptr && revision0.get()->GetType() == DepotRevisionType::Range);
		Assert(revision1.get() != nullptr && revision1.get()->GetType() == DepotRevisionType::Range);
		Assert(!revision0->IsNoneRevision() && !revision0->IsHaveRevision() && !revision0->IsHeadRevision());
		Assert(revision0->ToString() == revisionString1);
		Assert(revision1->ToString() == revisionString1);

		FDepotRevisionRange* rangeRevision = FDepotRevision::Cast<FDepotRevisionRange*>(revision1.get());
		Assert(rangeRevision != nullptr);
		Assert(rangeRevision->m_Start.get() != nullptr && rangeRevision->m_Start->ToString() == StringInfo::Format("#%d", starting));
		Assert(rangeRevision->m_End.get() != nullptr && rangeRevision->m_End->ToString() == StringInfo::Format("#%d", ending));

		FDepotRevisionNumber* startingRevision = FDepotRevision::Cast<FDepotRevisionNumber*>(rangeRevision->m_Start.get());
		Assert(startingRevision != nullptr);
		FDepotRevisionNumber* endingRevision = FDepotRevision::Cast<FDepotRevisionNumber*>(rangeRevision->m_End.get());
		Assert(endingRevision != nullptr);

		Assert(starting == startingRevision->m_Value);
		Assert(ending == endingRevision->m_Value);
		Assert(startingRevision->ToString() == StringInfo::Format("#%d", starting));
		Assert(endingRevision->ToString() == StringInfo::Format("#%d", ending));
	}
	// Test changelist ranges
	{
		int32_t starting = 2389;
		int32_t ending = 4569;
		DepotString revisionString0 = StringInfo::Format("@%d,%d", starting, ending);
		DepotString revisionString1 = StringInfo::Format("@%d,@%d", starting, ending);
		DepotRevision revision0 = FDepotRevision::FromString(revisionString0);
		DepotRevision revision1 = FDepotRevision::FromString(revisionString1);
		Assert(revision0.get() != nullptr && revision0.get()->GetType() == DepotRevisionType::Range);
		Assert(revision1.get() != nullptr && revision1.get()->GetType() == DepotRevisionType::Range);
		Assert(!revision0->IsNoneRevision() && !revision0->IsHaveRevision() && !revision0->IsHeadRevision());
		Assert(revision0->ToString() == revisionString1);
		Assert(revision1->ToString() == revisionString1);

		FDepotRevisionRange* rangeRevision = FDepotRevision::Cast<FDepotRevisionRange*>(revision1.get());
		Assert(rangeRevision != nullptr);
		Assert(rangeRevision->m_Start.get() != nullptr && rangeRevision->m_Start->ToString() == StringInfo::Format("@%d", starting));
		Assert(rangeRevision->m_End.get() != nullptr && rangeRevision->m_End->ToString() == StringInfo::Format("@%d", ending));

		FDepotRevisionChangelist* startingRevision = FDepotRevision::Cast<FDepotRevisionChangelist*>(rangeRevision->m_Start.get());
		Assert(startingRevision != nullptr);
		FDepotRevisionChangelist* endingRevision = FDepotRevision::Cast<FDepotRevisionChangelist*>(rangeRevision->m_End.get());
		Assert(endingRevision != nullptr);

		Assert(starting == startingRevision->m_Value);
		Assert(ending == endingRevision->m_Value);
		Assert(startingRevision->ToString() == StringInfo::Format("@%d", starting));
		Assert(endingRevision->ToString() == StringInfo::Format("@%d", ending));
	}
	// Test single changelist range
	{
		int32_t startingEnding = 2389;
		DepotString revisionString0 = StringInfo::Format("@%d,%d", startingEnding, startingEnding);
		DepotString revisionString1 = StringInfo::Format("@%d,@%d", startingEnding, startingEnding);
		DepotString revisionString2 = StringInfo::Format("@=%d", startingEnding);
		DepotRevision revision0 = FDepotRevision::FromString(revisionString0);
		DepotRevision revision1 = FDepotRevision::FromString(revisionString1);
		DepotRevision revision2 = FDepotRevision::FromString(revisionString2);
		Assert(revision0.get() != nullptr && revision0.get()->GetType() == DepotRevisionType::Range);
		Assert(revision1.get() != nullptr && revision1.get()->GetType() == DepotRevisionType::Range);
		Assert(revision2.get() != nullptr && revision2.get()->GetType() == DepotRevisionType::Range);
		Assert(!revision0->IsNoneRevision() && !revision0->IsHaveRevision() && !revision0->IsHeadRevision());
		Assert(revision0->ToString() == revisionString1);
		Assert(revision1->ToString() == revisionString1);
		Assert(revision2->ToString() == revisionString1);

		FDepotRevisionRange* rangeRevision = FDepotRevision::Cast<FDepotRevisionRange*>(revision1.get());
		Assert(rangeRevision != nullptr);
		Assert(rangeRevision->m_Start.get() != nullptr && rangeRevision->m_Start->ToString() == StringInfo::Format("@%d", startingEnding));
		Assert(rangeRevision->m_End.get() != nullptr && rangeRevision->m_End->ToString() == rangeRevision->m_Start->ToString());

		FDepotRevisionChangelist* startingRevision = FDepotRevision::Cast<FDepotRevisionChangelist*>(rangeRevision->m_Start.get());
		Assert(startingRevision != nullptr);
		FDepotRevisionChangelist* endingRevision = FDepotRevision::Cast<FDepotRevisionChangelist*>(rangeRevision->m_End.get());
		Assert(endingRevision != nullptr);

		Assert(startingEnding == startingRevision->m_Value);
		Assert(startingEnding == endingRevision->m_Value);
		Assert(startingRevision->ToString() == StringInfo::Format("@%d", startingEnding));
		Assert(endingRevision->ToString() == startingRevision->ToString());
	}
}

void TestDepotRevisionLabel(const TestContext& context)
{
	DepotString label = "mylabel";
	DepotString revisionString = StringInfo::Format("@%s", label.c_str());
	DepotRevision revision = FDepotRevision::FromString(revisionString);
	Assert(revision.get() != nullptr);

	FDepotRevisionLabel* labelRevision = FDepotRevision::Cast<FDepotRevisionLabel*>(revision.get());
	Assert(labelRevision != nullptr);
	Assert(label == labelRevision->m_Value);
	Assert(revisionString == labelRevision->ToString());
}

void TestDepotRevisionDate(const TestContext& context)
{
	DepotString dateString = "2019/08/15:11:24:45";
	time_t datetime = DepotInfo::StringToTime(dateString);
	DepotString revisionString = StringInfo::Format("@%s", dateString.c_str());
	DepotRevision revision = FDepotRevision::FromString(revisionString);
	Assert(revision.get() != nullptr);

	FDepotRevisionDate* dateRevision = FDepotRevision::Cast<FDepotRevisionDate*>(revision.get());
	Assert(dateRevision != nullptr);
	Assert(datetime == dateRevision->m_Value);
	Assert(revisionString == dateRevision->ToString());
}

void TestDepotRevisionDateShort(const TestContext& context)
{
	DepotString dateString = "2019/08/15";
	time_t datetime = DepotInfo::StringToTime(dateString);
	DepotString revisionString = StringInfo::Format("@%s", dateString.c_str());
	DepotRevision revision = FDepotRevision::FromString(revisionString);
	Assert(revision.get() != nullptr);

	FDepotRevisionDate* dateRevision = FDepotRevision::Cast<FDepotRevisionDate*>(revision.get());
	Assert(dateRevision != nullptr);
	Assert(datetime == dateRevision->m_Value);
	Assert(dateRevision->ToString() == revisionString+":00:00:00");
}
