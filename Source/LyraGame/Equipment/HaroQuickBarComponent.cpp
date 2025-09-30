// Fill out your copyright notice in the Description page of Project Settings.


#include "HaroQuickBarComponent.h"

#include "Equipment/LyraEquipmentDefinition.h"
#include "Equipment/LyraEquipmentInstance.h"
#include "Equipment/HaroEquipmentManagerComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/Pawn.h"
#include "Inventory/InventoryFragment_EquippableItem.h"
#include "NativeGameplayTags.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(HaroQuickBarComponent)

class FLifetimeProperty;
class ULyraEquipmentDefinition;

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Haro_QuickBar_Message_SlotsChanged, "Haro.QuickBar.Message.SlotsChanged");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Haro_QuickBar_Message_ActiveIndexChanged, "Haro.QuickBar.Message.ActiveIndexChanged");

UHaroQuickBarComponent::UHaroQuickBarComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void UHaroQuickBarComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, Slots);
	DOREPLIFETIME(ThisClass, ActiveSlotIndex);
}

void UHaroQuickBarComponent::BeginPlay()
{
	if (Slots.Num() < NumSlots)
	{
		Slots.AddDefaulted(NumSlots - Slots.Num());
	}

	Super::BeginPlay();
}

void UHaroQuickBarComponent::CycleActiveSlotForward()
{
	if (Slots.Num() < 2)
	{
		return;
	}

	const int32 OldIndex = (ActiveSlotIndex < 0 ? Slots.Num() - 1 : ActiveSlotIndex);
	int32 NewIndex = ActiveSlotIndex;
	do
	{
		NewIndex = (NewIndex + 1) % Slots.Num();
		if (Slots[NewIndex] != nullptr)
		{
			SetActiveSlotIndex(NewIndex);
			return;
		}
	} while (NewIndex != OldIndex);

	// 비어있지 않은 슬롯을 찾을 때가지 순환
}

void UHaroQuickBarComponent::CycleActiveSlotBackward()
{
	if (Slots.Num() < 2)
	{
		return;
	}

	const int32 OldIndex = (ActiveSlotIndex < 0 ? Slots.Num() - 1 : ActiveSlotIndex);
	int32 NewIndex = ActiveSlotIndex;
	do
	{
		NewIndex = (NewIndex - 1 + Slots.Num()) % Slots.Num();
		if (Slots[NewIndex] != nullptr)
		{
			SetActiveSlotIndex(NewIndex);
			return;
		}
	} while (NewIndex != OldIndex);
}

void UHaroQuickBarComponent::EquipItemInSlot(int32 SlotIndex, ULyraInventoryItemInstance* Item)
{
	// 장작 가능 Fragment 찾기
	if (const UInventoryFragment_EquippableItem* EquipInfo = Item->FindFragmentByClass<UInventoryFragment_EquippableItem>())
	{
		// 장비 정의 가져오기
		TSubclassOf<ULyraEquipmentDefinition> EquipDef = EquipInfo->EquipmentDefinition;
		if (EquipDef != nullptr)
		{
			if (UHaroEquipmentManagerComponent* EquipmentManager = FindEquipmentManager())
			{
				// 장비 생성 (비활성화 상태로 생성해옴.)
				ULyraEquipmentInstance* NewEquipment = EquipmentManager->EquipItem(EquipInfo->EquipmentDefinition, SlotIndex);
				if (NewEquipment)
				{
					NewEquipment->SetInstigator(Item);
				}

				// 새로 추가된 무기로 자동 전환 (활성화)
				SetActiveSlotIndex(SlotIndex);
			}
		}
		
	}

}

void UHaroQuickBarComponent::UnequipItemInSlot(int32 SlotIndex)
{
	if (UHaroEquipmentManagerComponent* EquipmentManager = FindEquipmentManager())
	{
		EquipmentManager->UnequipItem(SlotIndex); // 장비 해제
	}
}

UHaroEquipmentManagerComponent* UHaroQuickBarComponent::FindEquipmentManager() const
{
	if (AController* OwnerController = Cast<AController>(GetOwner()))
	{
		if (APawn* Pawn = OwnerController->GetPawn())
		{
			return Pawn->FindComponentByClass<UHaroEquipmentManagerComponent>();
		}
	}
	return nullptr;
}

