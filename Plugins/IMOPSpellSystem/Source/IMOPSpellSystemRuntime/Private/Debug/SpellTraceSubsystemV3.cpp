#include "Debug/SpellTraceSubsystemV3.h"

void USpellTraceSubsystemV3::Record(const FSpellEventV3& Ev)
{
	if (!Ev.RuntimeGuid.IsValid())
	{
		// Non-scoped events are still useful, but we can't bucket them reliably.
		// Put them into a synthetic runtime bucket:
		// (You can remove this if you prefer dropping them.)
		const_cast<FSpellEventV3&>(Ev).RuntimeGuid = FGuid(1,0,0,0);
	}

	FSpellTraceBufferV3& Buf = EventsByRuntime.FindOrAdd(Ev.RuntimeGuid);
	TArray<FSpellEventV3>& Bucket = Buf.Events;
	Bucket.Add(Ev);
	if (Bucket.Num() > MaxEventsPerRuntime)
	{
		const int32 Overflow = Bucket.Num() - MaxEventsPerRuntime;
		Bucket.RemoveAt(0, Overflow, EAllowShrinking::No);

	}

	FSpellTraceRowV3 Row;
	Row.RuntimeGuid = Ev.RuntimeGuid;
	Row.EventTag = Ev.EventTag;
	Row.TimeSeconds = Ev.TimeSeconds;
	Row.FrameNumber = Ev.FrameNumber;
	Row.Caster = Ev.Caster;
	Row.Sender = Ev.Sender;

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
}

void USpellTraceSubsystemV3::ClearAll()
{
	EventsByRuntime.Reset();
	RecentRows.Reset();
}
