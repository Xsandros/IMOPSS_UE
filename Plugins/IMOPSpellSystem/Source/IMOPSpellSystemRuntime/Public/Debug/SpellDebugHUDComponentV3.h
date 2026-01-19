#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Blueprint/UserWidget.h"
#include "SpellDebugHUDComponentV3.generated.h"

UCLASS(ClassGroup=(IMOP), meta=(BlueprintSpawnableComponent))
class IMOPSPELLSYSTEMRUNTIME_API USpellDebugHUDComponentV3 : public UActorComponent
{
	GENERATED_BODY()

public:
	USpellDebugHUDComponentV3();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IMOP|Debug")
	TSubclassOf<UUserWidget> DebugWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IMOP|Debug")
	int32 ZOrder = 5000;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IMOP|Debug")
	bool bStartVisible = false;

	/** If set, widget can filter to this runtime guid (BP reads it). */
	UPROPERTY(BlueprintReadWrite, Category="IMOP|Debug")
	FGuid FocusRuntimeGuid;

	UFUNCTION(BlueprintCallable, Category="IMOP|Debug")
	void Toggle();

	UFUNCTION(BlueprintCallable, Category="IMOP|Debug")
	void Show();

	UFUNCTION(BlueprintCallable, Category="IMOP|Debug")
	void Hide();

	UFUNCTION(BlueprintCallable, Category="IMOP|Debug")
	bool IsVisible() const { return bVisible; }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UUserWidget> Widget;

	bool bVisible = false;
};
