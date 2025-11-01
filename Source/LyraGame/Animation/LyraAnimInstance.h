// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Animation/AnimInstance.h"
#include "GameplayEffectTypes.h"
#include "LyraAnimInstance.generated.h"

class UAbilitySystemComponent;


/**
 * ULyraAnimInstance
 *
 *	The base game animation instance class used by this project.
 */
UCLASS(Config = Game)
class ULyraAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:

	ULyraAnimInstance(const FObjectInitializer& ObjectInitializer);

	virtual void InitializeWithAbilitySystem(UAbilitySystemComponent* ASC);

protected:

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif // WITH_EDITOR

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:

	// Gameplay tags that can be mapped to blueprint variables. The variables will automatically update as the tags are added or removed.
	// These should be used instead of manually querying for the gameplay tags.
	UPROPERTY(EditDefaultsOnly, Category = "GameplayTags")
	FGameplayTagBlueprintPropertyMap GameplayTagPropertyMap;

	UPROPERTY(BlueprintReadOnly, Category = "Character State Data")
	float GroundDistance = -1.0f;

#pragma region PROCEDURAL IK

public:
	UPROPERTY(BlueprintReadOnly, Category = "Procedural IK")
	class ALyraCharacter* LyraCharacter = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Procedural IK")
	FTransform RelativeHandTransform;

	UPROPERTY(BlueprintReadOnly, Category = "Procedural IK")
	FTransform SightTransform;

	UPROPERTY(BlueprintReadOnly, Category = "Procedural IK")
	float AimAlpha = 0.0f;


	// 장착한 무기를 애니메이션 인스턴스와 동기화하기 위함.
	// 새 무기를 장착할 때 콜백과 같은 기능을 하도록.
	UFUNCTION(BlueprintCallable, Category = "Procedural IK")
	void OnNewWeaponEquipped(float NewWeaponOffset);

	UFUNCTION(BlueprintCallable, Category = "Procedural IK")
	void SetIsAiming(bool NewAiming);

protected:
	void SetSightTransform();
	void SetRelativeHandTransform();
	void InterpAiming(float DeltaTime);

private:
	float SightOffset = 30.0f;

	void FinalizeWeaponSync();

	bool bInterpAiming = false;

	bool bIsAiming = false;

#pragma endregion

};
