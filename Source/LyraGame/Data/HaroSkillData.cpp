// Fill out your copyright notice in the Description page of Project Settings.
#include "HaroSkillData.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif // WITH_EDITOR

#include "System/LyraAssetManager.h"
#include "UObject/ObjectSaveContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(HaroSkillData)

const UHaroSkillData& UHaroSkillData::Get()
{
    return ULyraAssetManager::Get().GetSkillData();
}

#if WITH_EDITORONLY_DATA
void UHaroSkillData::PreSave(FObjectPreSaveContext SaveContext)
{
	Super::PreSave(SaveContext);

	RefreshSkillCache();

	// 남은 문제점: TSoftObjectPtr로 하다보니깐 실제 값이 바뀐 것은 감지 못함.
	// (경로 자체가 바뀔 때는 감지 함.)
	// 테스트를 하려면 껏다가 켜야됨...
	// TSoftObjectPtr 대신에 TObjectPtr로 변경로 변경하니 해결됨.
}
#endif

void UHaroSkillData::PostLoad()
{
	Super::PostLoad();

	// 캐시가 비어있으면 생성
	if (CachedSkillsByTag.Num() == 0)
	{
		RefreshSkillCache();
	}
}

void UHaroSkillData::RefreshSkillCache()
{
	// 캐시 초기화
	CachedSkillsByTag.Empty();

	if (!WeaponSkillDataTable)	return;

	// Get하면 될 줄 알았는데 UHaroSkillData 객체 자체는 로드가 되었지만
	// TSoftObjectPtr<UDataTable> WeaponSkillDataTable는 경로 정보만 가지고 있고 실제 DataTable은 아직 로드가 안되어서
	// LoadSynchronous()로 해야됨.
	// -> 이것도 그냥 TSoftObjectPtr 대신에 TObjectPtr로 변경하니 해결됨.
	if (UDataTable* Table = WeaponSkillDataTable.Get())
	{
		Table->ForeachRow<FHaroSkillDataRow>("RefreshSkillCache",
			[this](const FName& RowName, const FHaroSkillDataRow& Row)
			{
				if (Row.GameplayTag.IsValid())
				{
					// Entry 생성
					FHaroSkillDataEntry Entry;
					Entry.SkillID = RowName;
					Entry.SkillData = Row;

					// 해당 태그의 배열을 찾거나 새로 생성하고 스킬 Entry를 추가
					CachedSkillsByTag.FindOrAdd(Row.GameplayTag).Skills.Add(Entry);
				}
				return true; // continue iteration
			});
	}
}

TArray<FHaroSkillDataEntry> UHaroSkillData::GetSkillsByTag(const FGameplayTag& Tag) const
{
	// 캐시에서 해당 태그를 찾기
	const FHaroSkillDataArray* Found = CachedSkillsByTag.Find(Tag);

	// 찾았으면 Skills 배열 반환, 못 찾으면 빈 배열 반환
	return Found ? Found->Skills : TArray<FHaroSkillDataEntry>();
}
