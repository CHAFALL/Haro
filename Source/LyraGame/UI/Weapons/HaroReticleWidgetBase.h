// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CommonUserWidget.h"

#include "HaroReticleWidgetBase.generated.h"

class ULyraInventoryItemInstance;
class ULyraWeaponInstance;
class UObject;
struct FFrame;

/**
 * 
 */
UCLASS(Abstract)
class LYRAGAME_API UHaroReticleWidgetBase : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	UHaroReticleWidgetBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintImplementableEvent)
	void OnWeaponInitialized();

	UFUNCTION(BlueprintCallable)
	void InitializeFromWeapon(ULyraWeaponInstance* InWeapon);

	/** Returns the current weapon's diametrical spread angle, in degrees */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	float ComputeSpreadAngle() const;

	/** Returns the current weapon's maximum spread radius in screenspace units (pixels) */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	float ComputeMaxScreenspaceSpreadRadius() const;

	/**
	 * Returns true if the current weapon is at 'first shot accuracy'
	 * (the weapon allows it and it is at min spread)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool HasFirstShotAccuracy() const;

protected:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<ULyraWeaponInstance> WeaponInstance;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<ULyraInventoryItemInstance> InventoryInstance;
};
