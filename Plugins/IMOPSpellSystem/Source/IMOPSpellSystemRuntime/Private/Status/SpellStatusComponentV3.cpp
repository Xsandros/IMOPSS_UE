#include "Status/SpellStatusComponentV3.h"

USpellStatusComponentV3::USpellStatusComponentV3()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

bool USpellStatusComponentV3::HasStatus(FGameplayTag StatusTag) const
{
	if (!StatusTag.IsValid()) return false;
	for (const FActiveStatusV3& S : Active)
	{
		if (S.StatusTag == StatusTag) return true;
	}
	return false;
}

int32 USpellStatusComponentV3::GetStacks(FGameplayTag StatusTag) const
{
	if (!StatusTag.IsValid()) return 0;
	for (const FActiveStatusV3& S : Active)
	{
		if (S.StatusTag == StatusTag) return S.Stacks;
	}
	return 0;
}

bool USpellStatusComponentV3::UpsertStatus(const FActiveStatusV3& NewValue, EStatusStackPolicyV3 Policy, int32 MaxStacks)
{
	if (!NewValue.StatusTag.IsValid()) return false;

	for (FActiveStatusV3& Existing : Active)
	{
		if (Existing.StatusTag != NewValue.StatusTag) continue;

		switch (Policy)
		{
		case EStatusStackPolicyV3::IgnoreIfPresent:
			return false;

		case EStatusStackPolicyV3::Replace:
			Existing = NewValue;
			Existing.Stacks = FMath::Clamp(Existing.Stacks, 1, FMath::Max(1, MaxStacks));
			return true;

		case EStatusStackPolicyV3::RefreshDuration:
			Existing.TimeRemaining = NewValue.TimeRemaining;
			Existing.AppliedContext = NewValue.AppliedContext;
			return true;

		case EStatusStackPolicyV3::AddStacks:
			Existing.Stacks = FMath::Clamp(Existing.Stacks + NewValue.Stacks, 1, FMath::Max(1, MaxStacks));
			Existing.TimeRemaining = FMath::Max(Existing.TimeRemaining, NewValue.TimeRemaining);
			Existing.AppliedContext = NewValue.AppliedContext;
			return true;
		}
	}

	FActiveStatusV3 Insert = NewValue;
	Insert.Stacks = FMath::Clamp(Insert.Stacks, 1, FMath::Max(1, MaxStacks));
	Active.Add(Insert);
	return true;
}

bool USpellStatusComponentV3::RemoveStatus(FGameplayTag StatusTag, FActiveStatusV3* OutRemoved)
{
	if (!StatusTag.IsValid()) return false;

	for (int32 i = 0; i < Active.Num(); ++i)
	{
		if (Active[i].StatusTag == StatusTag)
		{
			if (OutRemoved) *OutRemoved = Active[i];
			Active.RemoveAtSwap(i);
			return true;
		}
	}
	return false;
}

void USpellStatusComponentV3::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USpellStatusComponentV3, Active);
}

void USpellStatusComponentV3::TickStatuses(float DeltaSeconds, TArray<FActiveStatusV3>& OutExpired)
{
	OutExpired.Reset();
	if (DeltaSeconds <= 0.f) return;

	// Stable deterministic iteration order: array order.
	for (int32 i = Active.Num() - 1; i >= 0; --i)
	{
		FActiveStatusV3& S = Active[i];

		// 0 or negative = permanent
		if (S.TimeRemaining <= 0.f)
		{
			continue;
		}

		S.TimeRemaining -= DeltaSeconds;

		if (S.TimeRemaining <= 0.f)
		{
			// Expired
			OutExpired.Add(S);
			Active.RemoveAtSwap(i);
		}
	}
}
