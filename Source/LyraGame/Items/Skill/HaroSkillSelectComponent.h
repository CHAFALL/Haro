// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Data/HaroSkillData.h" // 반환값으로 사용 중이니 완전체가 필요하므로 전방 선언 대신에 이를 사용.
#include "HaroSkillSelectComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LYRAGAME_API UHaroSkillSelectComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UHaroSkillSelectComponent(const FObjectInitializer& ObjectInitializer);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 태그에 따라 무기 스킬로 할지, 원소 스킬로 할지 등을 결정해주는 메인 함수 
	// 망치면 Skill.Weapon (얘는 무기인지만 알려주면 됨) -> GenerateSkillOptionsForWeapons()
	// 원소면 Skill.Element.Fire (얘는 세분화가 필요) -> GenerateSkillOptionsForElement(해당하는 태그)
	UFUNCTION(BlueprintCallable, Category = "Skill Selection")
	TArray<FHaroSkillDataEntry> GenerateSkillOptions(const FGameplayTag& SkillCategory);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Skill Selection")
	void ServerApplySelectedSkill(const FHaroSkillDataEntry& SelectedSkill);

	// 소유한 스킬 맵 조작 관련 헬퍼 함수들 (맵만 조작함.) -> 이것도 playerState로 넘어갈지도.
	bool RegisterSkillHandle(const FName& SkillID, const FGameplayAbilitySpecHandle& Handle);
	bool UnregisterSkillHandle(const FName& SkillID);

protected:
	virtual void BeginPlay() override;
		
	// 현재 플레이어가 보유한 무기 태그들에 대한 스킬 옵션 생성
	TArray<FHaroSkillDataEntry> GenerateSkillOptionsForWeapons();
	
private:
	// 스킬이 선택 가능한지 확인하는 헬퍼 함수들
	bool IsSkillSelectable(const FHaroSkillDataEntry& SkillEntry) const;
	bool CheckRequiredSkills(const FHaroSkillDataEntry& SkillEntry) const;
	bool CheckConflictSkills(const FHaroSkillDataEntry& SkillEntry) const;

	// 랜덤 선택 헬퍼
	TArray<FHaroSkillDataEntry> SelectRandomSkills(const TArray<FHaroSkillDataEntry>& AvailableSkills, int32 Count) const;

	// 선택된 스킬 부여 관련 헬퍼 함수들
	bool RemoveReplacedSkill(const TArray<FName>& ReplaceSkillIDs, UAbilitySystemComponent* ASC, /*OUT*/ UObject*& OutSourceObject);
	bool AddNewSkill(const FName& SkillID, const FHaroSkillDataRow& SkillData, UClass* AbilityClass, UAbilitySystemComponent* ASC, UObject* SourceObject);

	UAbilitySystemComponent* GetAbilitySystemComponent() const;

protected:
	// 현재 플레이어가 보유한 무기 태그들 -> playerstate로 뺄듯?
	UPROPERTY(BlueprintReadOnly, Category = "Current State")
	TArray<FGameplayTag> CurrentWeaponTags;

	// 클라 UI 표시용
	UPROPERTY(Replicated, BlueprintReadOnly)
	TArray<FName> OwnedSkillIDs;

	// 현재 플레이어가 보유한 스킬 {ID : Handle} -> playerstate로 뺄듯?
	UPROPERTY(BlueprintReadOnly, Category = "Current State")
	TMap<FName, FGameplayAbilitySpecHandle> OwnedSkillHandles; // 서버 전용

	// 한 번에 제공할 스킬 옵션 개수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	int32 MaxSkillOptions = 3;
};
