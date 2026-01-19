#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Misc/Guid.h"
#include "SpellDebugWidgetV3.generated.h"

class UScrollBox;
class UButton;
class UCheckBox;
class UTextBlock;

class USpellTraceSubsystemV3;

UCLASS(Abstract)
class IMOPSPELLSYSTEMRUNTIME_API USpellDebugWidgetV3 : public UUserWidget
{
	GENERATED_BODY()

public:
	// Tweaks (editable in derived BP defaults)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Debug")
	float RefreshInterval = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Debug")
	int32 MaxRowsToShow = 60;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Debug")
	bool bAutoFocus = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Debug")
	bool bFocusLastRuntime = true;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// Bound UI (must exist in BP layout)
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UScrollBox> SB_Events;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> Btn_Clear;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> Btn_ToggleFocus;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UCheckBox> Chk_AutoFocus;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> Txt_FocusGuid;

	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> Txt_SelectedActor;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> Txt_Attributes;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> Txt_Status;

	// Row widget class (set in BP defaults)
	UPROPERTY(EditDefaultsOnly, Category="IMOP|Debug")
	TSubclassOf<UUserWidget> RowWidgetClass;

private:
	FTimerHandle RefreshTimer;

	FGuid FocusRuntimeGuid;
	int64 LastSeenRevision = -1;

	// Handlers
	UFUNCTION() void HandleClear();
	UFUNCTION() void HandleToggleFocus();
	UFUNCTION() void HandleAutoFocusChanged(bool bIsChecked);

	// Refresh pipeline
	void RefreshUI();
	void UpdateFocusGuid();
	void RebuildEventsList(USpellTraceSubsystemV3* Trace);
	void UpdateSelectedPanel();

	USpellTraceSubsystemV3* GetTrace() const;
	FText FormatRowText(const struct FSpellTraceRowV3& Row) const;

	bool GuidIsValid(const FGuid& G) const { return G.IsValid(); }
};
