// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Equipment/LyraGameplayAbility_FromEquipment.h"
#include "HaroGameplayAbility_WeaponBase.generated.h"

class ULyraWeaponInstance;

/** Defines where an ability starts its trace from and where it should face */
UENUM(BlueprintType)
enum class EHaroAbilityTargetingSource : uint8
{
	// From the player's camera towards camera focus
	CameraTowardsFocus,
	// From the pawn's center, in the pawn's orientation
	PawnForward,
	// From the pawn's center, oriented towards camera focus
	PawnTowardsFocus,
	// From the weapon's muzzle or location, in the pawn's orientation
	WeaponForward,
	// From the weapon's muzzle or location, towards camera focus
	WeaponTowardsFocus,
	// Custom blueprint-specified source location
	Custom
};


/**
 * 
 */
UCLASS()
class LYRAGAME_API UHaroGameplayAbility_WeaponBase : public ULyraGameplayAbility_FromEquipment
{
	GENERATED_BODY()
	
public:
	UHaroGameplayAbility_WeaponBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	// 공통 타겟팅 함수들
	FVector GetWeaponTargetingSourceLocation() const;
	FTransform GetTargetingTransform(APawn* SourcePawn, EHaroAbilityTargetingSource Source) const;
};
