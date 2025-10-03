// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/LyraGameplayAbility.h"

#include "LyraGameplayAbility_FromEquipment.generated.h"

class ULyraEquipmentInstance;
class ULyraInventoryItemInstance;

/**
 * ULyraGameplayAbility_FromEquipment
 *
 * An ability granted by and associated with an equipment instance
 */
UCLASS()
class ULyraGameplayAbility_FromEquipment : public ULyraGameplayAbility
{
	GENERATED_BODY()

public:

	ULyraGameplayAbility_FromEquipment(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category="Lyra|Ability")
	ULyraEquipmentInstance* GetAssociatedEquipment() const;

	UFUNCTION(BlueprintCallable, Category = "Lyra|Ability")
	ULyraInventoryItemInstance* GetAssociatedItem() const;

	// (추가)
	// 슬롯 활성화 체크 함수
	UFUNCTION(BlueprintPure, Category = "Lyra|Ability")
	bool IsWeaponSlotActive() const;


#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

};
