#pragma once

#include "CoreMinimal.h"
#include "Actions/SpellActionExecutorV3.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

static FORCEINLINE UWorld* IMOP_GetWorldFromExecCtx(const FSpellExecContextV3& Ctx)
{
	return Ctx.WorldContext ? Ctx.WorldContext->GetWorld() : nullptr;
}

static FORCEINLINE UGameInstance* IMOP_GetGIFromExecCtx(const FSpellExecContextV3& Ctx)
{
	if (UWorld* W = IMOP_GetWorldFromExecCtx(Ctx))
	{
		return W->GetGameInstance();
	}
	return nullptr;
}
