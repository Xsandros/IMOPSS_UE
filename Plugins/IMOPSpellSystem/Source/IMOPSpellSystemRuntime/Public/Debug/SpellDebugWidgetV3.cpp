#include "Debug/SpellDebugWidgetV3.h"

#include "Debug/SpellTraceSubsystemV3.h"
#include "Debug/SpellDebugRowWidgetV3.h"

#include "Components/ScrollBox.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/TextBlock.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

#include "Attributes/SpellAttributeComponentV3.h"
#include "Status/SpellStatusComponentV3.h"


static FText TextOr(const FString& S, const TCHAR* Fallback)
{
	return FText::FromString(S.Len() ? S : FString(Fallback));
}

void USpellDebugWidgetV3::NativeConstruct()
{
	Super::NativeConstruct();

	// Bind UI events (no BP wiring needed)
	if (Btn_Clear)       Btn_Clear->OnClicked.AddDynamic(this, &USpellDebugWidgetV3::HandleClear);
	if (Btn_ToggleFocus) Btn_ToggleFocus->OnClicked.AddDynamic(this, &USpellDebugWidgetV3::HandleToggleFocus);
	if (Chk_AutoFocus)   Chk_AutoFocus->OnCheckStateChanged.AddDynamic(this, &USpellDebugWidgetV3::HandleAutoFocusChanged);

	if (Chk_AutoFocus) Chk_AutoFocus->SetIsChecked(bAutoFocus);

	// Start timer
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(RefreshTimer, this, &USpellDebugWidgetV3::RefreshUI, RefreshInterval, true);
	}

	RefreshUI();
}

void USpellDebugWidgetV3::NativeDestruct()
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(RefreshTimer);
	}
	Super::NativeDestruct();
}

void USpellDebugWidgetV3::HandleClear()
{
	if (USpellTraceSubsystemV3* Trace = GetTrace())
	{
		if (bFocusLastRuntime && FocusRuntimeGuid.IsValid())
		{
			Trace->ClearRuntime(FocusRuntimeGuid);
		}
		else
		{
			Trace->ClearAll();
		}
	}

	// Force a redraw even if revision gating is weird
	LastSeenRevision = -1;
	RefreshUI();
}

void USpellDebugWidgetV3::HandleToggleFocus()
{
	bFocusLastRuntime = !bFocusLastRuntime;
	LastSeenRevision = -1;
	RefreshUI();
}

void USpellDebugWidgetV3::HandleAutoFocusChanged(bool bIsChecked)
{
	bAutoFocus = bIsChecked;
	LastSeenRevision = -1;
	RefreshUI();
}

USpellTraceSubsystemV3* USpellDebugWidgetV3::GetTrace() const
{
	if (!GetWorld()) return nullptr;

	UGameInstance* GI = GetWorld()->GetGameInstance();
	if (!GI) return nullptr;

	return GI->GetSubsystem<USpellTraceSubsystemV3>();
}

void USpellDebugWidgetV3::RefreshUI()
{
	UpdateFocusGuid();

	if (USpellTraceSubsystemV3* Trace = GetTrace())
	{
		const int64 NewRev = Trace->GetRevision();
		if (NewRev != LastSeenRevision)
		{
			LastSeenRevision = NewRev;
			RebuildEventsList(Trace);
		}
	}

	UpdateSelectedPanel();
}

void USpellDebugWidgetV3::UpdateFocusGuid()
{
	// If you later want HUDComponent as source of truth, we can read it here.
	// For now: auto focus uses trace latest runtime.
	if (bAutoFocus)
	{
		if (USpellTraceSubsystemV3* Trace = GetTrace())
		{
			FGuid Latest;
			if (Trace->GetLatestRuntimeGuid(Latest) && Latest.IsValid())
			{
				FocusRuntimeGuid = Latest;
			}
		}
	}

	if (Txt_FocusGuid)
	{
		const FString S = FocusRuntimeGuid.IsValid() ? FocusRuntimeGuid.ToString(EGuidFormats::DigitsWithHyphens) : TEXT("(no focus)");
		Txt_FocusGuid->SetText(FText::FromString(S));
	}
}

