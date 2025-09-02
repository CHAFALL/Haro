// Fill out your copyright notice in the Description page of Project Settings.


#include "HaroSkillSelectionWidget.h"
#include "HaroSkillEntryWidget.h"
#include "Components/VerticalBox.h"

#include "Data/HaroSkillData.h"
#include "GameplayTagContainer.h"
#include "Player/LyraPlayerState.h"
#include "Items/Skill/HaroSkillSelectComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(HaroSkillSelectionWidget)

UHaroSkillSelectionWidget::UHaroSkillSelectionWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
    
}

void UHaroSkillSelectionWidget::SetSkillCategory(const FGameplayTag& InSkillCategoryTag)
{
    // NativeConstruct()보다 먼저 호출되어야 함.
    SkillCategoryTag = InSkillCategoryTag;

    if (VerticalBox_SkillElements && SkillWidgets.Num() > 0)
    {
        RefreshUI();
    }
}

void UHaroSkillSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();

    SetInfo();

}

void UHaroSkillSelectionWidget::SetInfo()
{

    // 기존 위젯들 정리
    VerticalBox_SkillElements->ClearChildren();

    // 최대 개수만큼 스킬 위젯 미리 생성
    for (int32 i = 0; i < MAX_SKILL_OPTIONS; i++)
    {
        if (!SkillEntryWidgetClass) continue;

        UHaroSkillEntryWidget* SkillWidget = CreateWidget<UHaroSkillEntryWidget>(this, SkillEntryWidgetClass);
        if (!SkillWidget) continue;

        // 초기에는 숨김 처리
        SkillWidget->SetVisibility(ESlateVisibility::Collapsed);

        // VerticalBox에 추가
        VerticalBox_SkillElements->AddChild(SkillWidget);
        SkillWidgets.Add(SkillWidget);

    }

    // 태그가 이미 설정되어 있다면 UI 갱신
    if (SkillCategoryTag.IsValid())
    {
        RefreshUI();
    }

}

void UHaroSkillSelectionWidget::RefreshUI()
{
    // 태그 유효성 확인
    if (!SkillCategoryTag.IsValid())
        return;


    if (UHaroSkillSelectComponent* SkillComp = GetSkillSelectComponent())
    {
        // 지역 변수로만 사용
        TArray<FHaroSkillDataEntry> SkillOptions = SkillComp->GenerateSkillOptions(SkillCategoryTag);

        for (int32 i = 0; i < SkillWidgets.Num(); i++)
        {
            if (i < SkillOptions.Num())
            {
                SkillWidgets[i]->SetInfo(this, SkillOptions[i]); // 각 위젯이 데이터 보관
                SkillWidgets[i]->SetVisibility(ESlateVisibility::Visible);
            }
            else
            {
                SkillWidgets[i]->SetVisibility(ESlateVisibility::Collapsed);
            }
        }
    }

    
}

UHaroSkillSelectComponent* UHaroSkillSelectionWidget::GetSkillSelectComponent() const
{
    if (ALyraPlayerState* PlayerState = GetOwningPlayerState<ALyraPlayerState>())
    {
        return PlayerState->FindComponentByClass<UHaroSkillSelectComponent>();
    }

    return nullptr;
}
