// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "HaroProjectileWeaponInstance.h"
#include "HaroChargingProjectileWeaponInstance.generated.h"

/**
 * 
 */
UCLASS()
class LYRAGAME_API UHaroChargingProjectileWeaponInstance : public UHaroProjectileWeaponInstance
{
	GENERATED_BODY()
	
public:
	UHaroChargingProjectileWeaponInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	void UpdateChargingDebugVisualization();
#endif

	// ========== 차징 상태 관리 ==========

	/** 차징 시간 설정 (InputTask에서 받은 값 사용할 것임) */
	UFUNCTION(BlueprintCallable, Category = "Charging")
	void UpdateChargingTime(float TimeHeld);

	/** 현재 설정된 차징 시간 반환 */
	UFUNCTION(BlueprintPure, Category = "Charging")
	float GetChargingTime() const { return ChargingTime; }

	/** 현재 차징 레벨(0~1) 반환 */
	UFUNCTION(BlueprintPure, Category = "Charging")
	float GetChargeLevel() const;

	/** 현재 차징된 투사체 속도 반환 */
	UFUNCTION(BlueprintPure, Category = "Charging")
	float GetChargedProjectileSpeed() const;

	/** 현재 차징된 데미지 배율 반환 */
	UFUNCTION(BlueprintPure, Category = "Charging")
	float GetChargedDamageMultiplier() const;

	/** 현재 차징된 투사체 크기 배율 반환 */
	UFUNCTION(BlueprintPure, Category = "Charging")
	float GetChargedSizeMultiplier() const;

	// Getter 함수들
	float GetMaxChargingTime() const { return MaxChargingTime; }

	// 차징된 투사체 설정 (현재 차징 상태 사용)
	virtual void ConfigureProjectile(AHaroProjectileBase* Projectile) const override;

protected:
#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "Charging Debugging")
	float Debug_ChargingTime = 0.0f;

	UPROPERTY(VisibleAnywhere, Category = "Charging Debugging")
	float Debug_CurrentChargeLevel = 0.0f;
#endif

	// ========== 차징 시간 설정 ==========

	/** 최대 차징 시간 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Charging|Time", meta = (ForceUnits = s))
	float MaxChargingTime = 3.0f;

	// ========== 투사체 속성 커브 (시간 기반) ==========

	/** 차징 시간(초)을 투사체 속도로 매핑하는 커브 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Charging|Curves")
	FRuntimeFloatCurve TimeToSpeedCurve;

	/** 차징 시간(초)을 데미지 배율로 매핑하는 커브 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Charging|Curves")
	FRuntimeFloatCurve TimeToDamageCurve;

	/** 차징 시간(초)을 크기 배율로 매핑하는 커브 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Charging|Curves")
	FRuntimeFloatCurve TimeToSizeCurve;

	// ========== 차징 옵션 ==========

	/** 기본 데미지 배율 (0초 차징시) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Charging|Fallback", meta = (ForceUnits = x))
	float DamageMultiplier = 1.0f;

	/** 기본 크기 배율 (0초 차징시) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Charging|Fallback", meta = (ForceUnits = x))
	float SizeMultiplier = 1.0f;

private:
	// ========== 런타임 상태 ==========

	/** 현재 설정된 차징 시간 (초) */
	float ChargingTime = 0.0f; // 실시간 처리는 아님. 
};
