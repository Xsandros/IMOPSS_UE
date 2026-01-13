#include "Targeting/TargetingSpatialHelpersV3.h"

#include "Actions/SpellActionExecutorV3.h"
#include "Stores/SpellTargetStoreV3.h"
#include "GameFramework/Actor.h"

static uint32 TieBreakerId(const AActor* A)
{
	// stabiler Tie-Breaker
	return A ? (uint32)A->GetUniqueID() : 0u;
}

FVector FTargetingSpatialHelpersV3::ResolveOriginLocation(const FSpellExecContextV3& Ctx, const FTargetOriginV3& Origin)
{
	switch (Origin.Kind)
	{
	case ETargetOriginKindV3::Caster:
		if (AActor* C = Ctx.GetCaster()) return C->GetActorLocation();
		return FVector::ZeroVector;

	case ETargetOriginKindV3::CasterView:
		if (AActor* C = Ctx.GetCaster())
		{
			// final-first: view ist game-spezifisch; default = actor location
			return C->GetActorLocation();
		}
		return FVector::ZeroVector;

	case ETargetOriginKindV3::Actor:
		if (Origin.Actor.IsValid()) return Origin.Actor->GetActorLocation();
		return FVector::ZeroVector;

	case ETargetOriginKindV3::WorldLocation:
		return Origin.WorldLocation;

	case ETargetOriginKindV3::TargetSetCenter:
		if (Ctx.TargetStore)
		{
			if (const FTargetSetV3* Set = Ctx.TargetStore->Find(Origin.TargetSetKey))
			{
				return ComputeCenter(Set->Targets);
			}
		}
		return FVector::ZeroVector;

	default:
		return FVector::ZeroVector;
	}
}

void FTargetingSpatialHelpersV3::SortByDistanceTo(TArray<FTargetRefV3>& InOut, const FVector& OriginLoc, bool bAscending)
{
	InOut.Sort([&](const FTargetRefV3& A, const FTargetRefV3& B)
	{
		const AActor* AA = A.Actor.Get();
		const AActor* BA = B.Actor.Get();

		const float DA = AA ? FVector::DistSquared(AA->GetActorLocation(), OriginLoc) : BIG_NUMBER;
		const float DB = BA ? FVector::DistSquared(BA->GetActorLocation(), OriginLoc) : BIG_NUMBER;

		if (DA == DB)
		{
			const uint32 TA = TieBreakerId(AA);
			const uint32 TB = TieBreakerId(BA);
			return bAscending ? (TA < TB) : (TA > TB);
		}

		return bAscending ? (DA < DB) : (DA > DB);
	});
}

FVector FTargetingSpatialHelpersV3::ComputeCenter(const TArray<FTargetRefV3>& Targets)
{
	FVector Sum = FVector::ZeroVector;
	int32 Count = 0;

	for (const FTargetRefV3& R : Targets)
	{
		if (AActor* A = R.Actor.Get())
		{
			Sum += A->GetActorLocation();
			++Count;
		}
	}

	return (Count > 0) ? (Sum / (float)Count) : FVector::ZeroVector;
}
