#include "Debug/SpellDebugBPLibraryV3.h"

#include "Events/SpellEventV3.h"

FString USpellDebugBPLibraryV3::DescribeSpellEvent(const FSpellEventV3& Ev)
{
	const AActor* Caster = Ev.Caster.Get();
	const UObject* Sender = Ev.Sender.Get();

	const FString CasterName = Caster ? Caster->GetName() : TEXT("None");
	const FString SenderName = Sender ? Sender->GetName() : TEXT("None");

	const FString TagStr = Ev.EventTag.IsValid() ? Ev.EventTag.ToString() : TEXT("None");
	const FString GuidStr = Ev.RuntimeGuid.IsValid() ? Ev.RuntimeGuid.ToString() : TEXT("Invalid");

	return FString::Printf(
		TEXT("SpellEvent Tag=%s Runtime=%s Caster=%s Sender=%s Mag=%.3f Frame=%d Time=%.3f"),
		*TagStr,
		*GuidStr,
		*CasterName,
		*SenderName,
		Ev.Magnitude,
		Ev.FrameNumber,
		Ev.TimeSeconds
	);
}
