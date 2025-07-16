// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LyraWeaponInstance.h"
#include "AbilitySystem/LyraAbilitySourceInterface.h"

#include "HaroProjectileWeaponInstance.generated.h"

// (아래의 이유에도 그냥 분리한 이유: 기존의 어빌리티나 다른 것들은 로직이 많이 바뀌어서 
// Ranged를 히트스캔으로 생각하고 구현했는데 이거만 통합 느낌으로 하면 나중에 헷갈릴까봐)
// 기존의 LyraRangedWeaponInstance랑 매우 유사해서 Base를 만들어서 상속 받는 식으로 하거나 RangedWeaponInstance에 추가하는 식을 고려중
// 그래야 나중에 수정 시, 문제 생기지 않음. (한쪽만 수정한다거나.)

class UPhysicalMaterial;
class AHaroProjectileBase;
/**
 * 
 */
UCLASS()
class LYRAGAME_API UHaroProjectileWeaponInstance : public ULyraWeaponInstance, public ILyraAbilitySourceInterface
{
	GENERATED_BODY()
public:
	UHaroProjectileWeaponInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	void UpdateDebugVisualization();
#endif


	// 무기 속성 Getter 함수들
	int32 GetProjectilesPerCartridge() const { return ProjectilesPerCartridge; }
	float GetCalculatedSpreadAngle() const { return CurrentSpreadAngle; }
	float GetCalculatedSpreadAngleMultiplier() const { return bHasFirstShotAccuracy ? 0.0f : CurrentSpreadAngleMultiplier; }
	bool HasFirstShotAccuracy() const { return bHasFirstShotAccuracy; }
	float GetSpreadExponent() const { return SpreadExponent; }
	float GetProjectileSpeed() const { return ProjectileSpeed; }
	float GetProjectileGravityScale() const { return ProjectileGravityScale; }
	TSubclassOf<AHaroProjectileBase> GetProjectileClass() const { return ProjectileClass; } // 고민 중.

protected:
#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "Spread|Fire Params")
	float Debug_MinHeat = 0.0f;

	UPROPERTY(VisibleAnywhere, Category = "Spread|Fire Params")
	float Debug_MaxHeat = 0.0f;

	UPROPERTY(VisibleAnywhere, Category = "Spread|Fire Params", meta = (ForceUnits = deg))
	float Debug_MinSpreadAngle = 0.0f;

	UPROPERTY(VisibleAnywhere, Category = "Spread|Fire Params", meta = (ForceUnits = deg))
	float Debug_MaxSpreadAngle = 0.0f;

	UPROPERTY(VisibleAnywhere, Category = "Spread Debugging")
	float Debug_CurrentHeat = 0.0f;

	UPROPERTY(VisibleAnywhere, Category = "Spread Debugging", meta = (ForceUnits = deg))
	float Debug_CurrentSpreadAngle = 0.0f;

	// 현재 적용 중인 *결합된* 확산각도 배율
	UPROPERTY(VisibleAnywhere, Category = "Spread Debugging", meta = (ForceUnits = x))
	float Debug_CurrentSpreadAngleMultiplier = 1.0f;
#endif

	// ========== 확산 시스템 변수들 ==========

	// 확산 지수, 무기에 확산이 있을 때 중심선 주변으로 얼마나 밀집되게 발사될지 결정
	// 높은 값일수록 중심에 가깝게 발사됨 (기본값 1.0은 확산 범위 내에서 균등 분포)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.1), Category = "Spread|Fire Params")
	float SpreadExponent = 1.0f;

	// 열을 확산각도로 매핑하는 커브
	// 이 커브의 X 범위는 일반적으로 무기의 최소/최대 열 범위를 설정
	// Y 범위는 최소 및 최대 확산각도를 정의하는 데 사용
	UPROPERTY(EditAnywhere, Category = "Spread|Fire Params")
	FRuntimeFloatCurve HeatToSpreadCurve;

	// 현재 열을 단일 발사가 추가로 '가열'할 양으로 매핑하는 커브
	// 일반적으로 발사당 열 추가량을 나타내는 단일 데이터 포인트를 가진 평평한 커브이지만,
	// 과열을 점진적으로 더 많은 열을 추가하여 처벌하는 등의 다른 형태도 가능
	UPROPERTY(EditAnywhere, Category = "Spread|Fire Params")
	FRuntimeFloatCurve HeatToHeatPerShotCurve;

	// 현재 열을 초당 열 냉각 속도로 매핑하는 커브
	// 일반적으로 열이 얼마나 빨리 식는지를 나타내는 단일 데이터 포인트를 가진 평평한 커브이지만,
	// 높은 열에서 회복 속도를 늦춰 과열을 처벌하는 등의 다른 형태도 가능
	UPROPERTY(EditAnywhere, Category = "Spread|Fire Params")
	FRuntimeFloatCurve HeatToCoolDownPerSecondCurve;

	// 확산 냉각 회복이 시작되기 전 발사 후 대기 시간 (초)
	UPROPERTY(EditAnywhere, Category = "Spread|Fire Params", meta = (ForceUnits = s))
	float SpreadRecoveryCooldownDelay = 0.0f;

	// 플레이어와 무기 확산이 모두 최소값일 때 완벽한 정확도를 가져야 하는지
	UPROPERTY(EditAnywhere, Category = "Spread|Fire Params")
	bool bAllowFirstShotAccuracy = false;

	// ========== 플레이어 상태 기반 확산 배율들 ==========

	// 조준 카메라 모드일 때의 배율
	UPROPERTY(EditAnywhere, Category = "Spread|Player Params", meta = (ForceUnits = x))
	float SpreadAngleMultiplier_Aiming = 1.0f;

