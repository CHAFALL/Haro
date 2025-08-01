// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h" 
#include "HaroProjectileBase.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

UENUM(BlueprintType)
enum class ECollisionDetectionType : uint8
{
	None,
	Hit,
	Overlap // 관통용.
};

UCLASS(BlueprintType, Abstract)
class LYRAGAME_API AHaroProjectileBase : public AActor
{
	GENERATED_BODY()
	
public:	
	AHaroProjectileBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable)
	void SetSpeed(float Speed);

	UFUNCTION(BlueprintCallable)
	void SetGravityScale(float NewGravityScale);

	UFUNCTION(BlueprintPure)
	FGameplayEffectSpecHandle GetDamageEffectSpec() const { return DamageEffectSpecHandle; }

	// 데미지 이펙트 스펙 설정 (어빌리티에서 호출)
	UFUNCTION(BlueprintCallable)
	void SetDamageEffectSpec(const FGameplayEffectSpecHandle& InDamageSpec);

protected:
	virtual void BeginPlay() override;
	virtual void Destroyed() override;

private:
	UFUNCTION()
	void HandleComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& HitResult);

	UFUNCTION()
	void HandleComponentOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleOtherComponentDeactivated(UActorComponent* OtherComponent);

	UFUNCTION()
	void HandleCollisionDetection(AActor* OtherActor, UPrimitiveComponent* OtherComponent, const FHitResult& HitResult);


protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> SphereCollisionComponent; // 이거 좀 더 다양하게 가능하도록 수정하는 것도 나쁘지 않을 듯.

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ProjectileMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "GameplayCue"))
	FGameplayTag HitGameplayCueTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bAttachToHitComponent = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	ECollisionDetectionType CollisionDetectionType = ECollisionDetectionType::None;

	virtual void ApplyDamageToTarget(AActor* TargetActor, const FHitResult& HitResult);

private:
	UPROPERTY()
	TWeakObjectPtr<UActorComponent> AttachingComponent;

	UPROPERTY()
	TSet<TWeakObjectPtr<AActor>> HitActors;

	// 데미지 이펙트 스펙 (어빌리티에서 설정)
	FGameplayEffectSpecHandle DamageEffectSpecHandle;
};



