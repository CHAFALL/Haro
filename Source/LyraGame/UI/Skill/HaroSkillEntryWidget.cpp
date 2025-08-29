// Fill out your copyright notice in the Description page of Project Settings.


#include "HaroSkillEntryWidget.h"
#include "HaroSkillSelectionWidget.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/RichTextBlock.h"
#include "Components/TextBlock.h"

#include "AbilitySystem/Abilities/LyraGameplayAbility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(HaroSkillEntryWidget)

UHaroSkillEntryWidget::UHaroSkillEntryWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UHaroSkillEntryWidget::InitializeUI(UHaroSkillSelectionWidget* OwnerWidget)
{
	CachedOwnerWidget = OwnerWidget;

	// 타입 같은거 받아서 그걸 보고 정보 넣기
}



void UHaroSkillEntryWidget::OnButtonClicked()
{
	// 해당 스킬을 골랐다는 로직 처리

	// 스킬 골랐으니 창 닫기
	if (UHaroSkillSelectionWidget* SkillSelectionWidget = CachedOwnerWidget.Get())
	{
		SkillSelectionWidget->DeactivateWidget();
		SkillSelectionWidget = nullptr;
	}
}
