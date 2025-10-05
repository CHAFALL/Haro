// Fill out your copyright notice in the Description page of Project Settings.


#include "HaroSkillEntryWidget.h"
#include "HaroSkillSelectionWidget.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/RichTextBlock.h"
#include "Components/TextBlock.h"
#include "Player/LyraPlayerState.h"
#include "Items/Skill/HaroSkillSelectComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(HaroSkillEntryWidget)

UHaroSkillEntryWidget::UHaroSkillEntryWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UHaroSkillEntryWidget::SetInfo(UHaroSkillSelectionWidget* InOwnerWidget, const FHaroSkillDataEntry& InSkillData)
{
    CachedOwnerWidget = InOwnerWidget;
    
    SkillData = InSkillData;

    RefreshUI();
}

void UHaroSkillEntryWidget::RefreshUI()
{
    if (!SkillData.SkillData.Name.IsEmpty())
    {
        // 스킬 이름 설정
        if (Text_SkillName)
            Text_SkillName->SetText(FText::FromString(SkillData.SkillData.Name));

        // 스킬 설명 설정
        if (Text_SkillDescription)
            Text_SkillDescription->SetText(FText::FromString(SkillData.SkillData.Description));

        // 스킬 아이콘 설정
        if (Image_Skill && SkillData.SkillData.IconTexture.IsValid())
        {
            if (UTexture2D* IconTexture = SkillData.SkillData.IconTexture.LoadSynchronous())
                Image_Skill->SetBrushFromTexture(IconTexture);
        }
    }
}

void UHaroSkillEntryWidget::OnButtonClicked()
{
	// 해당 스킬을 골랐다는 로직 처리
    if (UHaroSkillSelectComponent* SkillComp = GetSkillSelectComponent())
    {
        // 스킬 부여하고
        SkillComp->ServerApplySelectedSkill(SkillData);
        
        // 창 닫기 -> 이렇게 직접참조 하는 방식 말고도 델리게이트 방식도 가능. (스킬 선택 시 여러 시스템 반응 필요해진다면..)
        if (UHaroSkillSelectionWidget* SkillSelectionWidget = CachedOwnerWidget.Get())
        {
            SkillSelectionWidget->DeactivateWidget();
            SkillSelectionWidget = nullptr;
        }
    }

	
}

UHaroSkillSelectComponent* UHaroSkillEntryWidget::GetSkillSelectComponent() const
{
    if (APlayerController* PC = GetOwningPlayer())
    {
        if (ALyraPlayerState* PlayerState = PC->GetPlayerState<ALyraPlayerState>())
        {
            return PlayerState->FindComponentByClass<UHaroSkillSelectComponent>();
        }
    }

    return nullptr;
}