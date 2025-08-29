// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UI/LyraActivatableWidget.h"
#include "HaroSkillSelectionWidget.generated.h"


class UVerticalBox;
class UHaroSkillEntryWidget;

/**
 * 
 */
UCLASS()
class LYRAGAME_API UHaroSkillSelectionWidget : public ULyraActivatableWidget
{
	GENERATED_BODY()
	
public:
	UHaroSkillSelectionWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void NativeConstruct() override;

	// 스킬 데이터 조회 테스트 함수
	void TestSkillDataRetrieval();

protected:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UHaroSkillEntryWidget> SkillEntryWidgetClass;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VerticalBox_SkillElements;
};
