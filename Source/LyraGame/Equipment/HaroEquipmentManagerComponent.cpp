// Fill out your copyright notice in the Description page of Project Settings.


#include "HaroEquipmentManagerComponent.h"

#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/ActorChannel.h"
#include "LyraEquipmentDefinition.h"
#include "LyraEquipmentInstance.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(HaroEquipmentManagerComponent)

class FLifetimeProperty;
struct FReplicationFlags;

//////////////////////////////////////////////////////////////////////
// FHaroAppliedEquipmentEntry

FString FHaroAppliedEquipmentEntry::GetDebugString() const
{
	// GetNameSafe : 크래시 발생안함. nullptr이면 "None" 반환.
	return FString::Printf(TEXT("%s of %s"), *GetNameSafe(Instance), *GetNameSafe(EquipmentDefinition.Get()));
}

//////////////////////////////////////////////////////////////////////
// FHaroEquipmentList

void FHaroEquipmentList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		const FHaroAppliedEquipmentEntry& Entry = Entries[Index];
		if (Entry.Instance != nullptr)
		{
			Entry.Instance->OnUnequipped();
		}
	}
}

void FHaroEquipmentList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		const FHaroAppliedEquipmentEntry& Entry = Entries[Index];
		if (Entry.Instance != nullptr)
		{
			Entry.Instance->OnEquipped();
		}
	}
}

void FHaroEquipmentList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	// 	for (int32 Index : ChangedIndices)
	// 	{
	// 		const FGameplayTagStack& Stack = Stacks[Index];
	// 		TagToCountMap[Stack.Tag] = Stack.StackCount;
	// 	}
}

ULyraAbilitySystemComponent* FHaroEquipmentList::GetAbilitySystemComponent() const
{
	check(OwnerComponent);
	AActor* OwningActor = OwnerComponent->GetOwner();
	return Cast<ULyraAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwningActor));
}

ULyraEquipmentInstance* FHaroEquipmentList::AddEntry(TSubclassOf<ULyraEquipmentDefinition> EquipmentDefinition, int32 SlotIndex)
{
	ULyraEquipmentInstance* Result = nullptr;

	check(EquipmentDefinition != nullptr);
	check(OwnerComponent);
	check(OwnerComponent->GetOwner()->HasAuthority());

	const ULyraEquipmentDefinition* EquipmentCDO = GetDefault<ULyraEquipmentDefinition>(EquipmentDefinition);

	TSubclassOf<ULyraEquipmentInstance> InstanceType = EquipmentCDO->InstanceType;
	if (InstanceType == nullptr)
	{
		InstanceType = ULyraEquipmentInstance::StaticClass();
	}
	// 새 장비 슬롯 생성하고 정보 입력.
	FHaroAppliedEquipmentEntry& NewEntry = Entries.AddDefaulted_GetRef(); // 배열의 끝에 기본값으로 초기화된 새 요소를 추가하고 그 요소의 참조를 반환
	NewEntry.EquipmentDefinition = EquipmentDefinition;
	NewEntry.SlotIndex = SlotIndex;  // 슬롯 정보 추가
	// 생성은 여기서.(NewObject)
	NewEntry.Instance = NewObject<ULyraEquipmentInstance>(OwnerComponent->GetOwner(), InstanceType);  //@TODO: Using the actor instead of component as the outer due to UE-127172
	Result = NewEntry.Instance;

	if (ULyraAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		for (const TObjectPtr<const ULyraAbilitySet>& AbilitySet : EquipmentCDO->AbilitySetsToGrant)
		{
			// inout해서 그 주입된 handles를 가져오는구먼.
			AbilitySet->GiveToAbilitySystem(ASC, /*inout*/ &NewEntry.GrantedHandles, Result);
		}
	}
	else
	{
		//@TODO: Warning logging?
	}

	// 인스턴스 -> 메모리에만 존재하는 데이터 객체, 실제 월드 스폰은 별도로 필요.
	Result->SpawnEquipmentActors(EquipmentCDO->ActorsToSpawn);


	MarkItemDirty(NewEntry); // 이 아이템이 변경되었으니 클라이언트들에게 복제해달라고 함.

	return Result;
}

