// Fill out your copyright notice in the Description page of Project Settings.


#include "HaroWeaponUserInterface.h"

#include "Equipment/HaroEquipmentManagerComponent.h"
#include "GameFramework/Pawn.h"
#include "Weapons/LyraWeaponInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(HaroWeaponUserInterface)

struct FGeometry;

UHaroWeaponUserInterface::UHaroWeaponUserInterface(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UHaroWeaponUserInterface::NativeConstruct()
{
	Super::NativeConstruct();
}

void UHaroWeaponUserInterface::NativeDestruct()
{
	Super::NativeDestruct();
}

void UHaroWeaponUserInterface::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (APawn* Pawn = GetOwningPlayerPawn())
	{
		if (UHaroEquipmentManagerComponent* EquipmentManager = Pawn->FindComponentByClass<UHaroEquipmentManagerComponent>())
		{
			if (ULyraWeaponInstance* NewInstance = EquipmentManager->GetFirstActiveInstanceOfType<ULyraWeaponInstance>())
			{
				if (NewInstance != CurrentInstance && NewInstance->GetInstigator() != nullptr)
				{
					ULyraWeaponInstance* OldWeapon = CurrentInstance;
					CurrentInstance = NewInstance;
					RebuildWidgetFromWeapon();
					OnWeaponChanged(OldWeapon, CurrentInstance);
				}
			}
		}
	}
}

void UHaroWeaponUserInterface::RebuildWidgetFromWeapon()
{

}

