// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "HaroGameplayAbility_ProjectileWeapon.h"
#include "HaroGameplayAbility_ChargingProjectileWeapon.generated.h"

class UHaroRangedWeaponInstance;
class UAbilityTask_WaitInputRelease;

/**
 * 
 */
UCLASS()
class LYRAGAME_API UHaroGameplayAbility_ChargingProjectileWeapon : public UHaroGameplayAbility_ProjectileWeapon
{
	GENERATED_BODY()
	
public:
	UHaroGameplayAbility_ChargingProjectileWeapon(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UGameplayAbility interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~End of UGameplayAbility interface

protected:

	// 래퍼함수 -> C++에서 템플릿 함수(AddUObject)에 멤버 함수 포인터를 전달할 때, 해당 함수가 protected면 접근할 수 없다고 해서 래퍼함수를 통해 징검다리 만듬.
	void OnChargingTargetDataReadyCallback(const FGameplayAbilityTargetDataHandle& InData, FGameplayTag ApplicationTag);

	virtual void ConfigureProjectileDamageEffect(AHaroProjectileBase* Projectile, UHaroRangedWeaponInstance* WeaponInstance, ALyraCharacter* SourceCharacter) override;

	/** 입력 해제 시 호출 (차징 완료 및 발사) */
	UFUNCTION()
	void OnInputRelease(float TimeHeld);

	// 블루프린트 이벤트들 (UI/애니메이션용)
	UFUNCTION(BlueprintImplementableEvent, Category = "Charging")
	void OnChargingStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Charging")
	void OnChargingCancelled();

	// 새로 추가
	UFUNCTION(BlueprintImplementableEvent, Category = "Charging")
	void OnChargingCompleted();

private:
	// ========== 어빌리티 태스크들 ==========

	/** 입력 해제 대기 태스크 */
	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitInputRelease> InputReleaseTask;

private:
	/** 모든 태스크 정리 */
	void CleanupTasks();
};

// 나중에 알았는데 Input Action 이벤트 핸들러 (Enhanced Input에서 바인딩)
// Started/Ongoing/Completed를 써도 괜찮았을듯.