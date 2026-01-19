#include "Debug/SpellDebugBPLibraryV3.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

USpellTraceSubsystemV3* USpellDebugBPLibraryV3::GetTraceSubsystem(UObject* WorldContextObject)
{
	if (!WorldContextObject) return nullptr;
	if (UWorld* W = WorldContextObject->GetWorld())
	{
		if (UGameInstance* GI = W->GetGameInstance())
		{
			return GI->GetSubsystem<USpellTraceSubsystemV3>();
		}
	}
	return nullptr;
}

FString USpellDebugBPLibraryV3::FormatEventShort(const FSpellEventV3& Ev)
{
	const FString CasterName = Ev.Caster ? Ev.Caster->GetName() : TEXT("None");
	const FString SenderName = Ev.Sender ? Ev.Sender->GetName() : TEXT("None");
	return FString::Printf(TEXT("[%s] t=%.2f f=%d caster=%s sender=%s"),
		*Ev.EventTag.ToString(),
		Ev.TimeSeconds,
		Ev.FrameNumber,
		*CasterName,
		*SenderName);
}

FString USpellDebugBPLibraryV3::FormatRowShort(const FSpellTraceRowV3& Row)
{
	return FString::Printf(TEXT("[%s] t=%.2f f=%d guid=%s"),
		*Row.EventTag.ToString(),
		Row.TimeSeconds,
		Row.FrameNumber,
		*Row.RuntimeGuid.ToString(EGuidFormats::Short));
}
