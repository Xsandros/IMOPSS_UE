#include "Effects/EffectResolverV3.h"

#include "Actions/SpellPayloadsCoreV3.h"
#include "Attributes/AttributeSubsystemV3.h"
#include "Effects/EffectMitigationProviderV3.h"
#include "Effects/EffectResistanceComponentV3.h"
#include "Effects/EffectImmunityComponentV3.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Stores/SpellVariableStoreV3.h"

// ===== local helpers =====
static UAttributeSubsystemV3* GetAttrSubsystem(const FSpellExecContextV3& Ctx)
{
	if (!Ctx.WorldContext) return nullptr;

	UWorld* W = Ctx.WorldContext->GetWorld();
	if (!W) return nullptr;

	UGameInstance* GI = W->GetGameInstance();
	if (!GI) return nullptr;

	return GI->GetSubsystem<UAttributeSubsystemV3>();
}

static float ComputeResistanceFallback(AActor* Target, const FEffectSpecV3& Spec)
{
	// Fallback mapping: Effect.Damage.X -> Resistance.X
	// This avoids hard dependencies on a tag registry; you can later replace by canonical tags in SpellGameplayTagsV3.
	if (!Target) return 0.f;
	UEffectResistanceComponentV3* ResComp = Target->FindComponentByClass<UEffectResistanceComponentV3>();
	if (!ResComp) return 0.f;

	for (const FGameplayTag& Tag : Spec.Context.EffectTags)
	{
		const FString S = Tag.ToString();
		const FString Prefix = TEXT("Effect.Damage.");
		if (S.StartsWith(Prefix))
		{
			const FString Suffix = S.Mid(Prefix.Len());
			const FString ResTagStr = FString::Printf(TEXT("Resistance.%s"), *Suffix);
			const FGameplayTag ResTag = FGameplayTag::RequestGameplayTag(*ResTagStr, /*ErrorIfNotFound*/ false);
			if (ResTag.IsValid())
			{
				return ResComp->GetResistanceForTag(ResTag);
			}
		}
	}
	return 0.f;
}

static bool CheckImmunityFallback(AActor* Target, const FEffectSpecV3& Spec, FText& OutReason)
{
	if (!Target) return false;
	UEffectImmunityComponentV3* ImmComp = Target->FindComponentByClass<UEffectImmunityComponentV3>();
	if (!ImmComp) return false;

	// Fallback: Effect.Control.* -> Immunity.Control
	for (const FGameplayTag& Tag : Spec.Context.EffectTags)
	{
		const FString S = Tag.ToString();
		if (S.StartsWith(TEXT("Effect.Control")))
		{
			const FGameplayTag ImmTag = FGameplayTag::RequestGameplayTag(TEXT("Immunity.Control"), false);
			if (ImmTag.IsValid() && ImmComp->HasImmunityTag(ImmTag))
			{
				OutReason = FText::FromString(TEXT("Immune via Immunity.Control"));
				return true;
			}
		}
	}
	return false;
}

bool FEffectResolverV3::TryGetNumericFromSpellValue(const FSpellValueV3& V, float& Out)
{
	switch (V.Type)
	{
	case ESpellValueTypeV3::Float: Out = V.Float; return true;
	case ESpellValueTypeV3::Int:   Out = (float)V.Int; return true;
	case ESpellValueTypeV3::Bool:  Out = V.Bool ? 1.f : 0.f; return true;
	default: break;
	}
	Out = 0.f;
	return false;
}

float FEffectResolverV3::EvaluateMagnitude(const FSpellExecContextV3& ExecCtx, const FMagnitudeV3& Magnitude, AActor* Target, float& OutRaw)
{
	OutRaw = 0.f;

	if (!Target)
	{
		return 0.f;
	}

	switch (Magnitude.Source)
	{
	case EMagnitudeSourceV3::Constant:
		OutRaw = Magnitude.Constant;
		return Magnitude.Constant;

	case EMagnitudeSourceV3::FromVariable:
	{
		if(!ExecCtx.VariableStore)
		{
			 return 0.f;
		}
		FSpellValueV3 V;
			if (!ExecCtx.VariableStore->GetValue(Magnitude.VariableKey, V))
		{
			return 0.f;
		}
		float Num = 0.f;
		if (!TryGetNumericFromSpellValue(V, Num))
		{
			return 0.f;
		}
		OutRaw = Num;
		return Num;
	}

	case EMagnitudeSourceV3::FromAttribute:
	{
		UAttributeSubsystemV3* Attr = GetAttrSubsystem(ExecCtx);
		if (!Attr) return 0.f;

		float Cur = 0.f;
		if (!Attr->GetAttribute(Target, Magnitude.Attribute, Cur))
		{
			return 0.f;
		}
		OutRaw = Cur;
		return Cur;
	}

	default:
		return 0.f;
	}
}

