// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Curves/CurveFloat.h"

#include "LyraWeaponInstance.h"
#include "AbilitySystem/LyraAbilitySourceInterface.h"
#include "HaroWeaponTypes.h"

#include "HaroRangedWeaponInstance.generated.h"

class UPhysicalMaterial;
class AHaroProjectileBase;


/** 발사 모드 설정 */
// Config에 대한 내용은 HaroWeaponTypes.h에서 해놨음.
USTRUCT(BlueprintType)
struct FHaroFireModeConfig
{
    GENERATED_BODY()

    /** 입력 타입 (Primary/Secondary) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Mode")
    EHaroFireInputType InputType = EHaroFireInputType::Primary;

    /** 발사 타입 (Hitscan/Projectile) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Mode")
    EHaroWeaponFireType FireType = EHaroWeaponFireType::Hitscan;

    /** 히트스캔 설정 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Mode", meta = (EditCondition = "FireType == EHaroWeaponFireType::Hitscan", EditConditionHides))
    FHaroHitscanFireConfig HitscanConfig;

    /** 투사체 설정 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Mode", meta = (EditCondition = "FireType == EHaroWeaponFireType::Projectile", EditConditionHides))
    FHaroProjectileFireConfig ProjectileConfig;
};


/**
 * 듀얼 모드를 지원하는 통합 원거리 무기 인스턴스
 * (만약에 너무 방대해진다싶으면 Fragments 식으로 해서 모듈화 하기)
 */
UCLASS()
class LYRAGAME_API UHaroRangedWeaponInstance : public ULyraWeaponInstance, public ILyraAbilitySourceInterface
{
	GENERATED_BODY()
	
public:
    UHaroRangedWeaponInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void PostLoad() override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
    
    void UpdateDebugVisualization();
#endif

    // ========== 발사 모드 설정 접근자 ==========

    /** 입력 타입으로 발사 모드 찾기 (C++ 전용) */
    const FHaroFireModeConfig* GetFireModeForInput(EHaroFireInputType InputType) const;

    /** 입력 타입으로 발사 모드 찾기 (블루프린트용) */
    /*UFUNCTION(BlueprintPure, Category = "Fire Config")
    FHaroFireModeConfig GetFireModeForInputBP(EHaroFireInputType InputType) const;*/

    // ========== 차징 관련 함수들 ==========

    /** 차징 시간 설정 (InputTask에서 받은 값 사용할 것임) */
    UFUNCTION(BlueprintCallable, Category = "Charging")
    void UpdateChargingTime(float TimeHeld) { ChargingTime = TimeHeld; }

    /** 현재 설정된 차징 시간 반환 */
    UFUNCTION(BlueprintPure, Category = "Charging")
    float GetChargingTime() const { return ChargingTime; }

    // ========== 투사체 설정 함수들 ==========

    /** 투사체 설정 (입력 타입 기준) */
    UFUNCTION(BlueprintCallable, Category = "Projectile")
    void ConfigureProjectileForInput(AHaroProjectileBase* Projectile, EHaroFireInputType InputType) const;
    float GetChargedDamageMultiplier(EHaroFireInputType InputType) const;

    // ========== 히트스캔 관련 함수들 (입력 타입별) ==========
    int32 GetBulletsPerCartridge(EHaroFireInputType InputType) const;
    float GetMaxDamageRange(EHaroFireInputType InputType) const;
    float GetBulletTraceSweepRadius(EHaroFireInputType InputType) const;

    // ========== 투사체 관련 함수들 (입력 타입별) ==========
    int32 GetProjectilesPerCartridge(EHaroFireInputType InputType) const;
    float GetProjectileSpeed(EHaroFireInputType InputType) const;
    float GetProjectileSizeMultiplier(EHaroFireInputType InputType) const;
    float GetProjectileGravityScale(EHaroFireInputType InputType) const;
    float GetProjectileLifespan(EHaroFireInputType InputType) const;

    // ========== 확산 시스템 관련 함수들 ==========
    float GetCalculatedSpreadAngle() const { return CurrentSpreadAngle; }
    float GetCalculatedSpreadAngleMultiplier() const { return bHasFirstShotAccuracy ? 0.0f : CurrentSpreadAngleMultiplier; }
    bool HasFirstShotAccuracy() const { return bHasFirstShotAccuracy; }
    float GetSpreadExponent() const { return SpreadExponent; }
    
    

protected:

    #if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "Spread|Fire Params")
	float Debug_MinHeat = 0.0f;

	UPROPERTY(VisibleAnywhere, Category = "Spread|Fire Params")
	float Debug_MaxHeat = 0.0f;

