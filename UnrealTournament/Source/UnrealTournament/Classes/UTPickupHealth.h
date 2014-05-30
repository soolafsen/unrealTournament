// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTPickupHealth.generated.h"

UCLASS(Blueprintable, Abstract)
class AUTPickupHealth : public AUTPickup
{
	GENERATED_UCLASS_BODY()

	/** amount of health to restore */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Pickup)
	int32 HealAmount;
	/** if true, heal amount goes to SuperHealthMax instead of HealthMax */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Pickup)
	bool bSuperHeal;

	virtual void ProcessTouch_Implementation(APawn* TouchedBy) OVERRIDE;
	virtual void GiveTo_Implementation(APawn* Target) OVERRIDE;
};