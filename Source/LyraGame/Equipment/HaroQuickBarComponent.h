// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ControllerComponent.h"
#include "Inventory/LyraInventoryItemInstance.h"

#include "HaroQuickBarComponent.generated.h"

class AActor;
class ULyraEquipmentInstance;
class UHaroEquipmentManagerComponent;
class UObject;
struct FFrame;

/**
 * 
 */
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class LYRAGAME_API UHaroQuickBarComponent : public UControllerComponent
{
	GENERATED_BODY()
	

public:
	UHaroQuickBarComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "Lyra")
	void CycleActiveSlotForward();

	UFUNCTION(BlueprintCallable, Category = "Lyra")
	void CycleActiveSlotBackward();

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Lyra")
	void SetActiveSlotIndex(int32 NewIndex);

	UFUNCTION(BlueprintCallable, BlueprintPure = false)
	TArray<ULyraInventoryItemInstance*> GetSlots() const
	{
		return Slots;
	}

	UFUNCTION(BlueprintCallable, BlueprintPure = false)
	int32 GetActiveSlotIndex() const { return ActiveSlotIndex; }

	UFUNCTION(BlueprintCallable, BlueprintPure = false)
	ULyraInventoryItemInstance* GetActiveSlotItem() const;

	UFUNCTION(BlueprintCallable, BlueprintPure = false)
	int32 GetNextFreeItemSlot() const; // 빈 슬롯 찾기

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void AddItem(ULyraInventoryItemInstance* Item);  // 자동 슬롯 선택 (슬롯 상태에 따른)

	// AddItem helper 함수.
	// 아래 2 함수를 private로 안 뺀 이유: lyra를 보니 그냥 AddItem과 같은 로직을 블루프린트에서 처리했음.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void AddItemToSlot(int32 SlotIndex, ULyraInventoryItemInstance* Item); // (인벤토리 관점)
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void ReplaceItemInSlot(int32 SlotIndex, ULyraInventoryItemInstance* NewItem);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	ULyraInventoryItemInstance* RemoveItemFromSlot(int32 SlotIndex);

	virtual void BeginPlay() override;

private:
	void UnequipItemInSlot(int32 SlotIndex); // 현재 슬롯 아이템 해제 (장비 관점)
	void EquipItemInSlot(int32 SlotIndex, ULyraInventoryItemInstance* Item); // 현재 슬롯 아이템 장착

	UHaroEquipmentManagerComponent* FindEquipmentManager() const;

protected:
	UPROPERTY()
	int32 NumSlots = 2; // 내 게임은 2개로 함.

	UFUNCTION()
	void OnRep_Slots();

	UFUNCTION()
	void OnRep_ActiveSlotIndex();

private:
	UPROPERTY(ReplicatedUsing = OnRep_Slots)
	TArray<TObjectPtr<ULyraInventoryItemInstance>> Slots;

	UPROPERTY(ReplicatedUsing = OnRep_ActiveSlotIndex)
	int32 ActiveSlotIndex = -1;

	// 장비 매니저에서 관리
	//UPROPERTY()
	//TObjectPtr<ULyraEquipmentInstance> EquippedItem; // 실제 장착된 장비 (캐시)
};


USTRUCT(BlueprintType)
struct FHaroQuickBarSlotsChangedMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	TObjectPtr<AActor> Owner = nullptr; // 누구의 퀵바인가?

	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	TArray<TObjectPtr<ULyraInventoryItemInstance>> Slots;
};


USTRUCT(BlueprintType)
struct FHaroQuickBarActiveIndexChangedMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	TObjectPtr<AActor> Owner = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	int32 ActiveIndex = 0; // 새로 선택된 슬롯 번호
};


// 설계 결정: 무기 추가 시 자동 전환
//
// 구현된 방식:
// - AddItemToSlot() 호출 시 빈 슬롯에 무기 추가
// - EquipItem()으로 장비 생성 (비활성화 상태)
// - SetActiveSlotIndex()로 자동 전환 (새 무기 활성화)
//
// 결정 이유:
// - 플레이어가 새 무기를 주웠을 때 즉시 사용 가능
// - 기존 무기는 자동으로 비활성화되며 인스턴스는 유지됨
// - 원하면 즉시 기존 무기로 다시 전환 가능
//
// 처리 완료된 이슈:
// - 일시적 2개 활성화 방지: EquipItem()이 비활성화 상태로 생성
// - 같은 무기 중복 체크: 인벤토리 시스템의 Fragment에서 처리


// 설계 결정: 무기 슬롯 관리 시스템 (무기 전환 로직이 추가됨)
//
// 1. 무기 추가 (AddItemToSlot):
//    - 빈 슬롯에 새 무기 추가
//    - EquipItemInSlot() 호출 → 장비 생성 (비활성화 상태)
//    - SetActiveSlotIndex() 호출 → 새 무기로 자동 전환
//
// 2. 무기 전환 (SetActiveSlotIndex):
//    - 기존 슬롯 비활성화 (DeactivateItem)
//    - 새 슬롯 활성화 (ActivateItem)
//    - 장비 인스턴스는 유지, Handle만 활성화/비활성화
//
// 핵심 차이점:
// - AddItemToSlot: 새 장비 생성 + 자동 활성화
// - SetActiveSlotIndex: 기존 장비 간 전환 (생성/파괴 없음)
//
// 결정 이유:
// - 플레이어가 새 무기를 주웠을 때 즉시 사용 가능
// - 무기 전환은 Handle만 on/off하여 성능 최적화
// - 로그라이크 특성상 무기 상태(레벨, 강화) 보존 필요
