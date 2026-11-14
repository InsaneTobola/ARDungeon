// Copyright Druid Mechanics

#include "AbilitySystem/MMC/MMC_Knockup.h"
#include "AbilitySystem/AuraAttributeSet.h"

UMMC_Knockup::UMMC_Knockup()
{
	AddAttributeCapture(AirDef, UAuraAttributeSet::GetAirAttribute());
}

float UMMC_Knockup::CalculateBaseMagnitude_Implementation(
	const FGameplayEffectSpec& Spec) const
{
	const float Air = GetCapturedValue(AirDef, Spec);

	return Air * 1.1f;
}
