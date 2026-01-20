#include "Targeting/TargetingSubsystemV3.h"
#include "Targeting/TargetingLogV3.h"

#include "Actions/SpellActionExecutorV3.h"
#include "Targeting/TargetingBackendsV3.h"
#include "Targeting/TargetingBackend_DefaultV3.h"
#include "Targeting/TargetingGameHooksV3.h"
#include "Targeting/TargetingGameHooks_DefaultV3.h"
#include "Targeting/TargetingSpatialHelpersV3.h"
#include "Targeting/TargetingFiltersV3.h"
#include "Targeting/TargetingSelectionV3.h"
#include "Stores/SpellTargetStoreV3.h"

void UTargetingSubsystemV3::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Default Hooks
	if (!GameHooksObject)
	{
		GameHooksObject = NewObject<UTargetingGameHooks_DefaultV3>(this);
	}

	// Optional: Default Backend
	if (!BackendObject)
	{
		BackendObject = NewObject<UTargetingBackend_DefaultV3>(this);
	}
}

void UTargetingSubsystemV3::SetGameHooksObject(UObject* InHooks)
{
	GameHooksObject = InHooks;
}

void UTargetingSubsystemV3::SetBackendObject(UObject* InBackend)
{
	BackendObject = InBackend;
}


bool UTargetingSubsystemV3::AcquireTargets(
	const FSpellExecContextV3& Ctx,
	const FTargetAcquireRequestV3& Request,
	FTargetAcquireResponseV3& OutResponse) const
{
	OutResponse = FTargetAcquireResponseV3();

	UWorld* World = Ctx.GetWorld();
	if (!World)
	{
		OutResponse.Error = FText::FromString(TEXT("Targeting: World is null"));
		return false;
	}

	const ITargetingBackendV3* Backend = Cast<ITargetingBackendV3>(BackendObject);
	if (!Backend)
	{
		OutResponse.Error = FText::FromString(TEXT("Targeting: Backend is invalid"));
		return false;
	}

	const ITargetingGameHooksV3* Hooks = Cast<ITargetingGameHooksV3>(GameHooksObject);
	if (!Hooks)
	{
		OutResponse.Error = FText::FromString(TEXT("Targeting: GameHooks is invalid"));
		return false;
	}

	const FVector OriginLoc = FTargetingSpatialHelpersV3::ResolveOriginLocation(Ctx, Request.Origin);

	// 1) Acquire candidates
	TArray<FTargetRefV3> Candidates;

	switch (Request.Kind)
	{
	case ETargetAcquireKindV3::ExplicitActors:
		for (AActor* A : Request.ExplicitActors)
		{
			if (!A) continue;
			FTargetRefV3 R; R.Actor = A;
			Candidates.Add(R);
		}
		break;

	case ETargetAcquireKindV3::FromTargetSet:
		if (!Ctx.TargetStore)
		{
			OutResponse.Error = FText::FromString(TEXT("Targeting: TargetStore missing"));
			return false;
		}
		if (const FTargetSetV3* Set = Ctx.TargetStore->Find(Request.SourceTargetSet))
		{
			Candidates = Set->Targets;
		}
		break;

	case ETargetAcquireKindV3::RadiusQuery:
	default:
	{
			UE_LOG(LogIMOPTargetingV3, Log,
				TEXT("Acquire: Kind=%d Raw candidates from backend = %d"),
				(int32)Request.Kind,
				Candidates.Num());


			
			FTargetQueryResultV3 Q;

			// Apply per-spell object type override if backend is default backend
			const UTargetingBackend_DefaultV3* DefaultBackendConst = Cast<UTargetingBackend_DefaultV3>(BackendObject);
			UTargetingBackend_DefaultV3* DefaultBackend = const_cast<UTargetingBackend_DefaultV3*>(DefaultBackendConst);

			TArray<TEnumAsByte<EObjectTypeQuery>> SavedTypes;
			const bool bHasOverride = (DefaultBackend && Request.ObjectTypesOverride.Num() > 0);

			if (bHasOverride)
			{
				SavedTypes = DefaultBackend->ObjectTypes;
				DefaultBackend->ObjectTypes = Request.ObjectTypesOverride;
			}

			const bool bOk = Backend->RadiusQuery(World, OriginLoc, Request.Radius, Q, &OutResponse.Error);

			if (bHasOverride)
			{
				DefaultBackend->ObjectTypes = SavedTypes;
			}

			if (!bOk)
			{
				return false;
			}

			Candidates = Q.Candidates;

			// Coarse acquisition filters (NOT semantic filters)
			Candidates.RemoveAll([&](const FTargetRefV3& R)
			{
				AActor* A = R.Actor.Get();
				if (!A) return true;

				if (!Request.bAllowSelf)
				{
					if (Ctx.Caster && A == Ctx.Caster.Get())
					{
						return true;
					}
				}

				if (Request.RequiredActorClass)
				{
					if (!A->IsA(Request.RequiredActorClass))
					{
						return true;
					}
				}

				return false;
			});
			UE_LOG(LogIMOPTargetingV3, Log,
				TEXT("Acquire: Candidates after coarse filters = %d"),
				Candidates.Num());

			for (const FTargetRefV3& R : Candidates)
			{
				if (AActor* A = R.Actor.Get())
				{
					UE_LOG(LogIMOPTargetingV3, Verbose,
						TEXT("  Candidate: %s (%s)"),
						*A->GetName(),
						*A->GetClass()->GetName());
				}
			}

			
		break;
	}
	}

	OutResponse.Candidates = Candidates;

	// 2) Filter
	FTargetingFiltersV3::ApplyFilters(Ctx, *Hooks, OriginLoc, Request.Filters, Candidates);

	UE_LOG(LogIMOPTargetingV3, Log,
	TEXT("Acquire: Candidates after TargetFilters = %d"),
	Candidates.Num());

	
	// 3) Select
	TArray<FTargetRefV3> Selected;
	FTargetingSelectionV3::Select(Ctx, OriginLoc, Request.Select, Candidates, Selected);

	OutResponse.Selected = Selected;
	OutResponse.bSucceeded = true;
	
	UE_LOG(LogIMOPTargetingV3, Log,
	TEXT("Acquire: Final selected targets = %d"),
	OutResponse.Selected.Num());

	
	return true;
}
