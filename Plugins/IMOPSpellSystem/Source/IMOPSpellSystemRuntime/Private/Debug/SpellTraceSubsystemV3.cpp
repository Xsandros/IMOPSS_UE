#include "Debug/SpellTraceSubsystemV3.h"
#include "Algo/Reverse.h"



void USpellTraceSubsystemV3::Record(const FSpellEventV3& Ev)
{
	// Make a safe copy (never mutate const ref)
	FSpellEventV3 EvCopy = Ev;

	if (!EvCopy.RuntimeGuid.IsValid())
	{
		// Non-scoped events: bucket into synthetic runtime
		EvCopy.RuntimeGuid = FGuid(1,0,0,0);
	}

	// Optional: dedup identical events in same frame (safety net)
	if (RecentRows.Num() > 0)
	{
		const FSpellTraceRowV3& Last = RecentRows.Last();
		if (Last.RuntimeGuid == EvCopy.RuntimeGuid &&
			Last.EventTag == EvCopy.EventTag &&
			Last.FrameNumber == EvCopy.FrameNumber)
		{
			return;
		}
	}

	// Revision bumps only when we really record something
	++Revision;

	FSpellTraceBufferV3& Buf = EventsByRuntime.FindOrAdd(EvCopy.RuntimeGuid);
	TArray<FSpellEventV3>& Bucket = Buf.Events;

	Bucket.Add(EvCopy);
	if (Bucket.Num() > MaxEventsPerRuntime)
	{
		const int32 Overflow = Bucket.Num() - MaxEventsPerRuntime;
		Bucket.RemoveAt(0, Overflow, EAllowShrinking::No);
	}

	FSpellTraceRowV3 Row;
	Row.RuntimeGuid  = EvCopy.RuntimeGuid;
	Row.EventTag     = EvCopy.EventTag;
	Row.TimeSeconds  = EvCopy.TimeSeconds;
	Row.FrameNumber  = EvCopy.FrameNumber;
	Row.Caster       = EvCopy.Caster;
	Row.Sender       = EvCopy.Sender;

	RecentRows.Add(Row);

	const int32 MaxRows = MaxEventsPerRuntime * FMath::Max(1, MaxRecentRuntimes);
	if (RecentRows.Num() > MaxRows)
	{
		const int32 Overflow = RecentRows.Num() - MaxRows;
		RecentRows.RemoveAt(0, Overflow, EAllowShrinking::No);
	}
}


void USpellTraceSubsystemV3::GetRecentRows(TArray<FSpellTraceRowV3>& OutRows) const
{
	OutRows = RecentRows;
}

void USpellTraceSubsystemV3::GetEventsForRuntime(const FGuid& RuntimeGuid, TArray<FSpellEventV3>& OutEvents) const
{
	OutEvents.Reset();
	if (const FSpellTraceBufferV3* Found = EventsByRuntime.Find(RuntimeGuid))
	{
		OutEvents = Found->Events;
	}

}

void USpellTraceSubsystemV3::ClearRuntime(const FGuid& RuntimeGuid)
{
	EventsByRuntime.Remove(RuntimeGuid);
	RecentRows.RemoveAll([&](const FSpellTraceRowV3& R){ return R.RuntimeGuid == RuntimeGuid; });
	++Revision;
}

void USpellTraceSubsystemV3::ClearAll()
{
	EventsByRuntime.Reset();
	RecentRows.Reset();
	++Revision;
}

bool USpellTraceSubsystemV3::GetLatestRuntimeGuid(FGuid& OutRuntimeGuid) const
{
	for (int32 i = RecentRows.Num() - 1; i >= 0; --i)
	{
		const FGuid& G = RecentRows[i].RuntimeGuid;
		if (G.IsValid())
		{
			OutRuntimeGuid = G;
			return true;
		}
	}
	OutRuntimeGuid.Invalidate();
	return false;
}

void USpellTraceSubsystemV3::GetRecentRowsFiltered(const FGuid& RuntimeGuid, bool bFilterByRuntime, int32 MaxRows, TArray<FSpellTraceRowV3>& OutRows) const
{
	OutRows.Reset();

	const int32 Wanted = (MaxRows > 0) ? MaxRows : TNumericLimits<int32>::Max();

	for (int32 i = RecentRows.Num() - 1; i >= 0; --i)
	{
		const FSpellTraceRowV3& Row = RecentRows[i];
		if (bFilterByRuntime && Row.RuntimeGuid != RuntimeGuid) continue;

		OutRows.Add(Row);
		if (OutRows.Num() >= Wanted) break;
	}

	Algo::Reverse(OutRows);
}