void FHaroEquipmentList::RemoveEntry(int32 SlotIndex)
{
	// CreateIterator : 배열을 안전하게 순회하면서 요소를 제거할 수 있는 반복자 생성
	// 일반 for문으로 제거하는 것이랑 다르게 안전한 방법임. (인덱스 조정 같은 것을 해줌.)
	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		FHaroAppliedEquipmentEntry& Entry = *EntryIt;
		if (Entry.SlotIndex == SlotIndex)
		{
			if (ULyraAbilitySystemComponent* ASC = GetAbilitySystemComponent())
			{
				Entry.GrantedHandles.TakeFromAbilitySystem(ASC);
			}

			Entry.Instance->DestroyEquipmentActors();

			EntryIt.RemoveCurrent();
			MarkArrayDirty();
			return;
		}
	}
}

void FHaroEquipmentList::ActivateEntryHandles(FHaroAppliedEquipmentEntry& Entry)
{
	ULyraAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("ActivateEntryHandles: ASC not found"));
		return;
	}

	// Ability Handle 활성화 (입력 차단 해제)
	for (const FGameplayAbilitySpecHandle& Handle : Entry.GrantedHandles.AbilitySpecHandles)
	{
		if (FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle))
		{
			// 입력 차단 해제
			ASC->UnBlockAbilityByInputID(Spec->InputID);
		}
	}

	//// GameplayEffect 활성화 (무기 강화 효과 등)
	//for (const FActiveGameplayEffectHandle& Handle : Entry.GrantedHandles.GameplayEffectHandles)
	//{
	//	// Effect 레벨을 원래대로 복구 (0에서 원래 레벨로)
	//	ASC->SetActiveGameplayEffectLevel(Handle, 1);  // 또는 저장해둔 원래 레벨
	//}
}

void FHaroEquipmentList::DeactivateEntryHandles(FHaroAppliedEquipmentEntry& Entry)
{
	ULyraAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return;

	// Ability 비활성화
	for (const FGameplayAbilitySpecHandle& Handle : Entry.GrantedHandles.AbilitySpecHandles)
	{
		if (FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle))
		{
			ASC->BlockAbilityByInputID(Spec->InputID);
		}
	}

	//// GameplayEffect 비활성화 (강화 효과도 함께 꺼짐)
	//for (const FActiveGameplayEffectHandle& Handle : Entry.GrantedHandles.GameplayEffectHandles)
	//{
	//	// Effect 레벨을 0으로 설정 (효과 없음)
	//	ASC->SetActiveGameplayEffectLevel(Handle, 0);
	//}
}

//////////////////////////////////////////////////////////////////////
// UHaroEquipmentManagerComponent

UHaroEquipmentManagerComponent::UHaroEquipmentManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, EquipmentList(this)
{
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true; // InitializeComponent 함수가 자동으로 호출되도록 설정, 컴포넌트 초기화 로직이 실행됨.
}

void UHaroEquipmentManagerComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, EquipmentList);
}

ULyraEquipmentInstance* UHaroEquipmentManagerComponent::EquipItem(TSubclassOf<ULyraEquipmentDefinition> EquipmentClass, int32 SlotIndex)
{
	ULyraEquipmentInstance* Result = nullptr;
	if (EquipmentClass != nullptr)
	{
		Result = EquipmentList.AddEntry(EquipmentClass, SlotIndex); // SlotIndex 전달 추가
		if (Result != nullptr)
		{
			// 비활성화 상태로 생성
			// 이유: 순간적으로 2개가 활성화 됨을 방지.
			// UHaroQuickBarComponent에서 장착한 무기 활성화 하는 로직이 있음
			//Result->OnEquipped(); // 여기도 OnEquipped가 있네. (여긴 서버측)

			// 네트워크 최적화 기술.
			// Subobject는 메인 오브젝트(ex. UHaroEquipmentManagerComponent )랑 달리 자동 복제가 안되서 수동 등록 필요.
			// IsUsingRegisteredSubObjectList() 복제 신기술 가능 여부 확인.
			// IsReadyForReplication() 네트워크 복제 가능 여부 확인.
			if (IsUsingRegisteredSubObjectList() && IsReadyForReplication())
			{
				AddReplicatedSubObject(Result);
			}
		}
	}
	return Result;
}

