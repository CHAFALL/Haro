// Fill out your copyright notice in the Description page of Project Settings.


#include "HaroSkillSelectComponent.h"
#include "Data/HaroSkillData.h"
#include "Player/LyraPlayerState.h"
#include "AbilitySystemComponent.h"
#include "Equipment/LyraEquipmentInstance.h"
#include "Net/UnrealNetwork.h"



UHaroSkillSelectComponent::UHaroSkillSelectComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
}

void UHaroSkillSelectComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, OwnedSkillIDs);
}

void UHaroSkillSelectComponent::BeginPlay()
{
	Super::BeginPlay();

	// 테스트를 위해 여기서 무기 태그 임시로 넣기.
	CurrentWeaponTags.Add(FGameplayTag::RequestGameplayTag("Skill.Weapon.Bow"));
	CurrentWeaponTags.Add(FGameplayTag::RequestGameplayTag("Skill.Weapon.Rifle"));
}

TArray<FHaroSkillDataEntry> UHaroSkillSelectComponent::GenerateSkillOptions(const FGameplayTag& SkillCategory)
{
	// 무기 스킬인 경우
	if (SkillCategory.MatchesTag(FGameplayTag::RequestGameplayTag("Skill.Weapon")))
	{
		return GenerateSkillOptionsForWeapons();
	}

	// TODO
	// 나머지(원소, 강화 등)는 나중에 구현 예정.

	return TArray<FHaroSkillDataEntry>();
}

