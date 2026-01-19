#include "Targeting/TargetingBackend_DefaultV3.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"

#include "GameFramework/Actor.h"
#include "Kismet/KismetSystemLibrary.h"

bool UTargetingBackend_DefaultV3::RadiusQuery(
	const UWorld* World,
	const FVector& Origin,
	float Radius,
	FTargetQueryResultV3& OutResult,
	FText* OutError) const
{
	
	UE_LOG(LogTemp, Warning, TEXT("IMOP RadiusQuery using DefaultBackend. ObjectTypes=%d FallbackChannel=%d"),
	ObjectTypes.Num(), (int32)OverlapChannel);

	OutResult.Candidates.Reset();

	if (!World)
	{
		if (OutError) *OutError = FText::FromString(TEXT("RadiusQuery: World is null"));
		return false;
	}

	if (Radius <= 0.f)
	{
		if (OutError) *OutError = FText::FromString(TEXT("RadiusQuery: Radius <= 0"));
		return false;
	}

	// Build object types (preferred list, fallback to OverlapChannel)
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjTypes;
	if (ObjectTypes.Num() > 0)
	{
		ObjTypes = ObjectTypes;
	}
	else
	{
		ObjTypes.Add(UEngineTypes::ConvertToObjectType(OverlapChannel));
	}

	TArray<AActor*> Overlaps;

	const bool bHit = UKismetSystemLibrary::SphereOverlapActors(
		const_cast<UWorld*>(World),
		Origin,
		Radius,
		ObjTypes,
		AActor::StaticClass(),
		TArray<AActor*>(),
		Overlaps);

	if (!bHit)
	{
		return true; // empty is fine
	}

	// Convert to candidates (unique)
	for (AActor* A : Overlaps)
	{
		if (!A) continue;

		FTargetRefV3 R;
		R.Actor = A;
		OutResult.Candidates.Add(R);
	}

	return true;
}

UTargetingBackend_DefaultV3::UTargetingBackend_DefaultV3()
{
	// Default: find Characters + typical target dummies (Actors)
	ObjectTypes.Reset();
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
	// Optional: if you want physics props as targets too:
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_PhysicsBody));
}

