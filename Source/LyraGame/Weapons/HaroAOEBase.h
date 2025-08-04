// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayEffectTypes.h" 
#include "HaroAOEBase.generated.h"

class USphereComponent;
class UNiagaraComponent;

UENUM(BlueprintType)
enum class EHaroAOEType : uint8
{
	Explosion,
	DOTField,
};

UCLASS()
class LYRAGAME_API AHaroAOEBase : public AActor
{
	GENERATED_BODY()
	
public:	
	AHaroAOEBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable)
	void SetDamageEffectSpec(const FGameplayEffectSpecHandle& InDamageEffectSpec);

protected:
	virtual void BeginPlay() override;

	// AOE 실행 함수들
	UFUNCTION()
	void ExecuteExplosion();

	UFUNCTION()
	void ExecuteDOTField();

	TArray<AActor*> FindTargetsInRange();
	
	bool CheckLineOfSight(FVector ExplosionCenter, AActor* TargetActor, const TArray<AActor*>& AllTargets);
	
	float CalculateDistanceDamageLevel(float Distance);
	
	void ApplyDamageToTarget(AActor* Target, float DamageLevel, float Distance, FVector ExplosionCenter);
	
	bool IsValidTarget(AActor* Target) const;
	
	void OnExplosionCompleted();

	// 블루프린트 이벤트들
	UFUNCTION(BlueprintImplementableEvent, Category = "AOE")
	void OnExplosionStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "AOE")
	void OnExplosionFinished();

	UFUNCTION(BlueprintImplementableEvent, Category = "AOE")
	void OnDamageAppliedToTarget(AActor* Target, float Damage, float Distance, float DamageLevel);

protected:
	// 컴포넌트들
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNiagaraComponent> NiagaraComponent;

	// AOE 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Haro|AOE")
	EHaroAOEType AOEType = EHaroAOEType::Explosion;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Haro|AOE")
	float AOERadius = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Haro|AOE")
	float Lifetime = 5.f;

	// 데미지 이펙트 스펙 (투사체나 어빌리티에서 설정)
	FGameplayEffectSpecHandle DamageEffectSpecHandle;

	// 시야 확인 활성화 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AOE|Settings")
	bool bCheckLineOfSight = true;
};