// 활성 슬롯 변경 (Server RPC)
void UHaroQuickBarComponent::SetActiveSlotIndex_Implementation(int32 NewIndex)
{
	if (!Slots.IsValidIndex(NewIndex) || ActiveSlotIndex == NewIndex)
		return;

	if (UHaroEquipmentManagerComponent* EquipmentManager = FindEquipmentManager())
	{
		// 기존 슬롯 비활성화
		if (ActiveSlotIndex >= 0)
		{
			EquipmentManager->DeactivateItem(ActiveSlotIndex);
		}

		// 활성 슬롯 변경
		ActiveSlotIndex = NewIndex;

		// 새 슬롯 활성화
		EquipmentManager->ActivateItem(NewIndex);

		OnRep_ActiveSlotIndex();
	}
}

// 현재 활성 슬롯의 아이템 가져오기
ULyraInventoryItemInstance* UHaroQuickBarComponent::GetActiveSlotItem() const
{
	return Slots.IsValidIndex(ActiveSlotIndex) ? Slots[ActiveSlotIndex] : nullptr;
}

int32 UHaroQuickBarComponent::GetNextFreeItemSlot() const
{
	int32 SlotIndex = 0;
	for (const TObjectPtr<ULyraInventoryItemInstance>& ItemPtr : Slots)
	{
		if (ItemPtr == nullptr)
		{
			return SlotIndex;
		}
		++SlotIndex;
	}

	return INDEX_NONE;
}

void UHaroQuickBarComponent::AddItem(ULyraInventoryItemInstance* Item)
{
	if (!Item) return;

	int32 EmptySlot = GetNextFreeItemSlot();

	if (EmptySlot != INDEX_NONE)
	{
		// 빈 슬롯에 추가
		AddItemToSlot(EmptySlot, Item);
	}
	else
	{
		// 슬롯 다 참 → 현재 활성 슬롯과 교체
		ReplaceItemInSlot(ActiveSlotIndex, Item);
	}
}

void UHaroQuickBarComponent::AddItemToSlot(int32 SlotIndex, ULyraInventoryItemInstance* Item)
{
	if (Slots.IsValidIndex(SlotIndex) && (Item != nullptr))
	{
		if (Slots[SlotIndex] == nullptr)
		{
			Slots[SlotIndex] = Item;

			EquipItemInSlot(SlotIndex, Item);

			OnRep_Slots();
		}
	}
}

void UHaroQuickBarComponent::ReplaceItemInSlot(int32 SlotIndex, ULyraInventoryItemInstance* NewItem)
{
	if (!Slots.IsValidIndex(SlotIndex) || !NewItem) return;

	RemoveItemFromSlot(SlotIndex);
	AddItemToSlot(SlotIndex, NewItem);
}

ULyraInventoryItemInstance* UHaroQuickBarComponent::RemoveItemFromSlot(int32 SlotIndex)
{
	ULyraInventoryItemInstance* Result = nullptr;

	if (ActiveSlotIndex == SlotIndex)
	{
		UnequipItemInSlot(SlotIndex);
		ActiveSlotIndex = -1;
	}

	if (Slots.IsValidIndex(SlotIndex))
	{
		Result = Slots[SlotIndex];

		if (Result != nullptr)
		{
			Slots[SlotIndex] = nullptr;
			OnRep_Slots();
		}
	}

	return Result;
}

void UHaroQuickBarComponent::OnRep_Slots()
{
	FHaroQuickBarSlotsChangedMessage Message;
	Message.Owner = GetOwner(); // 컨트롤러
	Message.Slots = Slots;

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(this);
	MessageSystem.BroadcastMessage(TAG_Haro_QuickBar_Message_SlotsChanged, Message);
}

void UHaroQuickBarComponent::OnRep_ActiveSlotIndex()
{
	FHaroQuickBarActiveIndexChangedMessage Message;
	Message.Owner = GetOwner();
	Message.ActiveIndex = ActiveSlotIndex;

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(this);
	MessageSystem.BroadcastMessage(TAG_Haro_QuickBar_Message_ActiveIndexChanged, Message);
}

