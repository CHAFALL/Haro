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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// AOE 실행 함수들
	UFUNCTION()
	void ExecuteExplosion();

	UFUNCTION()
	void ExecuteDOTField();

	// AOE 완료 처리
	void OnAOECompleted();

	// DOT 관련
	UFUNCTION(BlueprintCallable)
	void OnOverlap(AActor* TargetActor);

	UFUNCTION(BlueprintCallable)
	void OnEndOverlap(AActor* TargetActor);

	TArray<AActor*> FindTargetsInRange();
	bool CheckLineOfSight(FVector ExplosionCenter, AActor* TargetActor, const TArray<AActor*>& AllTargets);
	bool IsValidTarget(AActor* Target) const;
	
	// 즉시 데미지 적용 (거리 기반)
	float CalculateDistanceDamageLevel(float Distance);
	void ApplyDistanceBasedDamageToTarget(AActor* Target, float DamageLevel);

	// 즉시 데미지 적용 (거리 기반 X)
	void ApplyDamageToTarget(AActor* Target);

	// Dot 데미지 적용 (Dot는 거리 기반 없음.)
	void ApplyDOTEffectToTarget(AActor* Target);
	void RemoveDOTEffectFromTarget(AActor* Target);

	// 블루프린트 이벤트들 (시각적 효과 관련)
	UFUNCTION(BlueprintImplementableEvent, Category = "AOE")
	void OnAOEStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "AOE")
	void OnAOEFinished();


protected:
	// 컴포넌트들
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNiagaraComponent> NiagaraComponent;

	// AOE 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AOE")
	EHaroAOEType AOEType = EHaroAOEType::Explosion;

	// 시야 확인 활성화 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AOE")
	bool bCheckLineOfSight = true;

	// 거리별 데미지 감소 활성화 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AOE")
	bool bUseDistanceBasedDamage = true;

	// DOT 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DOT")
	float DOTFieldLifeSpan = 5.0f; 

private:
	// 데미지 이펙트 스펙 (어빌리티->투사체->AOE)
	FGameplayEffectSpecHandle AOEDamageEffectSpecHandle;

	// Dot 효과 추적용
	TMap<FActiveGameplayEffectHandle, UAbilitySystemComponent*> ActiveEffectHandles;
};
