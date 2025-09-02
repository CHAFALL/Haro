// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CommonUserWidget.h"
#include "Data/HaroSkillData.h"
#include "HaroSkillEntryWidget.generated.h"

class URichTextBlock;
class UTextBlock;
class UButton;
class UImage;
class UHaroSkillSelectionWidget;
class UHaroSkillSelectComponent;

/**
 * 
 */
UCLASS()
class LYRAGAME_API UHaroSkillEntryWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	UHaroSkillEntryWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	UFUNCTION(BlueprintCallable)
	void SetInfo(UHaroSkillSelectionWidget* InOwnerWidget, const FHaroSkillDataEntry& InSkillData);

	void RefreshUI();

protected:
	UFUNCTION(BlueprintCallable)
	void OnButtonClicked();

private:
	UHaroSkillSelectComponent* GetSkillSelectComponent() const;

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
	UPROPERTY()
	FHaroSkillDataEntry SkillData;

	UPROPERTY()
	TWeakObjectPtr<UHaroSkillSelectionWidget> CachedOwnerWidget;
};
