#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"

#include "Events/SpellEventV3.h"
#include "Events/SpellEventListenerV3.h"

#include "SpellEventBusSubsystemV3.generated.h"

/**
 * Simple deterministic event bus:
 * - Listener subscribes with optional TagFilter:
 *   - empty/invalid filter => receive all events
 *   - otherwise receive if Ev.EventTag matches the filter (MatchesTag)
 * - Unsubscribe by handle.
 *
 * NOTE: This bus intentionally keeps payload-agnostic: SpellEventV3 is the common envelope.
 */

USTRUCT(BlueprintType)
struct FSpellEventSubscriptionHandleV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Events")
	int32 Id = 0;

	bool IsValid() const { return Id != 0; }
};

// IMPORTANT: USTRUCT must be in GLOBAL SCOPE (not nested in a UCLASS)
USTRUCT()
struct FSpellEventBusSubscriberV3
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Id = 0;

	UPROPERTY()
	TWeakObjectPtr<UObject> ListenerObj;

	UPROPERTY()
	FGameplayTag TagFilter;
};

UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API USpellEventBusSubsystemV3 : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Subscribe a listener. TagFilter can be empty to receive all events.
	FSpellEventSubscriptionHandleV3 Subscribe(UObject* ListenerObj, FGameplayTag TagFilter);

	// Convenience overload for types implementing ISpellEventListenerV3.
	FSpellEventSubscriptionHandleV3 Subscribe(ISpellEventListenerV3* Listener, FGameplayTag TagFilter);

	void Unsubscribe(const FSpellEventSubscriptionHandleV3& Handle);

	// Emit event (server-authoritative; callers decide replication later)
	void Emit(const FSpellEventV3& Ev);

	// Debug
	int32 GetSubscriberCount() const { return Subscribers.Num(); }

private:
	UPROPERTY()
	TArray<FSpellEventBusSubscriberV3> Subscribers;

	int32 NextId = 1;

private:
	static bool MatchesFilter(const FGameplayTag& EventTag, const FGameplayTag& FilterTag);
	static ISpellEventListenerV3* AsListener(UObject* Obj);
};
