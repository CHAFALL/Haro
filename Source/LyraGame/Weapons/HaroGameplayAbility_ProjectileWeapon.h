// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "HaroGameplayAbility_WeaponBase.h"
#include "HaroGameplayAbility_ProjectileWeapon.generated.h"

class AHaroProjectileBase;

/**
 * 
 */
UCLASS()
class LYRAGAME_API UHaroGameplayAbility_ProjectileWeapon : public UHaroGameplayAbility_WeaponBase
{
	GENERATED_BODY()
	
public:
	UHaroGameplayAbility_ProjectileWeapon(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "Haro|Ability")
	UHaroProjectileWeaponInstance* GetWeaponInstance() const;

	//~UGameplayAbility interface
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~End of UGameplayAbility interface

protected:
	void SpawnProjectile();
	void SpawnProjectileFromTargetData(const FGameplayAbilityTargetDataHandle& TargetData);

	// 투사체 스폰 후 호출되는 네이티브 함수
	virtual void OnProjectileSpawned(AHaroProjectileBase* SpawnedProjectile);

	// 서버 검증 함수 (치팅 방지)
	bool IsValidLaunchTransform(const FTransform& LaunchTransform, APawn* SourcePawn);

	void OnTargetDataReadyCallback(const FGameplayAbilityTargetDataHandle& InData, FGameplayTag ApplicationTag);

	UFUNCTION(BlueprintCallable)
	void StartProjectileTargeting();

	/** 투사체 발사 Transform 계산 */
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	virtual bool CalculateProjectileLaunchTransform(AActor* AvatarActor, AController* Controller, FTransform& OutLaunchTransform);

	// Called when target data is ready
	UFUNCTION(BlueprintImplementableEvent, Category = "Projectile")
	void OnProjectileTargetDataReady(const FGameplayAbilityTargetDataHandle& TargetData);

	UFUNCTION(BlueprintImplementableEvent, Category = "Projectile")
	void OnProjectileSpawnedBP(AHaroProjectileBase* SpawnedProjectile);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSubclassOf<AHaroProjectileBase> ProjectileClass;

	// 타겟팅 소스 설정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Targeting")
	EHaroAbilityTargetingSource TargetingSource = EHaroAbilityTargetingSource::CameraTowardsFocus;

	
private:
	FDelegateHandle OnTargetDataReadyCallbackDelegateHandle;
};


// TODO
// 인스턴스로부터 확산된 정도나 그런 것들을 가져와서 투사체 방향이라던가 그런거 계산하는 로직 추가 필요.
