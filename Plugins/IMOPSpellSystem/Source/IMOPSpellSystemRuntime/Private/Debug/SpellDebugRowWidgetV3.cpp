#include "Debug/SpellDebugRowWidgetV3.h"
#include "Components/TextBlock.h"

void USpellDebugRowWidgetV3::SetRowText(const FText& InText)
{
	if (Txt_Row)
	{
		Txt_Row->SetText(InText);
	}
}
