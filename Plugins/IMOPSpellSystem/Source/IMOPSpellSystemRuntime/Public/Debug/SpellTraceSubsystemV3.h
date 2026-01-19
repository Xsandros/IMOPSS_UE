#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Events/SpellEventV3.h"
#include "SpellTraceSubsystemV3.generated.h"

USTRUCT()
struct FSpellTraceBufferV3
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FSpellEventV3> Events;
};


USTRUCT(BlueprintType)
struct FSpellTraceRowV3
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FGuid RuntimeGuid;
	UPROPERTY(BlueprintReadOnly) FGameplayTag EventTag;
	UPROPERTY(BlueprintReadOnly) float TimeSeconds = 0.f;
	UPROPERTY(BlueprintReadOnly) int32 FrameNumber = 0;

	// Optional debug fields (nice for UI)
	UPROPERTY(BlueprintReadOnly) TWeakObjectPtr<AActor> Caster;
	UPROPERTY(BlueprintReadOnly) TWeakObjectPtr<UObject> Sender;
};

/**
 * Records spell events in a ring buffer, keyed by RuntimeGuid.
 * Intended for Debug UI and console.
 */
UCLASS(BlueprintType)
class IMOPSPELLSYSTEMRUNTIME_API USpellTraceSubsystemV3 : public UGameInstanceSubsystem

{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Debug|Trace")
	int32 MaxEventsPerRuntime = 128;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Debug|Trace")
	int32 MaxRecentRuntimes = 16;

	/** Record a full event (called from EventBus hook). */
	void Record(const FSpellEventV3& Ev);

	/** Lightweight rows for UI list (last N across all runtimes). */
	UFUNCTION(BlueprintCallable, Category="IMOP|Debug|Trace")
	void GetRecentRows(TArray<FSpellTraceRowV3>& OutRows) const;

	/** Full events for a runtime (for drilldown). */
	UFUNCTION(BlueprintCallable, Category="IMOP|Debug|Trace")
	void GetEventsForRuntime(const FGuid& RuntimeGuid, TArray<FSpellEventV3>& OutEvents) const;

	UFUNCTION(BlueprintCallable, Category="IMOP|Debug|Trace")
	void ClearRuntime(const FGuid& RuntimeGuid);

	UFUNCTION(BlueprintCallable, Category="IMOP|Debug|Trace")
	void ClearAll();

private:
	UPROPERTY()
	TMap<FGuid, FSpellTraceBufferV3> EventsByRuntime;


	UPROPERTY()
	TArray<FSpellTraceRowV3> RecentRows;
};
