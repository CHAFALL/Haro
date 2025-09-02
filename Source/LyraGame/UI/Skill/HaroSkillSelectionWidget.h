// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UI/LyraActivatableWidget.h"
#include "GameplayTagContainer.h"
#include "HaroSkillSelectionWidget.generated.h"


class UVerticalBox;
class UHaroSkillEntryWidget;
class UHaroSkillSelectComponent;


/**
 * 
 */
UCLASS()
class LYRAGAME_API UHaroSkillSelectionWidget : public ULyraActivatableWidget
{
	GENERATED_BODY()
	
public:
	UHaroSkillSelectionWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// 어빌리티에서 스킬 카테고리를 설정하는 함수
	UFUNCTION(BlueprintCallable, Category = "Skill Selection")
	void SetSkillCategory(const FGameplayTag& InSkillCategoryTag);

	// 초기 위젯 설정
	UFUNCTION(BlueprintCallable)
	void SetInfo();

	UFUNCTION(BlueprintCallable)
	void RefreshUI();

protected:
	virtual void NativeConstruct() override;

	UHaroSkillSelectComponent* GetSkillSelectComponent() const;

protected:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UHaroSkillEntryWidget> SkillEntryWidgetClass;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VerticalBox_SkillElements;

private:
	// 현재 스킬 카테고리 태그
	FGameplayTag SkillCategoryTag;

	// 생성된 스킬 위젯들.
	UPROPERTY()
	TArray<TObjectPtr<UHaroSkillEntryWidget>> SkillWidgets;

	// 최대 스킬 옵션 개수 (현재는 4개보단 많아질 수 없다고 판단)
	static constexpr int32 MAX_SKILL_OPTIONS = 4;
};
