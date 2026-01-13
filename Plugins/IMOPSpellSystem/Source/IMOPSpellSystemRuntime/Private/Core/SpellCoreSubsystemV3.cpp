#include "Core/SpellCoreSubsystemV3.h"

#include "Actions/SpellActionRegistryV3.h"
#include "Actions/ActionTagContractsV3.h"
#include "Core/SpellGameplayTagsV3.h"


DEFINE_LOG_CATEGORY_STATIC(LogIMOPCoreSubsystemV3, Log, All);



void USpellCoreSubsystemV3::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Ensure tags are registered/queried early (will error if missing)
    FIMOPSpellGameplayTagsV3::Get();

    ActionRegistry = NewObject<USpellActionRegistryV3>(this);
    check(ActionRegistry);

    FActionTagContractsV3::RegisterDefaults(*ActionRegistry);
    FActionTagContractsV3::DumpDefaultsToLog();

    UE_LOG(LogIMOPCoreSubsystemV3, Log, TEXT("SpellCoreSubsystemV3 initialized. Bindings=%d"),
        ActionRegistry ? ActionRegistry->FindBinding(FIMOPSpellGameplayTagsV3::Get().Action_EmitEvent) != nullptr : 0);
}