bool FEffectResolverV3::ApplyModifyAttribute(const FSpellExecContextV3& ExecCtx, const FEffectSpecV3& Spec, AActor* Target, float FinalMagnitude, FEffectResultV3& OutResult)
{
	UAttributeSubsystemV3* Attr = GetAttrSubsystem(ExecCtx);
	if (!Attr)
	{
		OutResult.Outcome = EEffectOutcomeV3::Failed;
		OutResult.Reason = FText::FromString(TEXT("No AttributeSubsystem"));
		return false;
	}

	float NewValue = 0.f;
	if (!Attr->ApplyOp(Target, Spec.ModifyAttribute.Attribute, Spec.ModifyAttribute.Op, FinalMagnitude, NewValue))
	{
		OutResult.Outcome = EEffectOutcomeV3::Failed;
		OutResult.Reason = FText::FromString(TEXT("Attribute op failed"));
		return false;
	}

	OutResult.Outcome = EEffectOutcomeV3::Applied;
	OutResult.Attribute = Spec.ModifyAttribute.Attribute;
	OutResult.FinalMagnitude = FinalMagnitude;
	return true;
}

bool FEffectResolverV3::ApplyForce(const FSpellExecContextV3& ExecCtx, const FEffectSpecV3& Spec, AActor* Target, FEffectResultV3& OutResult)
{
	if (!Target)
	{
		OutResult.Outcome = EEffectOutcomeV3::Failed;
		OutResult.Reason = FText::FromString(TEXT("Target null"));
		return false;
	}

	const FEffect_ApplyForceV3& F = Spec.ApplyForce;

	// Prefer primitive root, fallback to character movement for Launch.
	if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Target->GetRootComponent()))
	{
		if (F.Kind == EForceKindV3::RadialImpulse)
		{
			Prim->AddRadialImpulse(F.Origin, F.Radius, F.Strength, ERadialImpulseFalloff::RIF_Linear, F.bIgnoreMass);
		}
		else
		{
			const FVector Dir = F.Direction.GetSafeNormal();
			const FVector Imp = Dir * F.Strength;
			Prim->AddImpulse(Imp, NAME_None, F.bIgnoreMass);
		}
		OutResult.Outcome = EEffectOutcomeV3::Applied;
		return true;
	}

	if (ACharacter* C = Cast<ACharacter>(Target))
	{
		if (UCharacterMovementComponent* Move = C->GetCharacterMovement())
		{
			const FVector Dir = F.Direction.GetSafeNormal();
			const FVector Launch = Dir * F.Strength;
			C->LaunchCharacter(Launch, true, true);
			OutResult.Outcome = EEffectOutcomeV3::Applied;
			return true;
		}
	}

	OutResult.Outcome = EEffectOutcomeV3::Failed;
	OutResult.Reason = FText::FromString(TEXT("No physics root / character movement"));
	return false;
}

bool FEffectResolverV3::ResolveAndApply(const FSpellExecContextV3& ExecCtx, const FEffectSpecV3& Spec, AActor* Target, FEffectResultV3& OutResult)
{
	OutResult = FEffectResultV3{};

	if (!ExecCtx.bAuthority)
	{
		OutResult.Outcome = EEffectOutcomeV3::Failed;
		OutResult.Reason = FText::FromString(TEXT("Not authority"));
		return false;
	}

	if (!Target)
	{
		OutResult.Outcome = EEffectOutcomeV3::Failed;
		OutResult.Reason = FText::FromString(TEXT("Target null"));
		return false;
	}

	// Immunity fallback
	{
		FText ImmReason;
		if (CheckImmunityFallback(Target, Spec, ImmReason))
		{
			OutResult.Outcome = EEffectOutcomeV3::Immune;
			OutResult.Reason = ImmReason;
			return false;
		}
	}

	float Raw = 0.f;
	const float RawMag = (Spec.Kind == EEffectKindV3::ModifyAttribute)
		? EvaluateMagnitude(ExecCtx, Spec.ModifyAttribute.Magnitude, Target, Raw)
		: 0.f;

	OutResult.RawMagnitude = RawMag;

	// Mitigation fallback (Resistance)
	const float Resist = ComputeResistanceFallback(Target, Spec);
	const float MitFactor = FMath::Clamp(1.f - Resist, 0.f, 1.f);
	OutResult.MitigationFactor = MitFactor;

	const float FinalMag = RawMag * MitFactor;
	OutResult.FinalMagnitude = FinalMag;

	switch (Spec.Kind)
	{
	case EEffectKindV3::ModifyAttribute:
		return ApplyModifyAttribute(ExecCtx, Spec, Target, FinalMag, OutResult);

	case EEffectKindV3::ApplyForce:
		return ApplyForce(ExecCtx, Spec, Target, OutResult);

	default:
		OutResult.Outcome = EEffectOutcomeV3::Failed;
		OutResult.Reason = FText::FromString(TEXT("Unknown effect kind"));
		return false;
	}
}
