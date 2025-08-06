// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Curves/CurveFloat.h"
#include "GameplayTagContainer.h"
#include "HaroWeaponTypes.generated.h"


/** 무기 발사 타입 열거형 */
UENUM(BlueprintType)
enum class EHaroWeaponFireType : uint8
{
    Hitscan,        // 히트스캔
    Projectile      // 투사체 (차징 기능 포함, MaxChargingTime=0이면 일반 투사체)
};

/** 발사 입력 타입 열거형 */
UENUM(BlueprintType)
enum class EHaroFireInputType : uint8
{
    Primary,        // 좌클릭
    Secondary       // 우클릭
};


/** 히트스캔 발사 설정 */
USTRUCT(BlueprintType)
struct FHaroHitscanFireConfig
{
    GENERATED_BODY()

    // 한 번의 탄약 발사에서 발사되는 총알의 수 (일반적으로 1개, 샷건의 경우 더 많을 수 있음)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Config")
    int32 BulletsPerCartridge = 1;

    // 이 무기가 데미지를 입힐 수 있는 최대 거리
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Config", meta = (ForceUnits = cm))
    float MaxDamageRange = 25000.0f;

    // 총알 추적 스위프 구체의 반지름 (0.0이면 선 추적이 됨)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Config", meta = (ForceUnits = cm))
    float BulletTraceSweepRadius = 0.0f;

    // 거리(cm)를 연관된 게임플레이 이펙트의 기본 데미지에 대한 배수로 매핑하는 커브
    // 이 커브에 데이터가 없으면, 무기가 거리에 따른 감소가 없다고 가정됨
    UPROPERTY(EditAnywhere, Category = "Weapon Config")
    FRuntimeFloatCurve DistanceDamageFalloff;

    // 데미지가 가해지는 방식에 영향을 주는 특수 태그들의 목록
    // 이 태그들은 피격된 물체의 물리적 재질의 태그들과 비교됨
    // 두 개 이상의 태그가 있을 경우, 배수들이 곱셈적으로 결합됨
    UPROPERTY(EditAnywhere, Category = "Weapon Config")
    TMap<FGameplayTag, float> MaterialDamageMultiplier;
};

/** 투사체 발사 설정 (차징 기능 포함) */
USTRUCT(BlueprintType)
struct FHaroProjectileFireConfig
{
    GENERATED_BODY()

    // 최대 차징 시간이 0초이면 아래 옵션도 안 뜨게 하고 싶은데....
    // ========== 차징 시간 설정 ==========
    /** 최대 차징 시간 (초) - 0이면 일반 투사체, 0보다 크면 차징 투사체 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Charging|Time", meta = (ForceUnits = s))
    float MaxChargingTime = 0.0f;

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

    // ========== 투사체 설정 ==========

    // 단일 카트리지에서 발사할 투사체 수 (일반적으로 1개, 샷건의 경우 더 많을 수 있음)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile Config")
    int32 ProjectilesPerCartridge = 1;

    // 투사체 초기 속도
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile Config", meta = (ForceUnits = "cm/s"))
    float ProjectileSpeed = 1000.0f;

    // 투사체 중력 스케일 (0.0이면 중력 없음, 1.0이면 기본 중력)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile Config")
    float ProjectileGravityScale = 1.0f;

    // 투사체 생존 시간 (초)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile Config", meta = (ForceUnits = s))
    float ProjectileLifespan = 10.0f;

    // 거리(cm)를 연관된 게임플레이 이펙트의 기본 데미지 배율로 매핑하는 커브
    // 이 커브에 데이터가 없으면 무기는 거리에 따른 감쇠가 없다고 가정
    UPROPERTY(EditAnywhere, Category = "Projectile Config")
    FRuntimeFloatCurve DistanceDamageFalloff;

    // 데미지 처리 방식에 영향을 주는 특수 태그 목록
    // 이 태그들은 맞은 대상의 물리 재질 태그와 비교됨
    // 여러 태그가 있으면 배율이 곱셈으로 결합됨
    UPROPERTY(EditAnywhere, Category = "Projectile Config")
    TMap<FGameplayTag, float> MaterialDamageMultiplier;
};
