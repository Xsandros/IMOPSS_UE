#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SpellActionExecutorV3.generated.h"

class USpellEventBusSubsystemV3;
class USpellVariableStoreV3;
class USpellTargetStoreV3;
class USpellActionRegistryV3;
class USpellRuntimeV3;

USTRUCT(BlueprintType)
struct FSpellExecContextV3
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category="Spell|Context")
    TObjectPtr<UObject> WorldContext = nullptr;

    UPROPERTY(BlueprintReadWrite, Category="Spell|Context")
    TObjectPtr<AActor> Caster = nullptr;

    UPROPERTY(BlueprintReadWrite, Category="Spell|Context")
    TObjectPtr<USpellRuntimeV3> Runtime = nullptr;

    UPROPERTY(BlueprintReadWrite, Category="Spell|Context")
    TObjectPtr<USpellEventBusSubsystemV3> EventBus = nullptr;

    UPROPERTY(BlueprintReadWrite, Category="Spell|Context")
    TObjectPtr<USpellVariableStoreV3> VariableStore = nullptr;

    UPROPERTY(BlueprintReadWrite, Category="Spell|Context")
    TObjectPtr<USpellTargetStoreV3> TargetStore = nullptr;

    UPROPERTY(BlueprintReadWrite, Category="Spell|Context")
    TObjectPtr<USpellActionRegistryV3> ActionRegistry = nullptr;

    UPROPERTY(BlueprintReadWrite, Category="Spell|Context")
    int32 Seed = 0;

    UPROPERTY(BlueprintReadWrite, Category="Spell|Context")
    bool bAuthority = true;

    // ===== Canonical Accessors (FINAL) =====
    UWorld* GetWorld() const;
    AActor* GetCaster() const { return Caster.Get(); }

    // RNG läuft final über Runtime (deterministisch) – Implementierung in .cpp
    struct FRandomStream& GetRng() const;
};

UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class IMOPSPELLSYSTEMRUNTIME_API USpellActionExecutorV3 : public UObject
{
    GENERATED_BODY()
public:
    virtual const UScriptStruct* GetPayloadStruct() const PURE_VIRTUAL(USpellActionExecutorV3::GetPayloadStruct, return nullptr;);
    virtual void Execute(const FSpellExecContextV3& Ctx, const void* PayloadData) PURE_VIRTUAL(USpellActionExecutorV3::Execute, );
};
