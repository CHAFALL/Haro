// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CommonUserWidget.h"
#include "HaroSkillEntryWidget.generated.h"

class URichTextBlock;
class UTextBlock;
class UButton;
class UImage;
class UHaroSkillSelectionWidget;

/**
 * 
 */
UCLASS()
class LYRAGAME_API UHaroSkillEntryWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	UHaroSkillEntryWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	// 스킬 타입도 같이 해서 받거나 하기
	void InitializeUI(UHaroSkillSelectionWidget* OwnerWidget);

private:
	UFUNCTION()
	void OnButtonClicked();

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Skill;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Skill;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_SkillName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<URichTextBlock> Text_SkillDescription;

private:

	// 여기에 스킬 타입도 cache해야 할지도.

	UPROPERTY()
	TWeakObjectPtr<UHaroSkillSelectionWidget> CachedOwnerWidget;
};