TArray<FHaroSkillDataEntry> UHaroSkillSelectComponent::GenerateSkillOptionsForWeapons()
{
	if (CurrentWeaponTags.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("No weapon tags found. Cannot generate weapon skill options."));
		return TArray<FHaroSkillDataEntry>();
	}

	const UHaroSkillData& SkillData = UHaroSkillData::Get();
	TArray<FHaroSkillDataEntry> AllAvailableSkills;

	// 각 무기 태그에 해당하는 스킬들을 수집
	for (const FGameplayTag& WeaponTag : CurrentWeaponTags)
	{
		TArray<FHaroSkillDataEntry> WeaponSkills = SkillData.GetSkillsByTag(WeaponTag);

		for (const FHaroSkillDataEntry& SkillEntry : WeaponSkills)
		{
			// 선택 가능한 스킬인지 확인
			if (IsSkillSelectable(SkillEntry))
			{
				AllAvailableSkills.Add(SkillEntry);

				UE_LOG(LogTemp, Log, TEXT("Added selectable skill: %s (ID: %s)"),
					*SkillEntry.SkillData.Name, *SkillEntry.SkillID.ToString());
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Total available skills: %d"), AllAvailableSkills.Num());

	// 랜덤하게 MaxSkillOptions 개수만큼 선택
	return SelectRandomSkills(AllAvailableSkills, MaxSkillOptions);
}


bool UHaroSkillSelectComponent::IsSkillSelectable(const FHaroSkillDataEntry& SkillEntry) const
{
	// 1. 이미 보유한 스킬인지 확인 (중복 불허)
	if (OwnedSkillIDs.Contains(SkillEntry.SkillID)) return false;

	// 2. 필수 스킬 조건 확인
	if (!CheckRequiredSkills(SkillEntry)) return false;

	// 3. 충돌 스킬 조건 확인
	if (!CheckConflictSkills(SkillEntry)) return false;

	return true;
}

bool UHaroSkillSelectComponent::CheckRequiredSkills(const FHaroSkillDataEntry& SkillEntry) const
{
	const FHaroSkillDataRow& Skill = SkillEntry.SkillData;

	// 필수 스킬이 없으면 통과
	if (Skill.RequiredSkillIDs.IsEmpty()) return true;

	// 모든 필수 스킬을 보유하고 있는지 확인
	for (const FName& RequiredSkillID : Skill.RequiredSkillIDs)
	{
		if (!OwnedSkillIDs.Contains(RequiredSkillID))
			return false;
	}

	return true;
}

bool UHaroSkillSelectComponent::CheckConflictSkills(const FHaroSkillDataEntry& SkillEntry) const
{
	const FHaroSkillDataRow& Skill = SkillEntry.SkillData;

	// 충돌 스킬이 없으면 통과
	if (Skill.ConflictSkillIDs.IsEmpty()) return true;

	// 충돌하는 스킬을 하나라도 보유하고 있으면 선택 불가
	for (const FName& ConflictSkillID : Skill.ConflictSkillIDs)
	{
		if (OwnedSkillIDs.Contains(ConflictSkillID))
			return false;
	}

	return true;
}

TArray<FHaroSkillDataEntry> UHaroSkillSelectComponent::SelectRandomSkills(const TArray<FHaroSkillDataEntry>& AvailableSkills, int32 Count) const
{
	TArray<FHaroSkillDataEntry> SelectedSkills;
	TArray<FHaroSkillDataEntry> SkillPool = AvailableSkills;

	// 요청된 개수만큼 또는 사용 가능한 스킬 개수만큼 선택
	int32 SelectionCount = FMath::Min(Count, SkillPool.Num());

	for (int32 i = 0; i < SelectionCount; ++i)
	{
		if (SkillPool.IsEmpty())
		{
			break;
		}

		// 랜덤 인덱스 선택
		int32 RandomIndex = FMath::RandRange(0, SkillPool.Num() - 1);

		// 선택된 스킬을 결과 배열에 추가
		SelectedSkills.Add(SkillPool[RandomIndex]);

		// 중복 선택 방지를 위해 풀에서 제거
		SkillPool.RemoveAt(RandomIndex);
	}

	return SelectedSkills;
}

void UHaroSkillSelectComponent::ServerApplySelectedSkill_Implementation(const FHaroSkillDataEntry& SelectedSkill)
{
	const FName& SkillID = SelectedSkill.SkillID;
	const FHaroSkillDataRow& Skill = SelectedSkill.SkillData;

	// 이미 있는 스킬이면 return
	if (OwnedSkillIDs.Contains(SkillID)) return;

	UClass* AbilityClass = Skill.AbilityClass.LoadSynchronous();
	if (!AbilityClass) return;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return;

	// 어빌리티 클래스도 있고 여러 검증을 통과했으니 본격적으로 교체할 스킬 제거
	// 추가적으로 SourceObject를 가져옴.
	UObject* SourceObject = nullptr;
	if (!RemoveReplacedSkill(Skill.ReplaceSkillIDs, ASC, SourceObject))
		return; // 제거 실패하면 새 스킬도 추가하지 않음.


	if (!AddNewSkill(SkillID, Skill, AbilityClass, ASC, SourceObject))
		return;


	return;
}


bool UHaroSkillSelectComponent::RemoveReplacedSkill(const TArray<FName>& ReplaceSkillIDs, UAbilitySystemComponent* ASC, /*OUT*/ UObject*& OutSourceObject)
{

	OutSourceObject = nullptr;

	// 교체할 스킬이 없으면 성공으로 처리
	if (ReplaceSkillIDs.IsEmpty()) return true;

	for (const FName& ReplaceSkillID : ReplaceSkillIDs)
	{
		// 소유한 스킬인지 확인
		if (FGameplayAbilitySpecHandle* FoundHandle = OwnedSkillHandles.Find(ReplaceSkillID))
		{
			// SourceObject 추출 (첫 번째로 찾은 것 사용)
			if (!OutSourceObject)
			{
				FGameplayAbilitySpec* FoundSpec = ASC->FindAbilitySpecFromHandle(*FoundHandle);
				if (FoundSpec && FoundSpec->SourceObject.IsValid())
				{
					OutSourceObject = FoundSpec->SourceObject.Get();
				}
			}

			// ASC에서 어빌리티 제거
			ASC->ClearAbility(*FoundHandle);

			// 맵에서도 제거
			OwnedSkillHandles.Remove(ReplaceSkillID);
			OwnedSkillIDs.Remove(ReplaceSkillID);

			UE_LOG(LogTemp, Log, TEXT("Successfully replaced skill: %s"), *ReplaceSkillID.ToString());
		}
	}

	return true;
}

bool UHaroSkillSelectComponent::AddNewSkill(const FName& SkillID, const FHaroSkillDataRow& SkillData, UClass* AbilityClass, UAbilitySystemComponent* ASC, UObject* SourceObject)
{
	FGameplayAbilitySpec AbilitySpec(
		AbilityClass, // 이미 검증된 클래스 사용
		SkillData.SkillLevel,
		INDEX_NONE,
		SourceObject
	);

	// InputTag 할당
	if (SkillData.InputTag.IsValid())
	{
		AbilitySpec.DynamicAbilityTags.AddTag(SkillData.InputTag);
	}

	// 어빌리티 부여
	FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(AbilitySpec);

	if (Handle.IsValid())
	{
		// 이미 중복 체크 했으므로 바로
		OwnedSkillHandles.Add(SkillID, Handle);
		OwnedSkillIDs.AddUnique(SkillID);
		UE_LOG(LogTemp, Log, TEXT("Skill Added: %s (ID: %s, Level: %d)"),
			*SkillData.Name, *SkillID.ToString(), SkillData.SkillLevel);
		return true;
	}

	return false;

}


bool UHaroSkillSelectComponent::RegisterSkillHandle(const FName& SkillID, const FGameplayAbilitySpecHandle& Handle)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
		return false;

	if (!Handle.IsValid() || OwnedSkillHandles.Contains(SkillID)) return false;

	OwnedSkillHandles.Add(SkillID, Handle);
	OwnedSkillIDs.AddUnique(SkillID);

	return true;
}

bool UHaroSkillSelectComponent::UnregisterSkillHandle(const FName& SkillID)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
		return false;

	// 핸들 맵에서 찾기
	FGameplayAbilitySpecHandle* FoundHandle = OwnedSkillHandles.Find(SkillID);
	if (!FoundHandle || !FoundHandle->IsValid())
		return false;

	// 맵에서 제거
	OwnedSkillHandles.Remove(SkillID);
	OwnedSkillIDs.Remove(SkillID);

	return true;
}

UAbilitySystemComponent* UHaroSkillSelectComponent::GetAbilitySystemComponent() const
{
	if (ALyraPlayerState* PlayerState = Cast<ALyraPlayerState>(GetOwner()))
	{
		return PlayerState->GetAbilitySystemComponent();
	}
	return nullptr;
}