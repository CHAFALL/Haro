// Fill out your copyright notice in the Description page of Project Settings.


#include "HaroSkillSelectComponent.h"
#include "Data/HaroSkillData.h"
#include "Player/LyraPlayerState.h"
#include "AbilitySystemComponent.h"



UHaroSkillSelectComponent::UHaroSkillSelectComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

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

// 스킬 교체 같은 기능은 아직 구현 전임.
// 무기 개조는 기존 스킬과 선택된 스킬끼리 교체 되어야 하는 경우가 있음.
bool UHaroSkillSelectComponent::ApplySelectedSkill(const FHaroSkillDataEntry& SelectedSkill)
{
	const FName& SkillID = SelectedSkill.SkillID;
	const FHaroSkillDataRow& Skill = SelectedSkill.SkillData;

	// 이미 있는 스킬이면 return
	if (OwnedSkillIDs.Contains(SkillID)) return false;

	// 해당 어빌리티가 없으면 return
	if (!Skill.AbilityClass.IsValid()) return false;

	UClass* AbilityClass = Skill.AbilityClass.LoadSynchronous();
	if (!AbilityClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load AbilityClass for skill: %s"), *Skill.Name);
		return false;
	}

	// 어빌리티 부여
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return false;

	FGameplayAbilitySpec AbilitySpec(
		AbilityClass,       // Ability Class
		Skill.SkillLevel,   // Level
		INDEX_NONE,         // InputID
		this                // SourceObject
	);

	FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(AbilitySpec);

	if (Handle.IsValid())
	{
		// 위에서 이미 중복 체크했으므로 바로 넣어도 됨.
		OwnedSkillIDs.Add(SkillID);

		UE_LOG(LogTemp, Log, TEXT("Skill Applied: %s (ID: %s, Level: %d)"),
			*Skill.Name,
			*SkillID.ToString(),
			Skill.SkillLevel);

		return true;
	}

	return false;
}


UAbilitySystemComponent* UHaroSkillSelectComponent::GetAbilitySystemComponent() const
{
	if (ALyraPlayerState* PlayerState = Cast<ALyraPlayerState>(GetOwner()))
	{
		return PlayerState->GetAbilitySystemComponent();
	}
	return nullptr;
}