FText USpellDebugWidgetV3::FormatRowText(const FSpellTraceRowV3& Row) const
{
	const FString TagStr = Row.EventTag.ToString();
	const FString GuidShort = Row.RuntimeGuid.IsValid() ? Row.RuntimeGuid.ToString(EGuidFormats::Digits).Right(8) : TEXT("--------");
	return FText::FromString(FString::Printf(TEXT("[%d] %s\n%s"), Row.FrameNumber, *TagStr, *GuidShort));
}

void USpellDebugWidgetV3::RebuildEventsList(USpellTraceSubsystemV3* Trace)
{
	if (!SB_Events || !Trace) return;

	SB_Events->ClearChildren();

	const bool bFilter = bFocusLastRuntime && FocusRuntimeGuid.IsValid();

	TArray<FSpellTraceRowV3> Rows;
	Trace->GetRecentRowsFiltered(FocusRuntimeGuid, bFilter, MaxRowsToShow, Rows);

	if (!RowWidgetClass) return;

	for (const FSpellTraceRowV3& Row : Rows)
	{
		UUserWidget* RowW = CreateWidget<UUserWidget>(GetOwningPlayer(), RowWidgetClass);
		if (!RowW) continue;

		if (USpellDebugRowWidgetV3* Typed = Cast<USpellDebugRowWidgetV3>(RowW))
		{
			Typed->SetRowText(FormatRowText(Row));
		}

		SB_Events->AddChild(RowW);
	}
}

void USpellDebugWidgetV3::UpdateSelectedPanel()
{
	APlayerController* PC = GetOwningPlayer();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;

	if (!PC || !Pawn)
	{
		if (Txt_SelectedActor) Txt_SelectedActor->SetText(FText::FromString(TEXT("None")));
		if (Txt_Attributes)    Txt_Attributes->SetText(FText::GetEmpty());
		if (Txt_Status)        Txt_Status->SetText(FText::GetEmpty());
		return;
	}

	APlayerCameraManager* Cam = PC->PlayerCameraManager;
	if (!Cam) return;

	const FVector Start = Cam->GetCameraLocation();
	const FVector End   = Start + (Cam->GetCameraRotation().Vector() * 5000.f);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(IMOPSpellDebugTrace), false);
	Params.AddIgnoredActor(Pawn);

	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

	AActor* HitActor = bHit ? Hit.GetActor() : nullptr;
	if (Txt_SelectedActor) Txt_SelectedActor->SetText(FText::FromString(HitActor ? HitActor->GetName() : TEXT("None")));

	// Attributes
	if (Txt_Attributes)
	{
		if (USpellAttributeComponentV3* Attr = HitActor ? HitActor->FindComponentByClass<USpellAttributeComponentV3>() : nullptr)
		{
			float Health = 0.f, MaxHealth = 0.f;
			const bool bHasH = Attr->GetAttribute(FGameplayTag::RequestGameplayTag(TEXT("Attribute.Health")), Health);
			const bool bHasM = Attr->GetAttribute(FGameplayTag::RequestGameplayTag(TEXT("Attribute.MaxHealth")), MaxHealth);

			if (bHasH && bHasM)
			{
				Txt_Attributes->SetText(FText::FromString(FString::Printf(TEXT("Health: %.1f / %.1f"), Health, MaxHealth)));
			}
			else if (bHasH)
			{
				Txt_Attributes->SetText(FText::FromString(FString::Printf(TEXT("Health: %.1f"), Health)));
			}
			else
			{
				Txt_Attributes->SetText(FText::FromString(TEXT("(no health attr)")));
			}
		}
		else
		{
			Txt_Attributes->SetText(FText::FromString(TEXT("(no SpellAttributeComponentV3)")));
		}
	}

	// Status
	if (Txt_Status)
	{
		if (USpellStatusComponentV3* Status = HitActor ? HitActor->FindComponentByClass<USpellStatusComponentV3>() : nullptr)
		{
			TArray<FActiveStatusV3> Out;
			Status->GetAllStatuses(Out); // your BP wrapper, still callable in C++

			if (Out.Num() == 0)
			{
				Txt_Status->SetText(FText::FromString(TEXT("(no active statuses)")));
			}
			else
			{
				FString S;
				for (const FActiveStatusV3& St : Out)
				{
					S += FString::Printf(TEXT("%s x%d\n"), *St.StatusTag.ToString(), St.Stacks);
				}
				Txt_Status->SetText(FText::FromString(S));
			}
		}
		else
		{
			Txt_Status->SetText(FText::FromString(TEXT("(no SpellStatusComponentV3)")));
		}
	}
}
