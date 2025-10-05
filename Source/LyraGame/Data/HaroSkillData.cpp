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
	// TSoftObjectPtr 대신에 TObjectPtr로 변경로 변경하니 해결되어야 하는데 왜 안되지???
	// -> 좀 더 연구 필요.
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
	CachedSkillsByTag.Empty();
	CachedAbilityClassToSkillID.Empty();

	CacheDefaultSkills(DefaultSkillDataTable);
	CacheSkillsByCategory(WeaponSkillDataTable);

	// 갯수로 올바르게 캐싱 되었는지 파악.
	UE_LOG(LogTemp, Log,
		TEXT("[SkillData] Loaded %d default skills, %d skill categories"),
		CachedAbilityClassToSkillID.Num(),
		CachedSkillsByTag.Num());
}

void UHaroSkillData::CacheDefaultSkills(UDataTable* Table)
{
	if (!Table)
	{
		return;
	}

	Table->ForeachRow<FHaroSkillDataRow>("CacheDefaultSkills",
		[this](const FName& RowName, const FHaroSkillDataRow& Row)
		{
			if (UClass* LoadedClass = Row.AbilityClass.LoadSynchronous())
			{
				CachedAbilityClassToSkillID.Add(LoadedClass, RowName);
			}

			return true;
		});
}

void UHaroSkillData::CacheSkillsByCategory(UDataTable* Table)
{
	if (!Table)
	{
		return;
	}

	Table->ForeachRow<FHaroSkillDataRow>("CacheSkillsByCategory",
		[this](const FName& RowName, const FHaroSkillDataRow& Row)
		{
			if (Row.CategoryTag.IsValid())
			{
				// Entry 생성
				FHaroSkillDataEntry Entry;
				Entry.SkillID = RowName;
				Entry.SkillData = Row;

				// 해당 태그의 배열을 찾거나 새로 생성하고 스킬 Entry를 추가
				CachedSkillsByTag.FindOrAdd(Row.CategoryTag).Skills.Add(Entry);
			}

			return true; // continue iteration
		});
}

TArray<FHaroSkillDataEntry> UHaroSkillData::GetSkillsByTag(const FGameplayTag& Tag) const
{
	// 캐시에서 해당 태그를 찾기
	const FHaroSkillDataArray* Found = CachedSkillsByTag.Find(Tag);

	// 찾았으면 Skills 배열 반환, 못 찾으면 빈 배열 반환
	return Found ? Found->Skills : TArray<FHaroSkillDataEntry>();
}

FName UHaroSkillData::FindSkillIDByAbilityClass(TSubclassOf<ULyraGameplayAbility> AbilityClass) const
{
	if (!AbilityClass)
	{
		return NAME_None;
	}

	// 캐시 검색 (디폴트 스킬만)
	if (const FName* FoundSkillID = CachedAbilityClassToSkillID.Find(AbilityClass))
	{
		return *FoundSkillID;
	}

	return NAME_None;
}