	// 정지하거나 매우 천천히 움직일 때의 배율
	// (StandingStillSpeedThreshold에서 페이드 아웃 시작, StandingStillSpeedThreshold + StandingStillToMovingSpeedRange에서 완전히 사라짐)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread|Player Params", meta = (ForceUnits = x))
	float SpreadAngleMultiplier_StandingStill = 1.0f;

	// 정지 정확도로/에서 전환하는 속도 (값이 높을수록 빠름, 0은 즉시; @see FInterpTo)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread|Player Params")
	float TransitionRate_StandingStill = 5.0f;

	// 이 속도 이하는 정지 상태로 간주
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread|Player Params", meta = (ForceUnits = "cm/s"))
	float StandingStillSpeedThreshold = 80.0f;

	// StandingStillSpeedThreshold보다 이만큼 더 높은 속도까지 정지 보너스를 점진적으로 1.0으로 되돌림
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread|Player Params", meta = (ForceUnits = "cm/s"))
	float StandingStillToMovingSpeedRange = 20.0f;

	// 웅크릴 때의 배율, TransitionRate_Crouching을 기반으로 부드럽게 블렌딩
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread|Player Params", meta = (ForceUnits = x))
	float SpreadAngleMultiplier_Crouching = 1.0f;

	// 웅크리기 정확도로/에서 전환하는 속도 (값이 높을수록 빠름, 0은 즉시; @see FInterpTo)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread|Player Params")
	float TransitionRate_Crouching = 5.0f;

	// 점프/낙하 중 확산 배율, TransitionRate_JumpingOrFalling을 기반으로 부드럽게 블렌딩
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread|Player Params", meta = (ForceUnits = x))
	float SpreadAngleMultiplier_JumpingOrFalling = 1.0f;

	// 점프/낙하 정확도로/에서 전환하는 속도 (값이 높을수록 빠름, 0은 즉시; @see FInterpTo)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread|Player Params")
	float TransitionRate_JumpingOrFalling = 5.0f;

	// ========== 투사체 설정 변수들 ==========

	// 단일 카트리지에서 발사할 투사체 수 (일반적으로 1개, 샷건의 경우 더 많을 수 있음)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile Config")
	int32 ProjectilesPerCartridge = 1;

	// 투사체 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile Config")
	TSubclassOf<AHaroProjectileBase> ProjectileClass;

	// 투사체 초기 속도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile Config", meta = (ForceUnits = "cm/s"))
	float ProjectileSpeed = 10000.0f;

	// 투사체 중력 스케일 (0.0이면 중력 없음, 1.0이면 기본 중력)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile Config")
	float ProjectileGravityScale = 1.0f;

	// 투사체 생존 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile Config", meta = (ForceUnits = s))
	float ProjectileLifespan = 10.0f;

	// ========== 데미지 시스템 변수들 ==========

	// 거리(cm)를 연관된 게임플레이 이펙트의 기본 데미지 배율로 매핑하는 커브
	// 이 커브에 데이터가 없으면 무기는 거리에 따른 감쇠가 없다고 가정
	UPROPERTY(EditAnywhere, Category = "Projectile Config")
	FRuntimeFloatCurve DistanceDamageFalloff;

	// 데미지 처리 방식에 영향을 주는 특수 태그 목록
	// 이 태그들은 맞은 대상의 물리 재질 태그와 비교됨
	// 여러 태그가 있으면 배율이 곱셈으로 결합됨
	UPROPERTY(EditAnywhere, Category = "Projectile Config")
	TMap<FGameplayTag, float> MaterialDamageMultiplier;

private:
	// ========== 런타임 상태 변수들 ==========

	// 이 무기가 마지막으로 발사된 시간 (월드 시간 기준)
	double LastFireTime = 0.0;

	// 현재 열
	float CurrentHeat = 0.0f;

	// 현재 확산각도 (도 단위, 직경)
	float CurrentSpreadAngle = 0.0f;

	// 현재 첫 발 정확도를 가지고 있는가?
	bool bHasFirstShotAccuracy = false;

	// 현재 *결합된* 확산각도 배율
	float CurrentSpreadAngleMultiplier = 1.0f;

	// 현재 정지 배율
	float StandingStillMultiplier = 1.0f;

	// 현재 점프/낙하 배율
	float JumpFallMultiplier = 1.0f;

	// 현재 웅크리기 배율
	float CrouchingMultiplier = 1.0f;

public:
	void Tick(float DeltaSeconds);

	//~ULyraEquipmentInstance 인터페이스
	virtual void OnEquipped();
	virtual void OnUnequipped();
	//~ULyraEquipmentInstance 인터페이스 끝

	void AddSpread();

	//~ILyraAbilitySourceInterface 인터페이스
	virtual float GetDistanceAttenuation(float Distance, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr) const override;
	virtual float GetPhysicalMaterialAttenuation(const UPhysicalMaterial* PhysicalMaterial, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr) const override;
	//~ILyraAbilitySourceInterface 인터페이스 끝

private:
	void ComputeSpreadRange(float& MinSpread, float& MaxSpread);
	void ComputeHeatRange(float& MinHeat, float& MaxHeat);

	inline float ClampHeat(float NewHeat)
	{
		float MinHeat;
		float MaxHeat;
		ComputeHeatRange(/*out*/ MinHeat, /*out*/ MaxHeat);

		return FMath::Clamp(NewHeat, MinHeat, MaxHeat);
	}

	// 확산을 업데이트하고 확산이 최소값인지 반환
	bool UpdateSpread(float DeltaSeconds);

	// 배율들을 업데이트하고 최소값인지 반환
	bool UpdateMultipliers(float DeltaSeconds);


};

