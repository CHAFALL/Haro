// Fill out your copyright notice in the Description page of Project Settings.


#include "HaroSkillSelectComponent.h"
#include "Data/HaroSkillData.h"
#include "Player/LyraPlayerState.h"
#include "AbilitySystemComponent.h"
#include "Equipment/LyraEquipmentInstance.h"



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
	if (OwnedSkillHandles.Contains(SkillEntry.SkillID)) return false;

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
		if (!OwnedSkillHandles.Contains(RequiredSkillID))
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
		if (OwnedSkillHandles.Contains(ConflictSkillID))
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


bool UHaroSkillSelectComponent::ApplySelectedSkill(const FHaroSkillDataEntry& SelectedSkill)
{
	const FName& SkillID = SelectedSkill.SkillID;
	const FHaroSkillDataRow& Skill = SelectedSkill.SkillData;

	// 이미 있는 스킬이면 return
	if (OwnedSkillHandles.Contains(SkillID)) return false;

	UClass* AbilityClass = Skill.AbilityClass.LoadSynchronous();
	if (!AbilityClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load AbilityClass for skill: %s"), *Skill.Name);
		return false;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return false;


	// Temp : 테스트를 위해, 남은 문제점: 무기를 들을 때 어떻게 handle을 넘겨줄 것인가가 관건.
	// 사실 내가 직접 아이템을 만들어서 하면 그냥 쉽게 되지만
	// abilityset이랑 엮기기 시작하면 어려워짐.
	// 아니면 던전에 입장할 때의 무기 태그를 알꺼잖아? -> 그럼 그 태그를 통해서 조회를 하는거지, 게임이 던전에 들어갈 때만. -> 이것도 lyra 구조 때문에 의미가 없음.
	
	// 그리고 코드를 좀 뜯어보니 알게된 사실이 LyraEquipmentManagerComponent의 FLyraAppliedEquipmentEntry 무기에 대한 handles을 관리하고 있음
	// EquipmentList에 들어가는 entry는 현재 장착된 무기만이고, 슬롯의 무기를 바꿔낄때마다 새로 handle이 교체됨..... -> 이러면 안되는데????
	
	// 생긴 문제점 : 검 베기 스킬 -> 강화된 검 베기 스킬로 아이템을 먹고 바꿨는데 슬롯을 변경해서 무기를 바꿔끼면서 원래 검 베기 스킬로 돌아감....
	// 이게 왜 생기냐면 슬롯 무기 변경을 할 때마다 인스턴스를 새로 만듬....
	// -> 나만의 장비 관리자와 퀵바 관리자를 만들어야 할 확률이 높아짐... -> 그렇게 어려워 보이지는 않음.

	// 그리고 default 스킬들에 대한 data들을 따로 빼서 관리를 해야 할 수도 있겠다.
	// 지금은 모든 스킬을 같이 관리하고 있는데.
	FGameplayAbilitySpecHandle OldAbilityHandle;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.DynamicAbilityTags.HasTag(FGameplayTag::RequestGameplayTag("InputTag.Weapon.AltFire")))
		{
			OldAbilityHandle = Spec.Handle;
			break; // 첫 번째 것만 찾기 -> 그리고 무기를 2개 들고 다님 -> 이거야 뭐 태그를 통해서 따로 구분하면 되긴 함.
		}
	}

	RegisterSkillHandle(FName("SKL_000"), OldAbilityHandle);



	// 어빌리티 클래스도 있고 여러 검증을 통과했으니 본격적으로 교체할 스킬 제거
	// 추가적으로 SourceObject를 가져옴.
	UObject* SourceObject = nullptr;
	if (!RemoveReplacedSkill(Skill.ReplaceSkillIDs, ASC, SourceObject))
		return false; // 제거 실패하면 새 스킬도 추가하지 않음.

	
	if (!AddNewSkill(SkillID, Skill, AbilityClass, ASC, SourceObject))
		return false;


	return true;

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
		UE_LOG(LogTemp, Log, TEXT("Skill Added: %s (ID: %s, Level: %d)"),
			*SkillData.Name, *SkillID.ToString(), SkillData.SkillLevel);
		return true;
	}

	return false;

}


bool UHaroSkillSelectComponent::RegisterSkillHandle(const FName& SkillID, const FGameplayAbilitySpecHandle& Handle)
{
	if (!Handle.IsValid() || OwnedSkillHandles.Contains(SkillID)) return false;

	OwnedSkillHandles.Add(SkillID, Handle);

	return true;
}

bool UHaroSkillSelectComponent::UnregisterSkillHandle(const FName& SkillID)
{
	// 핸들 맵에서 찾기
	FGameplayAbilitySpecHandle* FoundHandle = OwnedSkillHandles.Find(SkillID);
	if (!FoundHandle || !FoundHandle->IsValid())
		return false;

	// 맵에서 제거
	OwnedSkillHandles.Remove(SkillID);

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