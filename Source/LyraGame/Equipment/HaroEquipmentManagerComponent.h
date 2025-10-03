// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/LyraAbilitySet.h"
#include "Components/PawnComponent.h"
#include "Net/Serialization/FastArraySerializer.h"

#include "HaroEquipmentManagerComponent.generated.h"

class UActorComponent;
class ULyraAbilitySystemComponent;
class ULyraEquipmentDefinition;
class ULyraEquipmentInstance;
class UHaroEquipmentManagerComponent; // 상호 참조 대비
class UObject;
struct FFrame;
struct FHaroEquipmentList; //
struct FNetDeltaSerializeInfo;
struct FReplicationFlags;

/** A single piece of applied equipment */
USTRUCT(BlueprintType)
struct FHaroAppliedEquipmentEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FHaroAppliedEquipmentEntry()
	{
	}

	FString GetDebugString() const;

private:
	friend FHaroEquipmentList;
	friend UHaroEquipmentManagerComponent;

	// 추가: 슬롯 인덱스
	UPROPERTY()
	int32 SlotIndex = -1;  // 0 = 주무기, 1 = 보조무기

	// The equipment class that got equipped
	UPROPERTY()
	TSubclassOf<ULyraEquipmentDefinition> EquipmentDefinition; // 장비 설계도

	UPROPERTY()
	TObjectPtr<ULyraEquipmentInstance> Instance = nullptr; // 실제 장비 인스턴스

	// Authority-only list of granted handles
	UPROPERTY(NotReplicated)
	FLyraAbilitySet_GrantedHandles GrantedHandles; // 이 장비가 부여한 능력들의 핸들 (서버에서만)

	// 추가: 활성화 상태 (네트워크 동기화에 필요할 것 같아서 넣음)
	UPROPERTY()
	bool bIsActive = false;
};

/** List of applied equipment */
USTRUCT(BlueprintType)
struct FHaroEquipmentList : public FFastArraySerializer // 네트워크 최적화 (변경된 부분만 복제)
{
	GENERATED_BODY()

	FHaroEquipmentList()
		: OwnerComponent(nullptr)
	{
	}

	FHaroEquipmentList(UActorComponent* InOwnerComponent)
		: OwnerComponent(InOwnerComponent) // 소유자 지정 생성자, UHaroEquipmentManagerComponent가 됨.
	{
	}

public:
	//~FFastArraySerializer contract
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize); // 클라에서 제거되기 직전
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize); // 클라에서 추가된 직후
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize); // 클라에서 변경된 직후
	//~End of FFastArraySerializer contract

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FHaroAppliedEquipmentEntry, FHaroEquipmentList>(Entries, DeltaParms, *this);
	}

	ULyraEquipmentInstance* AddEntry(TSubclassOf<ULyraEquipmentDefinition> EquipmentDefinition, int32 SlotIndex);
	void RemoveEntry(int32 SlotIndex);

private:
	ULyraAbilitySystemComponent* GetAbilitySystemComponent() const;

	friend UHaroEquipmentManagerComponent; // UHaroEquipmentManagerComponent 이 클래스에서만 접근 가능하게.

private:
	// Replicated list of equipment entries
	UPROPERTY()
	TArray<FHaroAppliedEquipmentEntry> Entries; // 모든 장착된 장비들


	// -> 아,,, GetAbilitySystemComponent(ASC)가 문제였던 이유가 이거때문이었구나.... (이미 다르게 해결함.)
	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent; // 이 리스트를 소유한 컴포넌트 (각 클라에서 자체적으로 설정)
};

template<>
struct TStructOpsTypeTraits<FHaroEquipmentList> : public TStructOpsTypeTraitsBase2<FHaroEquipmentList>
{
	enum { WithNetDeltaSerializer = true };
};







/**
 * Manages equipment applied to a pawn
 * 블루프린트에서 변수 타입으로 사용가능하되 수정 불가.
 */
UCLASS(BlueprintType, Const) 
class LYRAGAME_API UHaroEquipmentManagerComponent : public UPawnComponent
{
	GENERATED_BODY()
	
public:
	UHaroEquipmentManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly) // 서버에서만 호출 가능
	ULyraEquipmentInstance* EquipItem(TSubclassOf<ULyraEquipmentDefinition> EquipmentDefinition, int32 SlotIndex);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void UnequipItem(int32 SlotIndex); // (매개변수) 장비 인스턴스 -> 슬롯 인덱스로 변경.

	// 추가 (Item 활성화/비활성화)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void ActivateItem(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void DeactivateItem(int32 SlotIndex);

	// helper 함수
	void SetItemActiveState(int32 SlotIndex, bool bActive);

	//~UObject interface
	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	//~End of UObject interface

	//~UActorComponent interface
	//virtual void EndPlay() override;
	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
	virtual void ReadyForReplication() override;
	//~End of UActorComponent interface

	/** Returns the first equipped instance of a given type, or nullptr if none are found */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	ULyraEquipmentInstance* GetFirstInstanceOfType(TSubclassOf<ULyraEquipmentInstance> InstanceType);

	// (추가) 활성화 된 것만 한번 더 걸러냄 - 위의 함수에서
	UFUNCTION(BlueprintCallable, BlueprintPure)
	ULyraEquipmentInstance* GetFirstActiveInstanceOfType(TSubclassOf<ULyraEquipmentInstance> InstanceType);

	/** Returns all equipped instances of a given type, or an empty array if none are found */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<ULyraEquipmentInstance*> GetEquipmentInstancesOfType(TSubclassOf<ULyraEquipmentInstance> InstanceType) const;

	// (추가) 활성화 된 것만 한번 더 걸러냄 - 위의 함수에서
	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<ULyraEquipmentInstance*> GetActiveEquipmentInstancesOfType(TSubclassOf<ULyraEquipmentInstance> InstanceType) const;

	template <typename T>
	T* GetFirstInstanceOfType()
	{
		return (T*)GetFirstInstanceOfType(T::StaticClass());
	}

	// (추가) 활성화 된 것만 한번 더 걸러냄 - 위의 함수에서
	template <typename T>
	T* GetFirstActiveInstanceOfType()
	{
		return (T*)GetFirstActiveInstanceOfType(T::StaticClass());
	}

private:
	UPROPERTY(Replicated)
	FHaroEquipmentList EquipmentList;

};


// 설계 결정: EquipmentManager에 슬롯 개념 추가
// 
// 고민했던 두 가지 방안:
// 1. QuickBar에서 슬롯 관리 + 슬롯별 Instance 배열 추가
// 2. EquipmentManager에 슬롯 개념 추가
//
// 방안 2를 선택한 이유:
// - 주무기/보조무기별 스킬 강화 시스템에서 Handle과 슬롯 정보를 함께 다뤄야 함
// - 데이터 중복 관리 및 동기화 이슈 방지
// - 향후 무기 슬롯 외에 이동기 슬롯 등 추가 확장 가능성
// - 슬롯은 게임 규칙의 일부이므로 EquipmentManager가 관리하는 것이 자연스러움

// 이제 여기서 추가로 해야 하는 점:
// 스킬 선택 컴포넌트에 handle을 넘겨줘야 됨. (커스텀이 가능하도록)