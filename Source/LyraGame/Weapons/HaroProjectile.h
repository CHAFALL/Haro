// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayEffectTypes.h"
#include "HaroProjectile.generated.h"

class UGameplayEffect;

UCLASS(BlueprintType, Abstract)
class LYRAGAME_API AHaroProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHaroProjectile();

	// 투사체 발사 (어빌리티에서 호출)
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void FireInDirection(const FVector& ShootDirection);

    // 어빌리티 시스템 연동 설정
    UFUNCTION(BlueprintCallable, Category = "Projectile")
    void SetDamageEffect(TSubclassOf<UGameplayEffect> InDamageEffect) { DamageEffect = InDamageEffect; }

	// 데미지 설정 (어빌리티에서 설정)
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SetDamageAmount(float InDamage) { DamageAmount = InDamage; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

    virtual void Destroyed() override;

    // 기본 컴포넌트들
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USphereComponent> CollisionComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> ProjectileMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

    // 기본 설정들
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile")
    float InitialSpeed = 5000.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile")
    float LifeSpan = 3.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile")
    float GravityScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
    float CollisionRadius = 5.0f;

    // 데미지 관련
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Damage")
    TSubclassOf<UGameplayEffect> DamageEffect;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Damage")
    float DamageAmount = 50.0f;

    // 관통 시스템
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Penetration")
    bool bCanPenetrate = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Penetration", meta = (EditCondition = "bCanPenetrate"))
    int32 MaxPenetrationCount = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Penetration", meta = (EditCondition = "bCanPenetrate"))
    float PenetrationDamageReduction = 0.8f; // 관통할 때마다 데미지 80%로 감소

    // 어태치먼트 시스템
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment")
    bool bAttachToHitTarget = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment", meta = (EditCondition = "bAttachToHitTarget"))
    float AttachmentLifeTime = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment", meta = (EditCondition = "bAttachToHitTarget"))
    bool bAttachToBone = true; // 본에 어태치할지 컴포넌트에만 할지

    // 충돌 처리
    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:
    // Lyra 어빌리티 시스템을 통한 데미지 적용
    UFUNCTION(BlueprintCallable, Category = "Damage")
    void ApplyDamageToTarget(AActor* Target, const FHitResult& Hit);

    // 관통 처리
    bool HandlePenetration(AActor* HitActor, const FHitResult& Hit);

    // 어태치먼트 처리
    void HandleAttachment(AActor* HitActor, UPrimitiveComponent* HitComponent, const FHitResult& Hit);

    // 중복 데미지 방지
    UPROPERTY()
    TSet<TWeakObjectPtr<AActor>> HitActors;

    // 관통 상태 추적
    int32 CurrentPenetrationCount = 0;
    float CurrentDamageAmount = 0.0f; // 관통으로 인한 데미지 감소 추적

    // 어태치먼트 상태 
    bool bIsAttached = false;
};