void UHaroEquipmentManagerComponent::UnequipItem(int32 SlotIndex)
{
	for (FHaroAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (Entry.SlotIndex == SlotIndex)
		{
			if (IsUsingRegisteredSubObjectList())
			{
				RemoveReplicatedSubObject(Entry.Instance);
			}

			Entry.Instance->OnUnequipped();
			EquipmentList.RemoveEntry(SlotIndex);
			return;
		}
	}
}

void UHaroEquipmentManagerComponent::ActivateItem(int32 SlotIndex)
{
	for (FHaroAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (Entry.SlotIndex == SlotIndex)
		{
			// 이미 활성화되어 있으면 스킵
			if (Entry.bIsActive)
				return;

			// Handle 활성화
			EquipmentList.ActivateEntryHandles(Entry);

			// Instance 활성화 (비주얼, 액터 등)
			Entry.Instance->OnEquipped();

			// 상태 플래그 설정
			Entry.bIsActive = true;
			return;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("ActivateItemBySlot: No equipment found in slot %d"), SlotIndex);
}

void UHaroEquipmentManagerComponent::DeactivateItem(int32 SlotIndex)
{
	for (FHaroAppliedEquipmentEntry& Entry : EquipmentList.Entries) // (어차피 최대 2개라 이렇게 해도 될듯.)
	{
		if (Entry.SlotIndex == SlotIndex)
		{
			// 이미 비활성화되어 있으면 스킵
			if (!Entry.bIsActive)
				return;

			// Instance 비활성화 (비주얼, 액터 등)
			Entry.Instance->OnUnequipped();

			// Handle 비활성화
			EquipmentList.DeactivateEntryHandles(Entry);

			// 상태 플래그 설정
			Entry.bIsActive = false;
			return;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("DeactivateItemBySlot: No equipment found in slot %d"), SlotIndex);
}


// 구방식 (IsUsingRegisteredSubObjectList()가 false일때 발동.)
bool UHaroEquipmentManagerComponent::ReplicateSubobjects(UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	for (FHaroAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		ULyraEquipmentInstance* Instance = Entry.Instance;

		if (IsValid(Instance))
		{
			WroteSomething |= Channel->ReplicateSubobject(Instance, *Bunch, *RepFlags);
		}
	}

	return WroteSomething;
}

void UHaroEquipmentManagerComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

// 컴포넌트가 종료될 때 모든 장비를 안전하게 제거
// 캐릭터가 교체 되거나 죽고 리스폰 될 때 (액터 파괴만 안 하면 상관없는 일임.)
// 여기도 인덱스 기반으로 변경.
void UHaroEquipmentManagerComponent::UninitializeComponent()
{
	TArray<int32> AllSlotIndices;

	// SlotIndex 수집
	for (const FHaroAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		AllSlotIndices.Add(Entry.SlotIndex);
	}

	// SlotIndex로 제거
	for (int32 SlotIndex : AllSlotIndices)
	{
		UnequipItem(SlotIndex);
	}

	Super::UninitializeComponent();
}

void UHaroEquipmentManagerComponent::ReadyForReplication()
{
	Super::ReadyForReplication();

	// Register existing LyraEquipmentInstances
	if (IsUsingRegisteredSubObjectList())
	{
		for (const FHaroAppliedEquipmentEntry& Entry : EquipmentList.Entries)
		{
			ULyraEquipmentInstance* Instance = Entry.Instance;

			if (IsValid(Instance))
			{
				AddReplicatedSubObject(Instance);
			}
		}
	}
}

ULyraEquipmentInstance* UHaroEquipmentManagerComponent::GetFirstInstanceOfType(TSubclassOf<ULyraEquipmentInstance> InstanceType)
{
	for (FHaroAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (ULyraEquipmentInstance* Instance = Entry.Instance)
		{
			if (Instance->IsA(InstanceType))
			{
				return Instance;
			}
		}
	}

	return nullptr;
}

TArray<ULyraEquipmentInstance*> UHaroEquipmentManagerComponent::GetEquipmentInstancesOfType(TSubclassOf<ULyraEquipmentInstance> InstanceType) const
{
	TArray<ULyraEquipmentInstance*> Results;
	for (const FHaroAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (ULyraEquipmentInstance* Instance = Entry.Instance)
		{
			if (Instance->IsA(InstanceType))
			{
				Results.Add(Instance);
			}
		}
	}
	return Results;
}


