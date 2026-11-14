
#include "AuraAbilityTypes.h"

bool FAuraGameplayEffectContext::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
    // Keep the engine's base context layout in sync with the active Unreal version.
    const bool bBaseSuccess = FGameplayEffectContext::NetSerialize(Ar, Map, bOutSuccess);

    // Serialize only the custom Aura-specific flags after the base context.
    uint8 RepBits = 0;

    if (Ar.IsSaving())
    {
        if (bIsBlockedHit)
        {
            RepBits |= 1 << 0;
        }

        if (bIsCriticalHit)
        {
            RepBits |= 1 << 1;
        }
    }

    Ar.SerializeBits(&RepBits, 2);

    if (Ar.IsLoading())
    {
        bIsBlockedHit = (RepBits & (1 << 0)) != 0;
        bIsCriticalHit = (RepBits & (1 << 1)) != 0;
    }

    bOutSuccess = bBaseSuccess && bOutSuccess && !Ar.IsError();
    return bOutSuccess;
}
