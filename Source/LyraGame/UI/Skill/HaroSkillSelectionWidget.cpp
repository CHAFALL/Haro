// Fill out your copyright notice in the Description page of Project Settings.


#include "HaroSkillSelectionWidget.h"
#include "HaroSkillEntryWidget.h"
#include "Components/VerticalBox.h"

#include "Data/HaroSkillData.h"
#include "GameplayTagContainer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(HaroSkillSelectionWidget)

UHaroSkillSelectionWidget::UHaroSkillSelectionWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UHaroSkillSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();

    TestSkillDataRetrieval();

	VerticalBox_SkillElements->ClearChildren();

	// 여기에 다는 로직 추가 하기
	

	// 선택지 관련된 정보를 제공 받고 (아마 컴포넌트로 뺄 듯?)
	// 그거에 맞게 또 정보를 넘겨줘서 만들기

	// 고민 거리 - 이걸 어떻게 데이터 관리해야 좋을까?
	// 기본 스킬 (불, 얼음, 번개와 같은)
	// 망치 스킬 (무기에 따른)
	// 강화 스킬
}

void UHaroSkillSelectionWidget::TestSkillDataRetrieval()
{
    UE_LOG(LogTemp, Warning, TEXT("=== Starting Skill Data Retrieval Test ==="));

    // 테스트할 태그들
    TArray<FString> TestTags = {
        "Skill.Weapon.Bow",
        "Skill.Weapon.Rifle",
        "Skill.Weapon.Sword",  // 없을 수도 있는 태그
        "InvalidTag"           // 존재하지 않는 태그
    };

    for (const FString& TagString : TestTags)
    {
        UE_LOG(LogTemp, Warning, TEXT("--- Testing tag: %s ---"), *TagString);

        FGameplayTag TestTag = FGameplayTag::RequestGameplayTag(*TagString, false);

        if (!TestTag.IsValid())
        {
            UE_LOG(LogTemp, Error, TEXT("❌ Invalid GameplayTag: %s"), *TagString);
            continue;
        }

        // 스킬 데이터 조회
        TArray<FHaroSkillDataRow> Skills = UHaroSkillData::Get().GetSkillsByTag(TestTag);

        UE_LOG(LogTemp, Warning, TEXT("✅ Found %d skills for tag: %s"), Skills.Num(), *TagString);

        // 각 스킬의 상세 정보 출력
        for (int32 i = 0; i < Skills.Num(); i++)
        {
            const FHaroSkillDataRow& Skill = Skills[i];
            UE_LOG(LogTemp, Warning, TEXT("   [%d] Name: %s"), i, *Skill.Name);
            UE_LOG(LogTemp, Warning, TEXT("       Description: %s"), *Skill.Description);
            UE_LOG(LogTemp, Warning, TEXT("       GameplayTag: %s"), *Skill.GameplayTag.ToString());
            UE_LOG(LogTemp, Warning, TEXT("       HasIcon: %s"), Skill.IconTexture.IsValid() ? TEXT("Yes") : TEXT("No"));
            UE_LOG(LogTemp, Warning, TEXT("       HasAbility: %s"), Skill.AbilityClass.IsValid() ? TEXT("Yes") : TEXT("No"));
            UE_LOG(LogTemp, Warning, TEXT("       RequiredSkills: %d"), Skill.RequiredSkillIDs.Num());
            UE_LOG(LogTemp, Warning, TEXT("       ConflictSkills: %d"), Skill.ConflictSkillIDs.Num());
        }

        if (Skills.Num() == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("   ⚠️ No skills found for this tag"));
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("=== Skill Data Retrieval Test Complete ==="));
}
