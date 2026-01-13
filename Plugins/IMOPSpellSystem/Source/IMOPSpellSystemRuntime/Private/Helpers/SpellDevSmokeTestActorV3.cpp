#include "Helpers/SpellDevSmokeTestActorV3.h"
#include "Core/SpellCoreSubsystemV3.h"
#include "Actions/SpellActionRegistryV3.h"
#include "Core/SpellGameplayTagsV3.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPSmokeTestV3, Log, All);

void ASpellDevSmokeTestActorV3::BeginPlay()
{
	Super::BeginPlay();

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		UE_LOG(LogIMOPSmokeTestV3, Error, TEXT("No GameInstance."));
		return;
	}

	USpellCoreSubsystemV3* Core = GI->GetSubsystem<USpellCoreSubsystemV3>();
	if (!Core)
	{
		UE_LOG(LogIMOPSmokeTestV3, Error, TEXT("SpellCoreSubsystemV3 missing."));
		return;
	}

	USpellActionRegistryV3* Reg = Core->GetActionRegistry();
	if (!Reg)
	{
		UE_LOG(LogIMOPSmokeTestV3, Error, TEXT("ActionRegistry missing."));
		return;
	}

	TArray<FRegisteredActionBindingV3> All;
	Reg->GetAllBindings(All);

	UE_LOG(LogIMOPSmokeTestV3, Log, TEXT("SmokeTest: Registered Action Bindings = %d"), All.Num());

	const auto& SpellTags = FIMOPSpellGameplayTagsV3::Get();
	const FRegisteredActionBindingV3* EmitBind = Reg->FindBinding(SpellTags.Action_EmitEvent);
	UE_LOG(LogIMOPSmokeTestV3, Log, TEXT("FindBinding(%s) => %s"),
		*SpellTags.Action_EmitEvent.ToString(),
		EmitBind ? TEXT("OK") : TEXT("NOT FOUND"));

}
