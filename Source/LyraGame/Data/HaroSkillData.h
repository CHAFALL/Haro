// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Items/Skill/HaroSkillDataTypes.h"
#include "HaroSkillData.generated.h"

USTRUCT(BlueprintType)
struct FHaroSkillDataEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FName SkillID; // Row Name

	UPROPERTY(BlueprintReadOnly)
	FHaroSkillDataRow SkillData;
};

USTRUCT(BlueprintType)
struct FHaroSkillDataArray
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TArray<FHaroSkillDataEntry> Skills;

	// 편의 함수들
};

/**
 * 스킬 데이터를 GameplayTag 기반으로 캐싱하여 관리하는 클래스
 * DataTable을 사용하되 자주 조회하는 카테고리들을 PreSave에서 미리 캐싱
 */
UCLASS()
class LYRAGAME_API UHaroSkillData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	// 싱글톤 접근
	static const UHaroSkillData& Get();

	

public:
#if WITH_EDITORONLY_DATA
	// 미리 순서 같은 것을 정할때 이용.
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;
#endif

	virtual void PostLoad() override;

	// 오류 방지를 위해 필요해질 수 있음.
//#if WITH_EDITOR
//	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
//#endif

private:
	void RefreshSkillCache();
	void CacheDefaultSkills(UDataTable* Table);
	void CacheSkillsByCategory(UDataTable* Table);

public:
	UFUNCTION(BlueprintCallable)
	TArray<FHaroSkillDataEntry> GetSkillsByTag(const FGameplayTag& Tag) const;

	// 현재로써는 디폴트 스킬에서 찾는거로만 해둠.
	UFUNCTION(BlueprintCallable)
	FName FindSkillIDByAbilityClass(TSubclassOf<ULyraGameplayAbility> AbilityClass) const;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UDataTable> DefaultSkillDataTable; // 미리 로드됨.

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UDataTable> WeaponSkillDataTable; // 필요시 로드됨.

private:
	// 캐시 
	// TMap 안에 TArry를 넣으면 UPROPERTY가 난리침. -> 구조체 매핑으로 해보자 (구조체로 한 번 감쌈.)
	// FName으로 해서 찾으면 시간 복잡도 O(1)임 -> DataTable의 내부 구조를 보면 TMap<FName, uint8*> RowMap로 되어 있으므로.
	// 그래도 바로 값으로 저장하고자 함. -> 메모리 별로 안 잡아먹음.
	UPROPERTY()
	TMap<FGameplayTag, FHaroSkillDataArray> CachedSkillsByTag;

	// AbilityClass → SkillID 캐시 (디폴트 스킬들만)
	UPROPERTY()
	TMap<TSubclassOf<ULyraGameplayAbility>, FName> CachedAbilityClassToSkillID;
};
