#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SpellDebugRowWidgetV3.generated.h"

class UTextBlock;

UCLASS(Abstract)
class IMOPSPELLSYSTEMRUNTIME_API USpellDebugRowWidgetV3 : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="IMOP|Debug")
	void SetRowText(const FText& InText);

protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> Txt_Row;
};
