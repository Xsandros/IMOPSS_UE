#include "Status/SpellStatusSubsystemV3.h"

#include "EngineUtils.h"
#include "Status/SpellStatusComponentV3.h"
#include "GameFramework/Actor.h"

bool USpellStatusSubsystemV3::GetStatusComponent(AActor* Target, USpellStatusComponentV3*& OutComp) const
{
	OutComp = Target ? Target->FindComponentByClass<USpellStatusComponentV3>() : nullptr;
	return OutComp != nullptr;
}

bool USpellStatusSubsystemV3::ApplyStatus(
	const FSpellExecContextV3& ExecCtx,
	AActor* Target,
	const FSpellStatusDefinitionV3& Def,
	const FEffectContextV3& Context,
	float DurationOverride,
	int32 StacksOverride,
	FActiveStatusV3& OutApplied)
{
	OutApplied = FActiveStatusV3{};

	if (!ExecCtx.bAuthority || !Target || !Def.StatusTag.IsValid())
	{
		return false;
	}

	USpellStatusComponentV3* Comp = nullptr;
	if (!GetStatusComponent(Target, Comp))
	{
		// "final" choice: status requires component present (explicit opt-in).
		// Alternative: auto-add component (but that is usually undesirable in MP).
		return false;
	}

	const float Duration = (DurationOverride >= 0.f) ? DurationOverride : Def.DefaultDuration;
	const int32 Stacks = (StacksOverride > 0) ? StacksOverride : 1;

	FActiveStatusV3 NewS;
	NewS.StatusTag = Def.StatusTag;
	NewS.Stacks = Stacks;
	NewS.TimeRemaining = Duration;
	NewS.AppliedContext = Context;

	const bool bChanged = Comp->UpsertStatus(NewS, Def.StackPolicy, Def.MaxStacks);
	if (bChanged)
	{
		OutApplied = NewS;
	}
	return bChanged;
}

bool USpellStatusSubsystemV3::RemoveStatus(const FSpellExecContextV3& ExecCtx, AActor* Target, FGameplayTag StatusTag)
{
	if (!ExecCtx.bAuthority || !Target || !StatusTag.IsValid())
	{
		return false;
	}

	USpellStatusComponentV3* Comp = nullptr;
	if (!GetStatusComponent(Target, Comp))
	{
		return false;
	}

	return Comp->RemoveStatus(StatusTag);
}

void USpellStatusSubsystemV3::HandleExpiry(AActor* Target, const FActiveStatusV3& Status)
{
	// Hook point for future: events, handler interface, cross-spell reactions, etc.
	// For now, purely removes.
	if (!Target) return;

	USpellStatusComponentV3* Comp = nullptr;
	if (GetStatusComponent(Target, Comp))
	{
		Comp->RemoveStatus(Status.StatusTag);
	}
}

void USpellStatusSubsystemV3::Tick(float DeltaSeconds)
{
	UWorld* W = GetWorld();
	if (!W) return;

	// Server-authoritative expiry
	if (W->GetNetMode() == NM_Client) return;

	for (TActorIterator<AActor> It(W); It; ++It)
	{
		AActor* A = *It;

		USpellStatusComponentV3* Comp = nullptr;
		if (!GetStatusComponent(A, Comp)) continue;

		TArray<FActiveStatusV3> Expired;
		Comp->TickStatuses(DeltaSeconds, Expired);

		// Hook point for events/handlers later:
		// - Emit Event.Status.Expired
		// - Call ISpellStatusHandlerV3
		for (const FActiveStatusV3& S : Expired)
		{
			HandleExpiry(A, S);
		}
	}
}
