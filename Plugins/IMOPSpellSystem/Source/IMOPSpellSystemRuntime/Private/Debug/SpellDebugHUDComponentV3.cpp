#include "Debug/SpellDebugHUDComponentV3.h"
#include "GameFramework/PlayerController.h"

USpellDebugHUDComponentV3::USpellDebugHUDComponentV3()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USpellDebugHUDComponentV3::BeginPlay()
{
	Super::BeginPlay();

	if (bStartVisible)
	{
		Show();
	}
}

void USpellDebugHUDComponentV3::Toggle()
{
	if (bVisible) Hide();
	else Show();
}

void USpellDebugHUDComponentV3::Show()
{
	if (bVisible) return;

	AActor* Owner = GetOwner();
	APlayerController* PC = Cast<APlayerController>(Owner);
	if (!PC)
	{
		// If attached to Character, try get controller
		if (APawn* Pawn = Cast<APawn>(Owner))
		{
			PC = Cast<APlayerController>(Pawn->GetController());
		}
	}
	if (!PC || !DebugWidgetClass) return;

	if (!Widget)
	{
		Widget = CreateWidget<UUserWidget>(PC, DebugWidgetClass);
	}
	if (Widget && !Widget->IsInViewport())
	{
		Widget->AddToViewport(ZOrder);
	}
	bVisible = true;
}

void USpellDebugHUDComponentV3::Hide()
{
	if (!bVisible) return;
	if (Widget && Widget->IsInViewport())
	{
		Widget->RemoveFromParent();
	}
	bVisible = false;
}
