#include "Stores/SpellVariableStoreV3.h"

void USpellVariableStoreV3::SetValue(FName Key, const FSpellValueV3& Value)
{
    if (Key == NAME_None) return;
    Values.Add(Key, Value);
}

bool USpellVariableStoreV3::GetValue(FName Key, FSpellValueV3& OutValue) const
{
    if (Key == NAME_None) return false;
    if (const FSpellValueV3* Found = Values.Find(Key))
    {
        OutValue = *Found;
        return true;
    }
    return false;
}

bool USpellVariableStoreV3::ModifyFloat(FName Key, ESpellNumericOpV3 Op, float Operand)
{
    if (Key == NAME_None) return false;

    FSpellValueV3 Cur;
    if (!GetValue(Key, Cur))
    {
        Cur.Type = ESpellValueTypeV3::Float;
        Cur.Float = 0.f;
    }

    if (Cur.Type != ESpellValueTypeV3::Float)
    {
        // For Phase 1: only float numeric ops are supported deterministically
        return false;
    }

    float V = Cur.Float;

    switch (Op)
    {
    case ESpellNumericOpV3::Add: V += Operand; break;
    case ESpellNumericOpV3::Sub: V -= Operand; break;
    case ESpellNumericOpV3::Mul: V *= Operand; break;
    case ESpellNumericOpV3::Div: V = (FMath::IsNearlyZero(Operand) ? V : (V / Operand)); break;
    case ESpellNumericOpV3::Min: V = FMath::Min(V, Operand); break;
    case ESpellNumericOpV3::Max: V = FMath::Max(V, Operand); break;
    case ESpellNumericOpV3::Set: V = Operand; break;
    default: break;
    }

    Cur.Float = V;
    SetValue(Key, Cur);
    return true;
}
