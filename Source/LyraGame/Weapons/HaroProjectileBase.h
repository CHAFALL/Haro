// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayTagContainer.h"
#include "HaroProjectileBase.generated.h"

class UGameplayEffect;
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
	TObjectPtr<USphereComponent> SphereCollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ProjectileMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float Damage = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "GameplayCue"))
	FGameplayTag HitGameplayCueTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bAttachToHitComponent = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	ECollisionDetectionType CollisionDetectionType = ECollisionDetectionType::None;

private:
	UPROPERTY()
	TWeakObjectPtr<UActorComponent> AttachingComponent;

	UPROPERTY()
	TSet<TWeakObjectPtr<AActor>> HitActors;
};
