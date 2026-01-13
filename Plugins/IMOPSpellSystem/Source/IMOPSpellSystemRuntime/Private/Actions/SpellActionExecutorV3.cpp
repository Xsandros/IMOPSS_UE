#include "Actions/SpellActionExecutorV3.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Runtime/SpellRuntimeV3.h"   // <- hier darfst du Runtime voll includen

UWorld* FSpellExecContextV3::GetWorld() const
{
	if (WorldContext)
	{
		return WorldContext->GetWorld();
	}
	if (Caster)
	{
		return Caster->GetWorld();
	}
	if (Runtime)
	{
		return Runtime->GetWorld();
	}
	return nullptr;
}

FRandomStream& FSpellExecContextV3::GetRng() const
{
	check(Runtime);
	return Runtime->GetRandomStream(); // siehe Hinweis unten
}
