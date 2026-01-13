#include "Targeting/TargetingSubsystemV3.h"

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
		FTargetQueryResultV3 Q;
		if (!Backend->RadiusQuery(World, OriginLoc, Request.Radius, Q, &OutResponse.Error))
		{
			return false;
		}
		Candidates = Q.Candidates;
		break;
	}
	}

	OutResponse.Candidates = Candidates;

	// 2) Filter
	FTargetingFiltersV3::ApplyFilters(Ctx, *Hooks, OriginLoc, Request.Filters, Candidates);

	// 3) Select
	TArray<FTargetRefV3> Selected;
	FTargetingSelectionV3::Select(Ctx, OriginLoc, Request.Select, Candidates, Selected);

	OutResponse.Selected = Selected;
	OutResponse.bSucceeded = true;
	return true;
}
