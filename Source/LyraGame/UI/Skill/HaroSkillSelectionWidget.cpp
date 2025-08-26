// Fill out your copyright notice in the Description page of Project Settings.


#include "HaroSkillSelectionWidget.h"
#include "HaroSkillEntryWidget.h"
#include "Components/VerticalBox.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(HaroSkillSelectionWidget)

UHaroSkillSelectionWidget::UHaroSkillSelectionWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UHaroSkillSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	VerticalBox_SkillElements->ClearChildren();

	// 여기에 다는 로직 추가 하기
	

	// 선택지 관련된 정보를 제공 받고 (아마 컴포넌트로 뺄 듯?)
	// 그거에 맞게 또 정보를 넘겨줘서 만들기

	// 고민 거리 - 이걸 어떻게 데이터 관리해야 좋을까?
	// 기본 스킬 (불, 얼음, 번개와 같은)
	// 망치 스킬 (무기에 따른)
	// 강화 스킬
}
