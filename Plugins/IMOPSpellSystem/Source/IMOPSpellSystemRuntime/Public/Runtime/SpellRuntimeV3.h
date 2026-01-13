#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Events/SpellEventListenerV3.h"
#include "Events/SpellEventBusSubsystemV3.h"
#include "Spec/SpellSpecV3.h"
#include "Actions/SpellActionExecutorV3.h"
#include "Math/RandomStream.h"
#include "SpellRuntimeV3.generated.h"

class USpellVariableStoreV3;
class USpellTargetStoreV3;
class USpellActionRegistryV3;
class USpellEventBusSubsystemV3;

UCLASS(BlueprintType)
class IMOPSPELLSYSTEMRUNTIME_API USpellRuntimeV3 : public UObject, public ISpellEventListenerV3
{
    GENERATED_BODY()
public:
    void Init(UObject* WorldContextObj, AActor* InCaster, USpellSpecV3* InSpec, USpellActionRegistryV3* InRegistry, USpellEventBusSubsystemV3* InEventBus);

    void Start();
    void Stop();

    // ISpellEventListenerV3
    virtual void OnSpellEvent(const FSpellEventV3& Ev) override;
    virtual void BeginDestroy() override;

    // Accessors
    USpellVariableStoreV3* GetVariableStore() const { return VariableStore; }
    USpellTargetStoreV3* GetTargetStore() const { return TargetStore; }
    USpellSpecV3* GetSpec() const { return Spec; }

    // Executor caching
    USpellActionExecutorV3* GetOrCreateExecutor(TSubclassOf<USpellActionExecutorV3> ExecClass);

    FSpellExecContextV3 BuildExecContext() const;
    
    //Random
    FRandomStream& GetRandomStream() { return RandomStream; }
    const FRandomStream& GetRandomStream() const { return RandomStream; }

    void InitializeRng(int32 InSeed);

    // FINAL: startet ein Spell-Asset mit ExecContext
    void StartFromAsset(USpellSpecV3* InSpecAsset, const FSpellExecContextV3& Ctx);


private:
    UPROPERTY()
    TObjectPtr<UObject> WorldContext = nullptr;

    UPROPERTY()
    TObjectPtr<AActor> Caster = nullptr;

    UPROPERTY()
    TObjectPtr<USpellSpecV3> Spec = nullptr;

    UPROPERTY()
    TObjectPtr<USpellActionRegistryV3> Registry = nullptr;

    UPROPERTY()
    TObjectPtr<USpellEventBusSubsystemV3> EventBus = nullptr;

    UPROPERTY()
    TObjectPtr<USpellVariableStoreV3> VariableStore = nullptr;

    UPROPERTY()
    TObjectPtr<USpellTargetStoreV3> TargetStore = nullptr;

    UPROPERTY()
    TArray<FSpellEventSubscriptionHandleV3> Subscriptions;

    // Cache by class
    UPROPERTY()
    TMap<TSubclassOf<USpellActionExecutorV3>, TObjectPtr<USpellActionExecutorV3>> ExecutorCache;

    UPROPERTY()
    int32 Seed = 1337;
    
    UPROPERTY()
    int32 RngSeed = 0;

    FRandomStream RandomStream;

    UPROPERTY()
    bool bAuthority = true;

    bool bRunning = false;
    
    
};
