#include "Targeting/TargetingBackend_DefaultV3.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/KismetSystemLibrary.h"

bool UTargetingBackend_DefaultV3::RadiusQuery(
	const UWorld* World,
	const FVector& Origin,
	float Radius,
	FTargetQueryResultV3& OutResult,
	FText* OutError) const
{
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

	TArray<AActor*> Overlaps;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjTypes;
	ObjTypes.Add(UEngineTypes::ConvertToObjectType(OverlapChannel));

	// minimal overlap: pawns (oder was du per Channel setzt)
	const bool bHit = UKismetSystemLibrary::SphereOverlapActors(
		const_cast<UWorld*>(World),
		Origin,
		Radius,
		ObjTypes,
		/*ActorClassFilter*/ AActor::StaticClass(),
		/*ActorsToIgnore*/ TArray<AActor*>(),
		Overlaps);

	if (!bHit)
	{
		return true; // succeeded, just empty
	}

	for (AActor* A : Overlaps)
	{
		if (!A) continue;
		FTargetRefV3 R;
		R.Actor = A;
		OutResult.Candidates.Add(R);
	}

	return true;
}
