#include "Stores/SpellTargetStoreV3.h"

const FTargetSetV3* USpellTargetStoreV3::Find(FName Key) const
{
    return Sets.Find(Key);
}

bool USpellTargetStoreV3::Get(FName Key, FTargetSetV3& Out) const
{
    if (const FTargetSetV3* Found = Sets.Find(Key))
    {
        Out = *Found;
        return true;
    }
    return false;
}

void USpellTargetStoreV3::Set(FName Key, const FTargetSetV3& SetIn)
{
    Sets.Add(Key, SetIn);
}

void USpellTargetStoreV3::Clear(FName Key)
{
    Sets.Remove(Key);
}