	UPROPERTY(VisibleAnywhere, Category="Spread|Fire Params", meta=(ForceUnits=deg))
	float Debug_MinSpreadAngle = 0.0f;

	UPROPERTY(VisibleAnywhere, Category="Spread|Fire Params", meta=(ForceUnits=deg))
	float Debug_MaxSpreadAngle = 0.0f;

	UPROPERTY(VisibleAnywhere, Category="Spread Debugging")
	float Debug_CurrentHeat = 0.0f;

	UPROPERTY(VisibleAnywhere, Category="Spread Debugging", meta = (ForceUnits=deg))
	float Debug_CurrentSpreadAngle = 0.0f;

	// The current *combined* spread angle multiplier
	UPROPERTY(VisibleAnywhere, Category = "Spread Debugging", meta=(ForceUnits=x))
	float Debug_CurrentSpreadAngleMultiplier = 1.0f;

#endif

    // ========== 발사 모드 설정 ==========

    /** 발사 모드 배열 (최대 2개: Primary, Secondary) */
    UPROPERTY(EditAnywhere, Category = "Fire Modes", meta = (TitleProperty = "InputType"))
    TArray<FHaroFireModeConfig> FireModes;

    // ========== 확산 시스템 (기존 LyraRangedWeaponInstance와 동일) ==========
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.1), Category = "Spread|Fire Params")
    float SpreadExponent = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Spread|Fire Params")
    FRuntimeFloatCurve HeatToSpreadCurve;

    UPROPERTY(EditAnywhere, Category = "Spread|Fire Params")
    FRuntimeFloatCurve HeatToHeatPerShotCurve;

    UPROPERTY(EditAnywhere, Category = "Spread|Fire Params")
    FRuntimeFloatCurve HeatToCoolDownPerSecondCurve;

    UPROPERTY(EditAnywhere, Category = "Spread|Fire Params", meta = (ForceUnits = s))
    float SpreadRecoveryCooldownDelay = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Spread|Fire Params")
    bool bAllowFirstShotAccuracy = false;

    // ========== 플레이어 상태별 확산 배율들 ==========
    UPROPERTY(EditAnywhere, Category = "Spread|Player Params", meta = (ForceUnits = x))
    float SpreadAngleMultiplier_Aiming = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread|Player Params", meta = (ForceUnits = x))
    float SpreadAngleMultiplier_StandingStill = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread|Player Params")
    float TransitionRate_StandingStill = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread|Player Params", meta = (ForceUnits = "cm/s"))
    float StandingStillSpeedThreshold = 80.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread|Player Params", meta = (ForceUnits = "cm/s"))
    float StandingStillToMovingSpeedRange = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread|Player Params", meta = (ForceUnits = x))
    float SpreadAngleMultiplier_Crouching = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread|Player Params")
    float TransitionRate_Crouching = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread|Player Params", meta = (ForceUnits = x))
    float SpreadAngleMultiplier_JumpingOrFalling = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread|Player Params")
    float TransitionRate_JumpingOrFalling = 5.0f;

private:
    // ========== 런타임 상태 변수들 ==========

    /** 현재 설정된 차징 시간 (초) */
    float ChargingTime = 0.0f;

    double LastFireTime = 0.0;
    float CurrentHeat = 0.0f;
    float CurrentSpreadAngle = 0.0f;
    bool bHasFirstShotAccuracy = false;
    float CurrentSpreadAngleMultiplier = 1.0f;
    float StandingStillMultiplier = 1.0f;
    float JumpFallMultiplier = 1.0f;
    float CrouchingMultiplier = 1.0f;

public:
    void Tick(float DeltaSeconds);

    //~ULyraEquipmentInstance interface
    virtual void OnEquipped();
    virtual void OnUnequipped();
    //~End of ULyraEquipmentInstance interface

    void AddSpread();

    //~ILyraAbilitySourceInterface interface
    virtual float GetDistanceAttenuation(float Distance, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr) const override;
    virtual float GetPhysicalMaterialAttenuation(const UPhysicalMaterial* PhysicalMaterial, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr) const override;
    //~End of ILyraAbilitySourceInterface interface

private:
    // ========== 확산 시스템 헬퍼 함수들 ==========
    void ComputeSpreadRange(float& MinSpread, float& MaxSpread);
    void ComputeHeatRange(float& MinHeat, float& MaxHeat);

    inline float ClampHeat(float NewHeat)
    {
        float MinHeat;
        float MaxHeat;
        ComputeHeatRange(/*out*/ MinHeat, /*out*/ MaxHeat);

        return FMath::Clamp(NewHeat, MinHeat, MaxHeat);
    }

    bool UpdateSpread(float DeltaSeconds);
    bool UpdateMultipliers(float DeltaSeconds);
};
