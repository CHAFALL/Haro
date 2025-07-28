// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "HaroAOEBase.generated.h"

class UArrowComponent;
class UCapsuleComponent;

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

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleCollisionDetection(const FHitResult& HitResult);

protected:
	// 컴포넌트들
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UArrowComponent> ArrowComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCapsuleComponent> CollisionComponent;

	// AOE 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Haro|AOE")
	EHaroAOEType AOEType = EHaroAOEType::Explosion;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Haro|AOE")
	float Damage = 50.f; // 이거도 GE로 바뀌어야 하나? 그리고 ASC도 무기로부터나 투사체로 부터 받아야 하나?

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Haro|AOE")
	float AOERadius = 300.f;
};
