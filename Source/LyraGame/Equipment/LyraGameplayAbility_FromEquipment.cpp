// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraGameplayAbility_FromEquipment.h"
#include "LyraEquipmentInstance.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h" 
#include "LyraGameplayTags.h"


#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraGameplayAbility_FromEquipment)

ULyraGameplayAbility_FromEquipment::ULyraGameplayAbility_FromEquipment(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

ULyraEquipmentInstance* ULyraGameplayAbility_FromEquipment::GetAssociatedEquipment() const
{
	if (FGameplayAbilitySpec* Spec = UGameplayAbility::GetCurrentAbilitySpec())
	{
		return Cast<ULyraEquipmentInstance>(Spec->SourceObject.Get());
	}

	return nullptr;
}

ULyraInventoryItemInstance* ULyraGameplayAbility_FromEquipment::GetAssociatedItem() const
{
	if (ULyraEquipmentInstance* Equipment = GetAssociatedEquipment())
	{
		return Cast<ULyraInventoryItemInstance>(Equipment->GetInstigator());
	}
	return nullptr;
}

bool ULyraGameplayAbility_FromEquipment::IsWeaponSlotActive() const
{
	ULyraEquipmentInstance* Equipment = GetAssociatedEquipment();
	if (!Equipment)
		return false;

	return Equipment->bIsActive;
	
	// 태그 방식으로 할까 했는데 지금이 몇번 슬롯이 활성화되었고 현재 무기는 몇번 슬롯의 무기이다에 대한 처리가 
	// 추가로 필요해서 오히려 이 방식이 낫다고 판단.
}


#if WITH_EDITOR
EDataValidationResult ULyraGameplayAbility_FromEquipment::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (InstancingPolicy == EGameplayAbilityInstancingPolicy::NonInstanced)
	{
		Context.AddError(NSLOCTEXT("Lyra", "EquipmentAbilityMustBeInstanced", "Equipment ability must be instanced"));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}

#endif
